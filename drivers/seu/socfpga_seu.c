/*
 * SPDX-FileCopyrightText: Copyright (C) 2025 Altera Corporation
 *
 * SPDX-License-Identifier: MIT-0
 *
 * Driver implementation for SEU driver
 */

/* Single event upsets (SEUs) are rare and unintended changes in the internal memory
 * elements of an FPGA caused by cosmic radiation. The memory state change is a soft
 * error with no permanent damage but the FPGA may operate erroneously until
 * background scrubbing fixes the upset.
 */

#include <stdint.h>
#include <string.h>
#include <errno.h>
#include "osal_log.h"
#include "osal.h"
#include "socfpga_seu.h"
#include "socfpga_mbox_client.h"
#include "socfpga_cache.h"
#include "socfpga_interrupt.h"

#define SEU_READ_ERR_CMD           0x3c
#define SEU_READ_ERR_RESP		   16U
#define SEU_INSERT_ECC_ERR_CMD     0x42
#define SEU_INSERT_ECC_ERR_LEN	   1U
#define SEU_INSERT_ECC_ERR_RESP    4U
#define SEU_INSERT_SAFE_ERR_CMD    0x41
#define SEU_INSERT_SAFE_ERR_LEN	   2
#define SEU_INSERT_SAFE_ERR_RESP   4
#define SEU_READ_STATS             0x40
#define SEU_READ_STATS_LEN		   1
#define SEU_READ_STATS_RESP		   28U

/*This timeout is calculated through trial and error*/
#define COMMAND_TIMEOUT    2000

#define SEU_INSERT_SAFE_ARG_SIZE  8U
#define SEU_READ_STAT_RESP		  36U

/*NOTE:enabling seu interrupt only for seu injections*/
struct seu_context
{
    osal_semaphore_def_t seu_semphr_def;
    osal_semaphore_t seu_semphr;
	sdm_client_handle pxSeuHandle;
    seu_call_back_t seu_call_back;
};

struct seu_context seu_descriptor;

void seu_irq_handler()
{
    if (seu_descriptor.seu_call_back != NULL)
    {
        seu_descriptor.seu_call_back();
    }
    /*NOTE:This is a work-around,implement a better method*/
    interrupt_spi_disable(eSdmHpsSpareIntr0);
}

void seu_mailbox_complete( uint64_t *resp_data )
{
    (void) resp_data;
    osal_semaphore_post(seu_descriptor.seu_semphr);
}

void seu_set_call_back(seu_call_back_t call_back)
{
    seu_descriptor.seu_call_back = call_back;
}

int32_t seu_init()
{
	int32_t ret;

	ret = mbox_init();
	if( ret != 0 )
	{
		ERROR("Cannot initialise mailbox");
		return -EIO;
	}
    seu_descriptor.seu_semphr = osal_semaphore_create(
            &seu_descriptor.seu_semphr_def );
    seu_descriptor.pxSeuHandle = mbox_open_client();
    if (seu_descriptor.pxSeuHandle == NULL)
    {
        ERROR("Failed to open mbox client");
        return -EIO;
    }
    interrupt_register_isr(eSdmHpsSpareIntr0, seu_irq_handler, NULL);
    return 0;
}

