/*
 * FreeRTOS Common IO V0.1.3
 * Copyright (C) 2021 Amazon.com, Inc. or its affiliates.  All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * http://aws.amazon.com/freertos
 * http://www.FreeRTOS.org
 */

/*******************************************************************************
 * @file iot_test_i2c.c
 * @brief Functional Unit Test - I2C
 *******************************************************************************
 */

#include <errno.h>
#include "unity.h"
#include "unity_fixture.h"

#include "FreeRTOS.h"
#include "i2c_test.h"
#include "semphr.h"
#include "socfpga_i2c.h"

/* Max allowed length of data to write. */
#ifndef IOT_TEST_COMMON_IO_I2C_MAX_WRITE_LENGTH
#define IOT_TEST_COMMON_IO_I2C_MAX_WRITE_LENGTH    ( 10 )
#endif

/*-----------------------------------------------------------*/
#define testIotI2C_BAUDRATE               IOT_I2C_FAST_MODE_BPS
#define testIotI2C_BAUDRATE_FPLUS         IOT_I2C_FAST_PLUS_MODE_BPS
#define testIotI2C_BAUDRATE_LOW_SPEED     IOT_I2C_STANDARD_MODE_BPS
#define testIotI2C_DEFAULT_TIMEOUT        500 /**< 500 msec */
#define testIotI2C_FAST_TIMEOUT           100 /**< 100 msec */
#define testIotI2C_INVALID_IOCTL_INDEX    UINT8_MAX
#define testIotI2C_HANDLE_NUM             4
#define testIotI2C_MAX_TIMEOUT            pdMS_TO_TICKS( 10000 )
#define testIotI2C_MESSAGE_LENGTH         50
#define EEPROM_SIZE                       256000
#define MAX_I2C_INSTANCES                 5U
#define I2C_TEST_INSTANCE                 0U
#define I2C_TEST_BUFFER_SIZE              64
#define I2C_TEST_START_VALUE              0x00
/*-----------------------------------------------------------*/

typedef struct CallbackParam
{
    IotI2CHandle_t xHandle;
    uint8_t *pucBuffer;
    size_t xBufferSize;
    uint32_t ulRet;
} CallbackParam_t;

typedef enum
{
    eRead, eWrite
} UsrCntxt_t;

/*-----------------------------------------------------------*/

/* Globals values which can be overwritten by the test
 * framework invoking these tests */
/*-----------------------------------------------------------*/

uint16_t uctestIotI2CSlaveAddr = 0x50; /**< The slave address to be set for the I2C port. */
uint16_t uctestIotI2CInvalidSlaveAddr = 0xFFFF; /**< The slave address to be set for the I2C port. */
uint8_t uctestIotI2CDeviceRegister[ 2 ] =
{ 0 }; /**< The device register to be set for the I2C port. */
uint8_t uctestIotI2CEepromWbuf[ 34 ] =
{ 0 };
uint8_t uctestIotI2CEepromRbuf[ 32 ] =
{ 0 };
uint8_t cpBuf[ 32 ] =
{ 0 };
uint8_t cpBufRead[ 32 ] =
{ 0 };
uint8_t uctestIotI2CWriteVal = 0; /**< The write value to write to device. */
uint8_t uctestIotI2CInstanceIdx = 0; /**< The current I2C test instance index */
uint8_t uctestIotI2CInstanceNum = 6; /**< The total I2C test instance number */
uint16_t ucAssistedTestIotI2CSlaveAddr = 0; /**< The slave address to be set for the assisted test. */

/*-----------------------------------------------------------*/
/* Static Globals */
/*-----------------------------------------------------------*/
static SemaphoreHandle_t xtestIotI2CSemaphore = NULL;
static SemaphoreHandle_t xReadCompleteSemaphore = NULL;

static IotI2CHandle_t xI2CGlobalHandle = NULL;
static IotI2CHandle_t xI2CHandle = NULL;

/*-----------------------------------------------------------*/
/* Private Functions */
/*-----------------------------------------------------------*/

/*-----------------------------------------------------------*/

/* Define Test Group. */
TEST_GROUP(TEST_IOT_I2C);

/*-----------------------------------------------------------*/

/**
 * @brief Setup function called before each test in this group is executed.
 */
TEST_SETUP( TEST_IOT_I2C )
{
    xtestIotI2CSemaphore = xSemaphoreCreateBinary();
    TEST_ASSERT_NOT_EQUAL(NULL, xtestIotI2CSemaphore);
}

/*-----------------------------------------------------------*/

/**
 * @brief Tear down function called after each test in this group is executed.
 */
TEST_TEAR_DOWN( TEST_IOT_I2C )
{
}

/*-----------------------------------------------------------*/

/*
 * @brief fill the buffer with an incremental pattern
 *
 * Fills buf with nbytes of incremental pattern starting with start_num
 */

static void fill_buf( uint8_t *buf, uint32_t nbytes, uint8_t startNum )
{
    uint32_t i;
    for ( i = 0; i < nbytes; i++ )
    {
        *(buf + i) = startNum++;
    }
}

int compare_buf( uint8_t *buffer, size_t length, uint8_t start_value )
{
    for ( size_t i = 0; i < length; i++ )
    {
        if ( buffer[ i ] != start_value + i )
        {
            return 0;
        }
    }
    return 1;
}

/*-----------------------------------------------------------*/

/*
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
    for ( i = 0; i < nbytes; i++, startNum++ )
    {
        if ( (*(buf + i)) != startNum )
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

/*-----------------------------------------------------------*/

/**
 * @brief user callback function
 */
void prvI2CCallback( IotI2COperationStatus_t xOpStatus, void *pvParam )
{
    /* Disable unused parameter warning. */
    (void) pvParam;

    if ( xOpStatus == eI2CCompleted )
    {
        xSemaphoreGiveFromISR(xtestIotI2CSemaphore, NULL);
    }
}

/*-----------------------------------------------------------*/

/**
 * @brief user callback function
 */
static void prvReadWriteCallback( IotI2COperationStatus_t xOpStatus,
        void *pvParams )
{
    /* Disable unused parameter warning. */
    UsrCntxt_t *op = (UsrCntxt_t*) (pvParams);
    int32_t lRetVal;
    uint16_t devReg = 0x0020;

    if ( xOpStatus == eI2CCompleted )
    {
        if ( *op == eRead )
        {
            xSemaphoreGiveFromISR(xReadCompleteSemaphore, NULL);
        }
        else if ( *op == eWrite )
        {
            /* Initiate read sequence from callback */
            *op = eRead;
            uctestIotI2CEepromWbuf[ 0 ] = (devReg >> 8) & 0xFF;
            uctestIotI2CEepromWbuf[ 1 ] = devReg & 0xFF;

            iot_i2c_set_callback(xI2CGlobalHandle, prvReadWriteCallback, op);
            /*Add simple delay to wait for eeprom internal write cycle.
             * vtaskdelay cannot be used inside intrpt */
            for ( volatile uint64_t i = 0; i < 300000; i++ )
                ;

            lRetVal = iot_i2c_write_sync(xI2CGlobalHandle,
                    uctestIotI2CEepromWbuf, 2);
            TEST_ASSERT_EQUAL(0, lRetVal);

            lRetVal = iot_i2c_read_async(xI2CGlobalHandle, cpBufRead, 32);
            TEST_ASSERT_EQUAL(0, lRetVal);
        }
    }
}

/*-----------------------------------------------------------*/

/**
 * @brief Function to execute for test.
 */
void i2c_test()
{
    UNITY_BEGIN();

    RUN_TEST_GROUP(TEST_IOT_I2C);

    UNITY_END();
}

/*-----------------------------------------------------------*/

/**
 * @brief Function to define which tests to execute as part of this group.
 */
