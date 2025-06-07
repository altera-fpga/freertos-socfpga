/*
 * SPDX-FileCopyrightText: Copyright (C) 2025 Altera Corporation
 *
 * SPDX-License-Identifier: MIT-0
 *
 * Sample application for SPI
 */

/**
 * @file spi_sample.c
 * @brief Sample Application for SPI
 */

#include <string.h>
#include <stdio.h>

#include "FreeRTOS.h"
#include "FreeRTOSConfig.h"
#include "task.h"
#include "semphr.h"
#include "socfpga_spi.h"
#include "osal.h"

/**
 * @defgroup spi_sample SPI
 * @ingroup samples
 * 
 * Sample Application for SPI
 * 
 * @details
 * @subsubsection spi_desc Description 
 * This is a sample application to demonstrate the usage of SPI driver.
 * It writes data to an EEPROM using SPI and then reads it back.
 * The written data is compared with the read data to verify the operation.
 * 
 * @subsubsection spi_pre Prerequisites
 * - Test daughter card must be connected to the board.
 * - A serial EEPROM with SPI interface must be connected to the SPI bus.
 * 
 * @subsubsection spi_param Configurable Parameters
 * - SPI instance can be configured in @c SPI_INSTANCE macro.
 * - The memory address to write and read back can be configured in @c EEPROM_ADDRESS macro.
 * - The number of bytes to transfer can be configured in @c TRANSFER_SIZE macro.
 * - Slave select can be configured in @c SLAVE_SELECT_NUM macro.
 * @note Ensure the instance and slave select are valid. <br>
 *       The EEPROM commands and address can vary depending on the EEPROM used.
 * 
 * @subsubsection spi_how_to How to Run
 * 1. Follow the common README instructions to build and flash the application.
 * 2. Run the application on the board with the EEPROM connected.
 * 3. Observe the UART terminal for status messages.
 * 
 * @subsubsection i2c_result Expected Results
 * - The application sends the Write Enable and Write commands to store data in EEPROM.
 * - It then reads the data back using the Read command.
 * - The read and written data are compared, and a success message is printed on match.
 */

#define EEPROM_READ          0x03
#define EEPROM_WRITE         0x02
#define EEPROM_WR_DISABLE    0x04
#define EEPROM_WR_ENABLE     0x06
#define EEPROM_RD_SR         0x05
#define EEPROM_WR_SR         0x01

#define EEPROM_ADDRESS       0x2780
#define TRANSFER_SIZE        8

#define SPI_INSTANCE         0
#define SLAVE_SELECT_NUM     1
#define SPI_FREQ             500000U

SpiHandle_t pxSPIHandle;
uint8_t dummyBuf[ 10 ] = { 0 };

int32_t eeprom_enable_write()
{
    uint8_t cmd = EEPROM_WR_ENABLE;

    if (spi_ioctl(pxSPIHandle, eSPIActivateSlave, NULL) != 0)
    {
        return 0;
    }
    if (spi_write_sync(pxSPIHandle, &cmd, 1) != 0)
    {
        return 0;
    }
    if (spi_ioctl(pxSPIHandle, eSPIDeactivateSlave, NULL) != 0)
    {
        return 0;
    }

    return 1;
}

int32_t eeprom_write( uint8_t *buf, size_t size, uint16_t memAdd )
{
    uint8_t cmd[ 150 ] = { 0 };
    uint8_t rx_count = 0, i = 0;
    cmd[ 0 ] = EEPROM_WRITE;
    cmd[ 1 ] = (memAdd >> 8) & 0xFF;
    cmd[ 2 ] = memAdd & 0xFF;

    for (i = 0; i < size; i++)
    {
        cmd[ i + 3 ] = buf[ i ];
    }

    rx_count = size + 3;
    if (spi_ioctl(pxSPIHandle, eSPIActivateSlave, NULL) != 0)
    {
        return 0;
    }
    if (spi_write_sync(pxSPIHandle, cmd, rx_count) != 0)
    {
        return 0;
    }
    osal_task_delay(5);
    if (spi_ioctl(pxSPIHandle, eSPIDeactivateSlave, NULL) != 0)
    {
        return 0;
    }
    return 1;
}

int32_t eeprom_read( uint8_t *buf, size_t size, uint16_t memAdd )
{
    uint32_t i = 0;
    uint8_t cmd[ 20 ] = { 0 };
    cmd[ 0 ] = EEPROM_READ;
    cmd[ 1 ] = (memAdd >> 8) & 0xFF;
    cmd[ 2 ] = memAdd & 0xFF;

    for (i = 0; i < size; i++)
    {
        cmd[i + 3] = i + 0x0F;
    }

    if (spi_transfer_sync(pxSPIHandle, cmd, buf, (size + 3)) != 0)
    {
        return 0;
    }
    return 1;
}

void spi_task()
{
    uint8_t rdBuf[ 15 ] = { 0 };
    uint8_t wrBuf[ 10 ] = { 1, 2, 3, 4, 5, 6, 7, 8 };
    int32_t retval = 0;
    SpiMasterConfig_t xConfig;

    xConfig.eMode = eSPIMode3;
    xConfig.ulFreq = SPI_FREQ;

    printf("Sample application to write and read EEPROM using SPI\n\r");
    printf("Configuring SPI Master...\n\r");
    pxSPIHandle = spi_open(0);
    if (pxSPIHandle == NULL)
    {
        printf("SPI instance cannot be open\n\r");
        return;
    }

    retval = spi_ioctl(pxSPIHandle, eSPISetMasterConfig, &xConfig);
    if (retval != 0)
    {
        printf("Failed\n\r");
        return;
    }
    printf("Done\n\r");

    spi_select_slave(pxSPIHandle, 1);
    printf("Enabling write operation in EEPROM...\n\r");
    retval = eeprom_enable_write();
    if (retval != 1)
    {
        printf("Failed\n\r");
        return;
    }
    printf("Done\n\r");

    printf("Writing to EEPROM...\n\r");
    retval = eeprom_write(wrBuf, TRANSFER_SIZE, EEPROM_ADDRESS);
    if (retval != 1)
    {
        printf("Failed\n\r");
        return;
    }
    printf("Done\n\r");

    printf("Reading back from EEPROM...\n\r");
    retval = eeprom_read(rdBuf, TRANSFER_SIZE, EEPROM_ADDRESS);
    if (retval != 1)
    {
        printf("Failed\n\r");
    }
    printf("Done\n\r");

    retval = memcmp(wrBuf, &rdBuf[3], TRANSFER_SIZE);
    if (retval != 0)
    {
        printf("Comparison failed\n\r");
        return;
    }

    printf("Closing SPI instance...");
    retval = spi_close(pxSPIHandle);
    if (retval != 0)
    {
        printf("Failed\n\r");
        return;
    }
    printf("Done\n\r");

    printf("SPI sample application completed\n\r");
}
