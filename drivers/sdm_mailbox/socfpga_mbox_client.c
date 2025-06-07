/*
 * SPDX-FileCopyrightText: Copyright (C) 2025 Altera Corporation
 *
 * SPDX-License-Identifier: MIT-0
 *
 * HAL Implementation for SDM mailbox
 */

#include <stdint.h>
#include <string.h>
#include <errno.h>

#include "socfpga_sip_handler.h"
#include "socfpga_mbox_client.h"
#include "socfpga_interrupt.h"
#include "osal.h"
#include "osal_log.h"
#include "socfpga_cache.h"
#include "socfpga_defines.h"

#define MAX_CLIENT_INSTANCES       16U
#define MAX_JOB_ID                 16U
#define JOB_POOL_FULL              0xFFFFU
#define SIP_SMC_GET_RESP           0x420000C8U
#define SIP_SMC_GET_TRANS_ID       0x420000C9U
#define SIP_GENERIC_MAILBOX_CMD    0x420000EEU
#define FORMAT_TRANS_ID(cid, jid)            \
    ((((uint64_t) (cid) & 0xFU) << 4) |      \
    (((uint64_t) (jid) & 0xFU)))
#define GET_CLIENT_ID(trans_id)    ((trans_id) & 0xF0U) >> 4
#define GET_JOB_ID(trans_id)       ((trans_id) & 0xFU)
#define ATF_CLIENT_ID       1U
#define MBOX_TASK_CLOSED    0U
#define MBOX_TASK_OPEN      1U
#define MBOX_TASK_BUSY      2U

typedef struct
{
    uint64_t *resp_data;
    uint64_t resp_len;
}job_id_resp_map;
struct sdm_client_descriptor
{
    uint16_t job_pool;
    uint8_t job_head;
    uint8_t client_status;
    mbox_call_back_t call_back;
    osal_mutex_t client_mutex;
    osal_semaphore_t client_free;
    job_id_resp_map job_resp[MAX_JOB_ID];
};
static struct sdm_mbox_descriptor
{
    osal_mutex_t client_list_mutex;
    osal_semaphore_t call_complete_sem;
    uint8_t task_state;
} *mbox_descriptor;
static struct sdm_client_descriptor client_descriptors[ MAX_CLIENT_INSTANCES ];

void mbox_poll_resp_task( void *param );

void mbox_irq_handler(void*param);

static uint8_t assign_job_id( sdm_client_handle mbox_handle )
{
    uint8_t i = 0;
    if ((mbox_handle == NULL) || (mbox_handle->job_pool == (JOB_POOL_FULL)))
    {
        ERROR("Maximum allowed jobs running");
        return 0xFFU;
    }
    if (osal_mutex_lock(mbox_descriptor->client_list_mutex,
            OSAL_TIMEOUT_WAIT_FOREVER) == pdTRUE)
    {
        i = mbox_handle->job_head;
        /*Find free job_id*/
        while (((mbox_handle->job_pool) & (((uint16_t) 0x1U << i))) != 0U)
        {
            i++;
            i %= MAX_JOB_ID;
        }
        mbox_handle->job_pool |= ((uint16_t) 1U << i);
        mbox_handle->job_head = (i + 1U) % MAX_JOB_ID;
        if (osal_mutex_unlock(mbox_descriptor->client_list_mutex) == false)
        {
            ERROR("Failed to unlock mutex");
            return 0xFFU;
        }
    }
    return i;
}

static int8_t free_job_id( uint8_t client_id, uint8_t ucJobId )
{
    sdm_client_handle mbox_handle = &client_descriptors[client_id];
    if ((mbox_handle == NULL) || (mbox_handle->job_pool == 0U))
    {
        return -EIO;
    }
    if (osal_mutex_lock(mbox_descriptor->client_list_mutex,
            OSAL_TIMEOUT_WAIT_FOREVER) == pdTRUE)
    {
        mbox_handle->job_pool ^= ((uint16_t) 1U << ucJobId);
        if (osal_mutex_unlock(mbox_descriptor->client_list_mutex) == false)
        {
            ERROR("Failed to unlock mutex");
            return -EIO;
        }
    }
    return 0;
}