TEST_GROUP_RUNNER( TEST_IOT_I2C )
{
    RUN_TEST_CASE(TEST_IOT_I2C, AFQP_IotI2COpenCloseSuccess);
    RUN_TEST_CASE(TEST_IOT_I2C, AFQP_IotI2COpenCloseFail);
    RUN_TEST_CASE(TEST_IOT_I2C, AFQP_IotI2COpenCloseFailUnsupportedInst);
    RUN_TEST_CASE(TEST_IOT_I2C, AFQP_IotI2CSetGetConfigurationSuccess);
    RUN_TEST_CASE(TEST_IOT_I2C, AFQP_IotI2CSetGetConfigurationFail);
    RUN_TEST_CASE(TEST_IOT_I2C, AFQP_IotI2CWriteThenReadSyncSuccess);
    RUN_TEST_CASE(TEST_IOT_I2C, AFQP_IotI2CWriteThenReadAsyncSuccess);
    RUN_TEST_CASE(TEST_IOT_I2C, AFQP_IotI2C_EEPROMWriteReadSyncVerify);
    RUN_TEST_CASE(TEST_IOT_I2C, AFQP_IotI2C_EEPROMWriteReadAsyncVerify);
    RUN_TEST_CASE(TEST_IOT_I2C, AFQP_IotI2C_EEPROMWriteSyncReadAsyncVerify);
    RUN_TEST_CASE(TEST_IOT_I2C, AFQP_IotI2C_EEPROMWriteAsyncReadSyncVerify);
    RUN_TEST_CASE(TEST_IOT_I2C, AFQP_IotI2C_EEPROMWriteReadAsyncVerifyCallback);
    RUN_TEST_CASE(TEST_IOT_I2C, AFQP_IotI2C_RepeatedTransferTest);
    /* add some delay while switching the speed */
    vTaskDelay(10);
    RUN_TEST_CASE(TEST_IOT_I2C, AFQP_IotI2C_EEPROMWriteReadSyncVerifyFastMode);
    RUN_TEST_CASE(TEST_IOT_I2C,
            AFQP_IotI2C_EEPROMWriteReadSyncVerifyFastPlusMode);

    RUN_TEST_CASE(TEST_IOT_I2C, test_i2c_open_valid_instance);
    RUN_TEST_CASE(TEST_IOT_I2C, test_i2c_open_invalid_instance);
    RUN_TEST_CASE(TEST_IOT_I2C, test_i2c_open_already_open);

    RUN_TEST_CASE(TEST_IOT_I2C, test_i2c_read_sync_success);
    RUN_TEST_CASE(TEST_IOT_I2C, test_i2c_read_sync_null_handle);
    RUN_TEST_CASE(TEST_IOT_I2C, test_i2c_read_sync_null_buffer);
    RUN_TEST_CASE(TEST_IOT_I2C, test_i2c_read_sync_zero_bytes);
    RUN_TEST_CASE(TEST_IOT_I2C, test_i2c_read_sync_slave_address_not_set);
    RUN_TEST_CASE(TEST_IOT_I2C, test_i2c_read_sync_nack);
    RUN_TEST_CASE(TEST_IOT_I2C, test_i2c_read_sync_busy);

    RUN_TEST_CASE(TEST_IOT_I2C, test_i2c_write_sync_success);
    RUN_TEST_CASE(TEST_IOT_I2C, test_i2c_write_sync_null_handle);
    RUN_TEST_CASE(TEST_IOT_I2C, test_i2c_write_sync_null_buffer);
    RUN_TEST_CASE(TEST_IOT_I2C, test_i2c_write_sync_zero_bytes);
    RUN_TEST_CASE(TEST_IOT_I2C, test_i2c_write_sync_slave_address_not_set);
    RUN_TEST_CASE(TEST_IOT_I2C, test_i2c_write_sync_busy);

    RUN_TEST_CASE(TEST_IOT_I2C, test_i2c_read_async_success);
    RUN_TEST_CASE(TEST_IOT_I2C, test_i2c_read_async_null_handle);
    RUN_TEST_CASE(TEST_IOT_I2C, test_i2c_read_async_null_buffer);
    RUN_TEST_CASE(TEST_IOT_I2C, test_i2c_read_async_zero_bytes);
    RUN_TEST_CASE(TEST_IOT_I2C, test_i2c_read_async_slave_address_not_set);
    RUN_TEST_CASE(TEST_IOT_I2C, test_i2c_read_async_busy);

    RUN_TEST_CASE(TEST_IOT_I2C, test_i2c_write_async_success);
    RUN_TEST_CASE(TEST_IOT_I2C, test_i2c_write_async_null_handle);
    RUN_TEST_CASE(TEST_IOT_I2C, test_i2c_write_async_null_buffer);
    RUN_TEST_CASE(TEST_IOT_I2C, test_i2c_write_async_zero_bytes);
    RUN_TEST_CASE(TEST_IOT_I2C, test_i2c_write_async_slave_address_not_set);
    RUN_TEST_CASE(TEST_IOT_I2C, test_i2c_write_async_busy);

    RUN_TEST_CASE(TEST_IOT_I2C, test_i2c_ioctl_success_set_master_config);
    RUN_TEST_CASE(TEST_IOT_I2C, test_i2c_ioctl_null_handle);
    RUN_TEST_CASE(TEST_IOT_I2C, test_i2c_ioctl_null_buffer);
    RUN_TEST_CASE(TEST_IOT_I2C, test_i2c_ioctl_get_master_config);
    RUN_TEST_CASE(TEST_IOT_I2C, test_i2c_ioctl_set_slave_addr);
    RUN_TEST_CASE(TEST_IOT_I2C, test_i2c_ioctl_get_tx_no_of_bytes);
    RUN_TEST_CASE(TEST_IOT_I2C, test_i2c_ioctl_get_rx_no_of_bytes);
    RUN_TEST_CASE(TEST_IOT_I2C, test_ioctl_null_buffer);
    RUN_TEST_CASE(TEST_IOT_I2C, test_set_callback_null_handle);

    RUN_TEST_CASE(TEST_IOT_I2C, test_i2c_close_success);
    RUN_TEST_CASE(TEST_IOT_I2C, test_i2c_close_null_handle);
    RUN_TEST_CASE(TEST_IOT_I2C, test_i2c_close_not_opened);

    RUN_TEST_CASE(TEST_IOT_I2C, test_i2c_cancel_success);
    RUN_TEST_CASE(TEST_IOT_I2C, test_i2c_cancel_null_handle);
    RUN_TEST_CASE(TEST_IOT_I2C, test_i2c_cancel_not_opened);
    RUN_TEST_CASE(TEST_IOT_I2C, test_i2c_cancel_nothing_to_cancel);

    RUN_TEST_CASE(TEST_IOT_I2C, test_i2c_open_max_instances);
    RUN_TEST_CASE(TEST_IOT_I2C, test_i2c_read_sync_not_opened);
    RUN_TEST_CASE(TEST_IOT_I2C, test_i2c_write_sync_not_opened);
    RUN_TEST_CASE(TEST_IOT_I2C, test_i2c_read_async_not_opened);
    RUN_TEST_CASE(TEST_IOT_I2C, test_i2c_write_async_not_opened);
    RUN_TEST_CASE(TEST_IOT_I2C, test_i2c_ioctl_invalid_request);

    RUN_TEST_CASE(TEST_IOT_I2C, test_i2c_open_close_multiple_times);
    RUN_TEST_CASE(TEST_IOT_I2C, test_i2c_open_close_all_instances);
    RUN_TEST_CASE(TEST_IOT_I2C, test_i2c_open_close_with_errors);
}

/*-----------------------------------------------------------*/

/**
 * @brief Test Function to test iot_i2c_open and iot_i2c_close.
 */
TEST( TEST_IOT_I2C, AFQP_IotI2COpenCloseSuccess )
{
    int32_t lRetVal;

    /* Open i2c to initialize hardware */
    xI2CHandle = iot_i2c_open(uctestIotI2CInstanceIdx);
    TEST_ASSERT_NOT_EQUAL(NULL, xI2CHandle);

    lRetVal = iot_i2c_close(xI2CHandle);
    TEST_ASSERT_EQUAL(0, lRetVal);
}

/*-----------------------------------------------------------*/

/**
 * @brief Test Function to test iot_i2c_open and iot_i2c_close fail case.
 */
TEST( TEST_IOT_I2C, AFQP_IotI2COpenCloseFail )
{
    IotI2CHandle_t xI2CHandle_1 = NULL;
    int32_t lRetVal;

    /* Open i2c to initialize hardware */
    xI2CHandle = iot_i2c_open(uctestIotI2CInstanceIdx);
    TEST_ASSERT_NOT_EQUAL(NULL, xI2CHandle);

    if ( TEST_PROTECT() )
    {
        /* Open i2c to initialize hardware again */
        xI2CHandle_1 = iot_i2c_open(uctestIotI2CInstanceIdx);
        TEST_ASSERT_EQUAL(NULL, xI2CHandle_1);
    }

    lRetVal = iot_i2c_close(xI2CHandle);
    TEST_ASSERT_EQUAL(0, lRetVal);

    /* Close same i2c again which is not open */
    lRetVal = iot_i2c_close(xI2CHandle);
    TEST_ASSERT_EQUAL(-EINVAL, lRetVal);

    /* Close i2c with NULL pointer */
    lRetVal = iot_i2c_close(xI2CHandle_1);
    TEST_ASSERT_EQUAL(-EINVAL, lRetVal);
}

/*-----------------------------------------------------------*/

/**
 * @brief Test Function to test iot_i2c_open and iot_i2c_close with unsupported instance
 */
TEST( TEST_IOT_I2C, AFQP_IotI2COpenCloseFailUnsupportedInst )
{
    int32_t lRetVal;

    /* Open i2c to initialize hardware */
    xI2CHandle = iot_i2c_open(uctestIotI2CInstanceNum);
    TEST_ASSERT_EQUAL(NULL, xI2CHandle);

    lRetVal = iot_i2c_close(xI2CHandle);
    TEST_ASSERT_EQUAL(-EINVAL, lRetVal);
}

/*-----------------------------------------------------------*/

/**
 * @brief Test Function for I2C set and get configuration success
 */
TEST( TEST_IOT_I2C, AFQP_IotI2CSetGetConfigurationSuccess )
{
    IotI2CConfig_t xI2CConfig_write;
    IotI2CConfig_t xI2CConfig_read;
    int32_t lRetVal;

    xI2CConfig_write.ulBusFreq = testIotI2C_BAUDRATE;
    xI2CConfig_write.ulMasterTimeout = testIotI2C_DEFAULT_TIMEOUT;

    /* Open i2c to initialize hardware */
    xI2CHandle = iot_i2c_open(uctestIotI2CInstanceIdx);
    TEST_ASSERT_NOT_EQUAL(NULL, xI2CHandle);

    if ( TEST_PROTECT() )
    {
        /* Set default i2c configuration */
        lRetVal = iot_i2c_ioctl(xI2CHandle, eI2CSetMasterConfig,
                &xI2CConfig_write);
        TEST_ASSERT_EQUAL(0, lRetVal);

        /* Get i2c configuration */
        lRetVal = iot_i2c_ioctl(xI2CHandle, eI2CGetMasterConfig,
                &xI2CConfig_read);
        TEST_ASSERT_EQUAL(0, lRetVal);

        /* Make sure get the correct config from i2c */
        TEST_ASSERT_EQUAL(testIotI2C_BAUDRATE, xI2CConfig_read.ulBusFreq);

        xI2CConfig_write.ulBusFreq = testIotI2C_BAUDRATE_LOW_SPEED;

        /* Set new i2c configuration */
        lRetVal = iot_i2c_ioctl(xI2CHandle, eI2CSetMasterConfig,
                &xI2CConfig_write);
        TEST_ASSERT_EQUAL(0, lRetVal);

        /* Get new i2c configuration */
        lRetVal = iot_i2c_ioctl(xI2CHandle, eI2CGetMasterConfig,
                &xI2CConfig_read);
        TEST_ASSERT_EQUAL(0, lRetVal);

        /* Make sure get the correct config from i2c */
        TEST_ASSERT_EQUAL(testIotI2C_BAUDRATE_LOW_SPEED,
                xI2CConfig_read.ulBusFreq);
    }

    lRetVal = iot_i2c_close(xI2CHandle);
    TEST_ASSERT_EQUAL(0, lRetVal);
}

/*-----------------------------------------------------------*/

/**
 * @brief Test Function for I2C set and get configuration fail
 */