int32_t seu_insert_safe_err(seu_err_params_t err_params)
{
    int ret;
    uint32_t *mbox_args;
    uint64_t smc_resp[2];
    if (seu_descriptor.pxSeuHandle == NULL)
    {
        ERROR("Error opening mailbox client");
        return -EIO;
    }
    /*preserve bit fields of other params*/
    if ((err_params.injection_cycle >= 4) ||
            (err_params.cram_sel0 > 0xF) ||
            (err_params.cram_sel1 > 0xF) ||
            (err_params.no_of_injection > 0xF))
    {
        ERROR("Invalid  parameters");
        return -EINVAL;
    }

    /*check for valid cram combinations for basic error injections*/
    if ((err_params.cram_sel0  == err_params.cram_sel1) &&
            (err_params.no_of_injection))
    {
        ERROR("Invalid  parameters");
        return -EINVAL;
    }
	
	mbox_args = pvPortMalloc(SEU_INSERT_SAFE_ARG_SIZE);
	if (mbox_args == NULL)
	{
		ERROR("Failed to allocate memory");
		return -EIO;
	}
    /*prepare parameter set for error injection*/
    interrupt_enable(eSdmHpsSpareIntr0, GIC_INTERRUPT_PRIORITY_SEU);
    mbox_args[ 0 ] =
            (err_params.sector_addr <<
            16) |
            (err_params.injection_cycle << 4) | (err_params.no_of_injection);
    mbox_args[ 1 ] =(err_params.cram_sel0) |
            (err_params.cram_sel1 << 4);
    ret = mbox_set_callback(seu_descriptor.pxSeuHandle,
            seu_mailbox_complete );
    if (ret != 0)
    {
        ERROR("SDM mailbox error");
        return -EIO;
    }
	
	cache_force_write_back(mbox_args, SEU_INSERT_SAFE_ARG_SIZE);
    ret = mbox_send_command(seu_descriptor.pxSeuHandle, 
        SEU_INSERT_SAFE_ERR_CMD, mbox_args, 
        SEU_INSERT_SAFE_ARG_SIZE, NULL, 0, 
        smc_resp, sizeof(smc_resp));

    if (ret != 0)
    {
        ERROR("SDM mailbox error");
        vPortFree(mbox_args);
		return -EIO;
    }
    ret = osal_semaphore_wait(seu_descriptor.seu_semphr,
            COMMAND_TIMEOUT );
    if (ret != pdTRUE)
    {
        return -EIO;
    }

	vPortFree(mbox_args);
    return 0;
}

read_err_data_t seu_read_err()
{
    int ret;
    read_err_data_t err_data;
    uint32_t *mbox_resp;
    uint64_t smc_resp[2];

    ret = mbox_set_callback(seu_descriptor.pxSeuHandle,
            seu_mailbox_complete );
    memset( &err_data, 0, sizeof(read_err_data_t));
    if (ret != 0)
    {
        ERROR("SDM mailbox error");
        err_data.op_state = -EIO;
        return err_data;
    }
	mbox_resp = pvPortMalloc(SEU_READ_ERR_RESP);
	if (mbox_resp == NULL)
	{
		ERROR("Failed to allocate memory");
		return err_data;
	}
	cache_force_invalidate(mbox_resp, SEU_READ_ERR_RESP);
    ret = mbox_send_command(seu_descriptor.pxSeuHandle, 
        SEU_READ_ERR_CMD, NULL, 
        0, mbox_resp, sizeof(mbox_resp), 
        smc_resp, sizeof(smc_resp));
    if (ret != 0)
    {
		vPortFree(mbox_resp);
        ERROR("SDM mailbox error");
        err_data.op_state = -EIO;
        return err_data;
    }
    ret = osal_semaphore_wait(seu_descriptor.seu_semphr,
            COMMAND_TIMEOUT);
    if (ret != pdTRUE)
    {
		vPortFree(mbox_resp);
        ERROR("SDM mailbox error");
        err_data.op_state = -EIO;
        return err_data;
    }
    /*prepare and structure read error data*/
	cache_force_invalidate(mbox_resp, SEU_READ_ERR_RESP);
    err_data.op_state = 0;
    err_data.err_cnt = mbox_resp[ 0 ];
    err_data.sector_addr =
            (mbox_resp[ 1 ] >> 16) & 0xFF;
    err_data.err_type = (mbox_resp[ 2 ] >> 29) & 0x7;
    err_data.node_specific_status = mbox_resp[2] &
            0x7FF;
    err_data.correction_status = mbox_resp[2] &
            (1 << 28);

	vPortFree(mbox_resp);
    return err_data;
}

