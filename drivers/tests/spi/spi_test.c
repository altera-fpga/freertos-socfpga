/*
 * FreeRTOS Common IO V0.1.3
 * Copyright (C) 2020 Amazon.com, Inc. or its affiliates.  All Rights Reserved.
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
 * @file test_iot_spi.c
 * @brief Functional Unit Test - SPI
 *******************************************************************************
 */

/* Test includes */
#include <string.h>

/* Test includes */
#include "unity.h"
#include "unity_fixture.h"

/* Driver includes */
#include "spi_test.h"

/* FreeRTOS includes */
#include "FreeRTOS.h"
#include "FreeRTOSConfig.h"
#include "semphr.h"
#include "task.h"

#define SPI_BUFFER_SIZE                       ( 32 )
#define testIotSPI_DEFAULT_SEMAPHORE_DELAY    ( 3000U )
#define _MESSAGE_LENGTH                       ( 50 )
#define SLAVE_SELECT_NUM                      ( 1 )
#define EEPROM_READ                           ( 0x03 )
#define EEPROM_WRITE                          ( 0x02 )
#define EEPROM_WR_DISABLE                     ( 0x04 )
#define EEPROM_WR_ENABLE                      ( 0x06 )
#define EEPROM_RD_SR                          ( 0x05 )
#define EEPROM_WR_SR                          ( 0x01 )

#define EEPROM_ADDRESS                        ( 0x2780 )
#define EEPROM_ADDRESS_2                      ( 0x0044 )

/*-----------------------------------------------------------*/

/* Globals values which can be overwritten by the test
 * framework invoking these tests */
/*-----------------------------------------------------------*/
uint32_t ultestIotSpiInstance = 0; /* Test SPI Instance */
uint32_t ulAssistedTestIotSpiInstance = 0; /* Assisted Test SPI Instance */
uint32_t ultestIotSPIFrequency = 390625U; /* Test SPI Frequency */
uint32_t ultestIotSPIDummyValue = 0; /* Test SPI Dummy Value */
IotSPIMode_t xtestIotSPIDefaultConfigMode = eSPIMode3; /* Default SPI eSPIMode0 */
static SemaphoreHandle_t xtestIotSPISemaphore = NULL;
uint32_t ulAssistedTestIotSpiSlave = 0;
uint32_t ultestIotSpiSlave = 0;

/*-----------------------------------------------------------*/
/*Global buffer for multipage transfers*/
uint8_t wrBuf[ 2000 ];
uint8_t rdBuf[ 2000 ];
uint8_t cmpBuf[ 2000 ];

static void fill_buf( uint8_t *buf, uint32_t nbytes, uint8_t startNum )
{
    uint32_t i;
    for ( i = 0; i < nbytes; i++ )
    {
        *(buf + i) = startNum++;
    }
}

/**
 * @brief Application/POSIX defined callback for asynchronous operations
 * This callback function releases a semaphore every time it is called.
 */
static void prvSpiAsyncCallback( IotSPITransactionStatus_t xStatus,
        void *pvUserContext )
{
    /* Disable unused parameter warning. */
    (void) xStatus;
    (void) pvUserContext;

    xSemaphoreGiveFromISR(xtestIotSPISemaphore, NULL);
}

void prvEnableWrite( IotSPIHandle_t xSPIHandle )
{
    BaseType_t lRetVal;
    uint8_t cmd = EEPROM_WR_ENABLE;

    iot_spi_ioctl(xSPIHandle, eSPIActivateSlave, NULL);
    lRetVal = iot_spi_write_sync(xSPIHandle, &cmd, 1);
    iot_spi_ioctl(xSPIHandle, eSPIDeactivateSlave, NULL);
    TEST_ASSERT_EQUAL(0, lRetVal);
}

/*-----------------------------------------------------------*/

/*-----------------------------------------------------------*/
/* Define Test Group. */
TEST_GROUP(TEST_IOT_SPI);
/*-----------------------------------------------------------*/

/**
 * @brief Setup function called before each test in this group is executed.
 */
TEST_SETUP( TEST_IOT_SPI )
{
    xtestIotSPISemaphore = xSemaphoreCreateBinary();
    TEST_ASSERT_NOT_EQUAL(NULL, xtestIotSPISemaphore);
}
/*-----------------------------------------------------------*/

/**
 * @brief Tear down function called after each test in this group is executed.
 */
TEST_TEAR_DOWN( TEST_IOT_SPI )
{
}

/*-----------------------------------------------------------*/

/*Main function to invoke the tests*/
void spi_test()
{
    UNITY_BEGIN();

    RUN_TEST_GROUP(TEST_IOT_SPI);

    UNITY_END();
}

/**
 * @brief Function to define which tests to execute as part of this group.
 */
TEST_GROUP_RUNNER( TEST_IOT_SPI )
{
    /* non-data transfer tests */
    RUN_TEST_CASE(TEST_IOT_SPI, AFQP_IotSPI_OpenClose);
    RUN_TEST_CASE(TEST_IOT_SPI, AFQP_IotSPI_Init);
    RUN_TEST_CASE(TEST_IOT_SPI, AFQP_IotSPI_OpenFuzzing);
    RUN_TEST_CASE(TEST_IOT_SPI, AFQP_IotSPI_CloseFuzzing);
    RUN_TEST_CASE(TEST_IOT_SPI, AFQP_IotSPI_IoctlFuzzing);
    RUN_TEST_CASE(TEST_IOT_SPI, AFQP_IotSPI_ReadSyncFuzzing);
    RUN_TEST_CASE(TEST_IOT_SPI, AFQP_IotSPI_ReadAsyncFuzzing);
    RUN_TEST_CASE(TEST_IOT_SPI, AFQP_IotSPI_WriteSyncFuzzing);
    RUN_TEST_CASE(TEST_IOT_SPI, AFQP_IotSPI_WriteAsyncFuzzing);
    RUN_TEST_CASE(TEST_IOT_SPI, AFQP_IotSPI_TransferSyncFuzzing);
    RUN_TEST_CASE(TEST_IOT_SPI, AFQP_IotSPI_TransferAsyncFuzzing);
    /* data transfer tests */
    RUN_TEST_CASE(TEST_IOT_SPI, AFQP_IotSPI_EepromTransferSync);
    RUN_TEST_CASE(TEST_IOT_SPI, AFQP_IotSPI_EepromPageTransferSync);
    RUN_TEST_CASE(TEST_IOT_SPI, AFQP_IotSPI_EepromMultiPageTransferSync);

    RUN_TEST_CASE(TEST_IOT_SPI, test_spi_open_valid_instance);
    RUN_TEST_CASE(TEST_IOT_SPI, test_spi_open_invalid_instance);
    RUN_TEST_CASE(TEST_IOT_SPI, test_spi_open_same_instance_twice);

    RUN_TEST_CASE(TEST_IOT_SPI, test_spi_set_callback_valid_parameters);
    RUN_TEST_CASE(TEST_IOT_SPI, test_spi_set_callback_null_handle);
    RUN_TEST_CASE(TEST_IOT_SPI, test_spi_set_callback_null_callback);
    RUN_TEST_CASE(TEST_IOT_SPI, test_spi_set_callback_null_user_context);

    RUN_TEST_CASE(TEST_IOT_SPI, test_spi_ioctl_set_master_config_valid);
    RUN_TEST_CASE(TEST_IOT_SPI, test_spi_ioctl_set_master_config_null_handle);
    RUN_TEST_CASE(TEST_IOT_SPI, test_spi_ioctl_set_master_config_null_buffer);
    RUN_TEST_CASE(TEST_IOT_SPI, test_spi_ioctl_get_master_config_valid);
    RUN_TEST_CASE(TEST_IOT_SPI, test_spi_ioctl_get_master_config_null_handle);
    RUN_TEST_CASE(TEST_IOT_SPI, test_spi_ioctl_get_master_config_null_buffer);

    RUN_TEST_CASE(TEST_IOT_SPI, test_spi_read_sync_null_handle);
    RUN_TEST_CASE(TEST_IOT_SPI, test_spi_read_sync_null_buffer);
    RUN_TEST_CASE(TEST_IOT_SPI, test_spi_read_sync_zero_bytes);

    RUN_TEST_CASE(TEST_IOT_SPI, test_spi_read_async_null_handle);
    RUN_TEST_CASE(TEST_IOT_SPI, test_spi_read_async_null_buffer);
    RUN_TEST_CASE(TEST_IOT_SPI, test_spi_read_async_zero_bytes);

    RUN_TEST_CASE(TEST_IOT_SPI, test_spi_write_sync_valid_parameters);
    RUN_TEST_CASE(TEST_IOT_SPI, test_spi_write_sync_null_handle);
    RUN_TEST_CASE(TEST_IOT_SPI, test_spi_write_sync_null_buffer);
    RUN_TEST_CASE(TEST_IOT_SPI, test_spi_write_sync_zero_bytes);
    RUN_TEST_CASE(TEST_IOT_SPI, test_spi_write_sync_busy_bus);

    RUN_TEST_CASE(TEST_IOT_SPI, test_spi_write_async_valid_parameters);
    RUN_TEST_CASE(TEST_IOT_SPI, test_spi_write_async_null_handle);
    RUN_TEST_CASE(TEST_IOT_SPI, test_spi_write_async_null_buffer);
    RUN_TEST_CASE(TEST_IOT_SPI, test_spi_write_async_zero_bytes);
    RUN_TEST_CASE(TEST_IOT_SPI, test_spi_write_async_busy_bus);

    RUN_TEST_CASE(TEST_IOT_SPI, test_spi_transfer_sync_null_handle);
    RUN_TEST_CASE(TEST_IOT_SPI, test_spi_transfer_sync_null_tx_buffer);
    RUN_TEST_CASE(TEST_IOT_SPI, test_spi_transfer_sync_null_rx_buffer);
    RUN_TEST_CASE(TEST_IOT_SPI, test_spi_transfer_sync_zero_bytes);
    RUN_TEST_CASE(TEST_IOT_SPI, test_spi_transfer_sync_busy_bus);

    RUN_TEST_CASE(TEST_IOT_SPI, test_spi_transfer_async_valid_parameters);
    RUN_TEST_CASE(TEST_IOT_SPI, test_spi_transfer_async_null_handle);
    RUN_TEST_CASE(TEST_IOT_SPI, test_spi_transfer_async_null_tx_buffer);
    RUN_TEST_CASE(TEST_IOT_SPI, test_spi_transfer_async_null_rx_buffer);
    RUN_TEST_CASE(TEST_IOT_SPI, test_spi_transfer_async_zero_bytes);
    RUN_TEST_CASE(TEST_IOT_SPI, test_spi_transfer_async_busy_bus);

    RUN_TEST_CASE(TEST_IOT_SPI, test_spi_close_ValidHandle);
    RUN_TEST_CASE(TEST_IOT_SPI, test_spi_close_NullHandle);
    RUN_TEST_CASE(TEST_IOT_SPI, test_spi_close_NotOpenedHandle);
}