void mbox_poll_resp_task( void *param )
{
    (void) param;

    sdm_client_handle mbox_handle;
    uint64_t bitmask[4], smc_args[12];
    uint8_t trans_id, i, j;
    uint8_t client_id, job_id;
    int ret;
    cache_force_write_back(smc_args, sizeof(smc_args));
    cache_force_invalidate( smc_args, sizeof(smc_args));
    for ( ;;)
    {
        if (osal_semaphore_wait(mbox_descriptor->call_complete_sem,
                OSAL_TIMEOUT_WAIT_FOREVER) == pdTRUE)
        {
            if (mbox_descriptor->task_state == MBOX_TASK_CLOSED)
            {
                break;
            }
            /* response is available to poll */
            (void) memset(bitmask, 0,sizeof(bitmask));
            (void) memset(smc_args, 0, sizeof(smc_args));
            ret = smc_call( SIP_SMC_GET_TRANS_ID, bitmask );
            if (ret == 0)
            {
                /* finding the transaction id corresponding to the response */
                for (i = 0U; i < 4U; i++)
                {
                    /*
                     * Transaction Id is obtained as follows from the bit mask
                     *
                     * trans_id = (i * 64) + j;
                     * Where i is the bitmask which contains the response
                     * j is the bit in the bitmask which is high
                     */
                    j = 0U;
                    while (bitmask[i] != 0UL)
                    {
                        trans_id = (i * 64U);
                        /* 64 bits to process */
                        while (j < 64U)
                        {
                            if (((bitmask[i] >> j) & 1UL) == 1UL)
                            {
                                break;
                            }
                            j++;
                        }
                        /* incrementing j so that we dont process
                           the same bit again */
                        trans_id += j;
                        j++;
                        bitmask[i] = bitmask[i] >> j;
                        client_id = GET_CLIENT_ID(trans_id);
                        if (client_id == ATF_CLIENT_ID)
                        {
                            /* Ignoring any transactions belonging to ATF */
                            continue;
                        }
                        mbox_handle = &client_descriptors[client_id];
                        job_id = GET_JOB_ID(trans_id);
                        smc_args[0] = trans_id;
                        ret = smc_call( SIP_SMC_GET_RESP, smc_args);
                        if (ret != 0)
                        {
                            (void) memset(smc_args, 0, sizeof(smc_args));
                            ERROR("Failed to poll response");
                            continue;
                        }
                        if ((mbox_handle->job_resp[job_id].resp_data != NULL) &&
                                (mbox_handle->job_resp[job_id].resp_len != 0UL))
                        {
                            (void) memcpy(
                                    mbox_handle->job_resp[job_id].resp_data,
                                    smc_args,
                                    mbox_handle->job_resp[job_id].resp_len );
                        }
                        if (free_job_id(client_id, job_id) != 0)
                        {
                            ERROR("Failed to free job id");
                            continue;
                        }
                        if (client_descriptors[client_id].call_back != NULL)
                        {
                            client_descriptors[client_id].call_back(smc_args);
                        }
                        if (osal_semaphore_post(mbox_handle->client_free) ==
                                false)
                        {
                            ERROR("Failed to post semaphore");
                        }
                    }
                }
            }
        }
        mbox_descriptor->task_state = MBOX_TASK_OPEN;
        if (interrupt_enable(eSdmApsMailboxIntr, 14) != eErrOk)
        {
            ERROR("Failed to enable interrupt");
            return;
        }
    }
    osal_task_delete();
}

sdm_client_handle mbox_open_client( void )
{
    uint8_t i;
    sdm_client_handle xRetVal = NULL;
    if (osal_mutex_lock(mbox_descriptor->client_list_mutex,
            OSAL_TIMEOUT_WAIT_FOREVER) == pdTRUE)
    {
        for (i = 2U; i < MAX_CLIENT_INSTANCES; i++)
        {
            /*find unused/closed client*/
            if (client_descriptors[ i ].client_status == 0U)
            {
                client_descriptors[ i ].client_status = 1;
                xRetVal = &client_descriptors[ i ];
                client_descriptors[i].client_mutex = osal_mutex_create(
                        NULL);
                client_descriptors[i].client_free = osal_semaphore_create(
                        NULL);
                if (osal_semaphore_post(client_descriptors[i].client_free) ==
                        false)
                {
                    ERROR("Failed to post semaphore");
                    return NULL;
                }
                break;
            }
        }
        if (osal_mutex_unlock(mbox_descriptor->client_list_mutex) == false)
        {
            ERROR("Failed to unlock mutex");
            return NULL;
        }
    }
    return xRetVal;
}

