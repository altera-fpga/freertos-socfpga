/*
 * SPDX-FileCopyrightText: Copyright (C) 2025 Altera Corporation
 *
 * SPDX-License-Identifier: MIT-0
 *
 * Sample application implementation for i2c
 */
#include <string.h>
#include "osal.h"
#include "osal_log.h"
#include <task.h>
#include "socfpga_i2c.h"

/** 
 * @defgroup i2c_sample I2C
 * @ingroup samples
 * 
 * Sample application for I2C driver
 * 
 * @details
 * @subsubsection i2c_desc Description
 * This sample application demonstrates the use of the I2C driver to
 * communicate with an EEPROM device over the I2C bus. It writes a block
 * of N bytes to a specific memory address in the EEPROM and then reads
 * back the same number of bytes to verify data integrity.
 * 
 * @subsubsection i2c_pre Prerequisites
 * - The NAND daughter card must be connected to the board.
 * - An EEPROM device must be connected to the I2C bus.
 * 
 * @subsubsection i2c_param Configurable Parameters
 * - The I2C bus instance can be configured in @c I2CBUS macro.
 * - The I2C slave address can be configured in @c DEV_ADDR macro.
 * - The memory address in EEPROM can be configured in @c MEM_ADDR macro.
 * - The number of bytes to write and read back can be configured in @c NUM_TEST_BYTES macro.
 * @note Ensure the correct instance and slave address is configured. <br>
 *       The address range can vary depending on the EEPROM used.
 * 
 * @subsubsection i2c_how_to How to Run
 * 1. Follow the common README instructions to build and flash the application.
 * 2. Run the application on the board.
 * 3. Output can be observed in the UART terminal.
 * 
 * @subsubsection i2c_result Expected Results
 * - The application writes @c NUM_TEST_BYTES to the EEPROM at the specified address.
 * - It reads back the same number of bytes from the EEPROM.
 * - The read data and the written data are verified and a success message is displayed.
 */

/* Test configuration parameters */

/* The I2C controller instances used in this app */
#define I2CBUS            0

/* The I2C devices slave address */
#define DEV_ADDR          0x50

/* The EEPROM memory address used */
#define MEM_ADDR          0x0000

/* Memory address size */
#define MEM_ADDR_SZ       2

/* Bytes used in one transfer */
#define NUM_TEST_BYTES    32

/* Buffers */
uint8_t wbuf[ MEM_ADDR_SZ + NUM_TEST_BYTES ];
uint8_t rbuf[ NUM_TEST_BYTES ];

/**
 * @brief fill the buffer with an incremental pattern
 *
 * Fills buf with nbytes of incremental pattern starting with start_num
 */

static void fill_buf( uint8_t *buf, uint32_t nbytes, uint8_t startNum )
{
    uint32_t i;
    for (i = 0; i < nbytes; i++)
    {
        *(buf + i) = startNum++;
    }
}

/**
 * @brief verify the buffer contains and incremental pattern
 *
 * Verifies that buf contains an incremental pattern starting with start_num
 *
 * @return 0 on success -1 otherwise
 */
static int verify_buf( uint8_t *buf, uint32_t nbytes, uint8_t startNum )
{
    int ret = 0;
    uint32_t i;
    for (i = 0; i < nbytes; i++, startNum++)
    {
        if ((*(buf + i)) != startNum)
        {
            printf(
                    "ERROR: mismatch at index %d: expected 0x%2.2X got 0x%2.2X\n",
                    i, startNum, (*(buf + i)));
            ret = -1;
            break;
        }
    }
    return ret;
}

void i2c_task( void )
{
    int retval = 0;
    I2cHandle_t xHandle;
    I2cConfig_t config;
    uint16_t slave_addr;

    INFO("Sample application to write and read EEPORM using i2c driver");

    INFO("Configuring the i2c as master ...");

    /* Open and configure the I2C interface for standard speed */

    xHandle = i2c_open(I2CBUS);
    if (xHandle == NULL)
    {
        ERROR("Cannot open the i2c instance");
        return;
    }

    config.ulBusFreq = I2C_STANDARD_MODE_BPS;
    retval = i2c_ioctl(xHandle, eI2CSetMasterConfig, &config);
    if (retval != 0)
    {
        ERROR("Configuring i2c speed failed");
        i2c_close(xHandle);
        return;
    }

    /* set the slave address */
    slave_addr = DEV_ADDR;
    retval = i2c_ioctl(xHandle, eI2CSetSlaveAddr, (void*) (&slave_addr));
    if (retval != 0)
    {
        ERROR("Configuring slave address failed");
        i2c_close(xHandle);
        return;
    }

    INFO("Configuration done.");

    INFO("Performing EEPROM write, read and verify");

    /* Write the test bytes into EEPROM */

    /* First two bytes of the write buffer is the memory address in EEPROM */
    wbuf[ 0 ] = (uint8_t) ((MEM_ADDR >> 8) & 0xFF);
    wbuf[ 1 ] = (uint8_t) (MEM_ADDR & 0xFF);

    /* Add the test bytes */
    fill_buf((wbuf + 2), NUM_TEST_BYTES, 0x10);

    /* Program the EEPROM with the pattern */
    retval = i2c_write_sync(xHandle, wbuf,
            (MEM_ADDR_SZ + NUM_TEST_BYTES));
    if (retval != 0)
    {
        ERROR("EEPROM write failed");
        i2c_close(xHandle);
        return;
    }

    /* EEPROM may need a small delay between back to back write */
    osal_task_delay(10);

    /* Read back from the EEPROM */

    wbuf[ 0 ] = (uint8_t) ((MEM_ADDR >> 8) & 0xFF);
    wbuf[ 1 ] = (uint8_t) (MEM_ADDR & 0xFF);

    /* Write the memory address first */
    retval = i2c_write_sync(xHandle, wbuf, MEM_ADDR_SZ);
    if (retval != 0)
    {
        ERROR("ERROR: writing address to EEPROM failed");
        return;
    }

    /* perform the read */
    retval = i2c_read_sync(xHandle, rbuf, NUM_TEST_BYTES);
    if (retval != 0)
    {
        ERROR("ERROR: read from EEPROM failed");
        return;
    }
    retval = verify_buf(rbuf, NUM_TEST_BYTES, 0x10);

    if (retval == 0)
    {
        INFO("EEPROM write, read and verify successful");
    }
    else
    {
        ERROR("EEPROM write, read and verify failed");
    }

    INFO("I2c sample application completed.");
}

