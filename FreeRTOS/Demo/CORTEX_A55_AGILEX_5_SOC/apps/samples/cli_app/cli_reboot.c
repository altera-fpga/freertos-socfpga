/*
 * SPDX-FileCopyrightText: Copyright (C) 2025 Altera Corporation
 *
 * SPDX-License-Identifier: MIT-0
 *
 * Implementation of reboot command
 */

/**
 * @defgroup cli_reboot Reboot
 * @ingroup cli
 *
 * Perform system reboot operations
 *
 * @details
 * It supports the following commands:
 * - reboot <type>
 * - reboot help
 *
 * Typical usage:
 * - Use 'reboot cold' for a cold reboot.
 * - Use 'reboot warm' for a warm reboot.
 *
 * @subsection commands Commands
 * @subsubsection reboot reboot
 * Reboot system  <br>
 *
 * Usage:  <br>
 *   reboot <type>  <br>
 *
 * It requires the following arguments:
 * - type -           The reboot type. Valid values are:  <br>
 *   - cold  – COLD_REBOOT  <br>
 *   - warm  – WARM_REBOOT  <br>
 *
 * @subsubsection help reboot help
 * Show reboot command help  <br>
 *
 * Usage:  <br>
 *   reboot help  <br>
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <FreeRTOS.h>
#include <task.h>
#include <socfpga_reboot_mngr.h>
#include "FreeRTOS_CLI.h"
#include "cli_app.h"
#include "semphr.h"


void cold_reboot_callback( void *ulBuff )
{
    (void)ulBuff;
    printf("\r\n Initiating a COLD REBOOT \r\n");
}
void warm_reboot_callback( void *ulBuff )
{
    (void)ulBuff;
    printf("\r\n Initiating a WARM REBOOT \r\n");
}

BaseType_t cmd_reboot( char *pcWriteBuffer, size_t xWriteBufferLen,
        const char *pcCommandString )
{
    (void) pcWriteBuffer;
    (void) xWriteBufferLen;

    const char *pcParameter1, *pcParameter2;
    char *spacePos;
    BaseType_t xParameter1StringLength, xParameter2StringLength, xRet;
    xRet = pdFALSE;
    pcParameter1 = FreeRTOS_CLIGetParameter(pcCommandString, 1,
            &xParameter1StringLength);

    if (pcParameter1 == NULL)
    {
        xRet = pdTRUE;
    }

    if (strncmp(pcParameter1, "help", 4) == 0)
    {
        printf("\rPerform reboot operations"
                "\r\n\nIt supports the following subcommands:"
                "\r\n  reboot warm"
                "\r\n  reboot cold"
                "\r\n  reboot help"
                "\r\n\nTypical usage:"
                "\r\n- Use reboot command to do warm/cold reboot."
                "\r\n\nFor help on the specific commands please do:"
                "\r\n  reboot <type> help\r\n"
                );
        return pdFALSE;
    }

    pcParameter2 = FreeRTOS_CLIGetParameter(pcCommandString, 2,
            &xParameter2StringLength);

    spacePos = strchr(pcParameter1, ' ');
    if (spacePos != NULL)
    {
        *spacePos = '\0';
    }

    spacePos = strchr(pcParameter2, ' ');
    if (spacePos != NULL)
    {
        *spacePos = '\0';
    }

    /* check for warm reboot */
    if (strncmp(pcParameter1, "warm", 4) == 0)
    {
        if (strncmp(pcParameter2, "help", 4) == 0)
        {
            printf("\r\nDo warm reboot."
                    "\r\n\nUsage:"
                    "\r\n reboot warm"
                    );
            return pdFALSE;
        }
        reboot_mngr_set_callback(warm_reboot_callback, WARM_REBOOT);
        reboot_mngr_do_reboot(WARM_REBOOT);
    }
    else if (!strncmp(pcParameter1, "cold", strlen("cold")))
    {
        if (strncmp(pcParameter2, "help", 4) == 0)
        {
            printf("\r\nDo cold reboot."
                    "\r\n\nUsage:"
                    "\r\n reboot cold"
                    );
            return pdFALSE;
        }
        reboot_mngr_set_callback(cold_reboot_callback, COLD_REBOOT);
        reboot_mngr_do_reboot(COLD_REBOOT);
    }
    else
    {
        printf("Unsupported reboot command \r\n");
        xRet = pdTRUE;
    }

    if (xRet != pdFALSE)
    {
        printf("Incorrect command parameter(s).\r\n");
        printf("Type help to see the available commands r\n");
        xRet = pdFALSE;
    }
    return xRet;
}