TEST( TEST_IOT_I2C, AFQP_IotI2CSetGetConfigurationFail )
{
    IotI2CConfig_t xI2CConfig;
    int32_t lRetVal;

    /* Open i2c to initialize hardware */
    xI2CHandle = iot_i2c_open(uctestIotI2CInstanceIdx);
    TEST_ASSERT_NOT_EQUAL(NULL, xI2CHandle);

    if ( TEST_PROTECT() )
    {
        /* Set default i2c configuration with NULL pointer */
        lRetVal = iot_i2c_ioctl(xI2CHandle, eI2CSetMasterConfig, NULL);
        TEST_ASSERT_EQUAL(-EINVAL, lRetVal);

        /* Get i2c configuration with NULL pointer */
        lRetVal = iot_i2c_ioctl(xI2CHandle, eI2CGetMasterConfig, NULL);
        TEST_ASSERT_EQUAL(-EINVAL, lRetVal);

        /* i2c ioctl with unsupported request */
        lRetVal = iot_i2c_ioctl(xI2CHandle, testIotI2C_INVALID_IOCTL_INDEX,
                &xI2CConfig);
        TEST_ASSERT_EQUAL(-EINVAL, lRetVal);
    }

    lRetVal = iot_i2c_close(xI2CHandle);
    TEST_ASSERT_EQUAL(0, lRetVal);
}

/*-----------------------------------------------------------*/

/**
 * @brief Test Function for I2C async read success
 */
TEST( TEST_IOT_I2C, AFQP_IotI2CWriteThenReadAsyncSuccess )
{
    int32_t lRetVal;
    uint16_t readBytes;
    uint16_t writeBytes;

    uint8_t singleByte;

    IotI2CConfig_t xI2CConfig =
    { .ulBusFreq = testIotI2C_BAUDRATE_LOW_SPEED, .ulMasterTimeout =
    testIotI2C_DEFAULT_TIMEOUT };

    xI2CHandle = iot_i2c_open(uctestIotI2CInstanceIdx);
    TEST_ASSERT_NOT_EQUAL(NULL, xI2CHandle);

    if ( TEST_PROTECT() )
    {
        /* Set completion callback */
        iot_i2c_set_callback(xI2CHandle, prvI2CCallback, NULL);

        /* Set i2c configuration */
        lRetVal = iot_i2c_ioctl(xI2CHandle, eI2CSetMasterConfig, &xI2CConfig);
        TEST_ASSERT_EQUAL(0, lRetVal);

        /* Set i2c slave address for writing the device register */
        lRetVal = iot_i2c_ioctl(xI2CHandle, eI2CSetSlaveAddr,
                &uctestIotI2CSlaveAddr);
        TEST_ASSERT_EQUAL(0, lRetVal);

        /* Set i2c not stop between transaction */
        lRetVal = iot_i2c_ioctl(xI2CHandle, eI2CSendNoStopFlag, NULL);
        TEST_ASSERT_EQUAL(0, lRetVal);

        /* write the device register address. */
        lRetVal = iot_i2c_write_sync(xI2CHandle, uctestIotI2CDeviceRegister,
                sizeof(uctestIotI2CDeviceRegister));
        TEST_ASSERT_EQUAL(0, lRetVal);

        lRetVal = iot_i2c_ioctl(xI2CHandle, eI2CGetTxNoOfbytes, &writeBytes);
        TEST_ASSERT_EQUAL(0, lRetVal);

        /* Set i2c slave address to read from the device register. */
        lRetVal = iot_i2c_ioctl(xI2CHandle, eI2CSetSlaveAddr,
                &uctestIotI2CSlaveAddr);
        TEST_ASSERT_EQUAL(0, lRetVal);

        /* Read from i2c device for single byte. */
        lRetVal = iot_i2c_read_async(xI2CHandle, &singleByte,
                sizeof(singleByte));
        TEST_ASSERT_EQUAL(0, lRetVal);

        lRetVal = xSemaphoreTake(xtestIotI2CSemaphore, testIotI2C_MAX_TIMEOUT);
        TEST_ASSERT_EQUAL(pdTRUE, lRetVal);

        lRetVal = iot_i2c_ioctl(xI2CHandle, eI2CGetRxNoOfbytes, &readBytes);
        TEST_ASSERT_EQUAL(0, lRetVal);
        /* Assert the number of bytes being read is 1. */
        TEST_ASSERT_EQUAL(sizeof(singleByte), readBytes);

        /* Assert the number of bytes being written is 1. */
        TEST_ASSERT_EQUAL(sizeof(uctestIotI2CDeviceRegister), writeBytes);
    }

    lRetVal = iot_i2c_close(xI2CHandle);
    TEST_ASSERT_EQUAL(0, lRetVal);
}

/*-----------------------------------------------------------*/

/**
 * @brief Test Function for I2C sync write then read, in same transaction.
 */
TEST( TEST_IOT_I2C, AFQP_IotI2CWriteThenReadSyncSuccess )
{
    int32_t lRetVal;
    uint8_t singleByte;

    uint16_t readBytes;
    uint16_t writeBytes;
    uint16_t devReg = 0x0020;

    IotI2CConfig_t xI2CConfig =
    { .ulBusFreq = testIotI2C_BAUDRATE_LOW_SPEED, .ulMasterTimeout =
    testIotI2C_DEFAULT_TIMEOUT, };

    /* Open i2c to initialize hardware */
    xI2CHandle = iot_i2c_open(uctestIotI2CInstanceIdx);
    TEST_ASSERT_NOT_EQUAL(NULL, xI2CHandle);

    if ( TEST_PROTECT() )
    {
        /* Set i2c configuration */
        lRetVal = iot_i2c_ioctl(xI2CHandle, eI2CSetMasterConfig, &xI2CConfig);
        TEST_ASSERT_EQUAL(0, lRetVal);

        /* Set i2c slave address */
        lRetVal = iot_i2c_ioctl(xI2CHandle, eI2CSetSlaveAddr,
                &uctestIotI2CSlaveAddr);
        TEST_ASSERT_EQUAL(0, lRetVal);

        /* Set i2c configuration */
        lRetVal = iot_i2c_ioctl(xI2CHandle, eI2CSendNoStopFlag, NULL);
        TEST_ASSERT_EQUAL(0, lRetVal);
        uctestIotI2CDeviceRegister[ 0 ] = (devReg >> 8) & 0xFF;
        uctestIotI2CDeviceRegister[ 1 ] = devReg & 0xFF;
        /* write the device register address. */
        lRetVal = iot_i2c_write_sync(xI2CHandle, uctestIotI2CDeviceRegister,
                sizeof(uctestIotI2CDeviceRegister));
        TEST_ASSERT_EQUAL(0, lRetVal);

        lRetVal = iot_i2c_ioctl(xI2CHandle, eI2CGetTxNoOfbytes, &writeBytes);
        TEST_ASSERT_EQUAL(0, lRetVal);

        /* repeated start to read */
        lRetVal = iot_i2c_ioctl(xI2CHandle, eI2CSetSlaveAddr,
                &uctestIotI2CSlaveAddr);
        TEST_ASSERT_EQUAL(0, lRetVal);

        /* read from i2c device for single byte */
        lRetVal = iot_i2c_read_sync(xI2CHandle, &singleByte,
                sizeof(singleByte));
        TEST_ASSERT_EQUAL(0, lRetVal);

        lRetVal = iot_i2c_ioctl(xI2CHandle, eI2CGetRxNoOfbytes, &readBytes);
        TEST_ASSERT_EQUAL(0, lRetVal);

        /* Assert the number of bytes being written is 1. */
        TEST_ASSERT_EQUAL(sizeof(uctestIotI2CDeviceRegister), writeBytes);
        /* Assert the number of bytes being read is 1. */
        TEST_ASSERT_EQUAL(sizeof(singleByte), readBytes);
    }

    lRetVal = iot_i2c_close(xI2CHandle);
    TEST_ASSERT_EQUAL(0, lRetVal);
}

/*-----------------------------------------------------------*/

/**
 * @brief Test Function for I2C sync write read data success
 */
TEST( TEST_IOT_I2C, AFQP_IotI2C_EEPROMWriteReadSyncVerify)
{
    int32_t lRetVal;
    uint16_t devReg = 0x0020;
    int i;

    IotI2CConfig_t xI2CConfig =
    { .ulBusFreq = testIotI2C_BAUDRATE_LOW_SPEED, .ulMasterTimeout =
    testIotI2C_DEFAULT_TIMEOUT, };

    uctestIotI2CEepromWbuf[ 0 ] = (uint8_t) ((devReg >> 8) & 0xFF);
    uctestIotI2CEepromWbuf[ 1 ] = (uint8_t) (devReg & 0xFF);

    /* Open i2c to initialize hardware */
    xI2CHandle = iot_i2c_open(uctestIotI2CInstanceIdx);
    TEST_ASSERT_NOT_EQUAL(NULL, xI2CHandle);

    if ( TEST_PROTECT() )
    {
        for ( i = 0; i < 5; i++ )
        {
            fill_buf(&uctestIotI2CEepromWbuf[ 2 ], 32, i);
            /* Set i2c configuration */
            lRetVal = iot_i2c_ioctl(xI2CHandle, eI2CSetMasterConfig,
                    &xI2CConfig);
            TEST_ASSERT_EQUAL(0, lRetVal);

            /* Set i2c slave address */
            lRetVal = iot_i2c_ioctl(xI2CHandle, eI2CSetSlaveAddr,
                    &uctestIotI2CSlaveAddr);
            TEST_ASSERT_EQUAL(0, lRetVal);

            /* write the device register address. */
            lRetVal = iot_i2c_write_sync(xI2CHandle, uctestIotI2CEepromWbuf,
                    34);
            TEST_ASSERT_EQUAL(0, lRetVal);

            vTaskDelay(1);

            lRetVal = iot_i2c_write_sync(xI2CHandle, uctestIotI2CEepromWbuf, 2);
            TEST_ASSERT_EQUAL(0, lRetVal);

            lRetVal = iot_i2c_read_sync(xI2CHandle, uctestIotI2CEepromRbuf, 32);
            TEST_ASSERT_EQUAL(0, lRetVal);

            lRetVal = verify_buf(uctestIotI2CEepromRbuf, 32, i);
            TEST_ASSERT_EQUAL(0, lRetVal);
        }
    }

    lRetVal = iot_i2c_close(xI2CHandle);
    TEST_ASSERT_EQUAL(0, lRetVal);
}

/*-----------------------------------------------------------*/

/**
 * @brief Test Function for I2C async write read success
 */