/*-----------------------------------------------------------*/

/**
 * @brief Test Function to test spi open and close
 *
 */
TEST( TEST_IOT_SPI, AFQP_IotSPI_OpenClose )
{
    IotSPIHandle_t xSPIHandle;
    int32_t lRetVal;

    xSPIHandle = iot_spi_open(ultestIotSpiInstance);
    TEST_ASSERT_NOT_EQUAL(NULL, xSPIHandle);

    lRetVal = iot_spi_close(xSPIHandle);
    TEST_ASSERT_EQUAL(0, lRetVal);
}
/*-----------------------------------------------------------*/

/**
 * @brief Test Function to test spi init
 *
 */
TEST( TEST_IOT_SPI, AFQP_IotSPI_Init )
{
    IotSPIHandle_t xSPIHandle;
    IotSPIMode_t xMode;
    IotSPIMasterConfig_t xNewConfig, xOrigConfig, xConfirmConfig;
    int32_t lRetVal;

    xSPIHandle = iot_spi_open(ultestIotSpiInstance);
    TEST_ASSERT_NOT_EQUAL(NULL, xSPIHandle);

    if ( TEST_PROTECT() )
    {
        lRetVal = iot_spi_ioctl(xSPIHandle, eSPIGetMasterConfig, &xOrigConfig);
        TEST_ASSERT_EQUAL(0, lRetVal);

        memcpy(&xNewConfig, &xOrigConfig, sizeof(IotSPIMasterConfig_t));

        for ( xMode = eSPIMode0; xMode <= eSPIMode3; xMode++ )
        {
            xNewConfig.eMode = xMode;
            lRetVal = iot_spi_ioctl(xSPIHandle, eSPISetMasterConfig,
                    &xNewConfig);
            TEST_ASSERT_EQUAL(0, lRetVal);

            lRetVal = iot_spi_ioctl(xSPIHandle, eSPIGetMasterConfig,
                    &xConfirmConfig);
            TEST_ASSERT_EQUAL(0, lRetVal);
        }

        lRetVal = iot_spi_ioctl(xSPIHandle, eSPISetMasterConfig, &xOrigConfig);
        TEST_ASSERT_EQUAL(0, lRetVal);
    }

    lRetVal = iot_spi_close(xSPIHandle);
    TEST_ASSERT_EQUAL(0, lRetVal);
}
/*-----------------------------------------------------------*/

/**
 * @brief Test Function to test spi transfer sync
 *
 */
TEST( TEST_IOT_SPI, AFQP_IotSPI_TransferSync )
{
    IotSPIHandle_t xSPIHandle;
    IotSPIMasterConfig_t xConfig;
    int32_t lRetVal;
    uint8_t ucRxBuf[ 4 ] =
    { 0xff, 0xff, 0xff, 0xff };
    uint8_t ucTxBuf[ 4 ] =
    { 0x00, 0x02, 0x04, 0x06 };
    int32_t lLoop = 0;
    uint16_t xBytesTx, xBytesRx;

    xSPIHandle = iot_spi_open(ultestIotSpiInstance);
    TEST_ASSERT_NOT_EQUAL(NULL, xSPIHandle);

    if ( TEST_PROTECT() )
    {
        /* configure bus */
        xConfig.ulFreq = ultestIotSPIFrequency;
        xConfig.eMode = xtestIotSPIDefaultConfigMode;

        lRetVal = iot_spi_ioctl(xSPIHandle, eSPISetMasterConfig, &xConfig);
        TEST_ASSERT_EQUAL(0, lRetVal);

        /* Select slave before any operation */
        iot_spi_select_slave(xSPIHandle, SLAVE_SELECT_NUM);

        /*enable write in EEPROM*/
        prvEnableWrite(xSPIHandle);

        /* use some data to transfer.  Send even, expect odd */
        lRetVal = iot_spi_transfer_sync(xSPIHandle, ucTxBuf, ucRxBuf, 4);
        TEST_ASSERT_EQUAL(0, lRetVal);

        lRetVal = iot_spi_ioctl(xSPIHandle, eSPIGetRxNoOfbytes, &xBytesRx);
        TEST_ASSERT_EQUAL(0, lRetVal);
        TEST_ASSERT_EQUAL(4, xBytesRx);

        lRetVal = iot_spi_ioctl(xSPIHandle, eSPIGetTxNoOfbytes, &xBytesTx);
        TEST_ASSERT_EQUAL(0, lRetVal);
        TEST_ASSERT_EQUAL(4, xBytesTx);

        /* Expect slave to send odd numbers on successful master read */
        for ( lLoop = 0; lLoop < 4; lLoop++ )
        {
            TEST_ASSERT_EQUAL(ucRxBuf[ lLoop ], ((lLoop * 2) + 1));
        }
    }

    lRetVal = iot_spi_close(xSPIHandle);
    TEST_ASSERT_EQUAL(0, lRetVal);
}
/*-----------------------------------------------------------*/

/**
 * @brief Test Function to test spi transfer async
 *
 */