seu_stat_t seu_read_stat(uint8_t sec_addr)
{
    int ret;
    uint32_t mbox_arg, *mbox_resp;
    seu_stat_t seu_read_stats;
    uint64_t smc_resp[2];

    memset(&seu_read_stats,0, sizeof(seu_stat_t));
	
    mbox_arg = sec_addr << 16;
    ret = mbox_set_callback(seu_descriptor.pxSeuHandle,
            seu_mailbox_complete );
    if (ret != 0)
    {
        ERROR("SDM mailbox error");
        seu_read_stats.op_state = -EIO;
        return seu_read_stats;
    }

	mbox_resp = pvPortMalloc(SEU_READ_STAT_RESP);
	if (mbox_resp == NULL)
	{
		ERROR("Failed to allocate memory");
		return seu_read_stats;
	}
	cache_force_write_back(&mbox_arg, sizeof(mbox_arg));
	cache_force_invalidate(mbox_resp, SEU_READ_STAT_RESP);
    
	ret = mbox_send_command(seu_descriptor.pxSeuHandle, 
        SEU_READ_STATS, &mbox_arg, sizeof(mbox_arg),  
        mbox_resp, sizeof(mbox_resp), 
        smc_resp, sizeof(smc_resp));
    if (ret != 0)
    {
        ERROR("SDM mailbox error");
        seu_read_stats.op_state = -EIO;
		vPortFree(mbox_resp);
        return seu_read_stats;
    }
    ret = osal_semaphore_wait(seu_descriptor.seu_semphr,
            COMMAND_TIMEOUT );
    if (ret != pdTRUE)
    {
        ERROR("SDM mailbox error");
        seu_read_stats.op_state = -EIO;
		vPortFree(mbox_resp);
        return seu_read_stats;
    }
	cache_force_invalidate(mbox_resp, SEU_READ_STAT_RESP);
    /*prepare and structure read error stats*/
    seu_read_stats.op_state = 0;
    seu_read_stats.t_seu_cycle =  mbox_resp[ 0 ];
    seu_read_stats.t_seu_detect =  mbox_resp[ 1 ];
    seu_read_stats.t_seu_correct =  mbox_resp[ 2 ];
    seu_read_stats.t_seu_inject_detect =
            mbox_resp[ 3 ];
    seu_read_stats.t_sdm_seu_poll_interval =
            mbox_resp[ 4 ];

	vPortFree(mbox_resp);
    return seu_read_stats;
}

int32_t seu_insert_ecc_err(uint8_t err_type,uint8_t ram_id, uint8_t sector_addr)
{
    int ret;
    uint32_t mbox_arg;
    uint64_t smc_resp[2];

    if (seu_descriptor.pxSeuHandle == NULL)
    {
        ERROR("Error opening mailbox client");
        return -EIO;
    }
    /*preserve bit fields of other params*/
    if ((err_type > 2) || (ram_id > 0x1F))
    {
        ERROR("Invalid parameters");
        return -EINVAL;
    }

    /*prepare parameter set for error injection*/
    mbox_arg =
            (sector_addr << 16) | (ram_id << 2) | (err_type);
    ret = mbox_set_callback(seu_descriptor.pxSeuHandle,
            seu_mailbox_complete);
    if (ret != 0)
    {
        ERROR("SDM mailbox error");
        return -EIO;
    }
	cache_force_write_back(&mbox_arg, sizeof(mbox_arg));

    ret = mbox_send_command(seu_descriptor.pxSeuHandle, 
        SEU_INSERT_ECC_ERR_CMD, &mbox_arg, sizeof(mbox_arg),  
        NULL, 0, smc_resp, sizeof(smc_resp));
    if (ret != 0)
    {
        ERROR("SDM mailbox error");
        return -EIO;
    }
    ret = osal_semaphore_wait(seu_descriptor.seu_semphr,
            COMMAND_TIMEOUT);
    if (ret != pdTRUE)
    {
        return -EIO;
    }
    return 0;
}

int32_t seu_deinit()
{
    int32_t ret;
    ret = mbox_close_client(seu_descriptor.pxSeuHandle);
    if (ret != 0)
    {
        ERROR("Failed to close MBOX client");
        return -EIO;
    }
    ret = mbox_deinit();
    if (ret != 0)
    {
        ERROR("Failed to close MBOX");
        return -EIO;
    }
    return 0;
}