int32_t mbox_close_client( sdm_client_handle mbox_handle )
{
    if ((mbox_handle == NULL) || (mbox_handle->client_status == 0U))
    {
        return -EINVAL;
    }
    /*check if it has any outstanding jobs*/
    if (mbox_handle->job_pool != 0U)
    {
        return -EIO;
    }
    if (osal_mutex_lock(mbox_descriptor->client_list_mutex,
            OSAL_TIMEOUT_WAIT_FOREVER) == pdTRUE)
    {
        if (osal_mutex_lock( mbox_handle->client_mutex,
                OSAL_TIMEOUT_WAIT_FOREVER ) == pdTRUE)
        {
            mbox_handle->job_pool = 0;
            mbox_handle->job_head = 0;
            mbox_handle->client_status = 0;
            if (osal_semaphore_delete(mbox_handle->client_free) == false)
            {
                ERROR("Failed to delete semaphore");
                return -EIO;
            }
            (void) memset(mbox_handle->job_resp, 0,
                    sizeof(mbox_handle->job_resp));
            if (osal_mutex_unlock( mbox_handle->client_mutex ) == false)
            {
                ERROR("Failed to unlock mutex");
                return -EIO;
            }
        }
        if (osal_mutex_unlock(mbox_descriptor->client_list_mutex) == false)
        {
            ERROR("Failed to unlock mutex");
            return -EIO;
        }
        if (osal_mutex_delete(mbox_handle->client_mutex) == false)
        {
            ERROR("Failed to delete mutex");
            return -EIO;
        }
    }
    return 0;
}

int32_t mbox_send_command(sdm_client_handle mbox_handle,
        uint32_t command, uint32_t *command_args,
        uint32_t arg_size, uint32_t *resp, uint32_t resp_size,
        uint64_t *smc_resp, uint32_t smc_resp_len)
{
    uint64_t mbox_smc_args[5];

    if (mbox_handle == NULL)
    {
        return -EINVAL;
    }
    if (arg_size % 4 || resp_size % 4)
    {
        ERROR("Invalid size. Not multiple of 4 bytes");
        return -EINVAL;
    }

    if ((command_args != NULL) && (arg_size != 0))
    {
        cache_force_write_back(command_args, arg_size);
    }
    if ((resp != NULL) || (resp_size != 0))
    {
        cache_force_invalidate(resp, resp_size);
    }
    mbox_smc_args[0] = command;
    mbox_smc_args[1] = (uint64_t)(uintptr_t)command_args;
    mbox_smc_args[2] = arg_size;
    mbox_smc_args[3] = (uint64_t)(uintptr_t)resp;
    mbox_smc_args[4] = resp_size;

    return sip_svc_send(mbox_handle, SIP_GENERIC_MAILBOX_CMD,
            mbox_smc_args, sizeof(mbox_smc_args),
            smc_resp, smc_resp_len);
}
int32_t sip_svc_send( sdm_client_handle mbox_handle,
        uint64_t smc_func_id, uint64_t *mbox_args, uint32_t arg_len,
        uint64_t *resp_data, uint32_t resp_len )
{
    uint64_t smc_values[12];
    uint8_t client_id = 0xFFU, job_id, i;
    int ret;

    if ((mbox_handle == NULL) || (mbox_handle->client_status == 0U) ||
            ((mbox_args != NULL) && (arg_len == 0)) ||
            ((mbox_args == NULL) && (arg_len != 0)))
    {
        return -EIO;
    }
    else
    {
        for (i = 2; i < MAX_CLIENT_INSTANCES; i++)
        {
            if (memcmp((const void*) mbox_handle,
                    (const void*) &client_descriptors[i],
                    sizeof(client_descriptors[i])) == 0)
            {
                client_id = i;
                break;
            }
        }
        if (client_id == 0xFFU)
        {
            ERROR("Failed to find client");
            return -EIO;
        }
    }
    if (mbox_handle->call_back == NULL)
    {
        WARN("No callback registered");
    }
    if (osal_semaphore_wait( mbox_handle->client_free,
            OSAL_TIMEOUT_WAIT_FOREVER ) == pdTRUE)
    {
        if (osal_mutex_lock( mbox_handle->client_mutex,
                OSAL_TIMEOUT_WAIT_FOREVER ) == pdTRUE)
        {
            job_id = assign_job_id( mbox_handle );
            if (job_id >= MAX_JOB_ID)
            {
                ERROR("Failed to assign job id");
                return -EIO;
            }
            mbox_handle->job_resp[job_id].resp_data = resp_data;
            mbox_handle->job_resp[job_id].resp_len = resp_len;

            smc_values[0] = FORMAT_TRANS_ID( client_id, job_id );
            if (mbox_args != NULL)
            {
                (void) memcpy(&smc_values[1], mbox_args, arg_len);
            }
            ret = smc_call( smc_func_id, smc_values );
            if ((resp_data == NULL) || (resp_len == 0U) || (ret != 0))
            {
                if (free_job_id(client_id, job_id) != 0)
                {
                    ERROR("Failed to free job id");
                    return -EIO;
                }
                /* no response so immediately release semaphore */
                if (osal_semaphore_post(mbox_handle->client_free) == false)
                {
                    ERROR("Failed to post semaphore");
                    return -EIO;
                }
                if (ret != 0)
                {
                    ret = -ret;
                }
            }
            if (osal_mutex_unlock( mbox_handle->client_mutex ) == false)
            {
                ERROR("Failed to unlock mutex");
                return -EIO;
            }
        }
    }
    return ret;
}