TEST( TEST_IOT_SPI, AFQP_IotSPI_TransferAsync )
{
    IotSPIHandle_t xSPIHandle;
    IotSPIMasterConfig_t xOrigConfig, xTestConfig;
    int32_t lRetVal;
    uint8_t ucRxBuf[ 4 ] =
    { 0xff, 0xff, 0xff, 0xff };
    uint8_t ucTxBuf[ 4 ] =
    { 0x00, 0x02, 0x04, 0x06 };
    int32_t lLoop = 0;

    xSPIHandle = iot_spi_open(ultestIotSpiInstance);
    TEST_ASSERT_NOT_EQUAL(NULL, xSPIHandle);

    if ( TEST_PROTECT() )
    {
        /* Save the original configuration for later restore. */
        lRetVal = iot_spi_ioctl(xSPIHandle, eSPIGetMasterConfig, &xOrigConfig);
        TEST_ASSERT_EQUAL(0, lRetVal);

        /* configure bus */
        xTestConfig.ulFreq = ultestIotSPIFrequency;
        xTestConfig.eMode = xtestIotSPIDefaultConfigMode;

        lRetVal = iot_spi_ioctl(xSPIHandle, eSPISetMasterConfig, &xTestConfig);
        TEST_ASSERT_EQUAL(0, lRetVal);

        /* Set a callback for async call */
        iot_spi_set_callback(xSPIHandle, prvSpiAsyncCallback, NULL);

        /* Select slave before any operation */
        iot_spi_select_slave(xSPIHandle, SLAVE_SELECT_NUM);

        /* use some data to transfer.  Send even, expect odd */
        lRetVal = iot_spi_transfer_async(xSPIHandle, ucTxBuf, ucRxBuf, 4);
        TEST_ASSERT_EQUAL(0, lRetVal);

        /*Wait for the callback. */
        lRetVal = xSemaphoreTake(xtestIotSPISemaphore,
                testIotSPI_DEFAULT_SEMAPHORE_DELAY);
        TEST_ASSERT_EQUAL(pdTRUE, lRetVal);

        /* Expect slave to send odd numbers on successful master read */
        for ( lLoop = 0; lLoop < 4; lLoop++ )
        {
            TEST_ASSERT_EQUAL(ucRxBuf[ lLoop ], ((lLoop * 2) + 1));
        }

        /* Restore the original configuration saved in the beginning of this test,
         * in order to reset to the original state before this test. */
        lRetVal = iot_spi_ioctl(xSPIHandle, eSPISetMasterConfig, &xOrigConfig);
        TEST_ASSERT_EQUAL(0, lRetVal);
    }

    lRetVal = iot_spi_close(xSPIHandle);
    TEST_ASSERT_EQUAL(0, lRetVal);
}
/*-----------------------------------------------------------*/

/**
 * @brief Test Function to test spi cancel with nothing to cancel
 *
 */
TEST( TEST_IOT_SPI, AFQP_IotSPI_CancelFail )
{
    IotSPIHandle_t xSPIHandle;
    int32_t lRetVal;

    xSPIHandle = iot_spi_open(ultestIotSpiInstance);
    TEST_ASSERT_NOT_EQUAL(NULL, xSPIHandle);

    if ( TEST_PROTECT() )
    {
        lRetVal = iot_spi_cancel(xSPIHandle);

        TEST_ASSERT_EQUAL(-ENOSYS, lRetVal);
    }

    lRetVal = iot_spi_close(xSPIHandle);
    TEST_ASSERT_EQUAL(0, lRetVal);
}
/*-----------------------------------------------------------*/

/**
 * @brief Test Function to test spi cancel
 *
 */
TEST( TEST_IOT_SPI, AFQP_IotSPI_CancelSuccess )
{
    IotSPIHandle_t xSPIHandle;
    IotSPIMasterConfig_t xOrigConfig, xTestConfig;
    int32_t lRetVal;
    uint8_t ucRxBuf[ 4 ] =
    { 0xff, 0xff, 0xff, 0xff };
    uint8_t ucTxBuf[ 4 ] =
    { 0x00, 0x02, 0x04, 0x06 };

    xSPIHandle = iot_spi_open(ultestIotSpiInstance);
    TEST_ASSERT_NOT_EQUAL(NULL, xSPIHandle);

    if ( TEST_PROTECT() )
    {
        /** Save the original configuration for later restore. * / */
        lRetVal = iot_spi_ioctl(xSPIHandle, eSPIGetMasterConfig, &xOrigConfig);
        TEST_ASSERT_EQUAL(0, lRetVal);

        /* configure bus */
        xTestConfig.ulFreq = ultestIotSPIFrequency;
        xTestConfig.eMode = xtestIotSPIDefaultConfigMode;

        lRetVal = iot_spi_ioctl(xSPIHandle, eSPISetMasterConfig, &xTestConfig);
        TEST_ASSERT_EQUAL(0, lRetVal);

        /* use some data to transfer.  Send even, expect odd */
        lRetVal = iot_spi_transfer_async(xSPIHandle, ucTxBuf, ucRxBuf, 4);
        TEST_ASSERT_EQUAL(0, lRetVal);

        lRetVal = iot_spi_cancel(xSPIHandle);

        if ( lRetVal != -ENOSYS )
        {
            TEST_ASSERT_EQUAL(0, lRetVal);
        }

        /* Restore the original configuration saved in the beginning of this test,
         * in order to reset to the original state before this test. */
        lRetVal = iot_spi_ioctl(xSPIHandle, eSPISetMasterConfig, &xOrigConfig);
        TEST_ASSERT_EQUAL(0, lRetVal);
    }

    lRetVal = iot_spi_close(xSPIHandle);
    TEST_ASSERT_EQUAL(0, lRetVal);
}
/*-----------------------------------------------------------*/

/**
 * @brief Test Function to test spi open with invalid params
 *
 */
TEST( TEST_IOT_SPI, AFQP_IotSPI_OpenFuzzing )
{
    IotSPIHandle_t xSPIHandle, xSPIHandle2;
    int32_t lRetVal;

    /* Open invalid SPI handle */
    xSPIHandle = iot_spi_open(-1);
    TEST_ASSERT_EQUAL(NULL, xSPIHandle);

    xSPIHandle = iot_spi_open(ultestIotSpiInstance);
    TEST_ASSERT_NOT_EQUAL(NULL, xSPIHandle);

    if ( TEST_PROTECT() )
    {
        /* Opening the same SPI is not allowed */
        xSPIHandle2 = iot_spi_open(ultestIotSpiInstance);
        TEST_ASSERT_EQUAL(NULL, xSPIHandle2);
    }

    lRetVal = iot_spi_close(xSPIHandle);
    TEST_ASSERT_EQUAL(0, lRetVal);
}
/*-----------------------------------------------------------*/

/**
 * @brief Test Function to test spi close with invalid params
 *
 */
TEST( TEST_IOT_SPI, AFQP_IotSPI_CloseFuzzing )
{
    IotSPIHandle_t xSPIHandle;
    int32_t lRetVal;

    lRetVal = iot_spi_close( NULL);
    TEST_ASSERT_EQUAL(-EINVAL, lRetVal);

    xSPIHandle = iot_spi_open(ultestIotSpiInstance);
    TEST_ASSERT_NOT_EQUAL(NULL, xSPIHandle);

    /* Close with valid handle */
    lRetVal = iot_spi_close(xSPIHandle);
    TEST_ASSERT_EQUAL(0, lRetVal);

    /* Close with stale handle */
    lRetVal = iot_spi_close(xSPIHandle);
    TEST_ASSERT_EQUAL(-EINVAL, lRetVal);
}
/*-----------------------------------------------------------*/

/**
 * @brief Test Function to test spi ioctl with invalid params
 *
 */