TEST( TEST_IOT_I2C, AFQP_IotI2C_EEPROMWriteReadAsyncVerify )
{
    int32_t lRetVal;
    uint16_t devReg = 0x0020;
    int i;

    IotI2CConfig_t xI2CConfig =
    { .ulBusFreq = testIotI2C_BAUDRATE_LOW_SPEED, .ulMasterTimeout =
    testIotI2C_DEFAULT_TIMEOUT, };

    uctestIotI2CEepromWbuf[ 0 ] = (devReg >> 8) & 0xFF;
    uctestIotI2CEepromWbuf[ 1 ] = devReg & 0xFF;

    /* Open i2c to initialize hardware */
    xI2CHandle = iot_i2c_open(uctestIotI2CInstanceIdx);
    TEST_ASSERT_NOT_EQUAL(NULL, xI2CHandle);

    if ( TEST_PROTECT() )
    {
        for ( i = 0; i < 5; i++ )
        {
            fill_buf(&uctestIotI2CEepromWbuf[ 2 ], 32, i);
            /* Set i2c configuration */
            lRetVal = iot_i2c_ioctl(xI2CHandle, eI2CSetMasterConfig,
                    &xI2CConfig);
            TEST_ASSERT_EQUAL(0, lRetVal);

            /* Set i2c slave address */
            lRetVal = iot_i2c_ioctl(xI2CHandle, eI2CSetSlaveAddr,
                    &uctestIotI2CSlaveAddr);
            TEST_ASSERT_EQUAL(0, lRetVal);

            /* Set completion callback */
            iot_i2c_set_callback(xI2CHandle, prvI2CCallback, NULL);

            /* write the device register address. */
            lRetVal = iot_i2c_write_async(xI2CHandle, uctestIotI2CEepromWbuf,
                    34);
            TEST_ASSERT_EQUAL(0, lRetVal);

            lRetVal = xSemaphoreTake(xtestIotI2CSemaphore,
                    testIotI2C_MAX_TIMEOUT);
            TEST_ASSERT_EQUAL(pdTRUE, lRetVal);

            vTaskDelay(1);

            /* read from i2c device */

            lRetVal = iot_i2c_write_async(xI2CHandle, uctestIotI2CEepromWbuf,
                    2);
            TEST_ASSERT_EQUAL(0, lRetVal);

            lRetVal = xSemaphoreTake(xtestIotI2CSemaphore,
                    testIotI2C_MAX_TIMEOUT);
            TEST_ASSERT_EQUAL(pdTRUE, lRetVal);

            lRetVal = iot_i2c_read_async(xI2CHandle, uctestIotI2CEepromRbuf,
                    32);
            TEST_ASSERT_EQUAL(0, lRetVal);

            lRetVal = xSemaphoreTake(xtestIotI2CSemaphore,
                    testIotI2C_MAX_TIMEOUT);
            TEST_ASSERT_EQUAL(pdTRUE, lRetVal);

            lRetVal = verify_buf(uctestIotI2CEepromRbuf, 32, i);
            TEST_ASSERT_EQUAL(0, lRetVal);
        }
    }

    lRetVal = iot_i2c_close(xI2CHandle);
    TEST_ASSERT_EQUAL(0, lRetVal);
}

/*-----------------------------------------------------------*/

/**
 * @brief Test Function for I2C sync write and async read success
 */
TEST( TEST_IOT_I2C, AFQP_IotI2C_EEPROMWriteSyncReadAsyncVerify )
{
    int32_t lRetVal;
    uint16_t devReg = 0x0020;
    int i;

    IotI2CConfig_t xI2CConfig =
    { .ulBusFreq = testIotI2C_BAUDRATE_LOW_SPEED, .ulMasterTimeout =
    testIotI2C_DEFAULT_TIMEOUT, };

    uctestIotI2CEepromWbuf[ 0 ] = (devReg >> 8) & 0xFF;
    uctestIotI2CEepromWbuf[ 1 ] = devReg & 0xFF;

    /* Open i2c to initialize hardware */
    xI2CHandle = iot_i2c_open(uctestIotI2CInstanceIdx);
    TEST_ASSERT_NOT_EQUAL(NULL, xI2CHandle);

    if ( TEST_PROTECT() )
    {
        for ( i = 0; i < 5; i++ )
        {
            fill_buf(&uctestIotI2CEepromWbuf[ 2 ], 32, 0x10);
            /* Set i2c configuration */
            lRetVal = iot_i2c_ioctl(xI2CHandle, eI2CSetMasterConfig,
                    &xI2CConfig);
            TEST_ASSERT_EQUAL(0, lRetVal);

            /* Set i2c slave address */
            lRetVal = iot_i2c_ioctl(xI2CHandle, eI2CSetSlaveAddr,
                    &uctestIotI2CSlaveAddr);
            TEST_ASSERT_EQUAL(0, lRetVal);

            /* Set completion callback */
            iot_i2c_set_callback(xI2CHandle, prvI2CCallback, NULL);

            /* write the device register address. */
            lRetVal = iot_i2c_write_sync(xI2CHandle, uctestIotI2CEepromWbuf,
                    34);
            TEST_ASSERT_EQUAL(0, lRetVal);

            vTaskDelay(1);

            /* read from i2c device */

            lRetVal = iot_i2c_write_async(xI2CHandle, uctestIotI2CEepromWbuf,
                    2);
            TEST_ASSERT_EQUAL(0, lRetVal);

            lRetVal = xSemaphoreTake(xtestIotI2CSemaphore,
                    testIotI2C_MAX_TIMEOUT);
            TEST_ASSERT_EQUAL(pdTRUE, lRetVal);

            lRetVal = iot_i2c_read_async(xI2CHandle, uctestIotI2CEepromRbuf,
                    32);
            TEST_ASSERT_EQUAL(0, lRetVal);

            lRetVal = xSemaphoreTake(xtestIotI2CSemaphore,
                    testIotI2C_MAX_TIMEOUT);
            TEST_ASSERT_EQUAL(pdTRUE, lRetVal);

            lRetVal = verify_buf(uctestIotI2CEepromRbuf, 32, 0x10);
            TEST_ASSERT_EQUAL(0, lRetVal);
        }
    }

    lRetVal = iot_i2c_close(xI2CHandle);
    TEST_ASSERT_EQUAL(0, lRetVal);
}

/*-----------------------------------------------------------*/

/**
 * @brief Test Function for I2C async write and sync read success
 */
TEST( TEST_IOT_I2C, AFQP_IotI2C_EEPROMWriteAsyncReadSyncVerify )
{
    int32_t lRetVal;
    uint16_t devReg = 0x0020;
    int i;

    IotI2CConfig_t xI2CConfig =
    { .ulBusFreq = testIotI2C_BAUDRATE_LOW_SPEED, .ulMasterTimeout =
    testIotI2C_DEFAULT_TIMEOUT, };

    uctestIotI2CEepromWbuf[ 0 ] = (devReg >> 8) & 0xFF;
    uctestIotI2CEepromWbuf[ 1 ] = devReg & 0xFF;

    /* Open i2c to initialize hardware */
    xI2CHandle = iot_i2c_open(uctestIotI2CInstanceIdx);
    TEST_ASSERT_NOT_EQUAL(NULL, xI2CHandle);

    if ( TEST_PROTECT() )
    {
        for ( i = 0; i < 5; i++ )
        {
            fill_buf(&uctestIotI2CEepromWbuf[ 2 ], 32, i);
            /* Set i2c configuration */
            lRetVal = iot_i2c_ioctl(xI2CHandle, eI2CSetMasterConfig,
                    &xI2CConfig);
            TEST_ASSERT_EQUAL(0, lRetVal);

            /* Set i2c slave address */
            lRetVal = iot_i2c_ioctl(xI2CHandle, eI2CSetSlaveAddr,
                    &uctestIotI2CSlaveAddr);
            TEST_ASSERT_EQUAL(0, lRetVal);

            /* Set completion callback */
            iot_i2c_set_callback(xI2CHandle, prvI2CCallback, NULL);

            /* write the device register address. */
            lRetVal = iot_i2c_write_async(xI2CHandle, uctestIotI2CEepromWbuf,
                    34);
            TEST_ASSERT_EQUAL(0, lRetVal);

            lRetVal = xSemaphoreTake(xtestIotI2CSemaphore,
                    testIotI2C_MAX_TIMEOUT);
            TEST_ASSERT_EQUAL(pdTRUE, lRetVal);

            vTaskDelay(1);

            /* read from i2c device */

            lRetVal = iot_i2c_write_sync(xI2CHandle, uctestIotI2CEepromWbuf, 2);
            TEST_ASSERT_EQUAL(0, lRetVal);

            lRetVal = iot_i2c_read_sync(xI2CHandle, uctestIotI2CEepromRbuf, 32);
            TEST_ASSERT_EQUAL(0, lRetVal);

            lRetVal = verify_buf(uctestIotI2CEepromRbuf, 32, i);
            TEST_ASSERT_EQUAL(0, lRetVal);
        }
    }

    lRetVal = iot_i2c_close(xI2CHandle);
    TEST_ASSERT_EQUAL(0, lRetVal);
}

/*-----------------------------------------------------------*/

/**
 * @brief Test Function for I2C sync write read datasuccess in fast mode
 */
TEST( TEST_IOT_I2C, AFQP_IotI2C_EEPROMWriteReadSyncVerifyFastMode)
{
    int32_t lRetVal;
    uint16_t devReg = 0x0020;
    int i;

    IotI2CConfig_t xI2CConfig =
    { .ulBusFreq = testIotI2C_BAUDRATE, .ulMasterTimeout =
    testIotI2C_DEFAULT_TIMEOUT, };

    uctestIotI2CEepromWbuf[ 0 ] = (uint8_t) ((devReg >> 8) & 0xFF);
    uctestIotI2CEepromWbuf[ 1 ] = (uint8_t) (devReg & 0xFF);

    /* Open i2c to initialize hardware */
    xI2CHandle = iot_i2c_open(uctestIotI2CInstanceIdx);
    TEST_ASSERT_NOT_EQUAL(NULL, xI2CHandle);

    if ( TEST_PROTECT() )
    {
        for ( i = 0; i < 5; i++ )
        {
            fill_buf(&uctestIotI2CEepromWbuf[ 2 ], 32, i);
            /* Set i2c configuration */
            lRetVal = iot_i2c_ioctl(xI2CHandle, eI2CSetMasterConfig,
                    &xI2CConfig);
            TEST_ASSERT_EQUAL(0, lRetVal);

            /* Set i2c slave address */
            lRetVal = iot_i2c_ioctl(xI2CHandle, eI2CSetSlaveAddr,
                    &uctestIotI2CSlaveAddr);
            TEST_ASSERT_EQUAL(0, lRetVal);

            /* write the device register address. */
            lRetVal = iot_i2c_write_sync(xI2CHandle, uctestIotI2CEepromWbuf,
                    34);
            TEST_ASSERT_EQUAL(0, lRetVal);

            vTaskDelay(1);

            lRetVal = iot_i2c_write_sync(xI2CHandle, uctestIotI2CEepromWbuf, 2);
            TEST_ASSERT_EQUAL(0, lRetVal);

            lRetVal = iot_i2c_read_sync(xI2CHandle, uctestIotI2CEepromRbuf, 32);
            TEST_ASSERT_EQUAL(0, lRetVal);

            lRetVal = verify_buf(uctestIotI2CEepromRbuf, 32, i);
            TEST_ASSERT_EQUAL(0, lRetVal);
            /* add delay before next transmission in fast mode */
            vTaskDelay(1);
        }
    }

    lRetVal = iot_i2c_close(xI2CHandle);
    TEST_ASSERT_EQUAL(0, lRetVal);
}

