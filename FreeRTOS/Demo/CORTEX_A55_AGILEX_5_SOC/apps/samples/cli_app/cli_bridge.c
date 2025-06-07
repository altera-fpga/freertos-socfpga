/*
 * SPDX-FileCopyrightText: Copyright (C) 2025 Altera Corporation
 *
 * SPDX-License-Identifier: MIT-0
 *
 * Implementation of CLI commands for HPS bridge
 */

 /**
 * @defgroup cli_bridge Bridge
 * @ingroup cli
 *
 * Perform bridge operations
 *
 * @details
 * It supports the following subcommands:
 * - bridge disable <instance>
 * - bridge enable  <instance>
 * - bridge help
 *
 * Typical usage:
 * - Use 'bridge enable'  to enable a particular bridge.
 * - Use 'bridge disable' to disable a particular bridge.
 *
 * @subsection commands Commands
 * @subsubsection bridge_enable bridge enable
 * Enable the bridge  <br>
 *
 * Usage:  <br>
 *   bridge enable <instance>  <br>
 *
 * It requires the following arguments:
 * - instance - The instance of the bridge. The instances are mapped as below:  <br>
 *   - 0 - HPS2FPGA bridge  <br>
 *   - 1 - LWHPS2FPGA bridge  <br>
 *   - 2 - FPGA2HPS bridge  <br>
 *   - 3 - FPGA2SDRAM bridge  <br>
 *
 * @subsubsection bridge_disable bridge disable
 * Disable the bridge  <br>
 *
 * Usage:  <br>
 *   bridge disable <instance>  <br>
 *
 * It requires the following arguments:
 * - instance - The instance of the bridge. The instances are mapped as below:  <br>
 *   - 0 - HPS2FPGA bridge  <br>
 *   - 1 - LWHPS2FPGA bridge  <br>
 *   - 2 - FPGA2HPS bridge  <br>
 *   - 3 - FPGA2SDRAM bridge  <br>
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <FreeRTOS.h>
#include <task.h>
#include "FreeRTOS_CLI.h"
#include "cli_app.h"
#include "osal_log.h"
#include "socfpga_bridge.h"

BaseType_t cmd_bridge( char *pcWriteBuffer, size_t xWriteBufferLen,
        const char *pcCommandString )
{
    char *spacePos;
    const char *pcParameter1, *pcParameter2;
    BaseType_t xParameter1StringLength, xParameter2StringLength;

    int ret = 0;
    int bridge_num;
    int enable_flag = -1;

    (void) pcWriteBuffer;
    (void) xWriteBufferLen;

    pcParameter1 = FreeRTOS_CLIGetParameter(
            /* The command string itself. */
            pcCommandString,
            /* Return the first parameter. */
            1,
            /* Store the parameter string length. */
            &xParameter1StringLength);

    pcParameter2 = FreeRTOS_CLIGetParameter(
            /* The command string itself. */
            pcCommandString,
            /* Return the first parameter. */
            2,
            /* Store the parameter string length. */
            &xParameter2StringLength);

    spacePos = strchr(pcParameter1, ' ');
    if (spacePos != NULL)
    {
        *spacePos = '\0';
    }

    if( strncmp(pcParameter1, "help", 4) == 0 )
    {
        printf("\r\nPerform bridge operations"
               "\r\n\nIt supports the following subcommands:"
               "\r\n  bridge disable <bridge_number>"
               "\r\n  bridge enable <bridge_number>"
               "\r\n  bridge help"
               "\r\n\nTypical usage:"
               "\r\n - Use enable command to enable a particular bridge."
               "\r\n - Use disable command to disable a particular bridge."
               "\r\n\nFor help on the specific commands please do:"
               "\r\n  bridge <command> help\r\n"
               );

        return pdFALSE;
    }

    if( strncmp(pcParameter1, "enable", 6) == 0 )
    {
        if( strncmp(pcParameter2, "help", 4) == 0 )
        {
            printf("\rEnable the bridge"
                   "\r\n\nUsage:"
                   "\r\n  bridge enable <instance>"
                   "\r\n\nIt requires the following arguments:"
                   "\r\n  instance    The instance of the bridge. The instance are mapped as below:"
                   "\r\n  			  0 - HPS2FPGA bridge"
                   "\r\n    		  1 - LWHPS2FPGA bridge "
                   "\r\n    		  2 - FPGA2HPS bridge"
                   "\r\n    		  3 - FPGA2SDRAM bridge"
                   );

            return pdFALSE;
        }
        enable_flag = 1;
    }
    else if(strncmp(pcParameter1, "disable", 7) == 0 )
    {
        if( strncmp(pcParameter2, "help", 4) == 0 )
        {
            printf("\rDisable the bridge"
                   "\r\n\nUsage:"
                   "\r\n  bridge disable <instance>"
                   "\r\n\nIt requires the following arguments:"
                   "\r\n  instance    The instance of the bridge. The instance are mapped as below:"
                   "\r\n           	  0 - HPS2FPGA bridge"
                   "\r\n              1 - LWHPS2FPGA bridge "
                   "\r\n              2 - FPGA2HPS bridge"
                   "\r\n              3 - FPGA2SDRAM bridge"
                   );

            return pdFALSE;
        }
        enable_flag = 0;
    }
    else
    {
        printf("Invalid bridge Command\r\n");
        printf("Enter 'help' to view a list of available commands.\r\n");
        return pdFALSE;
    }

    bridge_num = atoi(pcParameter2);

    switch(bridge_num)
    {
        case 0 :
            if( enable_flag == 1)
            {
                ret = enable_hps2fpga_bridge();
            }
            else
            {
                ret = disable_hps2fpga_bridge();
            }
            break;

        case 1 :
            if( enable_flag == 1)
            {
                ret = enable_lwhps2fpga_bridge();
            }
            else
            {
                ret = disable_lwhps2fpga_bridge();
            }
            break;

        case 2 :
            if( enable_flag == 1)
            {
                ret = enable_fpga2hps_bridge();
            }
            else
            {
                ret = disable_fpga2hps_bridge();
            }
            break;

        case 3 :
            if( enable_flag == 1)
            {
                ret = enable_fpga2sdram_bridge();
            }
            else
            {
                ret = disable_fpga2sdram_bridge();
            }
            break;

        default :
            INFO("Invalid bridge number !!! \r\nPlease type help to see available options ");
            break;
    }

    if( enable_flag == 1)
    {
        if( ret == 1)
        {
            INFO("Enabled the bridge successfully");
        }
        else if( ret == -1 )
        {
            ERROR("Failed to enable the bridge");
        }
        else
        {
            /* do nothing */
        }
    }
    else
    {
        if( ret == 1)
        {
            INFO("Disabled the bridge successfully");
        }
        else if( ret == -1)
        {
            ERROR("Failed to disable the bridge");
        }
        else
        {
            /* do nothing */
        }
    }
    return pdFALSE;
}
