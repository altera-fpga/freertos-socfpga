/*
 * SPDX-FileCopyrightText: Copyright (C) 2025 Altera Corporation
 *
 * SPDX-License-Identifier: MIT-0
 *
 * HAL implementation for fpga manager
 */

#include <string.h>
#include "osal.h"
#include "osal_log.h"
#include "socfpga_sip_handler.h"
#include "socfpga_mbox_client.h"
#include "socfpga_fpga_manager.h"

static int send_fpga_bitstream( void *rbf_data, size_t rbf_size )
{
    int i;
    uint64_t res_buf[3];
    uint64_t sdm_args[2];
    uint32_t xfer_count = 0;
    int ret, smc_ret = 0, retry = 0;
    size_t buf_size = (rbf_size > BITSTREAM_CHUNK_SIZE) ? BITSTREAM_CHUNK_SIZE : rbf_size;

    ret = 0;

    while (rbf_size || xfer_count)
    {
        if (!smc_ret && rbf_size)
        {
            sdm_args[0] = (uint64_t)rbf_data;
            sdm_args[1] = buf_size;

            smc_ret = smc_call(FPGA_CONFIG_WRITE, sdm_args);
            if (smc_ret != SMC_CMD_SUCCESS && smc_ret != SMC_STATUS_BUSY)
            {
                return smc_ret;
            }
            rbf_size -= buf_size;
            rbf_data += buf_size;
            if (buf_size >= rbf_size)
            {
                buf_size = rbf_size;
            }
            xfer_count++;
            puts(".\r");
            fflush(stdout);
        }
        else
        {
            ret = smc_call(FPGA_CONFIG_WRITE_COMPLETE, res_buf);
            if (!ret) {
                for (i = 0; i < 3; i++) {
                    if (!res_buf[i])
                        break;
                    xfer_count--;
                    smc_ret = 0;
                    retry = 0;
                }
            }
            else if (ret !=
                    SMC_STATUS_BUSY)
                return ret;
            else if (!xfer_count)
                return SMC_STATUS_ERROR;

            if (++retry >= RECONFIG_STATUS_POLL_RETRY_MAX)
            {
                return TIME_OUT_ERR;
            }
            osal_delay_ms(20);
        }
    }
    return ret;
}

static int configure_fpga(uint8_t *rbf_ptr, uint32_t file_size)
{
    uint64_t sdm_args[8];
    int ret;

    /* clear all arguments for CONFIG_START command */
    memset(sdm_args, 0, sizeof(sdm_args));
    sdm_args[0] = 0;

    ret = smc_call(FPGA_CONFIG_START, sdm_args);
    if( ret == SMC_CMD_SUCCESS )
    {
        ret = send_fpga_bitstream(rbf_ptr, file_size);
        switch(ret)
        {
            case SMC_STATUS_BUSY :
                ERROR("SMC status busy");
                return RET_FAIL;

            case SMC_STATUS_REJECTED :
                ERROR("SMC status rejected");
                return RET_FAIL;

            case TIME_OUT_ERR:
                ERROR("Timeout occured");
                return RET_FAIL;

            case SMC_STATUS_ERROR:
                ERROR("SMC status error");
                return RET_FAIL;

            case SMC_CMD_SUCCESS:
                return RET_OK;
        }
    }
    else
    {
        ERROR("FPGA_CONFIG_START command failed");
        return RET_FAIL;
    }
    return RET_OK;
}

static int reconfig_status(void)
{
    int smc_ret, tries = 10;
    uint64_t sdm_args[ 8 ];
    while ( tries > 0 )
    {
        memset(sdm_args, 0, sizeof(sdm_args));
        smc_ret = smc_call(FPGA_CONFIG_ISDONE, sdm_args);

        /* configuration success */
        if ( smc_ret == STATUS_OK )
        {
            return STATUS_OK;
        }

        /* configuration failed */
        if ( smc_ret != 1 )
        {
            return smc_ret;
        }

        /*mailbox busy */
        tries--;

        osal_delay_ms(RECONFIG_DELAY_MS);
        puts(".\r");
    }

    return TIME_OUT_ERR;
}

int load_fpga_bitstream(uint8_t *rbf_ptr, uint32_t rbf_file_size)
{
    int resp_data;

    if( configure_fpga(rbf_ptr, rbf_file_size) == RET_FAIL )
    {
        ERROR("Failed to configure the FPGA");
        return RET_FAIL;
    }

    /* Make sure we don't send MBOX_RECONFIG_STATUS too fast */
    osal_delay_ms(RECONFIG_DELAY_MS);

    resp_data = reconfig_status();
    if( resp_data == SMC_STATUS_ERROR )
    {
        ERROR("Failed to configure the FPGA");
        return RET_FAIL;
    }

    return RET_OK;
}