/*-----------------------------------------------------------*/

/**
 * @brief Test Function for I2C sync write read datasuccess in fast plus mode
 */
TEST( TEST_IOT_I2C, AFQP_IotI2C_EEPROMWriteReadSyncVerifyFastPlusMode)
{
    int32_t lRetVal;
    uint16_t devReg = 0x0020;
    int i;

    IotI2CConfig_t xI2CConfig =
    { .ulBusFreq = testIotI2C_BAUDRATE_FPLUS, .ulMasterTimeout =
    testIotI2C_DEFAULT_TIMEOUT, };

    uctestIotI2CEepromWbuf[ 0 ] = (uint8_t) ((devReg >> 8) & 0xFF);
    uctestIotI2CEepromWbuf[ 1 ] = (uint8_t) (devReg & 0xFF);

    /* Open i2c to initialize hardware */
    xI2CHandle = iot_i2c_open(uctestIotI2CInstanceIdx);
    TEST_ASSERT_NOT_EQUAL(NULL, xI2CHandle);

    if ( TEST_PROTECT() )
    {
        for ( i = 0; i < 5; i++ )
        {
            fill_buf(&uctestIotI2CEepromWbuf[ 2 ], 32, i);
            /* Set i2c configuration */
            lRetVal = iot_i2c_ioctl(xI2CHandle, eI2CSetMasterConfig,
                    &xI2CConfig);
            TEST_ASSERT_EQUAL(0, lRetVal);

            /* Set i2c slave address */
            lRetVal = iot_i2c_ioctl(xI2CHandle, eI2CSetSlaveAddr,
                    &uctestIotI2CSlaveAddr);
            TEST_ASSERT_EQUAL(0, lRetVal);

            /* write the device register address. */
            lRetVal = iot_i2c_write_sync(xI2CHandle, uctestIotI2CEepromWbuf,
                    34);
            TEST_ASSERT_EQUAL(0, lRetVal);

            vTaskDelay(1);

            lRetVal = iot_i2c_write_sync(xI2CHandle, uctestIotI2CEepromWbuf, 2);
            TEST_ASSERT_EQUAL(0, lRetVal);

            lRetVal = iot_i2c_read_sync(xI2CHandle, uctestIotI2CEepromRbuf, 32);
            TEST_ASSERT_EQUAL(0, lRetVal);

            lRetVal = verify_buf(uctestIotI2CEepromRbuf, 32, i);
            TEST_ASSERT_EQUAL(0, lRetVal);
            /* add delay before next transmission in fast mode */
            vTaskDelay(1);
        }
    }

    lRetVal = iot_i2c_close(xI2CHandle);
    TEST_ASSERT_EQUAL(0, lRetVal);
}

/*-----------------------------------------------------------*/

/**
 * @brief Test Function for I2C continues write read success
 */
TEST( TEST_IOT_I2C, AFQP_IotI2C_EEPROMWriteReadAsyncVerifyCallback )
{
    int32_t lRetVal;
    uint16_t devReg = 0x0020;

    IotI2CConfig_t xI2CConfig =
    { .ulBusFreq = testIotI2C_BAUDRATE_LOW_SPEED, .ulMasterTimeout =
    testIotI2C_DEFAULT_TIMEOUT, };

    uctestIotI2CEepromWbuf[ 0 ] = (devReg >> 8) & 0xFF;
    uctestIotI2CEepromWbuf[ 1 ] = devReg & 0xFF;
    fill_buf(&uctestIotI2CEepromWbuf[ 2 ], 32, 0);

    /* Open i2c to initialize hardware */
    xI2CHandle = iot_i2c_open(uctestIotI2CInstanceIdx);
    TEST_ASSERT_NOT_EQUAL(NULL, xI2CHandle);

    if ( TEST_PROTECT() )
    {
        /* Set i2c configuration */
        lRetVal = iot_i2c_ioctl(xI2CHandle, eI2CSetMasterConfig, &xI2CConfig);
        TEST_ASSERT_EQUAL(0, lRetVal);

        /* Set i2c slave address */
        lRetVal = iot_i2c_ioctl(xI2CHandle, eI2CSetSlaveAddr,
                &uctestIotI2CSlaveAddr);
        TEST_ASSERT_EQUAL(0, lRetVal);

        /* Set completion callback */
        iot_i2c_set_callback(xI2CHandle, prvI2CCallback, NULL);

        /* write the device register address. */
        lRetVal = iot_i2c_write_async(xI2CHandle, uctestIotI2CEepromWbuf, 34);
        TEST_ASSERT_EQUAL(0, lRetVal);

        lRetVal = xSemaphoreTake(xtestIotI2CSemaphore, testIotI2C_MAX_TIMEOUT);
        TEST_ASSERT_EQUAL(pdTRUE, lRetVal);

        vTaskDelay(1);

        /* read from i2c device */
        lRetVal = iot_i2c_write_async(xI2CHandle, uctestIotI2CEepromWbuf, 2);
        TEST_ASSERT_EQUAL(0, lRetVal);

        lRetVal = xSemaphoreTake(xtestIotI2CSemaphore, testIotI2C_MAX_TIMEOUT);
        TEST_ASSERT_EQUAL(pdTRUE, lRetVal);

        lRetVal = iot_i2c_read_async(xI2CHandle, uctestIotI2CEepromRbuf, 32);
        TEST_ASSERT_EQUAL(0, lRetVal);

        lRetVal = xSemaphoreTake(xtestIotI2CSemaphore, testIotI2C_MAX_TIMEOUT);
        TEST_ASSERT_EQUAL(pdTRUE, lRetVal);

        lRetVal = verify_buf(uctestIotI2CEepromRbuf, 32, 0);
        TEST_ASSERT_EQUAL(0, lRetVal);
    }

    lRetVal = iot_i2c_close(xI2CHandle);
    TEST_ASSERT_EQUAL(0, lRetVal);
}

/*-----------------------------------------------------------*/

/**
 * @brief Test Function for I2C sync write read data success
 */
TEST( TEST_IOT_I2C, AFQP_IotI2C_EEPROMWriteReadNostopVerify)
{
    int32_t lRetVal;
    uint16_t devReg = 0x0020;

    IotI2CConfig_t xI2CConfig =
    { .ulBusFreq = testIotI2C_BAUDRATE_LOW_SPEED, .ulMasterTimeout =
    testIotI2C_DEFAULT_TIMEOUT, };

    uctestIotI2CEepromWbuf[ 0 ] = (uint8_t) ((devReg >> 8) & 0xFF);
    uctestIotI2CEepromWbuf[ 1 ] = (uint8_t) (devReg & 0xFF);
    fill_buf(&uctestIotI2CEepromWbuf[ 2 ], 32, 10);

    /* Open i2c to initialize hardware */
    xI2CHandle = iot_i2c_open(uctestIotI2CInstanceIdx);
    TEST_ASSERT_NOT_EQUAL(NULL, xI2CHandle);

    if ( TEST_PROTECT() )
    {
        /* Set i2c configuration */
        lRetVal = iot_i2c_ioctl(xI2CHandle, eI2CSetMasterConfig, &xI2CConfig);
        TEST_ASSERT_EQUAL(0, lRetVal);

        /* Set i2c slave address */
        lRetVal = iot_i2c_ioctl(xI2CHandle, eI2CSetSlaveAddr,
                &uctestIotI2CSlaveAddr);
        TEST_ASSERT_EQUAL(0, lRetVal);

        /* write the device register address. */
        lRetVal = iot_i2c_write_sync(xI2CHandle, uctestIotI2CEepromWbuf, 3);
        TEST_ASSERT_EQUAL(0, lRetVal);

        vTaskDelay(1);

        /* Set i2c not stop between transaction. This is achieved by not
         * configuring nostop flag */
        lRetVal = iot_i2c_ioctl(xI2CHandle, eI2CSendNoStopFlag, NULL);
        TEST_ASSERT_EQUAL(0, lRetVal);

        lRetVal = iot_i2c_write_sync(xI2CHandle, uctestIotI2CEepromWbuf, 2);
        TEST_ASSERT_EQUAL(0, lRetVal);

        lRetVal = iot_i2c_read_sync(xI2CHandle, uctestIotI2CEepromRbuf, 1);
        TEST_ASSERT_EQUAL(0, lRetVal);

        TEST_ASSERT_NOT_EQUAL(uctestIotI2CEepromWbuf[ 0 ],
                uctestIotI2CEepromRbuf[ 0 ]);

        /* Set i2c stop between transaction. This is achieved by not configuring
         * nostop flag*/

        lRetVal = iot_i2c_write_sync(xI2CHandle, uctestIotI2CEepromWbuf, 2);
        TEST_ASSERT_EQUAL(0, lRetVal);

        lRetVal = iot_i2c_read_sync(xI2CHandle, uctestIotI2CEepromRbuf, 1);
        TEST_ASSERT_EQUAL(0, lRetVal);

        TEST_ASSERT_EQUAL(uctestIotI2CEepromWbuf[ 0 ],
                uctestIotI2CEepromRbuf[ 0 ]);

    }

    lRetVal = iot_i2c_close(xI2CHandle);
    TEST_ASSERT_EQUAL(0, lRetVal);
}

/*-----------------------------------------------------------*/

/**
 * @brief Test Function for I2C sync write read data success
 */
