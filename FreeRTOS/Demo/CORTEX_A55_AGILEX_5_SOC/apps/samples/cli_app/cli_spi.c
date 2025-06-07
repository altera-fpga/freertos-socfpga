/*
 * SPDX-FileCopyrightText: Copyright (C) 2025 Altera Corporation
 *
 * SPDX-License-Identifier: MIT-0
 *
 * Implementation of CLI commands for SPI EEPROM
 */


/**
 * @defgroup cli_spi SPI
 * @ingroup cli
 *
 * Perform operations on EEPROM connected to SPI bus
 *
 * @details
 * It supports the following commands:
 * - spi read <instance> <slave_select> <register_address> <data_size>
 * - spi write <instance> <slave_select> <register_address> <data>...
 * - spi help
 *
 * Typical usage:
 * - Use 'spi write' command to write data to the SPI EEPROM.
 * - Use 'spi read' command to read data from the SPI EEPROM.
 *
 * @subsection commands Commands
 * @subsubsection spi_read spi read
 * Read from SPI EEPROM memory <br>
 *
 * Usage: <br>
 *   spi read <instance> <slave_select> <register_address> <data_size> <br>
 *
 * It requires the following arguments:
 * - instance -             The instance of the SPI driver. Valid values are 0 and 1.
 * - slave_select -         The number assigned to the slave device. Valid range is from 1 to 4.
 * - register_address -     The address of the memory location. It is a hexadecimal value from 0x00 to 0x3FFF.
 * - data_size -            Number of bytes to read.
 * @subsubsection spi_write spi write
 * Write to SPI EEPROM memory
 *
 * Usage: <br>
 * spi write <instance> <slave_select> <register_address> <data>... <br>
 *
 * It requires the following arguments:
 * - instance -             The instance of the SPI driver. Valid values are 0 and 1.
 * - slave_select -         The number assigned to the slave device. Valid range is from 1 to 4.
 * - register_address -     The address of the memory location. It is a hexadecimal value from 0x00 to 0x3FFF.
 * - data -                 Data written to the EEPROM memory. User can provide multiple values.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <FreeRTOS.h>
#include <task.h>
#include "semphr.h"
#include "FreeRTOS_CLI.h"
#include "cli_app.h"
#include "socfpga_spi.h"

#define EEPROM_READ         0x03
#define EEPROM_WRITE        0x02
#define EEPROM_WR_ENABLE    0x06
#define TRANSFER_SIZE       1
#define SPI_FREQ            500000U

BaseType_t cmd_spi( char *pcWriteBuffer, size_t xWriteBufferLen,
        const char *pcCommandString );
int32_t eeprom_read( uint8_t *buf, size_t size, uint16_t memAdd );
int32_t eeprom_write( uint8_t *buf, size_t size, uint16_t memAdd );
int32_t eeprom_enable_write();

SpiHandle_t spiHdl;

/**
 * @func : cmd_spi
 * @brief : spi command line execution
 */
