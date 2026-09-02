/*
 * SPDX-FileCopyrightText: Copyright (C) 2026 Altera Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 * Mailbox multi block task
 */

#include "FreeRTOS.h"
#include "task.h"
#include "osal.h"
#include "osal_log.h"
#include "socfpga_mbox_client.h"
#include "portmacro.h"

#define SIP_SMC_HWMON_TEMP        0x420000E8
#define SIP_SMC_HWMON_VOLT        0x420000E9
#define VCC_CHANNEL               0x4
#define READ_PEAK_TEMPERATURE     0x1
#define MBOX_XFER_TIMEOUT         1000U

extern uint32_t ulGetPhysicalCoreMpidr(void);

struct mbox_stress_state
{
    float tempActual;
    float voltActual;
};

static struct mbox_stress_state mbox_state;
static osal_semaphore_def_t mbox_sem_mem;
static osal_semaphore_t mbox_sem;
static volatile uint32_t hetero_mbox_error_count = 0;
static volatile uint32_t hetero_mbox_success_count = 0;
static volatile uint32_t hetero_mbox_current_core = 0;
static volatile bool hetero_mbox_stop_requested = false;
static volatile bool hetero_mbox_stopped = false;

static void mbox_stress_callback(uint64_t *resp_values)
{
    (void)resp_values;
    osal_semaphore_post(mbox_sem);
}

void hetero_mbox_task(void *arg)
{
    int ret;
    uint64_t channel, smc_fid, resp_data[2];
    sdm_client_handle_t mbox_handle;

    (void)arg;

    mbox_init();

    mbox_sem = osal_semaphore_create(&mbox_sem_mem);
    if (mbox_sem == NULL)
    {
        ERROR("Failed to create mailbox semaphore");
        hetero_mbox_error_count++;
        vTaskDelete(NULL);
    }
    mbox_handle = mbox_open_client();
    if (mbox_handle == NULL)
    {
        ERROR("Error opening mailbox client handle");
        hetero_mbox_error_count++;
        vTaskDelete(NULL);
    }

    (void)mbox_set_callback(mbox_handle, mbox_stress_callback);

    while (!hetero_mbox_stop_requested)
    {
        hetero_mbox_current_core = ulGetPhysicalCoreMpidr();
        smc_fid = SIP_SMC_HWMON_VOLT;
        channel = VCC_CHANNEL;
        ret = sip_svc_send(mbox_handle, smc_fid, &channel,
                           sizeof(channel), resp_data, sizeof(resp_data));
        if (ret == 0)
        {
            ret = osal_semaphore_wait(mbox_sem, MBOX_XFER_TIMEOUT);
            if (ret == pdTRUE)
            {
                if (resp_data[0] != 0)
                {
                    ERROR("Error in mailbox command:%ld", resp_data[0]);
                    hetero_mbox_error_count++;
                    continue;
                }
                mbox_state.voltActual = (float)resp_data[1] / 65536;
            }
            else
            {
                ERROR("Failed to get mailbox callback");
                hetero_mbox_error_count++;
                continue;
            }
        }
        else
        {
            ERROR("SDM mailbox error");
            hetero_mbox_error_count++;
            continue;
        }

        smc_fid = SIP_SMC_HWMON_TEMP;
        channel = READ_PEAK_TEMPERATURE;
        ret = sip_svc_send(mbox_handle, smc_fid, &channel,
                           sizeof(channel), resp_data, sizeof(resp_data));
        if (ret == 0)
        {
            ret = osal_semaphore_wait(mbox_sem, MBOX_XFER_TIMEOUT);
            if (ret == pdTRUE)
            {
                if (resp_data[0] != 0)
                {
                    ERROR("Error in mailbox command:%ld", resp_data[0]);
                    hetero_mbox_error_count++;
                    continue;
                }
                mbox_state.tempActual = (float)resp_data[1] / 256;
                hetero_mbox_success_count++;
            }
            else
            {
                ERROR("Failed to get mailbox callback");
                hetero_mbox_error_count++;
                continue;
            }
        }
        else
        {
            ERROR("SDM mailbox error");
            hetero_mbox_error_count++;
            continue;
        }
        osal_task_delay(10);
    }

    ret = mbox_close_client(mbox_handle);
    if (ret != 0)
    {
        ERROR("Failed to close mailbox client");
    }

    hetero_mbox_stopped = true;
    vTaskDelete(NULL);
}

void hetero_mbox_request_stop(void)
{
    hetero_mbox_stop_requested = true;
}

void hetero_mbox_print_status(void)
{
    PRINT("Mailbox: physical core 0x%lx ok %lu errors %lu",
          (unsigned long)hetero_mbox_current_core,
          (unsigned long)hetero_mbox_success_count,
          (unsigned long)hetero_mbox_error_count);
}
