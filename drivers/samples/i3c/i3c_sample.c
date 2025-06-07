/*
 * SPDX-FileCopyrightText: Copyright (C) 2025 Altera Corporation
 *
 * SPDX-License-Identifier: MIT-0
 *
 * Sample application for I3c
 */

/**
 * @file i3c_sample.c
 * @brief Sample Application for I3C
 */

#include <FreeRTOS.h>
#include <FreeRTOSConfig.h>
#include <task.h>
#include <stdbool.h>
#include <string.h>
#include <socfpga_i3c.h>
#include "osal.h"
#include "osal_log.h"
#include "i3c_sample.h"

/**
* @defgroup i3c_sample I3C
* @ingroup samples
* 
* Sample Application for I3C
* 
* @details
* @subsubsection i3c_desc Description 
* This is a sample application to demonstrate the usage of I3C driver.
* It writes data to an EEPROM using I3C and then reads it back.
* The written data is compared with the read data to verify the operation.
* The sample uses LPS27HHW temperature sensor as the I3C slave. The application also tries
* to read temperature from the sensor for 10 iterations.
*
* @subsubsection i3c_pre Prerequisites
* - Test daughter card must be connected to the board.
* - A serial EEPROM with I2C interface must be connected to the I3C bus.
* - A LPS27HHW temperature sensor must be connected to the I3C bus.
* 
* @subsubsection i3c_param Configurable Parameters
* - The memory address to write and read back can be configured in @c EEPROM_START_ADDRESS macro.
* - The number of bytes to transfer can be configured in @c NUM_TEST_BYTES macro.
* - The I2C test can be disabled or enabled with macro @c ENABLE_I2C_I3C_TEST in drivers/samples/i3c/i3c_sample.h
* @note Ensure the instance and slave select are valid. <br>
*       The EEPROM commands and address can vary depending on the EEPROM used.
* 
* @subsubsection i3c_how_to How to Run
* 1. Follow the common README instructions to build and flash the application.
* 2. Run the application on the board with the EEPROM connected.
* 3. Observe the UART terminal for status messages.
* 
* @subsubsection i3c_result Expected Results
* - The application sends the Write Enable and Write commands to store data in EEPROM.
* - It then reads the data back using the Read command.
* - The read and written data are compared, and a success message is printed on match.
* - Temperature in degree celsius from the LPS27HHW sensor can be observed for 10 iterations.
*/

#define I2C_ENABLE           1
#define I3C_INSTANCE         1
#define NUM_OF_DEVICES       8U
#define DEV_ADDRESS          0
#define NUM_TEST_BYTES       64

/* LPS27HHWregister addresses */
#define WHOAMI               0x0F

#define CTRL_1               0x10
#define CTRL_2               0x11
#define CTRL_3               0x12

#define FIFO_CTRL            0x13

#define TEMP_OUT_L           0x2B
#define TEMP_OUT_H           0x2C

#define STATUS               0x27

/* LPS27HHW Device ID */
#define LPS27HHW_DEV_ID      0xB3
#define EEPROM_ADDRESS       0x50
#define EEPROM_START_ADDR    0x00

uint8_t wrBuf[ NUM_TEST_BYTES + 5 ];
uint8_t rdBuf[ NUM_TEST_BYTES + 5 ];

struct i3c_I3cDevice connected_devices[] =
{
    { .staticAddress = DEV_ADDRESS,
      .deviceId = 0x0000020800B30,    // LPS27HHTW MEMS pressure sensor PID(TRM)
      .preferredDynamicAddress = 0 },

    { .staticAddress = EEPROM_ADDRESS, .deviceId = 0x0, //Mark the PID as zero for legacy I2C devices
      .preferredDynamicAddress = 0 },
};

/*Function to fill the buffer*/

static void fill_buffer( uint8_t *buf, uint32_t nbytes, uint8_t startNum )
{
    uint32_t i;
    for (i = 0; i < nbytes; i++)
    {
        *(buf + i) = startNum++;
    }
}

/*Function to verify the buffer content*/
static int verify_buffer( uint8_t *buf, uint32_t nbytes, uint8_t startNum )
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