BaseType_t cmd_spi( char *pcWriteBuffer, size_t xWriteBufferLen,
        const char *pcCommandString )
{
    (void)xWriteBufferLen;
    const char *pcParameter1;
    const char *pcParameter2;
    const char *pcParameter3;
    const char *pcParameter4;
    const char *pcParameter5;
    char tempString[ 6 ] = { 0 };
    char helpStr[5] = {0};
    uint8_t rbuf[259];
    const char *readCmd = "read";
    const char *writeCmd = "write";
    const char *helpCmd = "help";
    uint8_t spiInst;
    uint16_t slaveSel;
    uint16_t regAddr;
    uint8_t wbuf[256];
    uint8_t dataSize;
    uint8_t size;
    uint8_t idx;
    int32_t ret;
    int i;
    SpiMasterConfig_t xConfig;
    BaseType_t xParameter1StringLength;
    BaseType_t xParameter2StringLength;
    BaseType_t xParameter3StringLength;
    BaseType_t xParameter4StringLength;
    BaseType_t xParameter5StringLength;

    pcParameter1 = FreeRTOS_CLIGetParameter(pcCommandString, 1,
            &xParameter1StringLength);
    strncpy(tempString, pcParameter1, xParameter1StringLength);
    if (strcmp(tempString, readCmd) == 0)
    {
        pcParameter2 = FreeRTOS_CLIGetParameter(pcCommandString, 2,
                &xParameter2StringLength);
        pcParameter3 = FreeRTOS_CLIGetParameter(pcCommandString, 3,
                &xParameter3StringLength);
        pcParameter4 = FreeRTOS_CLIGetParameter(pcCommandString, 4,
                &xParameter4StringLength);
        pcParameter5 = FreeRTOS_CLIGetParameter(pcCommandString, 5,
                &xParameter5StringLength);

        strncpy(helpStr, pcParameter2, xParameter2StringLength);
        if (strcmp(helpStr, helpCmd) == 0)
        {
            printf("\rRead from SPI EEPROM memory"
                    "\r\n\nUsage:"
                    "\r\n  spi read <instance> <slave_select> <register_address> <data_size>"
                    "\r\n\nIt requires the following arguments:"
                    "\r\n  instance            The instance of the SPI driver. Valid values are 0 and 1."
                    "\r\n  slave_select        The number assigned to the slave device. Valid range is from 1 to 4."
                    "\r\n  register_address    The address of the memory location. It is a hexadecimal value from 0x00 to x3FFF."
                    "\r\n  data_size           Number of bytes to read.");
            return pdFALSE;
        }
        else if ((pcParameter2 == NULL) || (pcParameter3 == NULL) ||
                (pcParameter4 == NULL))
        {
            printf("\r\nERROR: Invalid arguments.");
            printf("\r\nEnter 'spi read help' for more information");
            return pdFAIL;
        }

        spiInst = strtol(pcParameter2, NULL, 10);
        if (spiInst > 2)
        {
            strncpy(pcWriteBuffer, "Invalid SPI instance",
                    strlen("Invalid SPI instance "));
            return pdFAIL;
        }
        slaveSel = strtol(pcParameter3, NULL, 16);
        spiHdl = spi_open(spiInst);
        if (spiHdl == NULL)
        {
            printf("\r\n SPI instance not initialized.");
            return pdFAIL;
        }
        xConfig.eMode = eSPIMode3;
        xConfig.ulFreq = SPI_FREQ;
        ret = spi_ioctl(spiHdl, eSPISetMasterConfig, &xConfig);
        if (ret != 0)
        {
            printf("Failed to configure SPI \n\r");
            spi_close(spiHdl);
            return pdFAIL;
        }
        ret = spi_select_slave(spiHdl, slaveSel);
        if (ret != 0)
        {
            printf("Failed to select slave \n\r");
            spi_close(spiHdl);
            return pdFAIL;
        }
        int16_t addr = strtol(pcParameter4, NULL, 16);
        if (addr < 0)
        {
            printf("Invalid address \n\r");
            spi_close(spiHdl);
            return pdFAIL;
        }
        regAddr = strtol(pcParameter4, NULL, 16);
        int8_t size = strtol(pcParameter5, NULL, 10);
        if (size < 0)
        {
            printf("Invalid size \n\r");
            spi_close(spiHdl);
            return pdFAIL;

        }
        dataSize = strtol(pcParameter5, NULL, 10);
        ret = eeprom_read(rbuf, dataSize, regAddr);
        if (ret != 0)
        {
            printf("Failed to read \n\r");
            spi_close(spiHdl);
            return pdFAIL;
        }
        ret = spi_close(spiHdl);
        if (ret != 0)
        {
            printf("Failed to close SPI\n\r");
            return pdFAIL;
        }
        printf("Data in 0x%x is \n\r", regAddr);
        /* valid data in memory starts from 3rd index */
        for (i=0; i < dataSize; i++)
        {
            printf("0x%x ", rbuf[i + 3]);
        }
    }
    else if (strcmp(tempString, writeCmd) == 0)
    {
        pcParameter2 = FreeRTOS_CLIGetParameter(pcCommandString, 2,
                &xParameter2StringLength);
        pcParameter3 = FreeRTOS_CLIGetParameter(pcCommandString, 3,
                &xParameter3StringLength);
        pcParameter4 = FreeRTOS_CLIGetParameter(pcCommandString, 4,
                &xParameter4StringLength);

        strncpy(helpStr, pcParameter2, xParameter2StringLength);
        if (strcmp(helpStr, helpCmd) == 0)
        {
            printf("\rWrite to SPI EEPROM memory"
                    "\r\n\nUsage:"
                    "\r\n  spi write <instance> <slave_select> <register_address> <data>..."
                    "\r\n\nIt requires the following arguments:"
                    "\r\n  instance            The instance of the SPI driver. Valid values are 0 and 1."
                    "\r\n  slave_select        The number assigned to the slave device. Valid range is from 1 to 4."
                    "\r\n  register_address    The address of the memory location. It is a hexadecimal value from 0x00 to x3FFF."
                    "\r\n  data                Data written to the EEPROM memory in hex . User can provide multiple values.");
            return pdFALSE;
        }
        else if ((pcParameter2 == NULL) || (pcParameter3 == NULL) ||
                (pcParameter4 == NULL))
        {
            printf("\r\nERROR: Invalid arguments");
            printf("\r\nEnter 'help' to view a list of available commands.");
            return pdFAIL;
        }

        spiInst = strtol(pcParameter2, NULL, 10);
        if (spiInst > 2)
        {
            strncpy(pcWriteBuffer, "Invalid SPI instance",
                    strlen("Invalid SPI instance "));
            return pdFAIL;
        }
        slaveSel = strtol(pcParameter3, NULL, 16);
        spiHdl = spi_open(spiInst);
        if (spiHdl == NULL)
        {
            printf("\r\n SPI init failed !!!");
            return pdFAIL;
        }
        xConfig.eMode = eSPIMode3;
        xConfig.ulFreq = SPI_FREQ;
        ret = spi_ioctl(spiHdl, eSPISetMasterConfig, &xConfig);
        if (ret != 0)
        {
            printf("Failed to configure SPI \n\r");
            spi_close(spiHdl);
            return pdFAIL;
        }
        ret = spi_select_slave(spiHdl, slaveSel);
        if (ret != 0)
        {
            printf("Failed to select slave \n\r");
            spi_close(spiHdl);
            return pdFAIL;
        }
        int16_t addr = strtol(pcParameter4, NULL, 16);
        if (addr < 0)
        {
            printf("Invalid address\n\r");
            spi_close(spiHdl);
            return pdFAIL;
        }
        regAddr = strtol(pcParameter4, NULL, 16);
        size = 0;
        idx = 5;
        while (pdTRUE)
        {
            pcParameter5 = FreeRTOS_CLIGetParameter(pcCommandString, idx,
                    &xParameter5StringLength);
            if (pcParameter5 == NULL)
            {
                break;
            }
            wbuf[size] = strtol(pcParameter5, NULL, 16);
            size++;
            idx++;
        }
        ret = eeprom_enable_write();
        if (ret != 0)
        {
            printf("Failed to enable write \n\r");
            spi_close(spiHdl);
            return pdFAIL;
        }
        ret = eeprom_write((uint8_t *)&wbuf, size, regAddr);
        if (ret != 0)
        {
            printf("Failed to write \n\r");
            spi_close(spiHdl);
            return pdFAIL;
        }
        /*Add delay for eeprom to execute internal write cycle*/
        vTaskDelay(10);
        ret = eeprom_read(rbuf, size, regAddr);
        if (ret != 0)
        {
            printf("Failed to readback from EEPROM \n\r");
            spi_close(spiHdl);
            return pdFAIL;
        }
        ret = spi_close(spiHdl);
        if (ret != 0)
        {
            printf("Failed to close SPI \n\r");
            return pdFAIL;
        }
        if (strncmp((char*)wbuf, (char*)&rbuf[3], size) == 0)
        {
            printf("Write success \n\r");
        }
        else
        {
            printf("Write failed \n\r");
            return pdFAIL;
        }
    }
    else if (strcmp(tempString, helpCmd) == 0)
    {
        printf("\rPerform operations on SPI bus"
                "\r\n\nIt supports the following commands:"
                "\r\n  spi read <instance> <slave_select> <register_address> <data_size>"
                "\r\n  spi write <instance> <slave_select> <register_address> <data>..."
                "\r\n  spi help"
                "\r\n\nTypical usage:"
                "\r\n  Use 'spi write' command to write data to the EEPROM."
                "\r\n  Use 'spi read' command to read data from the EEPROM."
                "\r\n\nFor command specific help, try:"
                "\r\n  spi <command> help");
        return pdFALSE;
    }
    else
    {
        printf("\r\nERROR: Invalid arguments"
                " \r\nEnter 'spi help' for more information.");
        return pdFAIL;
    }
    return pdFALSE;
}