TEST( TEST_IOT_SPI, AFQP_IotSPI_IoctlFuzzing )
{
    IotSPIHandle_t xSPIHandle;
    IotSPIMasterConfig_t xOrigConfig;
    IotSPIIoctlRequest_t xRequest;
    int32_t lRetVal;

    xSPIHandle = iot_spi_open(ultestIotSpiInstance);
    TEST_ASSERT_NOT_EQUAL(NULL, xSPIHandle);

    xOrigConfig.eMode = eSPIMode0;
    xOrigConfig.ulFreq = 1000000U;

    lRetVal = iot_spi_ioctl(xSPIHandle, eSPISetMasterConfig, &xOrigConfig);
    TEST_ASSERT_EQUAL(0, lRetVal);

    /* Try with invalid params.  Expect failures. */
    lRetVal = iot_spi_ioctl( NULL, eSPISetMasterConfig, &xOrigConfig);
    TEST_ASSERT_EQUAL(-EINVAL, lRetVal);

    lRetVal = iot_spi_ioctl(xSPIHandle, -1, &xOrigConfig);
    TEST_ASSERT_EQUAL(-EINVAL, lRetVal);

    for ( xRequest = eSPISetMasterConfig; xRequest <= eSPIGetRxNoOfbytes;
            xRequest++ )
    {
        lRetVal = iot_spi_ioctl(xSPIHandle, xRequest, NULL);
        TEST_ASSERT_EQUAL(-EINVAL, lRetVal);
    }

    lRetVal = iot_spi_close(xSPIHandle);
    TEST_ASSERT_EQUAL(0, lRetVal);
}
/*-----------------------------------------------------------*/

/**
 * @brief Test Function to test spi readSync with invalid params
 *
 */
TEST( TEST_IOT_SPI, AFQP_IotSPI_ReadSyncFuzzing )
{
    IotSPIHandle_t xSPIHandle;
    int32_t lRetVal;
    uint8_t ucBuffer[ SPI_BUFFER_SIZE ];

    xSPIHandle = iot_spi_open(ultestIotSpiInstance);
    TEST_ASSERT_NOT_EQUAL(NULL, xSPIHandle);

    /* Select slave before any operation */
    iot_spi_select_slave(xSPIHandle, SLAVE_SELECT_NUM);

    if ( TEST_PROTECT() )
    {
        lRetVal = iot_spi_read_sync( NULL, ucBuffer, SPI_BUFFER_SIZE);
        TEST_ASSERT_EQUAL(-EINVAL, lRetVal);

        lRetVal = iot_spi_read_sync(xSPIHandle, NULL, SPI_BUFFER_SIZE);
        TEST_ASSERT_EQUAL(-EINVAL, lRetVal);

        lRetVal = iot_spi_read_sync(xSPIHandle, ucBuffer, 0);
        TEST_ASSERT_EQUAL(-EINVAL, lRetVal);
    }

    lRetVal = iot_spi_close(xSPIHandle);
    TEST_ASSERT_EQUAL(0, lRetVal);
}
/*-----------------------------------------------------------*/

/**
 * @brief Test Function to test spi readAsync with invalid params
 *
 */
TEST( TEST_IOT_SPI, AFQP_IotSPI_ReadAsyncFuzzing )
{
    IotSPIHandle_t xSPIHandle;
    int32_t lRetVal;
    uint8_t ucBuffer[ SPI_BUFFER_SIZE ];

    xSPIHandle = iot_spi_open(ultestIotSpiInstance);
    TEST_ASSERT_NOT_EQUAL(NULL, xSPIHandle);

    /* Select slave before any operation */
    iot_spi_select_slave(xSPIHandle, SLAVE_SELECT_NUM);

    if ( TEST_PROTECT() )
    {
        lRetVal = iot_spi_read_async( NULL, ucBuffer, SPI_BUFFER_SIZE);
        TEST_ASSERT_EQUAL(-EINVAL, lRetVal);

        lRetVal = iot_spi_read_async(xSPIHandle, NULL, SPI_BUFFER_SIZE);
        TEST_ASSERT_EQUAL(-EINVAL, lRetVal);

        lRetVal = iot_spi_read_async(xSPIHandle, ucBuffer, 0);
        TEST_ASSERT_EQUAL(-EINVAL, lRetVal);
    }

    lRetVal = iot_spi_close(xSPIHandle);
    TEST_ASSERT_EQUAL(0, lRetVal);
}
/*-----------------------------------------------------------*/

/**
 * @brief Test Function to test spi writeSync with invalid params
 *
 */
TEST( TEST_IOT_SPI, AFQP_IotSPI_WriteSyncFuzzing )
{
    IotSPIHandle_t xSPIHandle;
    int32_t lRetVal;
    uint8_t ucBuffer[ SPI_BUFFER_SIZE ];

    xSPIHandle = iot_spi_open(ultestIotSpiInstance);
    TEST_ASSERT_NOT_EQUAL(NULL, xSPIHandle);

    /* Select slave before any operation */
    iot_spi_select_slave(xSPIHandle, SLAVE_SELECT_NUM);

    if ( TEST_PROTECT() )
    {
        lRetVal = iot_spi_write_sync( NULL, ucBuffer, SPI_BUFFER_SIZE);
        TEST_ASSERT_EQUAL(-EINVAL, lRetVal);

        lRetVal = iot_spi_write_sync(xSPIHandle, NULL, SPI_BUFFER_SIZE);
        TEST_ASSERT_EQUAL(-EINVAL, lRetVal);

        lRetVal = iot_spi_write_sync(xSPIHandle, ucBuffer, 0);
        TEST_ASSERT_EQUAL(-EINVAL, lRetVal);
    }

    lRetVal = iot_spi_close(xSPIHandle);
    TEST_ASSERT_EQUAL(0, lRetVal);
}
/*-----------------------------------------------------------*/

/**
 * @brief Test Function to test spi writeAsync with invalid params
 *
 */
TEST( TEST_IOT_SPI, AFQP_IotSPI_WriteAsyncFuzzing )
{
    IotSPIHandle_t xSPIHandle;
    int32_t lRetVal;
    uint8_t ucBuffer[ SPI_BUFFER_SIZE ];

    xSPIHandle = iot_spi_open(ultestIotSpiInstance);
    TEST_ASSERT_NOT_EQUAL(NULL, xSPIHandle);

    /* Select slave before any operation */
    iot_spi_select_slave(xSPIHandle, SLAVE_SELECT_NUM);

    if ( TEST_PROTECT() )
    {
        lRetVal = iot_spi_write_async( NULL, ucBuffer, SPI_BUFFER_SIZE);
        TEST_ASSERT_EQUAL(-EINVAL, lRetVal);

        lRetVal = iot_spi_write_async(xSPIHandle, NULL, SPI_BUFFER_SIZE);
        TEST_ASSERT_EQUAL(-EINVAL, lRetVal);

        lRetVal = iot_spi_write_async(xSPIHandle, ucBuffer, 0);
        TEST_ASSERT_EQUAL(-EINVAL, lRetVal);
    }

    lRetVal = iot_spi_close(xSPIHandle);
    TEST_ASSERT_EQUAL(0, lRetVal);
}
/*-----------------------------------------------------------*/

/**
 * @brief Test Function to test spi transferSync with invalid params
 *
 */
TEST( TEST_IOT_SPI, AFQP_IotSPI_TransferSyncFuzzing )
{
    IotSPIHandle_t xSPIHandle;
    int32_t lRetVal;
    uint8_t ucRxBuffer[ SPI_BUFFER_SIZE ];
    uint8_t ucTxBuffer[ SPI_BUFFER_SIZE ];

    xSPIHandle = iot_spi_open(ultestIotSpiInstance);
    TEST_ASSERT_NOT_EQUAL(NULL, xSPIHandle);

    /* Select slave before any operation */
    iot_spi_select_slave(xSPIHandle, SLAVE_SELECT_NUM);

    if ( TEST_PROTECT() )
    {
        lRetVal = iot_spi_transfer_sync( NULL, ucRxBuffer, ucTxBuffer,
        SPI_BUFFER_SIZE);
        TEST_ASSERT_EQUAL(-EINVAL, lRetVal);

        lRetVal = iot_spi_transfer_sync(xSPIHandle, NULL, ucTxBuffer,
        SPI_BUFFER_SIZE);
        TEST_ASSERT_EQUAL(-EINVAL, lRetVal);

        lRetVal = iot_spi_transfer_sync(xSPIHandle, ucRxBuffer, NULL,
        SPI_BUFFER_SIZE);
        TEST_ASSERT_EQUAL(-EINVAL, lRetVal);

        lRetVal = iot_spi_transfer_sync(xSPIHandle, ucRxBuffer, ucTxBuffer, 0);
        TEST_ASSERT_EQUAL(-EINVAL, lRetVal);
    }

    lRetVal = iot_spi_close(xSPIHandle);
    TEST_ASSERT_EQUAL(0, lRetVal);
}
/*-----------------------------------------------------------*/