/*Function to read  from  I2C EEPROM*/
int eeprom_i3c_i2c_read( uint8_t reg_address, uint8_t *data, uint32_t numBytes )
{

    int retval = 0;
    struct i3c_XferRequest xferRequest[ 2 ];
    uint8_t XferCmd[ 2 ];
    bool isI2c = true;

    XferCmd[ 0 ] = (reg_address >> 8) & 0xFF;
    XferCmd[ 1 ] = reg_address & 0xFF;

    xferRequest[ 0 ].buffer = XferCmd;
    xferRequest[ 0 ].length = 2;
    xferRequest[ 0 ].read = false;

    memset(data, 0, numBytes);
    xferRequest[ 1 ].buffer = data;
    xferRequest[ 1 ].length = numBytes;
    xferRequest[ 1 ].read = true;

    /* The I2C transfer function */
    retval = i3c_TransferSync(I3C_INSTANCE, EEPROM_ADDRESS, xferRequest, 2,
            isI2c);

    return retval;
}

/*Function to write to I2C EEPROM*/
int eeprom_i3c_i2c_write( uint8_t reg_address, uint8_t *data, uint8_t numBytes )
{
    int32_t retval = 0;
    uint8_t XferData[ numBytes + 2 ], i;
    bool isI2c = true;

    struct i3c_XferRequest xferRequest;

    XferData[ 0 ] = (reg_address >> 8) & 0xFF;
    XferData[ 1 ] = reg_address & 0xFF;

    for (i = 0; i < numBytes; i++)
    {
        XferData[ 2 + i ] = data[ i ];
    }
    xferRequest.buffer = XferData;
    xferRequest.length = numBytes + 2;
    xferRequest.read = false;

    retval = i3c_TransferSync(I3C_INSTANCE, EEPROM_ADDRESS, &xferRequest, 1,
            isI2c);

    printf("The return value for write is %d\r\n", retval);
    return retval;
}

int lps27hhw_read_register( uint8_t reg_address, uint8_t *data,
        uint8_t numBytes )
{
    int32_t retval = 0;
    struct i3c_XferRequest xferRequest[ 2 ];
    bool isI2c = false;

    xferRequest[ 0 ].buffer = &reg_address;
    xferRequest[ 0 ].length = sizeof(reg_address);
    xferRequest[ 0 ].read = 0;

    memset(data, 0, numBytes);
    xferRequest[ 1 ].buffer = data;
    xferRequest[ 1 ].length = numBytes;
    xferRequest[ 1 ].read = 1;

    retval = i3c_TransferSync(I3C_INSTANCE,
            connected_devices[ 0 ].dynamicAddress, xferRequest, 2, isI2c);

    return retval;
}

int lps27hhw_write_register( uint8_t reg_address, uint8_t *data,
        uint8_t numBytes )
{
    int32_t retval = 0;
    uint8_t xferData[ 10 ], i;
    bool isI2c = false;

    struct i3c_XferRequest xferRequest;

    xferData[ 0 ] = reg_address;
    for (i = 0; i < numBytes; i++)
    {
        xferData[ 1 + i ] = data[ i ];
    }

    xferRequest.buffer = xferData;
    xferRequest.length = numBytes + 1; /* for register address */
    xferRequest.read = 0; /* write request */

    retval = i3c_TransferSync(I3C_INSTANCE,
            connected_devices[ 0 ].dynamicAddress, &xferRequest, 1, isI2c);

    return retval;
}