int32_t mbox_set_callback( sdm_client_handle mbox_handle,
        mbox_call_back_t pxCallback )
{
    if (mbox_handle == NULL)
    {
        return -EIO;
    }
    else
    {
        if (osal_mutex_lock( mbox_handle->client_mutex,
                OSAL_TIMEOUT_WAIT_FOREVER ) == pdTRUE)
        {
            /*pxCallback can also be NULL*/
            mbox_handle->call_back = pxCallback;
        }
        if (osal_mutex_unlock( mbox_handle->client_mutex ) == false)
        {
            ERROR("Failed to unlock mutex");
            return -EIO;
        }
    }
    return 0;
}
int mbox_init( void )
{
    if (interrupt_register_isr( eSdmApsMailboxIntr, mbox_irq_handler,
            NULL ) != eErrOk)
    {
        ERROR("Failed to register interrupt");
        return -EIO;
    }
    if (interrupt_enable( eSdmApsMailboxIntr, 14) != eErrOk)
    {
        ERROR("Failed to enable interrupt");
        return -EIO;
    }

    if (mbox_descriptor == NULL)
    {
        mbox_descriptor = pvPortMalloc(sizeof(struct sdm_mbox_descriptor));
        mbox_descriptor->client_list_mutex = osal_mutex_create(NULL);
        mbox_descriptor->call_complete_sem = osal_semaphore_counting_create(
                NULL,
                MAX_CLIENT_INSTANCES, 0);
        if ((mbox_descriptor->client_list_mutex == NULL) ||
                (mbox_descriptor->call_complete_sem == NULL))
        {
            return -EIO;
        }
        if (osal_task_create(mbox_poll_resp_task, "Mailbox_Task", NULL,
                3) == false)
        {
            ERROR("Failed to create mailbox task");
            return -EIO;
        }
        mbox_descriptor->task_state = MBOX_TASK_OPEN;
    }
    else
    {
        INFO("Mailbox already initialised");
    }
    return 0;
}
int mbox_deinit(void)
{
    if (mbox_descriptor == NULL)
    {
        ERROR("Mailbox not initialised");
        return -EIO;
    }
    if (mbox_descriptor->task_state == MBOX_TASK_CLOSED)
    {
        ERROR("Task not initialised");
        return -EIO;
    }
    if (mbox_descriptor->task_state == MBOX_TASK_BUSY)
    {
        ERROR("Task is performing some function, cannot close");
        return -EIO;
    }

    if (interrupt_spi_disable(eSdmApsMailboxIntr) != eErrOk)
    {
        ERROR("Failed to disable interrupt");
        return -EIO;
    }
    /* posting semaphore so the task is not in a waiting state */
    mbox_descriptor->task_state = MBOX_TASK_CLOSED;
    if (osal_semaphore_post(mbox_descriptor->call_complete_sem) == false)
    {
        ERROR("Failed to post semaphore");
        return -EIO;
    }
    if (osal_mutex_delete(mbox_descriptor->client_list_mutex) == false)
    {
        ERROR("Failed to delete mutex");
        return -EIO;
    }
    if (osal_semaphore_delete(mbox_descriptor->call_complete_sem) == false)
    {
        ERROR("Failed to delete semaphore");
        return -EIO;
    }

    (void) memset(mbox_descriptor, 0, sizeof(struct sdm_mbox_descriptor));
    vPortFree(mbox_descriptor);
    mbox_descriptor = NULL;

    return 0;
}

void mbox_irq_handler(void*param)
{
    (void) param;
    /*
     * disable the interrupt as the interrupt keeps getting hit while a
     * response is available
     */
    if (interrupt_spi_disable(eSdmApsMailboxIntr) != eErrOk)
    {
        ERROR("Failed to disable interrupt");
        return;
    }
    mbox_descriptor->task_state = MBOX_TASK_BUSY;
    if (osal_semaphore_post( mbox_descriptor->call_complete_sem ) == false)
    {
        ERROR("Failed to post semaphore");
    }
}