/**
 * @brief Test Function to test spi transferAsync with invalid params
 *
 */
TEST( TEST_IOT_SPI, AFQP_IotSPI_TransferAsyncFuzzing )
{
    IotSPIHandle_t xSPIHandle;
    int32_t lRetVal;
    uint8_t ucRxBuffer[ SPI_BUFFER_SIZE ];
    uint8_t ucTxBuffer[ SPI_BUFFER_SIZE ];

    xSPIHandle = iot_spi_open(ultestIotSpiInstance);
    TEST_ASSERT_NOT_EQUAL(NULL, xSPIHandle);

    /* Select slave before any operation */
    iot_spi_select_slave(xSPIHandle, SLAVE_SELECT_NUM);

    if ( TEST_PROTECT() )
    {
        lRetVal = iot_spi_transfer_async( NULL, ucRxBuffer, ucTxBuffer,
        SPI_BUFFER_SIZE);
        TEST_ASSERT_EQUAL(-EINVAL, lRetVal);

        lRetVal = iot_spi_transfer_async(xSPIHandle, NULL, ucTxBuffer,
        SPI_BUFFER_SIZE);
        TEST_ASSERT_EQUAL(-EINVAL, lRetVal);

        lRetVal = iot_spi_transfer_async(xSPIHandle, ucRxBuffer, NULL,
        SPI_BUFFER_SIZE);
        TEST_ASSERT_EQUAL(-EINVAL, lRetVal);

        lRetVal = iot_spi_transfer_async(xSPIHandle, ucRxBuffer, ucTxBuffer, 0);
        TEST_ASSERT_EQUAL(-EINVAL, lRetVal);
    }

    lRetVal = iot_spi_close(xSPIHandle);
    TEST_ASSERT_EQUAL(0, lRetVal);
}
/*-----------------------------------------------------------*/

/**
 * @brief Test Function to test spi cancel with invalid params
 *
 */
TEST( TEST_IOT_SPI, AFQP_IotSPI_CancelFuzzing )
{
    IotSPIHandle_t xSPIHandle;
    int32_t lRetVal;

    xSPIHandle = iot_spi_open(ultestIotSpiInstance);
    TEST_ASSERT_NOT_EQUAL(NULL, xSPIHandle);

    if ( TEST_PROTECT() )
    {
        lRetVal = iot_spi_cancel( NULL);
        TEST_ASSERT_EQUAL(-ENOSYS, lRetVal);

        lRetVal = iot_spi_cancel(xSPIHandle);
        TEST_ASSERT_EQUAL(-ENOSYS, lRetVal);
    }

    lRetVal = iot_spi_close(xSPIHandle);
    TEST_ASSERT_EQUAL(0, lRetVal);
}
/*-----------------------------------------------------------*/

/**
 * @brief Test Function to test spi cancel with invalid params
 *
 */
TEST( TEST_IOT_SPI, AFQP_IotSPI_EepromTransferSync )
{

    IotSPIHandle_t xSPIHandle;
    IotSPIMasterConfig_t xConfig;
    int32_t lRetVal;
    int32_t i = 0;
    uint8_t ucTxBuf[ 70 ] =
    { 0 };
    uint8_t ucRxBuf[ 700 ] =
    { 0 };
    uint8_t cmd[ 100 ] =
    { 0 };
    uint16_t xBytesTx;
    uint16_t xBytesRx;

    cmd[ 0 ] = EEPROM_WRITE;
    cmd[ 1 ] = (EEPROM_ADDRESS >> 8) & 0xFF;
    cmd[ 2 ] = EEPROM_ADDRESS & 0xFF;
    for ( i = 0; i < 64; i++ )
    {
        ucTxBuf[ i ] = i + 1;
    }
    for ( i = 0; i < 64; i++ )
    {
        cmd[ 3 + i ] = ucTxBuf[ i ];
    }

    xSPIHandle = iot_spi_open(ultestIotSpiInstance);
    TEST_ASSERT_NOT_EQUAL(NULL, xSPIHandle);

    if ( TEST_PROTECT() )
    {
        /* configure bus */
        xConfig.ulFreq = ultestIotSPIFrequency;
        xConfig.eMode = xtestIotSPIDefaultConfigMode;

        lRetVal = iot_spi_ioctl(xSPIHandle, eSPISetMasterConfig, &xConfig);
        TEST_ASSERT_EQUAL(0, lRetVal);

        /* Select slave before any operation */
        iot_spi_select_slave(xSPIHandle, SLAVE_SELECT_NUM);

        /* EEPROM write enable */
        prvEnableWrite(xSPIHandle);

        /* write to EEPROM */
        iot_spi_ioctl(xSPIHandle, eSPIActivateSlave, NULL);
        lRetVal = iot_spi_write_sync(xSPIHandle, cmd, 67);
        vTaskDelay(pdMS_TO_TICKS(5));
        iot_spi_ioctl(xSPIHandle, eSPIDeactivateSlave, NULL);
        TEST_ASSERT_EQUAL(0, lRetVal);
        iot_spi_ioctl(xSPIHandle, eSPIGetTxNoOfbytes, &xBytesTx);

        /*send read command and read back using transfer API*/
        cmd[ 0 ] = EEPROM_READ;
        for ( i = 0; i < 64; i++ )
        {
            cmd[ 3 + i ] = i + 9;
        }
        lRetVal = iot_spi_transfer_sync(xSPIHandle, cmd, ucRxBuf, 67);
        TEST_ASSERT_EQUAL(0, lRetVal);

        iot_spi_ioctl(xSPIHandle, eSPIGetRxNoOfbytes, &xBytesRx);
        TEST_ASSERT_EQUAL(xBytesTx, xBytesRx);

        TEST_ASSERT_EQUAL(0, memcmp(ucTxBuf, &ucRxBuf[ 3 ], 64));

    }

    lRetVal = iot_spi_close(xSPIHandle);
    TEST_ASSERT_EQUAL(0, lRetVal);

}
/*-----------------------------------------------------------*/

/**
 * @brief Test Function to verify a page write
 *
 */
TEST( TEST_IOT_SPI, AFQP_IotSPI_EepromPageTransferSync )
{

    IotSPIHandle_t xSPIHandle;
    IotSPIMasterConfig_t xConfig;
    uint32_t size = 64;

    int32_t lRetVal;
    uint32_t lLoop = 0, i = 0;
    uint8_t ucTxBuf[ 150 ] =
    { 0 };
    uint8_t ucRxBuf[ 150 ] =
    { 0 };
    uint8_t cmd[ 200 ] =
    { 0 };
    uint16_t xBytesTx;
    uint16_t xBytesRx;

    cmd[ 0 ] = EEPROM_WRITE;
    cmd[ 1 ] = (EEPROM_ADDRESS >> 8) & 0xFF;
    cmd[ 2 ] = EEPROM_ADDRESS & 0xFF;
    for ( i = 0; i < size; i++ )
    {
        ucTxBuf[ i ] = i + 1;
    }
    for ( i = 0; i < size; i++ )
    {
        cmd[ 3 + i ] = ucTxBuf[ i ];
    }

    xSPIHandle = iot_spi_open(ultestIotSpiInstance);
    TEST_ASSERT_NOT_EQUAL(NULL, xSPIHandle);

    if ( TEST_PROTECT() )
    {
        /* configure bus */
        xConfig.ulFreq = ultestIotSPIFrequency;
        xConfig.eMode = xtestIotSPIDefaultConfigMode;

        lRetVal = iot_spi_ioctl(xSPIHandle, eSPISetMasterConfig, &xConfig);
        TEST_ASSERT_EQUAL(0, lRetVal);

        /* Select slave before any operation */
        iot_spi_select_slave(xSPIHandle, SLAVE_SELECT_NUM);

        /* EEPROM write enable */
        prvEnableWrite(xSPIHandle);

        /* write to EEPROM */
        iot_spi_ioctl(xSPIHandle, eSPIActivateSlave, NULL);
        lRetVal = iot_spi_write_sync(xSPIHandle, cmd, size + 3);

    vTaskDelay(pdMS_TO_TICKS(5));

        iot_spi_ioctl(xSPIHandle, eSPIDeactivateSlave, NULL);
        TEST_ASSERT_EQUAL(0, lRetVal);
        iot_spi_ioctl(xSPIHandle, eSPIGetTxNoOfbytes, &xBytesTx);

        /*send read command and read back using transfer API*/
        cmd[ 0 ] = EEPROM_READ;
        cmd[ 1 ] = (EEPROM_ADDRESS >> 8) & 0xFF;
        cmd[ 2 ] = EEPROM_ADDRESS & 0xFF;

        for ( i = 0; i < size; i++ )
        {
            cmd[ 3 + i ] = i + 0x0F;
        }

        lRetVal = iot_spi_transfer_sync(xSPIHandle, cmd, ucRxBuf, size + 3);
        TEST_ASSERT_EQUAL(0, lRetVal);

        iot_spi_ioctl(xSPIHandle, eSPIGetRxNoOfbytes, &xBytesRx);
        TEST_ASSERT_EQUAL(xBytesTx, xBytesRx);

        for ( lLoop = 0; lLoop < size; lLoop++ )
        {
            TEST_ASSERT_EQUAL(ucTxBuf[ lLoop ], ucRxBuf[ lLoop + 3 ]);
        }

    }

    lRetVal = iot_spi_close(xSPIHandle);
    TEST_ASSERT_EQUAL(0, lRetVal);

}