TEST( TEST_IOT_I2C, AFQP_IotI2C_RepeatedTransferTest)
{
    int32_t lRetVal;
    uint16_t devReg = 0x0020;
    int i;

    IotI2CConfig_t xI2CConfig =
    { .ulBusFreq = testIotI2C_BAUDRATE_LOW_SPEED, .ulMasterTimeout =
    testIotI2C_DEFAULT_TIMEOUT, };

    uctestIotI2CEepromWbuf[ 0 ] = (uint8_t) ((devReg >> 8) & 0xFF);
    uctestIotI2CEepromWbuf[ 1 ] = (uint8_t) (devReg & 0xFF);

    if ( TEST_PROTECT() )
    {
        for ( i = 0; i < 10; i++ )
        {
            fill_buf(&uctestIotI2CEepromWbuf[ 2 ], 32, i);

            /* Open i2c to initialize hardware */
            xI2CHandle = iot_i2c_open(uctestIotI2CInstanceIdx);
            TEST_ASSERT_NOT_EQUAL(NULL, xI2CHandle);
            /* Set i2c configuration */
            lRetVal = iot_i2c_ioctl(xI2CHandle, eI2CSetMasterConfig,
                    &xI2CConfig);
            TEST_ASSERT_EQUAL(0, lRetVal);

            /* Set i2c slave address */
            lRetVal = iot_i2c_ioctl(xI2CHandle, eI2CSetSlaveAddr,
                    &uctestIotI2CSlaveAddr);
            TEST_ASSERT_EQUAL(0, lRetVal);

            /* write the device register address. */
            lRetVal = iot_i2c_write_sync(xI2CHandle, uctestIotI2CEepromWbuf,
                    34);
            TEST_ASSERT_EQUAL(0, lRetVal);

            vTaskDelay(1);

            lRetVal = iot_i2c_write_sync(xI2CHandle, uctestIotI2CEepromWbuf, 2);
            TEST_ASSERT_EQUAL(0, lRetVal);

            lRetVal = iot_i2c_read_sync(xI2CHandle, uctestIotI2CEepromRbuf, 32);
            TEST_ASSERT_EQUAL(0, lRetVal);

            lRetVal = verify_buf(uctestIotI2CEepromRbuf, 32, i);
            TEST_ASSERT_EQUAL(0, lRetVal);

            lRetVal = iot_i2c_close(xI2CHandle);
            TEST_ASSERT_EQUAL(0, lRetVal);
        }
    }
}

TEST(TEST_IOT_I2C, test_i2c_open_valid_instance)
{
    int32_t result;
    int32_t validInstance = 0;
    I2cHandle_t handle = i2c_open(validInstance);
    TEST_ASSERT_NOT_NULL(handle);

    result = i2c_close(handle);
    TEST_ASSERT_EQUAL(0, result);
}

TEST(TEST_IOT_I2C, test_i2c_open_invalid_instance)
{
    int32_t invalidInstance = -1;
    I2cHandle_t handle = i2c_open(invalidInstance);
    TEST_ASSERT_NULL(handle);

    invalidInstance = 100;
    handle = i2c_open(invalidInstance);
    TEST_ASSERT_NULL(handle);
}

TEST(TEST_IOT_I2C, test_i2c_open_already_open)
{
    int32_t result;
    int32_t validInstance = 0;
    I2cHandle_t handle1 = i2c_open(validInstance);
    TEST_ASSERT_NOT_NULL(handle1);

    I2cHandle_t handle2 = i2c_open(validInstance);
    TEST_ASSERT_NULL(handle2);

    result = i2c_close(handle1);
    TEST_ASSERT_EQUAL(0, result);
}

TEST(TEST_IOT_I2C, test_i2c_read_sync_success)
{
    int32_t validInstance = 0;
    I2cHandle_t handle = i2c_open(validInstance);
    TEST_ASSERT_NOT_NULL(handle);

    int32_t lRetVal;
    lRetVal = i2c_ioctl(handle, eI2CSetSlaveAddr, &uctestIotI2CSlaveAddr);
    TEST_ASSERT_EQUAL(0, lRetVal);

    uint8_t buffer[ 10 ];
    size_t bytesToRead = sizeof(buffer);
    int32_t result = i2c_read_sync(handle, buffer, bytesToRead);

    TEST_ASSERT_EQUAL(0, result);

    lRetVal = i2c_close(handle);
    TEST_ASSERT_EQUAL(0, lRetVal);
}

TEST(TEST_IOT_I2C, test_i2c_read_sync_null_handle)
{
    uint8_t buffer[ 10 ];
    size_t bytesToRead = sizeof(buffer);

    int32_t result = i2c_read_sync(NULL, buffer, bytesToRead);
    TEST_ASSERT_EQUAL(-EINVAL, result);
}

TEST(TEST_IOT_I2C, test_i2c_read_sync_null_buffer)
{
    int32_t validInstance = 0;
    I2cHandle_t handle = i2c_open(validInstance);
    TEST_ASSERT_NOT_NULL(handle);

    size_t bytesToRead = 10;
    int32_t result = i2c_read_sync(handle, NULL, bytesToRead);

    TEST_ASSERT_EQUAL(-EINVAL, result);

    result = i2c_close(handle);
    TEST_ASSERT_EQUAL(0, result);
}

TEST(TEST_IOT_I2C, test_i2c_read_sync_zero_bytes)
{
    int32_t validInstance = 0;
    I2cHandle_t handle = i2c_open(validInstance);
    TEST_ASSERT_NOT_NULL(handle);

    uint8_t buffer[ 10 ];
    size_t bytesToRead = 0;
    int32_t result = i2c_read_sync(handle, buffer, bytesToRead);

    TEST_ASSERT_EQUAL(-EINVAL, result);

    result = i2c_close(handle);
    TEST_ASSERT_EQUAL(0, result);
}

TEST(TEST_IOT_I2C, test_i2c_read_sync_slave_address_not_set)
{
    int32_t validInstance = 0;
    I2cHandle_t handle = i2c_open(validInstance);
    TEST_ASSERT_NOT_NULL(handle);

    uint8_t buffer[ 10 ];
    size_t bytesToRead = sizeof(buffer);
    int32_t result = i2c_read_sync(handle, buffer, bytesToRead);

    TEST_ASSERT_EQUAL(-ENXIO, result);

    result = i2c_close(handle);
    TEST_ASSERT_EQUAL(0, result);
}

TEST(TEST_IOT_I2C, test_i2c_read_sync_nack)
{
    int32_t validInstance = 0;
    I2cHandle_t handle = i2c_open(validInstance);
    TEST_ASSERT_NOT_NULL(handle);

    /* Set i2c slave address */
    int32_t result;
    uint16_t slaveAddr = 0x40;
    result = i2c_ioctl(handle, eI2CSetSlaveAddr, &slaveAddr);
    TEST_ASSERT_EQUAL(0, result);

    uint8_t buffer[ 10 ];
    // // Simulate NACK condition
    // simulate_nack_condition(handle);

    result = i2c_read_sync(handle, buffer, sizeof(buffer));
    TEST_ASSERT_EQUAL(-EIO, result);

    result = i2c_close(handle);
    TEST_ASSERT_EQUAL(0, result);
}

TEST(TEST_IOT_I2C, test_i2c_read_sync_busy)
{
    int32_t validInstance = 0;
    I2cHandle_t handle = i2c_open(validInstance);
    TEST_ASSERT_NOT_NULL(handle);

    /* Set i2c slave address */
    int32_t result;
    result = i2c_ioctl(handle, eI2CSetSlaveAddr, (void*) &uctestIotI2CSlaveAddr);
    TEST_ASSERT_EQUAL(0, result);

    i2c_set_callback(handle, prvI2CCallback, NULL);

    uint8_t buffer[ 10 ] = {0};
    size_t bytesToRead = sizeof(buffer);

    result = i2c_read_async(handle, buffer, bytesToRead);
    TEST_ASSERT_EQUAL(0, result);

    result = i2c_read_sync(handle, buffer, sizeof(buffer));
    TEST_ASSERT_EQUAL(-EBUSY, result);

    result = i2c_close(handle);
    TEST_ASSERT_EQUAL(0, result);
}

TEST(TEST_IOT_I2C, test_i2c_write_sync_success)
{
    int32_t validInstance = 0;
    I2cHandle_t handle = i2c_open(validInstance);
    TEST_ASSERT_NOT_NULL(handle);

    /* Set i2c slave address */
    int32_t result;
    result = i2c_ioctl(handle, eI2CSetSlaveAddr, &uctestIotI2CSlaveAddr);
    TEST_ASSERT_EQUAL(0, result);

    uint8_t buffer[ 10 ] =
    { 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A };
    result = i2c_write_sync(handle, buffer, sizeof(buffer));
    TEST_ASSERT_EQUAL(0, result);

    result = i2c_close(handle);
    TEST_ASSERT_EQUAL(0, result);
}

TEST(TEST_IOT_I2C, test_i2c_write_sync_null_handle)
{
    uint8_t buffer[ 10 ] =
    { 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A };

    int32_t result = i2c_write_sync(NULL, buffer, sizeof(buffer));
    TEST_ASSERT_EQUAL(-EINVAL, result);
}

TEST(TEST_IOT_I2C, test_i2c_write_sync_null_buffer)
{
    int32_t validInstance = 0;
    I2cHandle_t handle = i2c_open(validInstance);
    TEST_ASSERT_NOT_NULL(handle);

    size_t bytesToRead = 10;
    int32_t result = i2c_write_sync(handle, NULL, bytesToRead);

    TEST_ASSERT_EQUAL(-EINVAL, result);

    result = i2c_close(handle);
    TEST_ASSERT_EQUAL(0, result);
}

TEST(TEST_IOT_I2C, test_i2c_write_sync_zero_bytes)
{
    int32_t validInstance = 0;
    I2cHandle_t handle = i2c_open(validInstance);
    TEST_ASSERT_NOT_NULL(handle);

    uint8_t buffer[ 10 ] =
    { 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A };
    int32_t result = i2c_write_sync(handle, buffer, 0);

    TEST_ASSERT_EQUAL(-EINVAL, result);

    result = i2c_close(handle);
    TEST_ASSERT_EQUAL(0, result);
}

TEST(TEST_IOT_I2C, test_i2c_write_sync_slave_address_not_set)
{
    int32_t validInstance = 0;
    I2cHandle_t handle = i2c_open(validInstance);
    TEST_ASSERT_NOT_NULL(handle);

    uint8_t buffer[ 10 ] =
    { 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A };
    int32_t result = i2c_write_sync(handle, buffer, sizeof(buffer));

    TEST_ASSERT_EQUAL(-ENXIO, result);

    result = i2c_close(handle);
    TEST_ASSERT_EQUAL(0, result);
}

