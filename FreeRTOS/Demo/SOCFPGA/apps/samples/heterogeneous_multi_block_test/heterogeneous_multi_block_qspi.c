/*
 * SPDX-FileCopyrightText: Copyright (C) 2026 Altera Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 * QSPI multi block task
 */

#include "FreeRTOS.h"
#include "task.h"
#include "osal_log.h"
#include "socfpga_flash.h"
#include "socfpga_cache.h"

#include <string.h>

#define HET_STRESS_QSPI_BUF_SIZE  1024U

struct QspiStressState
{
    flash_handle_t qspi_handle;
};

static struct QspiStressState qspi_state;
static volatile uint32_t hetero_qspi_error_count = 0;
static volatile uint32_t hetero_qspi_success_count = 0;
static volatile uint32_t hetero_qspi_current_core = 0;
static volatile bool hetero_qspi_stop_requested = false;
static volatile bool hetero_qspi_stopped = false;

static uint8_t qspi_mem_addr = 0x0;

extern uint32_t ulGetPhysicalCoreMpidr(void);

void hetero_qspi_task(void *arg)
{
    (void)arg;
    int lRet = 0;

    uint8_t qspi_tx_buf[HET_STRESS_QSPI_BUF_SIZE];
    uint8_t qspi_rx_buf[HET_STRESS_QSPI_BUF_SIZE];

    qspi_state.qspi_handle = flash_open(0);
    if (qspi_state.qspi_handle == NULL)
    {
        ERROR("Failed to open QSPI instance");
        hetero_qspi_error_count++;
        vTaskDelete(NULL);
        return;
    }

    for (uint32_t i = 0; i < HET_STRESS_QSPI_BUF_SIZE; i++)
    {
        qspi_tx_buf[i] = i % 256;
    }

    cache_force_write_back((void *)qspi_tx_buf, sizeof(qspi_tx_buf));

    for (int i = 0; i < 5; i++)
    {
        lRet = flash_erase_sectors(qspi_state.qspi_handle,
                qspi_mem_addr, HET_STRESS_QSPI_BUF_SIZE);
        if (lRet < 0)
        {
            ERROR("Erase failed for erase number : %d", i);
            hetero_qspi_error_count++;
            break;
        }
        lRet = flash_write_sync(qspi_state.qspi_handle, qspi_mem_addr,
                qspi_tx_buf, HET_STRESS_QSPI_BUF_SIZE);
        if (lRet != 0)
        {
            ERROR("Write number : %d failed", i);
            hetero_qspi_error_count++;
            break;
        }

        cache_force_write_back((void *)qspi_tx_buf, sizeof(qspi_tx_buf));
        qspi_mem_addr = qspi_mem_addr + 0x1000;
    }

    qspi_mem_addr = 0x0;

    while (!hetero_qspi_stop_requested)
    {
        hetero_qspi_current_core = ulGetPhysicalCoreMpidr();
        memset(qspi_rx_buf, 0, sizeof(qspi_rx_buf));
        lRet = flash_read_sync(qspi_state.qspi_handle, qspi_mem_addr,
                qspi_rx_buf, HET_STRESS_QSPI_BUF_SIZE);
        if (lRet != 0)
        {
            ERROR("Read number : %lu failed", (unsigned long)hetero_qspi_success_count);
            hetero_qspi_error_count++;
            continue;
        }

        cache_force_invalidate((void *)qspi_rx_buf, sizeof(qspi_rx_buf));

        if (qspi_tx_buf[HET_STRESS_QSPI_BUF_SIZE - 1] !=
                qspi_rx_buf[HET_STRESS_QSPI_BUF_SIZE - 1])
        {
            hetero_qspi_error_count++;
        }

        hetero_qspi_success_count++;
        qspi_mem_addr = ((qspi_mem_addr + 1) % 5) * 0x1000;
    }

    lRet = flash_close(qspi_state.qspi_handle);
    if (lRet != 0)
    {
        ERROR("Failed to close the QSPI handle");
    }

    hetero_qspi_stopped = true;
    vTaskDelete(NULL);
}

void hetero_qspi_request_stop(void)
{
    hetero_qspi_stop_requested = true;
}

void hetero_qspi_print_status(void)
{
    PRINT("QSPI: physical core 0x%lx ok %lu errors %lu",
          (unsigned long)hetero_qspi_current_core,
          (unsigned long)hetero_qspi_success_count,
          (unsigned long)hetero_qspi_error_count);
}
