/*
 * SPDX-FileCopyrightText: Copyright (C) 2025 Altera Corporation
 *
 * SPDX-License-Identifier: MIT-0
 *
 * Implementation of CLI commands for QSPI flash
 */

/**
 * @defgroup cli_qspi QSPI
 * @ingroup cli
 *
 * Perform flash operations on a flash device connected to QSPI bus
 *
 * @details
 * It supports the following commands:
 * - qspi erase <instance> <address> <bytes>
 * - qspi write <instance> <address> <data>...
 * - qspi read  <instance> <address> <bytes>
 * - qspi help
 *
 * Typical usage:
 * - Use 'qspi erase' command to erase the QSPI flash.
 * - Use 'qspi write' command to write data to the QSPI flash.
 * - Use 'qspi read' command to read data from the QSPI flash.
 *
 * @subsection qspi_commands Commands
 * @subsubsection qspi_erase qspi erase
 * Erase QSPI flash data  <br>
 *
 * Usage: <br>
 *   qspi erase <instance> <address> <bytes> <br>
 *
 * It requires the following arguments:
 * - instance -  Instance of the QSPI flash device.
 * - address -   Target memory address (hex or decimal).
 * - bytes -     Number of bytes to erase (positive integer).
 *
 * @subsubsection qspi_write qspi write
 * Write data to QSPI flash  <br>
 *
 * Usage: <br>
 *   qspi write <instance> <address> <data>... <br>
 *
 * It requires the following arguments:
 * - instance  Instance of the QSPI flash device.
 * - address   Target memory address (hex or decimal).
 * - data      One or more data values to write (each a positive integer).
 * <br>
 * @subsubsection qspi_read qspi read
 * Read data from QSPI flash
 *
 * Usage: <br>
 *   qspi read <instance> <address> <bytes>
 *
 * It requires the following arguments:
 * - instance  Instance of the QSPI flash device.
 * - address   Target memory address (hex or decimal).
 * - bytes     Number of bytes to read (positive integer).
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <FreeRTOS.h>
#include <socfpga_uart.h>
#include <FreeRTOS_CLI.h>
#include "cli_app.h"
#include "socfpga_flash.h"
#include "socfpga_qspi.h"
#include "socfpga_cache.h"

#define READ_CMD         ("read")
#define WRITE_CMD        ("write")
#define ERASE_CMD        ("erase")
#define SECTOR_SIZE      (1024 * 4)

#define CLI_QSPI_OK      0
#define CLI_QSPI_FAIL    1

uint8_t wrBuf[ 256 ] = { 0 };
uint8_t rdBuf[ 256 ] = { 0 };

BaseType_t cmd_qspi( char *pcWriteBuffer, size_t xWriteBufferLen,
        const char *pcCommandString )
{
    (void) xWriteBufferLen;
    (void) pcWriteBuffer;

    const char *pcParam1;
    const char *pcParam2;
    const char *pcParam3;
    const char *pcParam4;
    BaseType_t Param1StrLen;
    BaseType_t Param2StrLen;
    BaseType_t Param3StrLen;
    BaseType_t Param4StrLen;
    BaseType_t lRet = pdTRUE;

    char tempStr[ 6 ] = { 0 };
    uint32_t instance;
    FlashHandle_t flash_handle;
    uint32_t targ_addr;
    uint32_t size;
    uint32_t status;
    int32_t idx;
    int32_t erase_cnt;

    pcParam1 = FreeRTOS_CLIGetParameter(pcCommandString, 1, &Param1StrLen); /* get first command */
    strncpy(tempStr, pcParam1, Param1StrLen);

    if (strncmp( pcParam1, "help", 4) == 0)
    {
        printf("\r\nPerform flash operations on a flash device connected to qspi"
                "\r\n\nIt supports the following subcommands:"
                "\r\n  qspi erase <instance> <address> <bytes>"
                "\r\n  qspi write <instance> <address> <value>"
                "\r\n  qspi read <instance> <address> <bytes>"
                "\r\n  qspi help"
                "\r\n\nTypical usage:"
                "\r\n- Use erase command to erase the qspi."
                "\r\n- Use write command to write data to the qspi."
                "\r\n- Use read  command to read data from the qspi."
                "\r\n\nFor help on the specific commands please do:"
                "\r\n  qspi <command> help\r\n"
                );

        return pdFALSE;
    }
    pcParam2 = FreeRTOS_CLIGetParameter(pcCommandString, 2, &Param2StrLen); /* get instance */

    if (strncmp(pcParam1, ERASE_CMD, 5) == 0)
    {
        if (strncmp( pcParam2, "help", 4 ) == 0)
        {
            printf("\r\nErase qspi flash data"
                    "\r\n\nUsage:"
                    "\r\n  qspi erase <instance> <address> <bytes>"
                    "\r\n\nIt requires the following arguments:"
                    "\r\n  instance  instance of the qspi flash device."
                    "\r\n  address   target memory address."
                    "\r\n  bytes     number of bytes to be erased. The argument should be a postivie integer."
                    );

            return pdFALSE;
        }

        pcParam3 = FreeRTOS_CLIGetParameter(pcCommandString, 3, &Param3StrLen); /* get address */
        pcParam4 = FreeRTOS_CLIGetParameter(pcCommandString, 4, &Param4StrLen); /* get no. of bytes to be erased */

        if ((pcParam2 == NULL) || (pcParam3 == NULL) || (pcParam4 == NULL))
        {
            printf("\r\nIncorrect number of arguments for qspi erase command.");
            printf("\r\nEnter 'help' to view the list of available commands.");
            return pdFAIL;
        }

        instance = strtol(pcParam2, NULL, 10);
        if (instance > 4)
        {
            printf("\r\nInvalid QSPI instance\r\n");
            return pdFAIL;
        }

        flash_handle = flash_open(instance);
        if (flash_handle == NULL)
        {
            printf("\r\nInvalid QSPI instance\r\n");
            return pdFAIL;
        }

        int32_t addr = strtol(pcParam3, NULL, 16);
        targ_addr = strtol(pcParam3, NULL, 16);
        if (addr < 0)
        {
            printf("\r\nInvalid address");
            flash_close(flash_handle);
            return pdFAIL;
        }
        int32_t eraseSize = strtol(pcParam4, NULL, 10);
        if (eraseSize < 0)
        {
            printf("\r\nInvalid size");
            flash_close(flash_handle);
            return pdFAIL;
        }
        size = strtol(pcParam4, NULL, 10);
        erase_cnt = flash_erase_sectors(flash_handle, targ_addr, size);
        if (erase_cnt >= 0)
        {
            status = CLI_QSPI_OK;
            printf("\r\nErase completed");
        }
        else
        {
            status = CLI_QSPI_FAIL;
            printf("\r\nErase Failed");
        }

        flash_close(flash_handle);
        osal_delay_ms(10);

    }
    else if (strcmp(tempStr, WRITE_CMD) == 0)
    {
        if (strncmp( pcParam2, "help", 4 ) == 0)
        {
            printf("\r\nWrite data to qspi flash"
                    "\r\n\nUsage:"
                    "\r\n  qspi write <instance> <address> <data>"
                    "\r\n\nIt requires the following arguments:"
                    "\r\n  instance  instance of the qspi flash device."
                    "\r\n  address   target memory address."
                    "\r\n  data      One or more 8 bit hexa decimal values separated by space"
                    );

            return pdFALSE;
        }

        pcParam3 = FreeRTOS_CLIGetParameter(pcCommandString, 3, &Param3StrLen);
        pcParam4 = FreeRTOS_CLIGetParameter(pcCommandString, 4, &Param4StrLen);

        if ((pcParam2 == NULL) || (pcParam3 == NULL) || (pcParam4 == NULL))
        {
            printf("\r\nIncorrect number of arguments for qspi read command.");
            printf("\r\nEnter 'help' to view the list of available commands.");
            return pdFAIL;
        }

        instance = strtol(pcParam2, NULL, 10);
        if (instance > 4)
        {
            printf("\r\nInvalid QSPI instance");
            return pdFAIL;
        }

        flash_handle = flash_open(instance);
        if (flash_handle == NULL)
        {
            printf("\r\n QSPI instance is busy");
            return pdFAIL;
        }

        int32_t addr = strtol(pcParam3, NULL, 10);
        if (addr < 0)
        {
            printf("\r\nInvalid address");
            flash_close(flash_handle);
            return pdFAIL;
        }
        targ_addr = strtol(pcParam3, NULL, 16);

        size = 0;
        idx = 4;
        memset(wrBuf, 0, sizeof(wrBuf));
        while (true)
        {
            pcParam4 = FreeRTOS_CLIGetParameter(pcCommandString, idx,
                    &Param4StrLen);
            if (pcParam4 == NULL)
            {
                break;
            }
            wrBuf[ size ] = strtol(pcParam4, NULL, 16);
            size++;
            idx++;
        }
        cache_force_write_back((void*) wrBuf, (size));
        status = flash_write_sync(flash_handle, targ_addr, wrBuf, size);
        if (status != 0)
        {
            printf("\r\nWrite Failed");
            flash_close(flash_handle);
            return pdFAIL;
        }
        memset(rdBuf, 0, sizeof(rdBuf));
        cache_force_invalidate((void*) rdBuf, (size));
        status = flash_read_sync(flash_handle, targ_addr, rdBuf, size);
        if (status != 0)
        {
            printf("\r\nRead Failed");
            flash_close(flash_handle);
            return pdFAIL;
        }
        cache_force_invalidate((void*) rdBuf, (size));

        status = memcmp(wrBuf, rdBuf, size);
        if (status != 0)
        {
            printf("\r\nWrite Failed");
        }


        flash_close(flash_handle);

        printf("\r\nWrite Success");
    }
    else if (strcmp(tempStr, READ_CMD) == 0)
    {
        if (strncmp( pcParam2, "help", 4 ) == 0)
        {
            printf("\r\nRead data from qspi flash"
                    "\r\n\nUsage:"
                    "\r\n  qspi read <instance> <address> <bytes>"
                    "\r\n\nIt requires the following arguments:"
                    "\r\n  instance  instance of the qspi flash device."
                    "\r\n  address   target memory address."
                    "\r\n  bytes     number of bytes to be read. The argument should be a postive integer."
                    );

            return pdFALSE;
        }

        pcParam3 = FreeRTOS_CLIGetParameter(pcCommandString, 3, &Param3StrLen);
        pcParam4 = FreeRTOS_CLIGetParameter(pcCommandString, 4, &Param4StrLen);

        if ((pcParam2 == NULL) || (pcParam3 == NULL) || (pcParam4 == NULL))
        {
            printf("\r\nIncorrect number of arguments for qspi read command.");
            printf("\r\nEnter 'help' to view the list of available commands.");
            return pdFAIL;
        }

        instance = strtol(pcParam2, NULL, 10);
        if (instance > 4)
        {
            printf("\r\nInvalid QSPI instance");
            return pdFAIL;
        }

        flash_handle = flash_open(instance);
        if (flash_handle == NULL)
        {
            printf("\r\n QSPI instance is busy");
            return pdFAIL;
        }
        int32_t addr = strtol(pcParam3, NULL, 10);
        if (addr < 0)
        {
            printf("\r\nInvalid address");
            flash_close(flash_handle);
            return pdFAIL;
        }
        targ_addr = strtol(pcParam3, NULL, 16);

        int32_t eraseSize = strtol(pcParam4, NULL, 10);
        if (eraseSize < 0)
        {
            printf("\r\nInvalid size");
            flash_close(flash_handle);
            return pdFAIL;
        }
        size = strtol(pcParam4, NULL, 10);
        memset(rdBuf, 0, sizeof(rdBuf));
        cache_force_invalidate((void*) rdBuf, (size));
        status = flash_read_sync(flash_handle, targ_addr, rdBuf, (size));
        if (status != 0)
        {
            printf("\r\nRead Failed");
            flash_close(flash_handle);
            return pdFAIL;
        }
        cache_force_invalidate((void*) rdBuf, (size));

        flash_close(flash_handle);

        printf("\r\n%d bytes data in 0x%x: ", size, targ_addr);
        for (uint32_t i = 0; i < size; i++)
        {
            printf(" 0x%X", rdBuf[ i ]);
        }
    }
    else
    {
        printf("\r\nInvalid QSPI command.");
        printf("\r\nEnter help to see the list of available commands.");
        return pdFAIL;
    }
    /*For cli return pdFALSE for success*/
    if (lRet == pdTRUE)
    {
        lRet =  pdFALSE;
    }
    return lRet;

}