TEST(TEST_IOT_I2C, test_i2c_write_sync_busy)
{
    int32_t validInstance = 0;
    I2cHandle_t handle = i2c_open(validInstance);
    TEST_ASSERT_NOT_NULL(handle);

    /* Set i2c slave address */
    int32_t lRetVal;
    lRetVal = i2c_ioctl(handle, eI2CSetSlaveAddr, &uctestIotI2CSlaveAddr);
    TEST_ASSERT_EQUAL(0, lRetVal);

    i2c_set_callback(handle, prvI2CCallback, NULL);

    uint8_t buffer[ 10 ] =
    { 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A };
    size_t bytesToRead = sizeof(buffer);
    int32_t result = i2c_write_async(handle, buffer, bytesToRead);

    TEST_ASSERT_EQUAL(0, result);

    result = i2c_write_sync(handle, buffer, sizeof(buffer));

    TEST_ASSERT_EQUAL(-EBUSY, result);

    result = i2c_close(handle);
    TEST_ASSERT_EQUAL(0, result);
}

TEST(TEST_IOT_I2C, test_i2c_read_async_success)
{
    int32_t validInstance = 0;
    I2cHandle_t handle = i2c_open(validInstance);
    TEST_ASSERT_NOT_NULL(handle);

    /* Set i2c slave address */
    int32_t lRetVal;
    lRetVal = i2c_ioctl(handle, eI2CSetSlaveAddr, &uctestIotI2CSlaveAddr);
    TEST_ASSERT_EQUAL(0, lRetVal);

    i2c_set_callback(handle, prvI2CCallback, NULL);

    uint8_t buffer[ 10 ];
    int32_t result = i2c_read_async(handle, buffer, sizeof(buffer));

    TEST_ASSERT_EQUAL(0, result);

    result = i2c_close(handle);
    TEST_ASSERT_EQUAL(0, result);
}

TEST(TEST_IOT_I2C, test_i2c_read_async_null_handle)
{
    uint8_t buffer[ 10 ];
    int32_t result = i2c_read_async(NULL, buffer, sizeof(buffer));

    TEST_ASSERT_EQUAL(-EINVAL, result);
}

TEST(TEST_IOT_I2C, test_i2c_read_async_null_buffer)
{
    int32_t validInstance = 0;
    I2cHandle_t handle = i2c_open(validInstance);
    TEST_ASSERT_NOT_NULL(handle);

    int32_t result = i2c_read_async(handle, NULL, 10);

    TEST_ASSERT_EQUAL(-EINVAL, result);

    result = i2c_close(handle);
    TEST_ASSERT_EQUAL(0, result);
}

TEST(TEST_IOT_I2C, test_i2c_read_async_zero_bytes)
{
    int32_t validInstance = 0;
    I2cHandle_t handle = i2c_open(validInstance);
    TEST_ASSERT_NOT_NULL(handle);

    uint8_t buffer[ 10 ];
    int32_t result = i2c_read_async(handle, buffer, 0);

    TEST_ASSERT_EQUAL(-EINVAL, result);

    result = i2c_close(handle);
    TEST_ASSERT_EQUAL(0, result);
}

TEST(TEST_IOT_I2C, test_i2c_read_async_slave_address_not_set)
{
    int32_t validInstance = 0;
    I2cHandle_t handle = i2c_open(validInstance);
    TEST_ASSERT_NOT_NULL(handle);

    uint8_t buffer[ 10 ];
    int32_t result = i2c_read_async(handle, buffer, sizeof(buffer));

    TEST_ASSERT_EQUAL(-ENXIO, result);

    result = i2c_close(handle);
    TEST_ASSERT_EQUAL(0, result);
}

TEST(TEST_IOT_I2C, test_i2c_read_async_busy)
{
    int32_t validInstance = 0;
    I2cHandle_t handle = i2c_open(validInstance);
    TEST_ASSERT_NOT_NULL(handle);

    /* Set i2c slave address */
    int32_t result;
    result = i2c_ioctl(handle, eI2CSetSlaveAddr, &uctestIotI2CSlaveAddr);
    TEST_ASSERT_EQUAL(0, result);

    // Set the callback
    i2c_set_callback(handle, prvI2CCallback, NULL);

    uint8_t buffer[ 10 ];
    size_t bytesToRead = sizeof(buffer);
    result = i2c_read_async(handle, buffer, bytesToRead);
    TEST_ASSERT_EQUAL(0, result);

    result = i2c_read_async(handle, buffer, sizeof(buffer));
    TEST_ASSERT_EQUAL(-EBUSY, result);

    result = i2c_close(handle);
    TEST_ASSERT_EQUAL(0, result);
}

TEST(TEST_IOT_I2C, test_i2c_write_async_success)
{
    int32_t validInstance = 0;
    I2cHandle_t handle = i2c_open(validInstance);
    TEST_ASSERT_NOT_NULL(handle);

    /* Set i2c slave address */
    int32_t lRetVal;
    lRetVal = i2c_ioctl(handle, eI2CSetSlaveAddr, &uctestIotI2CSlaveAddr);
    TEST_ASSERT_EQUAL(0, lRetVal);

    // Set the callback
    i2c_set_callback(handle, prvI2CCallback, NULL);

    uint8_t buffer[ 10 ] =
    { 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A };

    int32_t result = i2c_write_async(handle, buffer, sizeof(buffer));
    TEST_ASSERT_EQUAL(0, result);

    result = i2c_close(handle);
    TEST_ASSERT_EQUAL(0, result);
}

TEST(TEST_IOT_I2C, test_i2c_write_async_null_handle)
{
    uint8_t buffer[ 10 ] =
    { 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A };

    int32_t result = i2c_write_async(NULL, buffer, sizeof(buffer));
    TEST_ASSERT_EQUAL(-EINVAL, result);
}

TEST(TEST_IOT_I2C, test_i2c_write_async_null_buffer)
{
    int32_t validInstance = 0;
    I2cHandle_t handle = i2c_open(validInstance);
    TEST_ASSERT_NOT_NULL(handle);

    size_t bytesToRead = 10;

    int32_t result = i2c_write_async(handle, NULL, bytesToRead);
    TEST_ASSERT_EQUAL(-EINVAL, result);

    result = i2c_close(handle);
    TEST_ASSERT_EQUAL(0, result);
}

TEST(TEST_IOT_I2C, test_i2c_write_async_zero_bytes)
{
    int32_t validInstance = 0;
    I2cHandle_t handle = i2c_open(validInstance);
    TEST_ASSERT_NOT_NULL(handle);

    uint8_t buffer[ 10 ] =
    { 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A };
    int32_t result = i2c_write_async(handle, buffer, 0);

    TEST_ASSERT_EQUAL(-EINVAL, result);

    result = i2c_close(handle);
    TEST_ASSERT_EQUAL(0, result);
}

TEST(TEST_IOT_I2C, test_i2c_write_async_slave_address_not_set)
{
    int32_t validInstance = 0;
    I2cHandle_t handle = i2c_open(validInstance);
    TEST_ASSERT_NOT_NULL(handle);

    uint8_t buffer[ 10 ] =
    { 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A };
    int32_t result = i2c_write_async(handle, buffer, sizeof(buffer));

    TEST_ASSERT_EQUAL(-ENXIO, result);

    result = i2c_close(handle);
}

TEST(TEST_IOT_I2C, test_i2c_write_async_busy)
{
    int32_t validInstance = 0;
    I2cHandle_t handle = i2c_open(validInstance);
    TEST_ASSERT_NOT_NULL(handle);

    /* Set i2c slave address */
    int32_t result;
    result = i2c_ioctl(handle, eI2CSetSlaveAddr, &uctestIotI2CSlaveAddr);
    TEST_ASSERT_EQUAL(0, result);

    // Set the callback
    i2c_set_callback(handle, prvI2CCallback, NULL);

    uint8_t buffer[ 10 ] =
    { 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A };
    size_t bytesToRead = sizeof(buffer);
    result = i2c_write_async(handle, buffer, bytesToRead);

    TEST_ASSERT_EQUAL(0, result);

    result = i2c_write_async(handle, buffer, sizeof(buffer));

    TEST_ASSERT_EQUAL(-EBUSY, result);

    result = i2c_close(handle);
    TEST_ASSERT_EQUAL(0, result);
}

TEST(TEST_IOT_I2C, test_i2c_ioctl_success_set_master_config)
{
    int32_t validInstance = 0; // Assume 0 is a valid instance number
    I2cHandle_t handle = i2c_open(validInstance);
    TEST_ASSERT_NOT_NULL(handle);

    I2cConfig_t config;
    int32_t result = i2c_ioctl(handle, eI2CSetMasterConfig, &config);

    TEST_ASSERT_EQUAL(0, result);

    result = i2c_close(handle);
    TEST_ASSERT_EQUAL(0, result);
}

TEST(TEST_IOT_I2C, test_i2c_ioctl_null_handle)
{
    I2cConfig_t config;
    int32_t result = i2c_ioctl(NULL, eI2CSetMasterConfig, &config);

    TEST_ASSERT_EQUAL(-EINVAL, result);
}

TEST(TEST_IOT_I2C, test_i2c_ioctl_null_buffer)
{
    int32_t validInstance = 0;
    I2cHandle_t handle = i2c_open(validInstance);
    TEST_ASSERT_NOT_NULL(handle);

    int32_t result = i2c_ioctl(handle, eI2CSetMasterConfig, NULL);

    TEST_ASSERT_EQUAL(-EINVAL, result);

    result = i2c_close(handle);
    TEST_ASSERT_EQUAL(0, result);
}

TEST(TEST_IOT_I2C, test_i2c_ioctl_get_master_config)
{
    int32_t validInstance = 0;
    I2cHandle_t handle = i2c_open(validInstance);
    TEST_ASSERT_NOT_NULL(handle);

    I2cConfig_t config;
    int32_t result = i2c_ioctl(handle, eI2CGetMasterConfig, &config);

    TEST_ASSERT_EQUAL(0, result);

    result = i2c_close(handle);
    TEST_ASSERT_EQUAL(0, result);
}

TEST(TEST_IOT_I2C, test_i2c_ioctl_set_slave_addr)
{
    int32_t validInstance = 0;
    I2cHandle_t handle = i2c_open(validInstance);
    TEST_ASSERT_NOT_NULL(handle);

    uint16_t slaveAddr = 0x50;
    int32_t result = i2c_ioctl(handle, eI2CSetSlaveAddr, &slaveAddr);

    TEST_ASSERT_EQUAL(0, result);

    result = i2c_close(handle);
    TEST_ASSERT_EQUAL(0, result);
}