int32_t eeprom_enable_write()
{
    uint8_t cmd = EEPROM_WR_ENABLE;
    int32_t ret;

    ret = spi_ioctl(spiHdl, eSPIActivateSlave, NULL);
    if (ret != 0)
    {
        return ret;
    }
    ret = spi_write_sync(spiHdl, &cmd, 1);
    if (ret != 0)
    {
        return ret;
    }
    ret = spi_ioctl(spiHdl, eSPIDeactivateSlave, NULL);
    if (ret != 0)
    {
        return ret;
    }
    return 0;
}

int32_t eeprom_write( uint8_t *buf, size_t size, uint16_t memAdd )
{
    uint8_t cmd[ 150 ] = { 0 };
    uint8_t rx_count = 0, i = 0;
    int32_t ret;
    cmd[ 0 ] = EEPROM_WRITE;
    cmd[ 1 ] = (memAdd >> 8) & 0xFF;
    cmd[ 2 ] = memAdd & 0xFF;

    for (i = 0; i < size; i++)
    {
        cmd[ i + 3 ] = buf[ i ];
    }

    rx_count = size + 3;
    ret = spi_ioctl(spiHdl, eSPIActivateSlave, NULL);
    if (ret != 0)
    {
        return ret;
    }
    ret = spi_write_sync(spiHdl, cmd, rx_count);
    if (ret != 0)
    {
        return ret;
    }
    ret = spi_ioctl(spiHdl, eSPIDeactivateSlave, NULL);
    if (ret != 0)
    {
        return ret;
    }
    return 0;
}

int32_t eeprom_read( uint8_t *buf, size_t size, uint16_t memAdd )
{
    uint32_t i = 0;
    uint8_t cmd[ 20 ] = { 0 };
    int32_t ret;
    cmd[ 0 ] = EEPROM_READ;
    cmd[ 1 ] = (memAdd >> 8) & 0xFF;
    cmd[ 2 ] = memAdd & 0xFF;

    for (i = 0; i < size; i++)
    {
        cmd[i + 3] = i + 0x0F;
    }

    ret = spi_transfer_sync(spiHdl, cmd, buf, (size + 3));
    if (ret != 0)
    {
        return ret;
    }
    return 0;
}
