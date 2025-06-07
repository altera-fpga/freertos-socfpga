/*
 * SPDX-FileCopyrightText: Copyright (C) 2025 Altera Corporation
 *
 * SPDX-License-Identifier: MIT-0
 *
 * HAL driver implementation for reboot
 */

#include "socfpga_defines.h"
#include "socfpga_reboot_mngr.h"
#include "socfpga_sip_handler.h"
#include "osal_log.h"

#define COLD_REBOOT_CMD    0x84000009U
#define WARM_REBOOT_CMD    0x84000012U

static reboot_callback_t cold_boot_callback = NULL;
static reboot_callback_t warm_boot_callback = NULL;

uint32_t reboot_mngr_set_callback( reboot_callback_t call_back, uint32_t event )
{
    uint32_t ret = REBOOT_SUCCESS;

    if (event == COLD_REBOOT)
    {
        cold_boot_callback = call_back;
    }
    else if (event == WARM_REBOOT)
    {
        warm_boot_callback = call_back;
    }
    else
    {
        ret = REBOOT_FAILURE;
    }

    return ret;
}

uint32_t reboot_mngr_do_reboot( uint32_t event )
{
    uint32_t reset_command = COLD_REBOOT_CMD;
    uint32_t ret = REBOOT_SUCCESS;
    uint64_t mail_box_args[3]={ 0U, 0U, 0U };

    switch (event)
    {
    case COLD_REBOOT:
        reset_command = COLD_REBOOT_CMD;
        if (cold_boot_callback != NULL)
        {
            cold_boot_callback(NULL);
        }
        break;

    case WARM_REBOOT:
        reset_command = WARM_REBOOT_CMD;
        if (warm_boot_callback != NULL)
        {
            warm_boot_callback(NULL);
        }
        break;

    default:
        ret = REBOOT_FAILURE;
        ERROR("\r\n Not a valid reboot option\r\n");
        break;
    }

    if (ret == REBOOT_SUCCESS)
    {
        ret = smc_call(reset_command, mail_box_args);
    }

    return ret;
}