TEST( TEST_IOT_SPI, AFQP_IotSPI_EepromMultiPageTransferSync )
{

    IotSPIHandle_t xSPIHandle;
    IotSPIMasterConfig_t xConfig;
    uint32_t offset = 0;
    uint32_t chunkSize = 64;
    uint32_t totalSize = 1024;
    uint8_t cmd[ 200 ] =
    { 0 };

    int32_t lRetVal;
    uint32_t lLoop = 0;


    /*Get the incremental pattern*/
    fill_buf(wrBuf, chunkSize, 0);

    for ( lLoop = 0; lLoop < chunkSize; lLoop++ )
    {
        cmd[ 3 + lLoop ] = wrBuf[ lLoop ];
    }


    for (lLoop = 0; lLoop < totalSize; lLoop += chunkSize)
    {
        /*Copy the data to compare later*/
        memcpy(&cmpBuf[lLoop], wrBuf, 64);
    }

    xSPIHandle = iot_spi_open(ultestIotSpiInstance);
    TEST_ASSERT_NOT_EQUAL(NULL, xSPIHandle);

    if ( TEST_PROTECT() )
    {
        /* configure bus */
        xConfig.ulFreq = ultestIotSPIFrequency;
        xConfig.eMode = xtestIotSPIDefaultConfigMode;

        lRetVal = iot_spi_ioctl(xSPIHandle, eSPISetMasterConfig, &xConfig);
        TEST_ASSERT_EQUAL(0, lRetVal);

        /* Select slave before any operation */
        iot_spi_select_slave(xSPIHandle, SLAVE_SELECT_NUM);

        /* write to EEPROM  page wise*/
    for(offset = 0; offset < totalSize; offset += chunkSize)
    {
            cmd[ 0 ] = EEPROM_WRITE;
            cmd[ 1 ] = (offset >> 8) & 0xFF;
            cmd[ 2 ] = offset & 0xFF;

            /* EEPROM write enable */
            prvEnableWrite(xSPIHandle);

            /* write to EEPROM */
            iot_spi_ioctl(xSPIHandle, eSPIActivateSlave, NULL);
            lRetVal = iot_spi_write_sync(xSPIHandle, cmd, chunkSize + 3);

        /*5 millisecond delay after each page write*/
        vTaskDelay(pdMS_TO_TICKS(5));

            iot_spi_ioctl(xSPIHandle, eSPIDeactivateSlave, NULL);
    }

        /*send read command and read back using transfer API*/
        cmd[ 0 ] = EEPROM_READ;
        cmd[ 1 ] = (0 >> 8) & 0xFF;
        cmd[ 2 ] = 0 & 0xFF;

        for ( lLoop = 0; lLoop < chunkSize; lLoop++ )
        {
            cmd[ 3 + lLoop ] = lLoop + 0x0F;
        }

        lRetVal = iot_spi_transfer_sync(xSPIHandle, cmd, rdBuf, totalSize + 3);
        TEST_ASSERT_EQUAL(0, lRetVal);

        for ( lLoop = 0; lLoop < totalSize; lLoop++ )
        {
            TEST_ASSERT_EQUAL(cmpBuf[ lLoop ], rdBuf[ lLoop + 3 ]);
        }

    }

    lRetVal = iot_spi_close(xSPIHandle);
    TEST_ASSERT_EQUAL(0, lRetVal);
}

TEST( TEST_IOT_SPI, test_spi_open_valid_instance )
{
    SpiHandle_t handle = spi_open(0);
    TEST_ASSERT_NOT_NULL(handle);

    int32_t result = spi_close(handle);
    TEST_ASSERT_EQUAL(0, result);
}

TEST( TEST_IOT_SPI, test_spi_open_invalid_instance )
{
    SpiHandle_t handle = spi_open(-1);
    TEST_ASSERT_NULL(handle);
}

TEST( TEST_IOT_SPI, test_spi_open_same_instance_twice )
{
    SpiHandle_t handle1 = spi_open(0);
    TEST_ASSERT_NOT_NULL(handle1);

    SpiHandle_t handle2 = spi_open(0);
    TEST_ASSERT_NULL(handle2);

    int32_t result = spi_close(handle1);
    TEST_ASSERT_EQUAL(0, result);
}

TEST( TEST_IOT_SPI, test_spi_set_callback_valid_parameters )
{
    SpiHandle_t handle = spi_open(0);
    TEST_ASSERT_NOT_NULL(handle);

    int32_t result = spi_set_callback(handle, prvSpiAsyncCallback, (void *)0x1234);
    TEST_ASSERT_EQUAL(0, result);

    result = spi_close(handle);
    TEST_ASSERT_EQUAL(0, result);
}

TEST( TEST_IOT_SPI, test_spi_set_callback_null_handle )
{
    int32_t result = spi_set_callback(NULL, prvSpiAsyncCallback, (void *)0x1234);
    TEST_ASSERT_EQUAL(-EINVAL, result);
}

TEST( TEST_IOT_SPI, test_spi_set_callback_null_callback )
{
    SpiHandle_t handle = spi_open(0);
    TEST_ASSERT_NOT_NULL(handle);

    int32_t result = spi_set_callback(handle, NULL, (void *)0x1234);
    TEST_ASSERT_EQUAL(0, result);

    result = spi_close(handle);
    TEST_ASSERT_EQUAL(0, result);
}

TEST( TEST_IOT_SPI, test_spi_set_callback_null_user_context )
{
    SpiHandle_t handle = spi_open(0);
    TEST_ASSERT_NOT_NULL(handle);

    int32_t result = spi_set_callback(handle, prvSpiAsyncCallback, NULL);
    TEST_ASSERT_EQUAL(0, result);

    result = spi_close(handle);
    TEST_ASSERT_EQUAL(0, result);
}

TEST( TEST_IOT_SPI, test_spi_ioctl_set_master_config_valid )
{
    SpiHandle_t handle = spi_open(0);
    TEST_ASSERT_NOT_NULL(handle);

    SpiMasterConfig_t config;
    config.eMode = eSPIMode3;
    config.ulFreq = 500000U;

    int32_t result = spi_ioctl(handle, eSPISetMasterConfig, &config);
    TEST_ASSERT_EQUAL(0, result);

    result = spi_close(handle);
    TEST_ASSERT_EQUAL(0, result);
}

TEST( TEST_IOT_SPI, test_spi_ioctl_set_master_config_null_handle )
{
    SpiMasterConfig_t config;
    config.eMode = eSPIMode3;
    config.ulFreq = 500000U;

    int32_t result = spi_ioctl(NULL, eSPISetMasterConfig, &config);
    TEST_ASSERT_EQUAL(-EINVAL, result);
}