TEST(TEST_IOT_I2C, test_i2c_ioctl_get_tx_no_of_bytes)
{
    int32_t validInstance = 0;
    I2cHandle_t handle = i2c_open(validInstance);
    TEST_ASSERT_NOT_NULL(handle);

    uint16_t txBytes;
    int32_t result = i2c_ioctl(handle, eI2CGetTxNoOfbytes, &txBytes);

    TEST_ASSERT_EQUAL(0, result);

    result = i2c_close(handle);
    TEST_ASSERT_EQUAL(0, result);
}

TEST(TEST_IOT_I2C, test_i2c_ioctl_get_rx_no_of_bytes)
{
    int32_t validInstance = 0;
    I2cHandle_t handle = i2c_open(validInstance);
    TEST_ASSERT_NOT_NULL(handle);

    uint16_t rxBytes;
    int32_t result = i2c_ioctl(handle, eI2CGetRxNoOfbytes, &rxBytes);

    TEST_ASSERT_EQUAL(0, result);

    result = i2c_close(handle);
    TEST_ASSERT_EQUAL(0, result);
}

TEST(TEST_IOT_I2C, test_i2c_close_success)
{
    int32_t validInstance = 0;
    I2cHandle_t handle = i2c_open(validInstance);
    TEST_ASSERT_NOT_NULL(handle);

    int32_t result = i2c_close(handle);

    TEST_ASSERT_EQUAL(0, result);
}

TEST(TEST_IOT_I2C, test_i2c_close_null_handle)
{
    int32_t result = i2c_close(NULL);

    TEST_ASSERT_EQUAL(-EINVAL, result);
}

TEST(TEST_IOT_I2C, test_i2c_close_not_opened)
{
    int32_t validInstance = 0; // Assume 0 is a valid instance number
    I2cHandle_t handle = i2c_open(validInstance);
    TEST_ASSERT_NOT_NULL(handle);

    int32_t result = i2c_close(handle);
    TEST_ASSERT_EQUAL(0, result);

    result = i2c_close(handle);
    TEST_ASSERT_EQUAL(-EINVAL, result);
}

TEST(TEST_IOT_I2C, test_i2c_cancel_success)
{
    int32_t lRetVal;
    int32_t validInstance = 0; // Assume 0 is a valid instance number
    I2cConfig_t config = { .ulBusFreq = testIotI2C_BAUDRATE_LOW_SPEED, .ulMasterTimeout = testIotI2C_DEFAULT_TIMEOUT };

    I2cHandle_t handle = i2c_open(validInstance);
    TEST_ASSERT_NOT_NULL(handle);

    i2c_set_callback(handle, prvI2CCallback, NULL);

    lRetVal = i2c_ioctl(handle, eI2CSetMasterConfig, &config);
    TEST_ASSERT_EQUAL(0, lRetVal);

    lRetVal = i2c_ioctl(handle, eI2CSetSlaveAddr, &uctestIotI2CSlaveAddr);
    TEST_ASSERT_EQUAL(0, lRetVal);

    // Start an I2C transaction (assuming i2c_write_async is available)
    uint8_t buffer[ 10 ] =
    { 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A };

    lRetVal = i2c_write_async(handle, buffer, sizeof(buffer));
    TEST_ASSERT_EQUAL(0, lRetVal);

    lRetVal = i2c_cancel(handle);
    TEST_ASSERT_EQUAL(0, lRetVal);

    xSemaphoreTake(xtestIotI2CSemaphore, testIotI2C_MAX_TIMEOUT);

    lRetVal = i2c_close(handle);
    TEST_ASSERT_EQUAL(0, lRetVal);
}

TEST(TEST_IOT_I2C, test_i2c_cancel_null_handle)
{
    int32_t result = i2c_cancel(NULL);

    TEST_ASSERT_EQUAL(-EINVAL, result);
}

TEST(TEST_IOT_I2C, test_i2c_cancel_not_opened)
{
    I2cHandle_t handle = i2c_open(0xFFFFFFFF);
    TEST_ASSERT_NULL(handle);

    int32_t result = i2c_cancel(handle);

    TEST_ASSERT_EQUAL(-EINVAL, result);
}

TEST(TEST_IOT_I2C, test_i2c_cancel_nothing_to_cancel)
{
    int32_t validInstance = 0; // Assume 0 is a valid instance number
    I2cHandle_t handle = i2c_open(validInstance);
    TEST_ASSERT_NOT_NULL(handle);

    int32_t result = i2c_cancel(handle);

    TEST_ASSERT_EQUAL(-EPERM, result);

    result = i2c_close(handle);
    TEST_ASSERT_EQUAL(0, result);
}

TEST(TEST_IOT_I2C, test_i2c_open_max_instances)
{
    int32_t result;
    for ( uint32_t instance = 0; instance < MAX_I2C_INSTANCES; instance++ )
    {
        I2cHandle_t handle = i2c_open(instance);
        TEST_ASSERT_NOT_NULL(handle);

        result = i2c_close(handle);
        TEST_ASSERT_EQUAL(0, result);
    }
}

TEST(TEST_IOT_I2C, test_i2c_read_sync_not_opened)
{
    int32_t validInstance = 0;
    I2cHandle_t handle = i2c_open(validInstance);
    TEST_ASSERT_NOT_NULL(handle);

    int32_t result = i2c_close(handle);
    TEST_ASSERT_EQUAL(0, result);

    uint8_t buffer[ 10 ];
    result = i2c_read_sync(handle, buffer, sizeof(buffer));

    TEST_ASSERT_EQUAL(-EINVAL, result);
}

TEST(TEST_IOT_I2C, test_i2c_write_sync_not_opened)
{
    int32_t validInstance = 0;
    I2cHandle_t handle = i2c_open(validInstance);
    TEST_ASSERT_NOT_NULL(handle);

    int32_t result = i2c_close(handle);
    TEST_ASSERT_EQUAL(0, result);

    uint8_t buffer[ 10 ] =
    { 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A };
    result = i2c_write_sync(handle, buffer, sizeof(buffer));

    TEST_ASSERT_EQUAL(-EINVAL, result);
}

TEST(TEST_IOT_I2C, test_i2c_read_async_not_opened)
{
    int32_t validInstance = 0;
    I2cHandle_t handle = i2c_open(validInstance);
    TEST_ASSERT_NOT_NULL(handle);

    int32_t result = i2c_close(handle);
    TEST_ASSERT_EQUAL(0, result);

    uint8_t buffer[ 10 ];
    result = i2c_read_async(handle, buffer, sizeof(buffer));

    TEST_ASSERT_EQUAL(-EINVAL, result);
}

TEST(TEST_IOT_I2C, test_i2c_write_async_not_opened)
{
    int32_t validInstance = 0;
    I2cHandle_t handle = i2c_open(validInstance);
    TEST_ASSERT_NOT_NULL(handle);

    int32_t result = i2c_close(handle);
    TEST_ASSERT_EQUAL(0, result);

    uint8_t buffer[ 10 ] =
    { 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A };
    result = i2c_write_async(handle, buffer, sizeof(buffer));

    TEST_ASSERT_EQUAL(-EINVAL, result);
}

TEST(TEST_IOT_I2C, test_i2c_ioctl_invalid_request)
{
    int32_t validInstance = 0;
    I2cHandle_t handle = i2c_open(validInstance);
    TEST_ASSERT_NOT_NULL(handle);

    int32_t result = i2c_ioctl(handle, (I2cIoctlRequest_t) 999, NULL);

    TEST_ASSERT_EQUAL(-EINVAL, result);

    result = i2c_close(handle);
    TEST_ASSERT_EQUAL(0, result);
}

TEST(TEST_IOT_I2C, test_i2c_open_close_multiple_times)
{
    int32_t validInstance = 0;
    for ( int i = 0; i < 10; i++ )
    {
        I2cHandle_t handle = i2c_open(validInstance);
        TEST_ASSERT_NOT_NULL(handle);
        int32_t result = i2c_close(handle);
        TEST_ASSERT_EQUAL(0, result);
    }
}

TEST(TEST_IOT_I2C, test_i2c_open_close_all_instances)
{
    for ( uint32_t instance = 0; instance < MAX_I2C_INSTANCES; instance++ )
    {
        I2cHandle_t handle = i2c_open(instance);
        TEST_ASSERT_NOT_NULL(handle);
        int32_t result = i2c_close(handle);
        TEST_ASSERT_EQUAL(0, result);
    }
}

TEST(TEST_IOT_I2C, test_i2c_open_close_with_errors)
{
    int32_t validInstance = 0;
    I2cHandle_t handle = i2c_open(validInstance);
    TEST_ASSERT_NOT_NULL(handle);

    // Attempt to close an invalid handle
    int32_t result = i2c_close(NULL);
    TEST_ASSERT_EQUAL(-EINVAL, result);

    // Close the valid handle
    result = i2c_close(handle);
    TEST_ASSERT_EQUAL(0, result);

    // Attempt to close the same handle again
    result = i2c_close(handle);
    TEST_ASSERT_EQUAL(-EINVAL, result);
}

TEST(TEST_IOT_I2C, test_set_callback_null_handle)
{
    i2c_set_callback(NULL, prvI2CCallback, NULL);
}

TEST(TEST_IOT_I2C, test_ioctl_null_buffer)
{
    int32_t result;
    I2cHandle_t handle = i2c_open(I2C_TEST_INSTANCE);
    TEST_ASSERT_NOT_NULL(handle);

    result = i2c_ioctl(handle, eI2CSetSlaveAddr, NULL);
    TEST_ASSERT_EQUAL(-EINVAL, result);

    result = i2c_ioctl(handle, eI2CGetBusState, NULL);
    TEST_ASSERT_EQUAL(-EINVAL, result);

    result = i2c_ioctl(handle, eI2CGetRxNoOfbytes, NULL);
    TEST_ASSERT_EQUAL(-EINVAL, result);

    result = i2c_ioctl(handle, eI2CGetTxNoOfbytes, NULL);
    TEST_ASSERT_EQUAL(-EINVAL, result);

    result = i2c_close(handle);
    TEST_ASSERT_EQUAL(0, result);
}

/*-----------------------------------------------------------*/
