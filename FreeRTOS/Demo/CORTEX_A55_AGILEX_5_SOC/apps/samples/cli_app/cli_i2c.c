/*
 * SPDX-FileCopyrightText: Copyright (C) 2025 Altera Corporation
 *
 * SPDX-License-Identifier: MIT-0
 *
 * Implementation of CLI commands for I2C bus
 */

/**
 * @defgroup cli_i2c I2C
 * @ingroup cli
 *
 * Perform I2C operations
 *
 * @details
 * It supports the following subcommands:
 * - i2c read  <instance> <target_address> <register_address>
 * - i2c write <instance> <target_address> <register_address> <data>
 * - i2c help
 *
 * Typical usage:
 * - Use 'i2c read' command to perform write operations on an I2C device.
 * - Use 'i2c write' command to perform write operations on an I2C device.
 *
 * @subsection commands Commands
 * @subsubsection i2c_read i2c read
 * Read one byte of data from an I2C device  <br>
 *
 * Usage:  <br>
 *   i2c read <instance> <target_address> <register_address>  <br>
 *
 * It requires the following arguments:
 * - instance -         The instance of the I2C bus. Valid range is 0 to 4.  <br>
 * - target_address -   The address of the I2C slave. Supports 7-bit address in hexadecimal format (e.g., 0x50).  <br>
 * - register_address - The register address to read from. Hex value in the range 0x0000 to 0x3FFF.  <br>
 *
 * @subsubsection i2c_write i2c write
 * Write a single byte of data to an I2C device  <br>
 *
 * Usage:  <br>
 *   i2c write <instance> <target_address> <register_address> <data>  <br>
 *
 * It requires the following arguments:
 * - instance -         The instance of the I2C bus. Valid range is 0 to 4.  <br>
 * - target_address -   The address of the I2C slave. Supports 7-bit address in hexadecimal format (e.g., 0x50).  <br>
 * - register_address - The register address to write to. Hex value in the range 0x0000 to 0x3FFF.  <br>
 * - data -             The data byte to write. Valid range is 0x00 to 0xFF.  <br>
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <FreeRTOS.h>
#include <task.h>
#include "semphr.h"
#include <socfpga_uart.h>
#include <socfpga_timer.h>
#include "FreeRTOS_CLI.h"
#include "cli_app.h"
#include "socfpga_i2c.h"
#include "osal_log.h"

BaseType_t cmd_i2c( char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString )
{
    (void)xWriteBufferLen;
    const char *pcParameter1;
    const char *pcParameter2;
    const char *pcParameter3;
    const char *pcParameter4;
    const char *pcParameter5;
    char tempString[ 6 ] = { 0 };
    const char *readCmd = "read";
    const char *writeCmd = "write";
    int data = 0;
    uint8_t i2cInst;
    I2cHandle_t i2cHdl;
    uint16_t slaveId;
    uint16_t regAddr;
    uint8_t wbuf[3];
    uint8_t rbuf;
    I2cConfig_t config;
    BaseType_t xParameter1StringLength;
    BaseType_t xParameter2StringLength;
    BaseType_t xParameter3StringLength;
    BaseType_t xParameter4StringLength;
    BaseType_t xParameter5StringLength;

    pcParameter1 = FreeRTOS_CLIGetParameter(pcCommandString, 1,
                                            &xParameter1StringLength);
    strncpy(tempString, pcParameter1, xParameter1StringLength);

    if( strncmp(tempString, "help", 4) == 0 )
    {
         printf("\rPerform i2c operations"
                "\r\n\nIt supports the following subcommands:"
                "\r\n  i2c read  <instance> <target_address> <register_address>"
                "\r\n  i2c write <instance> <target_address> <register_address> <value>"
                "\r\n  i2c help"
                "\r\n\nTypical usage:"
                "\r\n- Use read/write command to perform write/read operations on i2c device."
                "\r\n\nFor help on the specific commands please do:"
                "\r\n  i2c <command> help\r\n");

        return pdFALSE;
    }
    pcParameter2 = FreeRTOS_CLIGetParameter(pcCommandString, 2,
                                                &xParameter2StringLength);
    if ( strcmp(tempString, readCmd) == 0 )
    {
        if( strncmp(pcParameter2, "help", 4) == 0)
        {
            printf("\r\nRead one byte of data from an i2c device."
                    "\r\n\nUsage:"
                    "\r\n  i2c read  <instance> <target_address> <register_address>"
                    "\r\n\nIt requires the following arguments:"
                    "\r\n  instance            The instance of the I2C bus. Valid range is 0 to 4."
                    "\r\n  target_address      The address of I2C slave.Supports 7 bit address in hexadecimal format. Example: 0x50."
                    "\r\n  register_address    The register address to read data. It is a hexadecimal value in the range 0x0000 to 0x3FFF.");
            return pdFALSE;
        }

        pcParameter3 = FreeRTOS_CLIGetParameter(pcCommandString, 3,
                                                &xParameter3StringLength);
        pcParameter4 = FreeRTOS_CLIGetParameter(pcCommandString, 4,
                                                &xParameter4StringLength);
        if( pcParameter2 == NULL || pcParameter3 == NULL || pcParameter4 == NULL )
        {
            ERROR("Incorrect Number of arguments for i2c read command");
            printf("\r\nEnter 'help' to view a list of available commands.");
            return pdFALSE;
        }

        i2cInst = strtol(pcParameter2, NULL, 10);
        if ( i2cInst > 4 )
        {
            strncpy(pcWriteBuffer, "Invalid I2C instance",
                    strlen("Invalid I2C instance "));
            return pdFAIL;
        }
        slaveId = strtol(pcParameter3, NULL, 16);
        if( (slaveId >> 8) != 0)
        {
            ERROR("Invalid argument. Try i2c read help");
            return pdFALSE;
        }
        i2cHdl = i2c_open(i2cInst);
        if( i2cHdl == NULL )
        {
            ERROR("I2C Bus already in use");
            return pdFALSE;
        }
        config.ulBusFreq = I2C_STANDARD_MODE_BPS;
        i2c_ioctl(i2cHdl, eI2CSetMasterConfig, &config);
        i2c_ioctl(i2cHdl, eI2CSetSlaveAddr, &slaveId);

        regAddr = strtol(pcParameter4, NULL, 16);
        if( (regAddr > 0x3FFF) != 0)
        {
            ERROR("Invalid argument. Try i2c read help");
            i2c_close(i2cHdl);
            return pdFALSE;
        }
        wbuf[ 0 ] = (uint8_t) ((regAddr >> 8) & 0xFF);
        wbuf[ 1 ] = (uint8_t) (regAddr & 0xFF);
        if (i2c_write_sync(i2cHdl, wbuf, 2) != 0)
        {
            printf("Read failed. \n\r");
            i2c_close(i2cHdl);
            return pdFAIL;
        }
        if (i2c_read_sync(i2cHdl, &rbuf, 1) != 0)
        {
            printf("Read failed \n\r");
            i2c_close(i2cHdl);
            return pdFAIL;
        }
        i2c_close(i2cHdl);
        printf("Data in 0x%x is 0x%x \n\r", regAddr, rbuf);
    }
    else if ( strcmp(tempString, writeCmd) == 0 )
    {
        if( strncmp(pcParameter2, "help", 4) == 0)
        {
            printf("\r\nWrite a single byte data to an i2c device"
                    "\r\n\nUsage:"
                    "\r\n  i2c write  <instance> <target_address> <register_address> <data>"
                    "\r\n\nIt requires the following arguments:"
                    "\r\n  instance            The instance of the I2C bus. Valid range is 0 to 4."
                    "\r\n  target_address      The address of I2C slave. Supports 7 bit address in hexadecimal format. Example: 0x50."
                    "\r\n  register_address    The register address to read data. Valid address range is 0x0000 to 0x3FFF."
                    "\r\n  data                The data to be written to the device. It is a hexadecimal value in the range 0x00 to 0xFF."
                  );
            return pdFALSE;
        }

        pcParameter3 = FreeRTOS_CLIGetParameter(pcCommandString, 3,
                                                &xParameter3StringLength);
        pcParameter4 = FreeRTOS_CLIGetParameter(pcCommandString, 4,
                                                &xParameter4StringLength);
        pcParameter5 = FreeRTOS_CLIGetParameter(pcCommandString, 5,
                                                &xParameter5StringLength);
        if( pcParameter2 == NULL || pcParameter3 == NULL || pcParameter4 == NULL || pcParameter5 == NULL )
        {
            ERROR("Incorrect Number of arguments for i2c write command");
            printf("\r\nEnter 'help' to view a list of available commands.");
            return pdFALSE;
        }

        i2cInst = strtol(pcParameter2, NULL, 10);
        if ( i2cInst > 4 )
        {
            strncpy(pcWriteBuffer, "Invalid I2C instance",
                    strlen("Invalid I2C instance "));
            return pdFAIL;
        }
        slaveId = strtol(pcParameter3, NULL, 16);
        if( (slaveId >> 8) != 0)
        {
            ERROR("Invalid argument. Try i2c write help");
            return pdFALSE;
        }
        i2cHdl = i2c_open(i2cInst);
        if( i2cHdl == NULL )
        {
            ERROR("I2C Bus already in use");
            return pdFALSE;
        }
        config.ulBusFreq = I2C_STANDARD_MODE_BPS;
        i2c_ioctl(i2cHdl, eI2CSetMasterConfig, &config);
        i2c_ioctl(i2cHdl, eI2CSetSlaveAddr, &slaveId);

        regAddr = strtol(pcParameter4, NULL, 16);
        if( regAddr > 0x3FFF )
        {
            ERROR("Invalid argument. Try i2c write help");
            i2c_close(i2cHdl);
            return pdFALSE;
        }
        if( (xParameter5StringLength > 4) || (xParameter5StringLength < 3) )
        {
            ERROR("Invalid argument. Try i2c write help");
            i2c_close(i2cHdl);
            return pdFALSE;
        }
        data = strtol(pcParameter5, NULL, 16);
        if( (data < 0) || (data > 0xFF) )
        {
            ERROR("Invalid argument. Try i2c write help");
            i2c_close(i2cHdl);
            return pdFALSE;
        }
        wbuf[ 0 ] = (uint8_t) ((regAddr >> 8) & 0xFF);
        wbuf[ 1 ] = (uint8_t) (regAddr & 0xFF);
        wbuf[ 2 ]  = (uint8_t) data;
        if ( i2c_write_sync(i2cHdl, wbuf, 3) != 0)
        {
            printf("Write failed \n\r");
            i2c_close(i2cHdl);
            return pdFAIL;
        }
        i2c_close(i2cHdl);
        printf("Write completed\n\r");
    }
    else
    {
        strncpy(pcWriteBuffer, "Invalid I2C command",
                strlen("Invalid I2C command "));
        return pdFAIL;
    }
    return pdFALSE;
}