void i3c_task( void )
{

    struct i3c_DevList DevList =
    { .numDevices = 2, .list = connected_devices, };

    int32_t retval;
    uint8_t reg_address = 0x0F;
    uint8_t whoamI = 0, data[ 4 ];
    uint32_t itr = 0;

    /*Configure and initialize the i3c controller instances
     * Setup the role of the instance(primary/secondary)
     * Initializes the address allotment table*/

    retval = i3c_open(I3C_INSTANCE);
    if (retval != 0)
    {
        printf("i3c not intialized\r\n");
    }

    /*Adds the device connected to the controller other than the I2C devices*/
    retval = i3c_ioctl(I3C_INSTANCE, I3C_IOCTL_TARGET_ATTACH, &DevList);
    if (retval != 0)
    {
        printf("I3C Target not attached\r\n");
    }

    /*Used for enumerating the i3c bus. Performs DASA*/
    retval = i3c_ioctl(I3C_INSTANCE, I3C_IOCTL_BUS_INIT,
            &connected_devices[ 0 ]);
    if (retval != 0)
    {
        printf("Dynamic address not assigned to devices\r\n");
    }
#ifdef ENABLE_I2C_I3C_TEST
    printf("Starting I2C TEST\r\n");
    /*Perform Test for the I2C device*/
    retval = i3c_ioctl(I3C_INSTANCE, I2C_IOCTL_ADDRESS_VALID,
            &connected_devices[ 1 ]);
    if (retval != 0)
    {
        printf("Invalid I2C device access\r\n");
    }
    else
    {
        fill_buffer(wrBuf, NUM_TEST_BYTES, 0x30);
        printf("WRITE DATA\r\n");
        for (int i = 0; i < NUM_TEST_BYTES; i++)
        {
            printf("%d\r\n", wrBuf[ i ]);
        }

        retval = eeprom_i3c_i2c_write(EEPROM_START_ADDR, wrBuf, NUM_TEST_BYTES);
        if (retval != 0)
        {
            printf("Failed to write  data to the EEPROM\r\n");
        }
        osal_task_delay(100);
        retval = eeprom_i3c_i2c_read(EEPROM_START_ADDR, rdBuf, NUM_TEST_BYTES);
        printf("READ DATA\r\n");
        for (int i = 0; i < NUM_TEST_BYTES; i++)
        {
            printf("%d\r\n", rdBuf[ i ]);
        }
        if (retval != 0)
        {
            printf("Failed to read  data from the EEPROM\r\n");
        }
        retval = verify_buffer(rdBuf, NUM_TEST_BYTES, 0x30);
        if (retval == 0)
        {
            printf("EEPROM read and write successful\r\n");
        }
        else
        {
            printf("EEPROM read and write mismatch\r\n");
        }
    }
#endif
    printf("Starting I3C TEST\r\n");
    /*Get the assigned dynamic address of the I3C device*/
    retval = i3c_ioctl(I3C_INSTANCE, I3C_IOCTL_GET_DYNADDRESS,
            &connected_devices[ 0 ]);
    if (retval != 0)
    {
        printf("Dynamic address not assigned to devices after DASA\r\n");
    }
    else
    {
        /* Performs read transfer of the whoamI register of LPS27HHW device
         * The read is synchronous in nature*/
        reg_address = WHOAMI;
        whoamI = 0;
        retval = lps27hhw_read_register(reg_address, &whoamI, 1);
        if (retval != 0)
        {
            printf("Read Transfer not successful\r\n");
        }
        else
        {
            printf("LPS27HHW Device ID received : 0x%x, Actual= 0x%x\r\n",
                    whoamI, LPS27HHW_DEV_ID);
        }

        /*
         * perform SW reset
         */

        reg_address = CTRL_2;
        memset(data, 0, sizeof(data));
        retval = lps27hhw_read_register(reg_address, &data[ 0 ], 1);

        reg_address = CTRL_2;
        data[ 0 ] |= 0x4; /* CTRL 2 val */
        retval = lps27hhw_write_register(reg_address, &data[ 0 ], 1);

        do
        {
            reg_address = CTRL_2;
            data[ 0 ] = 0; // memset(data,0, sizeof(data));
            retval = lps27hhw_read_register(reg_address, &data[ 0 ], 1);
        } while (data[ 0 ] & 0x4);   /* wait until SW bit is cleared */

        reg_address = CTRL_1;
        data[ 0 ] = 0x12; /* SET BDU and ODR */
        retval = lps27hhw_write_register(reg_address, &data[ 0 ], 1);

        {
            /* read back ctrl_1 to verify */
            osal_task_delay(10);
            reg_address = CTRL_1;
            data[ 0 ] = 0;
            retval = lps27hhw_read_register(reg_address, &data[ 0 ], 1);
        }

        /* read temperature continuously for 10 times */
        while (itr < 10)
        {
            /* read the status register to check if new temp data is generated */
            /* read temp reading*/
            reg_address = STATUS;
            data[ 0 ] = 0;
            retval = lps27hhw_read_register(reg_address, &data[ 0 ], 1);
            osal_task_delay(100);
            if ((retval == 0) && (data[ 0 ] & 0x2))   /* bit 1 indicates new temp data */
            {
                /* read temp reading*/
                reg_address = TEMP_OUT_L;
                data[ 0 ] = 0;
                retval = lps27hhw_read_register(reg_address, &data[ 0 ], 1);
                osal_task_delay(100);
                if (retval != 0)
                {
                    printf("Write Transfer not successful\r\n");
                }
                else
                {
                    reg_address = TEMP_OUT_H;
                    data[ 1 ] = 0;
                    retval = lps27hhw_read_register(reg_address, &data[ 1 ], 1);
                    if (retval != 0)
                    {
                        printf("Write Transfer not successful\r\n");
                    }
                    else
                    {
                        printf("Temperature reading =  %d *C\r\n",
                                ((data[ 1 ] << 8) | (data[ 0 ])) / 100);
                    }
                }
            }
            osal_task_delay(1000);
            itr++;
        }
    }
}