TEST( TEST_IOT_SPI, test_spi_ioctl_set_master_config_null_buffer )
{
    SpiHandle_t handle = spi_open(0);
    TEST_ASSERT_NOT_NULL(handle);

    int32_t result = spi_ioctl(handle, eSPISetMasterConfig, NULL);
    TEST_ASSERT_EQUAL(-EINVAL, result);

    result =  spi_close(handle);
    TEST_ASSERT_EQUAL(0, result);
}

TEST( TEST_IOT_SPI, test_spi_ioctl_get_master_config_valid )
{
    SpiHandle_t handle = spi_open(0);
    TEST_ASSERT_NOT_NULL(handle);

    SpiMasterConfig_t config;
    int32_t result = spi_ioctl(handle, eSPIGetMasterConfig, &config);
    TEST_ASSERT_EQUAL(0, result);

    result = spi_close(handle);
    TEST_ASSERT_EQUAL(0, result);
}

TEST( TEST_IOT_SPI, test_spi_ioctl_get_master_config_null_handle )
{
    SpiMasterConfig_t config;
    int32_t result = spi_ioctl(NULL, eSPIGetMasterConfig, &config);
    TEST_ASSERT_EQUAL(-EINVAL, result);
}

TEST( TEST_IOT_SPI, test_spi_ioctl_get_master_config_null_buffer )
{
    SpiHandle_t handle = spi_open(0);
    TEST_ASSERT_NOT_NULL(handle);

    int32_t result = spi_ioctl(handle, eSPIGetMasterConfig, NULL);
    TEST_ASSERT_EQUAL(-EINVAL, result);

    result = spi_close(handle);
    TEST_ASSERT_EQUAL(0, result);
}

TEST( TEST_IOT_SPI, test_spi_read_sync_null_handle )
{
    uint8_t buffer[10];
    int32_t result = spi_read_sync(NULL, buffer, sizeof(buffer));
    TEST_ASSERT_EQUAL(-EINVAL, result);
}

TEST( TEST_IOT_SPI, test_spi_read_sync_null_buffer )
{
    SpiHandle_t handle = spi_open(0);
    TEST_ASSERT_NOT_NULL(handle);

    int32_t result = spi_read_sync(handle, NULL, 10);
    TEST_ASSERT_EQUAL(-EINVAL, result);

    result = spi_close(handle);
    TEST_ASSERT_EQUAL(0, result);
}

TEST( TEST_IOT_SPI, test_spi_read_sync_zero_bytes )
{
    SpiHandle_t handle = spi_open(0);
    TEST_ASSERT_NOT_NULL(handle);

    uint8_t buffer[3];
    int32_t result = spi_read_sync(handle, buffer, 0);
    TEST_ASSERT_EQUAL(-EINVAL, result);

     result = spi_close(handle);
     TEST_ASSERT_EQUAL(0, result);
}

TEST( TEST_IOT_SPI, test_spi_read_async_null_handle )
{
    uint8_t buffer[10];
    int32_t result = spi_read_async(NULL, buffer, sizeof(buffer));
    TEST_ASSERT_EQUAL(-EINVAL, result);
}

TEST( TEST_IOT_SPI, test_spi_read_async_null_buffer )
{
    SpiHandle_t handle = spi_open(0);
    TEST_ASSERT_NOT_NULL(handle);

    int32_t result = spi_read_async(handle, NULL, 10);
    TEST_ASSERT_EQUAL(-EINVAL, result);

    result = spi_close(handle);
    TEST_ASSERT_EQUAL(0, result);
}

TEST( TEST_IOT_SPI, test_spi_read_async_zero_bytes )
{
    SpiHandle_t handle = spi_open(0);
    TEST_ASSERT_NOT_NULL(handle);

    uint8_t buffer[10];
    int32_t result = spi_read_async(handle, buffer, 0);
    TEST_ASSERT_EQUAL(-EINVAL, result);

    result = spi_close(handle);
    TEST_ASSERT_EQUAL(0, result);
}

TEST( TEST_IOT_SPI, test_spi_write_sync_valid_parameters )
{
    SpiHandle_t handle = spi_open(0);
    TEST_ASSERT_NOT_NULL(handle);

    SpiMasterConfig_t config;
    config.eMode = eSPIMode3;
    config.ulFreq = 500000U;

    int32_t result = spi_ioctl(handle, eSPISetMasterConfig, &config);
    TEST_ASSERT_EQUAL(0, result);

    result = spi_select_slave(handle, 1);
    TEST_ASSERT_EQUAL(0, result);

    result = spi_ioctl(handle, eSPIActivateSlave, NULL);
    TEST_ASSERT_EQUAL(0, result);

    uint8_t buffer[10] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A};
    result = spi_write_sync(handle, buffer, sizeof(buffer));
    TEST_ASSERT_EQUAL(0, result);

    result = spi_ioctl(handle, eSPIDeactivateSlave, NULL);
    TEST_ASSERT_EQUAL(0, result);

    result = spi_close(handle);
    TEST_ASSERT_EQUAL(0, result);
}

TEST( TEST_IOT_SPI, test_spi_write_sync_null_handle )
{
    uint8_t buffer[10] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A};
    int32_t result = spi_write_sync(NULL, buffer, sizeof(buffer));
    TEST_ASSERT_EQUAL(-EINVAL, result);
}

TEST( TEST_IOT_SPI, test_spi_write_sync_null_buffer )
{
    SpiHandle_t handle = spi_open(0);
    TEST_ASSERT_NOT_NULL(handle);

    int32_t result = spi_write_sync(handle, NULL, 10);
    TEST_ASSERT_EQUAL(-EINVAL, result);

    result =  spi_close(handle);
    TEST_ASSERT_EQUAL(0, result);
}

TEST( TEST_IOT_SPI, test_spi_write_sync_zero_bytes )
{
    SpiHandle_t handle = spi_open(0);
    TEST_ASSERT_NOT_NULL(handle);

    uint8_t buffer[10] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A};
    int32_t result = spi_write_sync(handle, buffer, 0);
    TEST_ASSERT_EQUAL(-EINVAL, result);

    result = spi_close(handle);
    TEST_ASSERT_EQUAL(0, result);
}

TEST( TEST_IOT_SPI, test_spi_write_sync_busy_bus )
{
    SpiHandle_t handle = spi_open(0);
    TEST_ASSERT_NOT_NULL(handle);

    spi_set_callback(handle, prvSpiAsyncCallback, (void *)0x1234);

    uint8_t buffer[10] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A};
    int32_t result = spi_write_async(handle, buffer, sizeof(buffer));
    TEST_ASSERT_EQUAL(0, result);

    result = spi_write_sync(handle, buffer, sizeof(buffer));
    TEST_ASSERT_EQUAL(-EBUSY, result);

    result = spi_close(handle);
    TEST_ASSERT_EQUAL(0, result);
}

TEST( TEST_IOT_SPI, test_spi_write_async_valid_parameters )
{
    SpiHandle_t handle = spi_open(0);
    TEST_ASSERT_NOT_NULL(handle);

    spi_set_callback(handle, prvSpiAsyncCallback, (void *)0x1234);

    uint8_t buffer[10] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A};
    int32_t result = spi_write_async(handle, buffer, sizeof(buffer));
    TEST_ASSERT_EQUAL(0, result);

    result = spi_close(handle);
    TEST_ASSERT_EQUAL(0, result);
}

TEST( TEST_IOT_SPI, test_spi_write_async_null_handle )
{
    uint8_t buffer[10] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A};
    int32_t result = spi_write_async(NULL, buffer, sizeof(buffer));
    TEST_ASSERT_EQUAL(-EINVAL, result);
}

TEST( TEST_IOT_SPI, test_spi_write_async_null_buffer )
{
    SpiHandle_t handle = spi_open(0); // Assuming 1 is a valid instance number
    TEST_ASSERT_NOT_NULL(handle);

    spi_set_callback(handle, prvSpiAsyncCallback, (void *)0x1234);

    int32_t result = spi_write_async(handle, NULL, 10);
    TEST_ASSERT_EQUAL(-EINVAL, result);

    result = spi_close(handle);
    TEST_ASSERT_EQUAL(0, result);
}

TEST( TEST_IOT_SPI, test_spi_write_async_zero_bytes )
{
    SpiHandle_t handle = spi_open(0);
    TEST_ASSERT_NOT_NULL(handle);

    spi_set_callback(handle, prvSpiAsyncCallback, (void *)0x1234);

    uint8_t buffer[10] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A};
    int32_t result = spi_write_async(handle, buffer, 0);
    TEST_ASSERT_EQUAL(-EINVAL, result);

    result = spi_close(handle);
    TEST_ASSERT_EQUAL(0, result);
}

TEST( TEST_IOT_SPI, test_spi_write_async_busy_bus )
{
    SpiHandle_t handle = spi_open(0);
    TEST_ASSERT_NOT_NULL(handle);

    spi_set_callback(handle, prvSpiAsyncCallback, (void *)0x1234);

    uint8_t buffer[10] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A};
    int32_t result = spi_write_async(handle, buffer, sizeof(buffer));
    TEST_ASSERT_EQUAL(0, result);

    result = spi_write_async(handle, buffer, sizeof(buffer));
    TEST_ASSERT_EQUAL(-EBUSY, result);

    result = spi_close(handle);
    TEST_ASSERT_EQUAL(0, result);
}


TEST(TEST_IOT_SPI, test_spi_close_ValidHandle) {
    SpiHandle_t handle = spi_open(0);
    TEST_ASSERT_NOT_NULL(handle);

    int32_t status = spi_close(handle);
    TEST_ASSERT_EQUAL(0, status);
}

TEST(TEST_IOT_SPI, test_spi_close_NullHandle) {
    int32_t status = spi_close(NULL);
    TEST_ASSERT_EQUAL(-EINVAL, status);
}

TEST(TEST_IOT_SPI, test_spi_close_NotOpenedHandle) {
    SpiHandle_t handle = NULL;
    int32_t status = spi_close(handle);
    TEST_ASSERT_EQUAL(-EINVAL, status);
}

TEST( TEST_IOT_SPI, test_spi_transfer_sync_null_handle )
{
    uint8_t txBuffer[10] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A};
    uint8_t rxBuffer[10];
    int32_t result = spi_transfer_sync(NULL, txBuffer, rxBuffer, sizeof(txBuffer));
    TEST_ASSERT_EQUAL(-EINVAL, result);
}

TEST( TEST_IOT_SPI, test_spi_transfer_sync_null_tx_buffer )
{
    SpiHandle_t handle = spi_open(0); // Assuming 1 is a valid instance number
    TEST_ASSERT_NOT_NULL(handle);

    uint8_t rxBuffer[10];
    int32_t result = spi_transfer_sync(handle, NULL, rxBuffer, 10);
    TEST_ASSERT_EQUAL(-EINVAL, result);

    result = spi_close(handle); // Clean up
    TEST_ASSERT_EQUAL(0, result);
}

TEST( TEST_IOT_SPI, test_spi_transfer_sync_null_rx_buffer )
{
    SpiHandle_t handle = spi_open(0); // Assuming 1 is a valid instance number
    TEST_ASSERT_NOT_NULL(handle);

    uint8_t txBuffer[10] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A};
    int32_t result = spi_transfer_sync(handle, txBuffer, NULL, 10);
    TEST_ASSERT_EQUAL(-EINVAL, result);

    result = spi_close(handle);
    TEST_ASSERT_EQUAL(0, result);
}

TEST( TEST_IOT_SPI, test_spi_transfer_sync_zero_bytes )
{
    SpiHandle_t handle = spi_open(1);
    TEST_ASSERT_NOT_NULL(handle);

    uint8_t txBuffer[10] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A};
    uint8_t rxBuffer[10];
    int32_t result = spi_transfer_sync(handle, txBuffer, rxBuffer, 0);
    TEST_ASSERT_EQUAL(-EINVAL, result);

    result = spi_close(handle);
    TEST_ASSERT_EQUAL(0, result);
}

TEST( TEST_IOT_SPI, test_spi_transfer_sync_busy_bus )
{
    SpiHandle_t handle = spi_open(0);
    TEST_ASSERT_NOT_NULL(handle);

    uint8_t txBuffer[10] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A};
    uint8_t rxBuffer[10];
    int32_t result = spi_transfer_async(handle, txBuffer, rxBuffer, sizeof(txBuffer));
    TEST_ASSERT_EQUAL(0, result);

    result = spi_transfer_sync(handle, txBuffer, rxBuffer, sizeof(txBuffer));
    TEST_ASSERT_EQUAL(-EBUSY, result);

    result = spi_close(handle);
    TEST_ASSERT_EQUAL(0, result);
}

TEST( TEST_IOT_SPI, test_spi_transfer_async_valid_parameters )
{
    SpiHandle_t handle = spi_open(0);
    TEST_ASSERT_NOT_NULL(handle);

    spi_set_callback(handle, prvSpiAsyncCallback, (void *)0x1234);

    uint8_t txBuffer[10] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A};
    uint8_t rxBuffer[10];
    int32_t result = spi_transfer_async(handle, txBuffer, rxBuffer, sizeof(txBuffer));
    TEST_ASSERT_EQUAL(0, result);

    result = spi_close(handle);
    TEST_ASSERT_EQUAL(0, result);
}

TEST( TEST_IOT_SPI, test_spi_transfer_async_null_handle )
{
    uint8_t txBuffer[10] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A};
    uint8_t rxBuffer[10];
    int32_t result = spi_transfer_async(NULL, txBuffer, rxBuffer, sizeof(txBuffer));
    TEST_ASSERT_EQUAL(-EINVAL, result);
}

TEST( TEST_IOT_SPI, test_spi_transfer_async_null_tx_buffer )
{
    SpiHandle_t handle = spi_open(0);
    TEST_ASSERT_NOT_NULL(handle);

    spi_set_callback(handle, prvSpiAsyncCallback, (void *)0x1234);

    uint8_t rxBuffer[10];
    int32_t result = spi_transfer_async(handle, NULL, rxBuffer, 10);
    TEST_ASSERT_EQUAL(-EINVAL, result);

    result = spi_close(handle);
    TEST_ASSERT_EQUAL(0, result);
}

TEST( TEST_IOT_SPI, test_spi_transfer_async_null_rx_buffer )
{
    SpiHandle_t handle = spi_open(0);
    TEST_ASSERT_NOT_NULL(handle);

    spi_set_callback(handle, prvSpiAsyncCallback, (void *)0x1234);

    uint8_t txBuffer[10] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A};
    int32_t result = spi_transfer_async(handle, txBuffer, NULL, 10);
    TEST_ASSERT_EQUAL(-EINVAL, result);

    result = spi_close(handle);
    TEST_ASSERT_EQUAL(0, result);
}

TEST( TEST_IOT_SPI, test_spi_transfer_async_zero_bytes )
{
    SpiHandle_t handle = spi_open(0);
    TEST_ASSERT_NOT_NULL(handle);

    spi_set_callback(handle, prvSpiAsyncCallback, (void *)0x1234);

    uint8_t txBuffer[10] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A};
    uint8_t rxBuffer[10];
    int32_t result = spi_transfer_async(handle, txBuffer, rxBuffer, 0);
    TEST_ASSERT_EQUAL(-EINVAL, result);

    result = spi_close(handle);
    TEST_ASSERT_EQUAL(0, result);
}

TEST( TEST_IOT_SPI, test_spi_transfer_async_busy_bus )
{
    SpiHandle_t handle = spi_open(0);
    TEST_ASSERT_NOT_NULL(handle);

    spi_set_callback(handle, prvSpiAsyncCallback, (void *)0x1234);

    uint8_t txBuffer[10] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A};
    uint8_t rxBuffer[10];
    int32_t result = spi_transfer_async(handle, txBuffer, rxBuffer, sizeof(txBuffer));
    TEST_ASSERT_EQUAL(0, result);

    result = spi_transfer_async(handle, txBuffer, rxBuffer, sizeof(txBuffer));
    TEST_ASSERT_EQUAL(-EBUSY, result);

    result = spi_close(handle);
    TEST_ASSERT_EQUAL(0, result);
}

/*-----------------------------------------------------------*/
