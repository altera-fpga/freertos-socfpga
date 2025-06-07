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

/* Standard includes */
#include <string.h>

/* Test includes. */
#include "unity_fixture.h"
#include "unity.h"

/* Driver model includes */
#include "FreeRTOS.h"
#include "FreeRTOSConfig.h"
#include "semphr.h"
#include "task.h"
#include "uart_test.h"

#include "uart_test_print.h"

/*-----------------------------------------------------------*/
/* Test Defines */
#define testIotUART_DEFAULT_SEMPAHORE_DELAY            ( 60 )                                                       /** Amount of time a function waits for every callback to finish. */
#define testIotUART_TEST_BAUD_RATE                     ( 38400 )                                                    /** Alternative baud rate to test. */
#define testIotUART_TEST_BAUD_RATE_DEFAULT             ( IOT_UART_BAUD_RATE_DEFAULT )                               /** Default baud rate. */
#define testIotUART_BAUD_RATE_STRING                   ( "Baudrate: 38400\n" )                                      /** Signal external device to change to different baudrate. */
#define testIotUART_BAUD_RATE_DEFAULT_STRING           ( "Baudrate: 115200\n" )                                     /** Signal external device to revert back to default baudrate. */
#define testIotUART_BAUD_RATE_BUFFER_TEST_LENGTH       ( sizeof( testIotUART_BAUD_RATE_STRING ) - 1 )
#define testIotUART_BAUD_RATE_BUFFER_DEFAULT_LENGTH    ( sizeof( testIotUART_BAUD_RATE_DEFAULT_STRING ) - 1 )
#define testIotUART_BUFFER_STRING                      ( "Hello" )
#define testIotUART_BUFFER_LENGTH                      ( sizeof( testIotUART_BUFFER_STRING ) - 1 )

#define testIotUART_BAUD_RATE_FAST                     ( 115200 )
#define testIotUART_BAUD_RATE_SLOW                     ( 9600 )
#define testIotUARTBUFFERSIZE                          ( 5 )
#define UART_LARGE_BUFF_SIZE                           ( 4000 )
#define testIotUART_BUFFER_LENGTH_LARGE                ( 200 )
#define testIotUART_DELAY                              ( 1000 )
#define testIotUART_7BIT_WORD_LENGTH                   ( 7 )
#define testIotUART_8BIT_WORD_LENGTH                   ( 8 )

#define TASK_PRIORITY (configMAX_PRIORITIES - 2)

#define UART_WRITE(str) uart_write_sync(xUartHandle, (uint8_t *)str, sizeof(str)-1)
#define TEST_LOOPBACK_STR ("SET LOOPBACK 5\n")
#define TEST_PASS_STR ("TEST PASSED\n")
#define TEST_FAIL_STR ("TEST FAILED\n")
#define TEST_BAUD_STR ("SET BAUD 38400\n")
#define TEST_BAUD_DEFAULT_STR ("SET BAUD 115200\n")
#define nITR (1)
/*-----------------------------------------------------------*/

/* Define Test Group. */
TEST_GROUP(TEST_IOT_UART);
TEST_GROUP(TEST_IOT_UART_DATA);
/*-----------------------------------------------------------*/
/*-----------------------------------------------------------*/

/*-----------------------------------------------------------*/

/* Globals values which can be overwritten by the test
 * framework invoking these tests */
/*-----------------------------------------------------------*/
uint8_t uctestIotUartPort = 0; /** Default UART port for testing */
//uint32_t ultestIotUartFlowControl = 0;
uint32_t ultestIotUartParity = 0;
uint32_t ultestIotUartWordLength = 0;
uint32_t ultestIotUartStopBits = 0;

uint8_t largeRxBuff[ UART_LARGE_BUFF_SIZE ] =
{ 0 };
uint8_t largeTxBuff[ UART_LARGE_BUFF_SIZE ] =
{ 0 };

uint8_t cpBuffer[ testIotUARTBUFFERSIZE ] =
{ 0 };
uint8_t cpBufferRead[ testIotUARTBUFFERSIZE ] =
{ 0 };

uint32_t txSize[ 10 ] =
{ 234, 890, 562, 22, 1, 292, 471, 553, 875, 100 };
uint32_t rxSize[ 10 ] =
{ 1, 476, 911, 711, 69, 420, 663, 576, 72, 101 };

/*-----------------------------------------------------------*/
/* Semaphore Handles */
/*-----------------------------------------------------------*/
SemaphoreHandle_t xReadCompleteSemaphore = NULL;
SemaphoreHandle_t xWriteCompleteSemaphore = NULL;
SemaphoreHandle_t xRxSyncSemaphore;
SemaphoreHandle_t xRxAsyncSemaphore;
SemaphoreHandle_t xRxBaudSemaphore;
SemaphoreHandle_t xRxSizeSemaphore;
SemaphoreHandle_t xCfgDoneSemaphore;
SemaphoreHandle_t xTxAssistSemaphore;

/* note:  config1 is different in each field from config2 */
static IotUARTConfig_t xSampleConfig1 =
{ .ulBaudrate = testIotUART_BAUD_RATE_FAST, .xParity = eUartParityNone,
        .xStopbits = eUartStopBitsOne, .ucWordlength =
        testIotUART_8BIT_WORD_LENGTH, };
static IotUARTConfig_t xSampleConfig2 =
{ .ulBaudrate = testIotUART_BAUD_RATE_SLOW, .xParity = eUartParityOdd,
        .xStopbits = eUartStopBitsTwo, .ucWordlength =
        testIotUART_8BIT_WORD_LENGTH, };

/**
 * @brief Application/POSIX defined callback for asynchronous write operation on
 * UART for testing the loopback function.
 * This callback function releases a semaphore every time it is called.
 */

void uart_test( void )
{
    UNITY_BEGIN();

    /*non-data transfer tests*/
    RUN_TEST_GROUP(TEST_IOT_UART);

    /*data transfer tests*/
    RUN_TEST_GROUP(TEST_IOT_UART_DATA);

    UNITY_END();
    dump_buffer();
}

static void prvReadWriteCallback( IotUARTOperationStatus_t xOpStatus,
        void *pvParams )
{
    /* Disable unused parameter warning. */
    (void) pvParams;

    if ( xOpStatus == eUartReadCompleted )
    {
        xSemaphoreGiveFromISR(xReadCompleteSemaphore, NULL);
    }
    else if ( xOpStatus == eUartWriteCompleted )
    {
        xSemaphoreGiveFromISR(xWriteCompleteSemaphore, NULL);
    }
}

/**
 * @brief Setup function called before each test in this group is executed.
 */
TEST_SETUP( TEST_IOT_UART )
{
    /* xSemaphoreCreateBinaryStatic succeeds as long as buffer parameter is not NULL. */
    xReadCompleteSemaphore = xSemaphoreCreateBinary();
    xWriteCompleteSemaphore = xSemaphoreCreateBinary();
}

TEST_SETUP( TEST_IOT_UART_DATA )
{
    /* xSemaphoreCreateBinaryStatic succeeds as long as buffer parameter is not NULL. */
}

/*-----------------------------------------------------------*/

/**
 * @brief Tear down function called after each test in this group is executed.
 */
TEST_TEAR_DOWN( TEST_IOT_UART )
{
}

TEST_TEAR_DOWN( TEST_IOT_UART_DATA )
{
}

/*-----------------------------------------------------------*/

/**
 * @brief Function to define which tests to execute as part of this group.
 */
TEST_GROUP_RUNNER( TEST_IOT_UART )
{
    RUN_TEST_CASE(TEST_IOT_UART, AFQP_IotUARTIoctlFuzzing);
    RUN_TEST_CASE(TEST_IOT_UART, AFQP_IotUARTReadSyncFuzzing);
    RUN_TEST_CASE(TEST_IOT_UART, AFQP_IotUARTReadAsyncFuzzing);
    RUN_TEST_CASE(TEST_IOT_UART, AFQP_IotUARTWriteSyncFuzzing);
    RUN_TEST_CASE(TEST_IOT_UART, AFQP_IotUARTWriteAsyncFuzzing);
    RUN_TEST_CASE(TEST_IOT_UART, AFQP_IotUARTIoctlWhenBusy);
    RUN_TEST_CASE(TEST_IOT_UART, AFQP_IotUARTIoctlGetSet);
    RUN_TEST_CASE(TEST_IOT_UART, AFQP_IotUARTOpenCloseCancelFuzzing);
}

TEST_GROUP_RUNNER(TEST_IOT_UART_DATA)
{
    RUN_TEST_CASE(TEST_IOT_UART_DATA, AFQP_AssistedIotUARTWriteReadSync);
    RUN_TEST_CASE(TEST_IOT_UART_DATA, AFQP_AssistedIotUARTBaudChange);
    RUN_TEST_CASE(TEST_IOT_UART_DATA, AFQP_IotUARTWriteReadAsyncWithCallback);
    RUN_TEST_CASE(TEST_IOT_UART_DATA, UARTWriteSyncReadAsync);
    RUN_TEST_CASE(TEST_IOT_UART_DATA, UARTWriteReadDiffSize);
}
/*-----------------------------------------------------------*/

uint8_t *pucRxPtr;

void rx_sync_thread( void *const pvParam )
{
    uint32_t lRead, i = 0;
    IotUARTHandle_t xUartHandle;

    xUartHandle = (IotUARTHandle_t) pvParam;

    for ( i = 0; i < nITR; i++ )
    {
        lRead = iot_uart_read_sync(xUartHandle, cpBufferRead,
        testIotUART_BUFFER_LENGTH);
        TEST_ASSERT_EQUAL(0, lRead);

        xSemaphoreGive(xRxSyncSemaphore);
    }
    vTaskDelete(NULL);
}

void rx_async_thread( void *const param )
{
    IotUARTHandle_t xUartHandle;
    uint32_t lRead, i = 0;

    xUartHandle = (IotUARTHandle_t) param;

    for ( i = 0; i < nITR; i++ )
    {
        lRead = iot_uart_read_async(xUartHandle, cpBufferRead,
        testIotUART_BUFFER_LENGTH);
        xSemaphoreTake(xReadCompleteSemaphore, portMAX_DELAY);
        TEST_ASSERT_EQUAL(0, lRead);

        xSemaphoreGive(xRxAsyncSemaphore);
    }
    vTaskDelete(NULL);
}

void rx_baud_thread( void *const param )
{
    IotUARTHandle_t xUartHandle;
    uint32_t lRead, i = 0;

    xUartHandle = (IotUARTHandle_t) param;

    for ( i = 0; i < 6; i++ )
    {
        if ( xSemaphoreTake(xCfgDoneSemaphore, portMAX_DELAY) )
        {
            lRead = iot_uart_read_sync(xUartHandle, cpBufferRead,
            testIotUART_BUFFER_LENGTH);
            TEST_ASSERT_EQUAL(0, lRead);

            xSemaphoreGive(xRxBaudSemaphore);
        }
    }
    vTaskDelete(NULL);
}

void rx_size_thread( void *const param )
{
    IotUARTHandle_t xUartHandle;
    uint32_t lRead, i = 0;

    xUartHandle = (IotUARTHandle_t) param;

    for ( i = 0; i < 10; i++ )
    {
        lRead = iot_uart_read_sync(xUartHandle, largeRxBuff, rxSize[ i ]);
        TEST_ASSERT_EQUAL(0, lRead);
    }
    xSemaphoreGive(xRxSizeSemaphore);

    vTaskDelete(NULL);
}

/*-----------------------------------------------------------*/

TEST(TEST_IOT_UART_DATA, UARTWriteReadDiffSize)
{
    int32_t i = 0, lWrite, lClose;
    BaseType_t xTakeReturn;
    IotUARTHandle_t xUartHandle;
    uint32_t ulStrCmp = 1;

    xUartHandle = iot_uart_open(uctestIotUartPort);
    TEST_ASSERT_NOT_EQUAL(NULL, xUartHandle);

    xTaskCreate(rx_size_thread, "RxSizeThread",
    configMINIMAL_STACK_SIZE, (void* const ) xUartHandle, TASK_PRIORITY,
    NULL);
    xRxSizeSemaphore = xSemaphoreCreateBinary();

    memset(largeTxBuff, 7, 4000);

    if ( TEST_PROTECT() )
    {
        iot_uart_ioctl(xUartHandle, eUartSetConfig, &xSampleConfig1);

        UART_WRITE("SET LOOPBACK 4000\n");

        for ( i = 0; i < 10; i++ )
        {
            lWrite = iot_uart_write_sync(xUartHandle, largeTxBuff, txSize[ i ]);
            TEST_ASSERT_EQUAL(0, lWrite);
        }
        xTakeReturn = xSemaphoreTake(xRxSizeSemaphore, portMAX_DELAY);
    }

    ulStrCmp = strncmp((char*) largeTxBuff, (char*) largeRxBuff,
    UART_LARGE_BUFF_SIZE);

    if ( ulStrCmp == 0 || xTakeReturn == 1 )
    {
        UART_WRITE(TEST_PASS_STR);
    }
    else if ( ulStrCmp == 1 || xTakeReturn == 0 )
    {
        UART_WRITE(TEST_FAIL_STR);
    }

    lClose = iot_uart_close(xUartHandle);
    TEST_ASSERT_EQUAL(0, lClose);
}

TEST(TEST_IOT_UART_DATA, UARTWriteSyncReadAsync)
{
    int32_t i = 0, lWrite, lClose;
    BaseType_t xTakeReturn;
    IotUARTHandle_t xUartHandle;
    uint32_t ulStrCmp = 1;

    xUartHandle = iot_uart_open(uctestIotUartPort);
    TEST_ASSERT_NOT_EQUAL(NULL, xUartHandle);

    iot_uart_set_callback(xUartHandle, prvReadWriteCallback, NULL);

    xTaskCreate(rx_async_thread, "AsyncRxThread",
    configMINIMAL_STACK_SIZE, (void* const ) xUartHandle, TASK_PRIORITY,
    NULL);
    xRxAsyncSemaphore = xSemaphoreCreateBinary();

    memcpy(cpBuffer, testIotUART_BUFFER_STRING, testIotUART_BUFFER_LENGTH);

    if ( TEST_PROTECT() )
    {
        iot_uart_ioctl(xUartHandle, eUartSetConfig, &xSampleConfig1);

        UART_WRITE(TEST_LOOPBACK_STR);

        for ( i = 0; i < nITR; i++ )
        {
            lWrite = iot_uart_write_sync(xUartHandle, cpBuffer,
            testIotUART_BUFFER_LENGTH);
            TEST_ASSERT_EQUAL(0, lWrite);

            xTakeReturn = xSemaphoreTake(xRxAsyncSemaphore, portMAX_DELAY);
        }
    }

    ulStrCmp = strncmp((char*) cpBuffer, (char*) cpBufferRead,
    testIotUART_BUFFER_LENGTH);

    if ( ulStrCmp == 0 || xTakeReturn == 1 )
    {
        UART_WRITE(TEST_PASS_STR);
    }
    else if ( ulStrCmp == 1 || xTakeReturn == 0 )
    {
        UART_WRITE(TEST_FAIL_STR);
    }

    lClose = iot_uart_close(xUartHandle);
    TEST_ASSERT_EQUAL(0, lClose);
}

/**
 * hardware loopback. The Tx and Rx pin on the vendor board are shorted with a
 * connector and the test is run by transmitting some bytes on write sync and checking
 * if same characters are read synchronously.
 */

TEST( TEST_IOT_UART_DATA, AFQP_AssistedIotUARTWriteReadSync )
{
    int32_t i = 0, lWrite, lClose;
    BaseType_t xTakeReturn;
    IotUARTHandle_t xUartHandle;
    uint32_t ulStrCmp = 1;

    xUartHandle = iot_uart_open(uctestIotUartPort);
    TEST_ASSERT_NOT_EQUAL(NULL, xUartHandle);

    xTaskCreate(rx_sync_thread, "TestRxThread",
    configMINIMAL_STACK_SIZE, (void* const ) xUartHandle, TASK_PRIORITY,
    NULL);
    xRxSyncSemaphore = xSemaphoreCreateBinary();

    memcpy(cpBuffer, testIotUART_BUFFER_STRING, testIotUART_BUFFER_LENGTH);

    if ( TEST_PROTECT() )
    {
        iot_uart_ioctl(xUartHandle, eUartSetConfig, &xSampleConfig1);

        UART_WRITE(TEST_LOOPBACK_STR);

        for ( i = 0; i < nITR; i++ )
        {
            lWrite = iot_uart_write_sync(xUartHandle, cpBuffer,
            testIotUART_BUFFER_LENGTH);
            TEST_ASSERT_EQUAL(0, lWrite);

            xTakeReturn = xSemaphoreTake(xRxSyncSemaphore, portMAX_DELAY);
        }
    }

    ulStrCmp = strncmp((char*) cpBuffer, (char*) cpBufferRead,
    testIotUART_BUFFER_LENGTH);

    if ( ulStrCmp == 0 || xTakeReturn == 1 )
    {
        UART_WRITE(TEST_PASS_STR);
    }
    else if ( ulStrCmp == 1 || xTakeReturn == 0 )
    {
        UART_WRITE(TEST_FAIL_STR);
    }

    lClose = iot_uart_close(xUartHandle);
    TEST_ASSERT_EQUAL(0, lClose);
}
/*-----------------------------------------------------------*/
/*-----------------------------------------------------------*/

/**
 * @brief Test Function to check UART Ioctl API specifications.
 */
TEST( TEST_IOT_UART, AFQP_IotUARTIoctlGetSet )
{
    IotUARTHandle_t xUartHandle;
    int32_t lIoctl, lClose;
    uint16_t ulTransferAmount;
    IotUARTConfig_t xgetConfigBuffer, xConfigBuffer, xOriginalConfig;

    xUartHandle = iot_uart_open(uctestIotUartPort);
    TEST_ASSERT_NOT_EQUAL(NULL, xUartHandle);

    if ( TEST_PROTECT() )
    {
        /* Save original UART config */
        lIoctl = iot_uart_ioctl(xUartHandle, eUartGetConfig, &xOriginalConfig);
        TEST_ASSERT_EQUAL(0, lIoctl);

        /* Initialize to a known value, then one by one change, and make sure it changes */
        memcpy(&xConfigBuffer, &xSampleConfig1, sizeof( IotUARTConfig_t ));
        lIoctl = iot_uart_ioctl(xUartHandle, eUartSetConfig, &xConfigBuffer);
        TEST_ASSERT_EQUAL(0, lIoctl);

        /* Test changing baudrate config */
        xConfigBuffer.ulBaudrate = xSampleConfig2.ulBaudrate;
        lIoctl = iot_uart_ioctl(xUartHandle, eUartSetConfig, &xConfigBuffer);
        TEST_ASSERT_EQUAL(0, lIoctl);

        lIoctl = iot_uart_ioctl(xUartHandle, eUartGetConfig, &xgetConfigBuffer);
        TEST_ASSERT_EQUAL(0, lIoctl);

        if ( xgetConfigBuffer.ulBaudrate <= testIotUART_BAUD_RATE_SLOW + 100
                || xgetConfigBuffer.ulBaudrate
                        >= testIotUART_BAUD_RATE_SLOW - 100 )
        {
            xgetConfigBuffer.ulBaudrate = testIotUART_BAUD_RATE_SLOW;
        }

        TEST_ASSERT_EQUAL(xSampleConfig2.ulBaudrate,
                xgetConfigBuffer.ulBaudrate);

        /* Test changing control flow config if specified by the test framework
         if ( ultestIotUartFlowControl )
         {
         xConfigBuffer.ucFlowControl = xSampleConfig2.ucFlowControl;
         lIoctl = iot_uart_ioctl(xUartHandle, eUartSetConfig,
         &xConfigBuffer);
         TEST_ASSERT_EQUAL(0, lIoctl);

         lIoctl = iot_uart_ioctl(xUartHandle, eUartGetConfig,
         &xgetConfigBuffer);
         TEST_ASSERT_EQUAL(0, lIoctl);
         TEST_ASSERT_EQUAL(xSampleConfig2.ucFlowControl,
         xgetConfigBuffer.ucFlowControl);
         }
         */

        /* Test changing stop bit config */
        xConfigBuffer.xStopbits = xSampleConfig2.xStopbits;
        lIoctl = iot_uart_ioctl(xUartHandle, eUartSetConfig, &xConfigBuffer);
        TEST_ASSERT_EQUAL(0, lIoctl);

        lIoctl = iot_uart_ioctl(xUartHandle, eUartGetConfig, &xgetConfigBuffer);
        TEST_ASSERT_EQUAL(0, lIoctl);
        TEST_ASSERT_EQUAL(xSampleConfig2.xStopbits, xgetConfigBuffer.xStopbits);

        /* Test changing parity config */
        xConfigBuffer.xParity = xSampleConfig2.xParity;
        lIoctl = iot_uart_ioctl(xUartHandle, eUartSetConfig, &xConfigBuffer);
        TEST_ASSERT_EQUAL(0, lIoctl);

        lIoctl = iot_uart_ioctl(xUartHandle, eUartGetConfig, &xgetConfigBuffer);
        TEST_ASSERT_EQUAL(0, lIoctl);
        TEST_ASSERT_EQUAL(xSampleConfig2.xParity, xgetConfigBuffer.xParity);

        /* Test changing Wordlength config */
        xConfigBuffer.ucWordlength = xSampleConfig2.ucWordlength;
        lIoctl = iot_uart_ioctl(xUartHandle, eUartSetConfig, &xConfigBuffer);
        TEST_ASSERT_EQUAL(0, lIoctl);

        lIoctl = iot_uart_ioctl(xUartHandle, eUartGetConfig, &xgetConfigBuffer);
        TEST_ASSERT_EQUAL(0, lIoctl);
        TEST_ASSERT_EQUAL(xSampleConfig2.ucWordlength,
                xgetConfigBuffer.ucWordlength);
        if ( xOriginalConfig.ulBaudrate != 0 )
        {
            /* Set config back to original */
            lIoctl = iot_uart_ioctl(xUartHandle, eUartSetConfig,
                    &xOriginalConfig);
            TEST_ASSERT_EQUAL(0, lIoctl);
        }

        /* Getting transfer byte amount only works on an operation in flight. */
        lIoctl = iot_uart_ioctl(xUartHandle, eGetTxNoOfbytes, &ulTransferAmount);
        TEST_ASSERT_EQUAL(0, lIoctl);

        lIoctl = iot_uart_ioctl(xUartHandle, eGetRxNoOfbytes, &ulTransferAmount);
        TEST_ASSERT_EQUAL(0, lIoctl);
    }

    lClose = iot_uart_close(xUartHandle);
    TEST_ASSERT_EQUAL(0, lClose);
}
/*-----------------------------------------------------------*/

/**
 * @brief Test Function to check if UART can be configured to a different baudrate.
 *  Works by signal external device to change to a new baudrate. Sending and receiving a message.
 *  Then signals to return back to default baudrate.
 *-----------------------------------------------------------*/
TEST( TEST_IOT_UART_DATA, AFQP_AssistedIotUARTBaudChange )
{
    IotUARTHandle_t xUartHandle;
    int32_t lIoctl, lWrite, lClose, i;
    BaseType_t xRxReturn;
    uint32_t ulStrCmp = 1;

    xCfgDoneSemaphore = xSemaphoreCreateBinary();
    xRxBaudSemaphore = xSemaphoreCreateBinary();

    xUartHandle = iot_uart_open(uctestIotUartPort);
    TEST_ASSERT_NOT_EQUAL(NULL, xUartHandle);

    iot_uart_ioctl(xUartHandle, eUartSetConfig, &xSampleConfig1);

    xTaskCreate(rx_baud_thread, "TestRxThread",
    configMINIMAL_STACK_SIZE, (void* const ) xUartHandle, TASK_PRIORITY,
    NULL);

    if ( TEST_PROTECT() )
    {
        for ( i = 0; i < 3; i++ )
        {
            /* Signal other device to change baudrate to 38400 */
            UART_WRITE(TEST_BAUD_STR);

            IotUARTConfig_t pvConfigBuffer;

            lIoctl = iot_uart_ioctl(xUartHandle, eUartGetConfig,
                    &pvConfigBuffer);
            TEST_ASSERT_EQUAL(0, lIoctl);

            pvConfigBuffer.ulBaudrate = testIotUART_TEST_BAUD_RATE;

            /* Change baud rate to 38400 */
            lIoctl = iot_uart_ioctl(xUartHandle, eUartSetConfig,
                    &pvConfigBuffer);
            TEST_ASSERT_EQUAL(0, lIoctl);
            xSemaphoreGive(xCfgDoneSemaphore);

            /* Wait for other UART to change baudrate. */
            vTaskDelay(pdMS_TO_TICKS(testIotUART_DELAY));

            memcpy(cpBuffer, testIotUART_BUFFER_STRING,
            testIotUART_BUFFER_LENGTH);

            UART_WRITE(TEST_LOOPBACK_STR);

            lWrite = iot_uart_write_sync(xUartHandle, cpBuffer,
            testIotUART_BUFFER_LENGTH);
            TEST_ASSERT_EQUAL(0, lWrite);

            xRxReturn = xSemaphoreTake(xRxBaudSemaphore, portMAX_DELAY);

            /* Baudrate reverted successfully */

            ulStrCmp = strncmp((char*) cpBuffer, (char*) cpBufferRead,
            testIotUART_BUFFER_LENGTH);

            if ( ulStrCmp == 0 || xRxReturn == 1 )
            {
                UART_WRITE(TEST_PASS_STR);
            }
            else if ( ulStrCmp == 1 || xRxReturn == 0 )
            {
                UART_WRITE(TEST_FAIL_STR);
            }

            /* Signal other device for baud: 115200 */
            UART_WRITE(TEST_BAUD_DEFAULT_STR);

            pvConfigBuffer.ulBaudrate = testIotUART_BAUD_RATE_FAST;

            /* Configure baudrate 115200*/
            lIoctl = iot_uart_ioctl(xUartHandle, eUartSetConfig,
                    &pvConfigBuffer);
            TEST_ASSERT_EQUAL(0, lIoctl);
            xSemaphoreGive(xCfgDoneSemaphore);

            /* Wait for other UART to change baudrate. */
            vTaskDelay(pdMS_TO_TICKS(testIotUART_DELAY));

            memcpy(cpBuffer, testIotUART_BUFFER_STRING,
            testIotUART_BUFFER_LENGTH);

            UART_WRITE(TEST_LOOPBACK_STR);

            lWrite = iot_uart_write_sync(xUartHandle, cpBuffer,
            testIotUART_BUFFER_LENGTH);
            TEST_ASSERT_EQUAL(0, lWrite);

            xRxReturn = xSemaphoreTake(xRxBaudSemaphore, portMAX_DELAY);

            ulStrCmp = strncmp((char*) cpBuffer, (char*) cpBufferRead,
            testIotUART_BUFFER_LENGTH);

            if ( ulStrCmp == 0 || xRxReturn == 1 )
            {
                UART_WRITE(TEST_PASS_STR);
            }
            else if ( ulStrCmp == 1 || xRxReturn == 0 )
            {
                UART_WRITE(TEST_FAIL_STR);
            }
        }
    }

    lClose = iot_uart_close(xUartHandle);
    TEST_ASSERT_EQUAL(0, lClose);
}
/*-----------------------------------------------------------*/

/*-----------------------------------------------------------*/

/**
 * @brief Test function to test the asynchronous read/write with UART by doing a
 * hardware loopback. The Tx and Rx pin on the vendor board are shorted with a
 * connector and the test is run by transmitting some bytes on write async and checking
 * if same characters are read asynchronously.
 * This test also checks if we are getting Tx bytes in flight.
 */

TEST( TEST_IOT_UART, AFQP_IotUARTWriteAsyncReadAsyncLoopbackTest )
{
    IotUARTHandle_t xUartHandle;
    int32_t lIoctl, lWrite, lClose, lRead;
    uint16_t ulTransferAmount;

    uint8_t cpBuffer[ testIotUART_BUFFER_LENGTH + 1 ] =
    { 0 };
    uint8_t cpBufferRead[ testIotUART_BUFFER_LENGTH + 1 ] =
    { 0 };
    uint8_t uStringCompare = 1;
    BaseType_t xCallbackReturn;

    memcpy(cpBuffer, testIotUART_BUFFER_STRING, testIotUART_BUFFER_LENGTH);

    xUartHandle = iot_uart_open(uctestIotUartPort);
    TEST_ASSERT_NOT_EQUAL(NULL, xUartHandle);

    if ( TEST_PROTECT() )
    {
        iot_uart_set_callback(xUartHandle, prvReadWriteCallback, NULL);

        lRead = iot_uart_read_async(xUartHandle, cpBufferRead,
        testIotUART_BUFFER_LENGTH);
        TEST_ASSERT_EQUAL(0, lRead);

        lWrite = iot_uart_write_async(xUartHandle, cpBuffer,
        testIotUART_BUFFER_LENGTH);
        TEST_ASSERT_EQUAL(0, lWrite);

        xCallbackReturn = xSemaphoreTake(xWriteCompleteSemaphore,
                testIotUART_DEFAULT_SEMPAHORE_DELAY);
        TEST_ASSERT_EQUAL(pdTRUE, xCallbackReturn);

        xCallbackReturn = xSemaphoreTake(xReadCompleteSemaphore,
                testIotUART_DEFAULT_SEMPAHORE_DELAY);
        TEST_ASSERT_EQUAL(pdTRUE, xCallbackReturn);

        lIoctl = iot_uart_ioctl(xUartHandle, eGetTxNoOfbytes, &ulTransferAmount);
        TEST_ASSERT_EQUAL(0, lIoctl);
        TEST_ASSERT_GREATER_THAN(0, ulTransferAmount);

        uStringCompare = memcmp(cpBuffer, cpBufferRead,
        testIotUART_BUFFER_LENGTH);
        TEST_ASSERT_EQUAL(0, uStringCompare);
    }

    lClose = iot_uart_close(xUartHandle);
    TEST_ASSERT_EQUAL(0, lClose);
}
/*-----------------------------------------------------------*/
/*-----------------------------------------------------------*/

/**
 * This is Assisted Test function to test the asynchronous read/write with UART by doing a
 * loopback with RPI3. The Tx and Rx pin on the vendor board are connected to RX and TX of
 * RPI3. Test is run by transmitting some bytes on write async and checking
 * if same characters are read asynchronously on RPI3. Validation happens on RPI3
 * This test also checks if we are getting Tx bytes in flight.
 */
TEST( TEST_IOT_UART, AFQP_AssistedIotUARTWriteAsync )
{
    IotUARTHandle_t xUartHandle;
    int32_t lIoctl, lWrite, lClose;
    uint8_t i;
    uint16_t ulTransferAmount_1, ulTransferAmount_2;
    uint8_t cpBufferLarge[ testIotUART_BUFFER_LENGTH_LARGE ] =
    { 0 };

    for ( i = 0; i < testIotUART_BUFFER_LENGTH_LARGE; i++ )
    {
        cpBufferLarge[ i ] = 0xAA;
    }

    cpBufferLarge[ testIotUART_BUFFER_LENGTH_LARGE - 1 ] = '\n';

    xUartHandle = iot_uart_open(uctestIotUartPort);
    TEST_ASSERT_NOT_EQUAL(NULL, xUartHandle);

    if ( TEST_PROTECT() )
    {
        lWrite = iot_uart_write_async(xUartHandle, cpBufferLarge,
        testIotUART_BUFFER_LENGTH_LARGE);
        TEST_ASSERT_EQUAL(0, lWrite);

        lIoctl = iot_uart_ioctl(xUartHandle, eGetTxNoOfbytes,
                &ulTransferAmount_1);
        TEST_ASSERT_EQUAL(0, lIoctl);
        TEST_ASSERT_GREATER_THAN(0, ulTransferAmount_1);

        /* Delay for 1 sec. */
        vTaskDelay( testIotUART_DELAY);

        lIoctl = iot_uart_ioctl(xUartHandle, eGetTxNoOfbytes,
                &ulTransferAmount_2);
        TEST_ASSERT_EQUAL(0, lIoctl);
        TEST_ASSERT_GREATER_THAN(ulTransferAmount_1, ulTransferAmount_2);

        /* Delay for 1 sec. */
        vTaskDelay( testIotUART_DELAY);
    }

    lClose = iot_uart_close(xUartHandle);
    TEST_ASSERT_EQUAL(0, lClose);
}
/*-----------------------------------------------------------*/

/*-----------------------------------------------------------*/

/**
 * @brief Test function to test the asynchronous read with UART by doing a
 * hardware loopback. The Tx and Rx pin on the vendor board are shorted with a
 * connector and the test is run by transmitting some bytes on write async and checking
 * if same characters are read asynchronously.
 */
TEST( TEST_IOT_UART_DATA, AFQP_IotUARTWriteReadAsyncWithCallback )
{
    int32_t i = 0, lWrite, lClose;
    BaseType_t xTakeReturn;
    IotUARTHandle_t xUartHandle;
    uint32_t ulStrCmp = 1;

    xUartHandle = iot_uart_open(uctestIotUartPort);
    TEST_ASSERT_NOT_EQUAL(NULL, xUartHandle);

    iot_uart_ioctl(xUartHandle, eUartSetConfig, &xSampleConfig1);

    iot_uart_set_callback(xUartHandle, prvReadWriteCallback, NULL);

    xRxAsyncSemaphore = xSemaphoreCreateBinary();

    memcpy(cpBuffer, testIotUART_BUFFER_STRING, testIotUART_BUFFER_LENGTH);

    xTaskCreate(rx_async_thread, "TestRxThread",
    configMINIMAL_STACK_SIZE, (void* const ) xUartHandle, TASK_PRIORITY,
    NULL);

    if ( TEST_PROTECT() )
    {
        UART_WRITE(TEST_LOOPBACK_STR);

        for ( i = 0; i < nITR; i++ )
        {
            lWrite = iot_uart_write_async(xUartHandle, cpBuffer,
            testIotUART_BUFFER_LENGTH);
            TEST_ASSERT_EQUAL(0, lWrite);

            xSemaphoreTake(xWriteCompleteSemaphore, portMAX_DELAY);

            xTakeReturn = xSemaphoreTake(xRxAsyncSemaphore, portMAX_DELAY);
        }
    }

    ulStrCmp = strncmp((char*) cpBuffer, (char*) cpBufferRead,
    testIotUART_BUFFER_LENGTH);

    if ( ulStrCmp == 0 || xTakeReturn == 1 )
    {
        UART_WRITE(TEST_PASS_STR);
    }
    else if ( ulStrCmp == 1 || xTakeReturn == 0 )
    {
        UART_WRITE(TEST_FAIL_STR);
    }

    lClose = iot_uart_close(xUartHandle);
    TEST_ASSERT_EQUAL(0, lClose);
}
/*-----------------------------------------------------------*/

/*-----------------------------------------------------------*/

/**
 * @brief Test Function to test iot_uart_cancel
 *
 *-----------------------------------------------------------*/
TEST(TEST_IOT_UART, AFQP_IotUARTCancel)
{
    IotUARTHandle_t xUartHandle;
    int32_t lWrite, lClose, lCancel;
    BaseType_t xCallbackReturn;

    uint8_t cSmallBuf[ 2 ] =
    { 'H', 'I' };
    uint8_t cpBuffer[ testIotUARTBUFFERSIZE ] =
    { 0 };
    uint8_t uSmallBuflen = sizeof(cSmallBuf) / sizeof(uint8_t);

    memcpy(cpBuffer, testIotUART_BUFFER_STRING, testIotUART_BUFFER_LENGTH);

    xUartHandle = iot_uart_open(uctestIotUartPort);
    TEST_ASSERT_NOT_EQUAL(NULL, xUartHandle);

    if ( TEST_PROTECT() )
    {
        iot_uart_set_callback(xUartHandle, prvReadWriteCallback, NULL);

        lWrite = iot_uart_write_async(xUartHandle, cpBuffer,
        testIotUART_BUFFER_LENGTH);
        TEST_ASSERT_EQUAL(0, lWrite);

        lCancel = iot_uart_cancel(xUartHandle);
        TEST_ASSERT_EQUAL(0, lCancel);

        /* Wait to make sure operation was really canceled. */
        xCallbackReturn = xSemaphoreTake(xWriteCompleteSemaphore,
                testIotUART_DEFAULT_SEMPAHORE_DELAY);
        TEST_ASSERT_EQUAL(pdFALSE, xCallbackReturn);

        lWrite = iot_uart_write_async(xUartHandle, cSmallBuf, uSmallBuflen);
        TEST_ASSERT_EQUAL(0, lWrite);

        /* Wait to make sure operation has completed. */
        xCallbackReturn = xSemaphoreTake(xWriteCompleteSemaphore,
                testIotUART_DEFAULT_SEMPAHORE_DELAY);
        TEST_ASSERT_EQUAL(pdTRUE, xCallbackReturn);

        /* Since  cSmallBuf is small buffer, write is already complete. Hence nothing to cancel */
        lCancel = iot_uart_cancel(xUartHandle);
        TEST_ASSERT_EQUAL(-ENOSYS, lCancel);
    }

    lClose = iot_uart_close(xUartHandle);
    TEST_ASSERT_EQUAL(0, lClose);
}
/*-----------------------------------------------------------*/

/*-----------------------------------------------------------*/

/**
 * @brief Test Function to fuzz iot_uart_write_async
 *-----------------------------------------------------------*/
TEST(TEST_IOT_UART, AFQP_IotUARTWriteAsyncFuzzing)
{
    IotUARTHandle_t xUartHandle;
    int32_t lClose, lWrite;

    uint8_t cpBuffer[ testIotUARTBUFFERSIZE ] =
    { 0 };

    memcpy(cpBuffer, testIotUART_BUFFER_STRING, testIotUART_BUFFER_LENGTH);

    lWrite = iot_uart_write_async(NULL, cpBuffer,
    testIotUART_BUFFER_LENGTH);
    TEST_ASSERT_EQUAL(-EINVAL, lWrite);

    xUartHandle = iot_uart_open(uctestIotUartPort);
    TEST_ASSERT_NOT_EQUAL(NULL, xUartHandle);

    if ( TEST_PROTECT() )
    {
        lWrite = iot_uart_write_async(xUartHandle, NULL,
        testIotUART_BUFFER_LENGTH);
        TEST_ASSERT_EQUAL(-EINVAL, lWrite);

        lWrite = iot_uart_write_async(xUartHandle, cpBuffer, 0);
        TEST_ASSERT_EQUAL(-EINVAL, lWrite);
    }

    lClose = iot_uart_close(xUartHandle);
    TEST_ASSERT_EQUAL(0, lClose);
}
/*-----------------------------------------------------------*/

/*-----------------------------------------------------------*/

/**
 * @brief Test Function to fuzz iot_uart_write_sync
 *-----------------------------------------------------------*/
TEST(TEST_IOT_UART, AFQP_IotUARTWriteSyncFuzzing)
{
    IotUARTHandle_t xUartHandle;
    int32_t lClose, lWrite;

    uint8_t cpBuffer[ testIotUARTBUFFERSIZE ] =
    { 0 };

    memcpy(cpBuffer, testIotUART_BUFFER_STRING, testIotUART_BUFFER_LENGTH);

    lWrite = iot_uart_write_sync(NULL, cpBuffer, testIotUART_BUFFER_LENGTH);
    TEST_ASSERT_EQUAL(-EINVAL, lWrite);

    xUartHandle = iot_uart_open(uctestIotUartPort);
    TEST_ASSERT_NOT_EQUAL(NULL, xUartHandle);

    if ( TEST_PROTECT() )
    {
        lWrite = iot_uart_write_sync(xUartHandle, NULL,
        testIotUART_BUFFER_LENGTH);
        TEST_ASSERT_EQUAL(-EINVAL, lWrite);

        lWrite = iot_uart_write_sync(xUartHandle, cpBuffer, 0);
        TEST_ASSERT_EQUAL(-EINVAL, lWrite);
    }

    lClose = iot_uart_close(xUartHandle);
    TEST_ASSERT_EQUAL(0, lClose);
}
/*-----------------------------------------------------------*/

/**
 * @brief Test Function to fuzz iot_uart_read_async
 *-----------------------------------------------------------*/
TEST(TEST_IOT_UART, AFQP_IotUARTReadAsyncFuzzing)
{
    IotUARTHandle_t xUartHandle;
    int32_t lClose, lRead;

    uint8_t cpBufferRead[ testIotUARTBUFFERSIZE ] =
    { 0 };

    lRead = iot_uart_read_async(NULL, cpBufferRead,
    testIotUART_BUFFER_LENGTH);
    TEST_ASSERT_EQUAL(-EINVAL, lRead);

    xUartHandle = iot_uart_open(uctestIotUartPort);
    TEST_ASSERT_NOT_EQUAL(NULL, xUartHandle);

    if ( TEST_PROTECT() )
    {
        lRead = iot_uart_read_async(xUartHandle, NULL,
        testIotUART_BUFFER_LENGTH);
        TEST_ASSERT_EQUAL(-EINVAL, lRead);

        lRead = iot_uart_read_async(xUartHandle, cpBufferRead, 0);
        TEST_ASSERT_EQUAL(-EINVAL, lRead);
    }

    lClose = iot_uart_close(xUartHandle);
    TEST_ASSERT_EQUAL(0, lClose);
}
/*-----------------------------------------------------------*/

/*-----------------------------------------------------------*/

/**
 * @brief Test Function to fuzz iot_uart_read_sync
 *-----------------------------------------------------------*/
TEST(TEST_IOT_UART, AFQP_IotUARTReadSyncFuzzing)
{
    IotUARTHandle_t xUartHandle;
    int32_t lClose, lRead;

    uint8_t cpBufferRead[ testIotUARTBUFFERSIZE ] =
    { 0 };

    lRead = iot_uart_read_sync(NULL, cpBufferRead, testIotUART_BUFFER_LENGTH);
    TEST_ASSERT_EQUAL(-EINVAL, lRead);

    xUartHandle = iot_uart_open(uctestIotUartPort);
    TEST_ASSERT_NOT_EQUAL(NULL, xUartHandle);

    if ( TEST_PROTECT() )
    {
        lRead = iot_uart_read_sync(xUartHandle, NULL,
        testIotUART_BUFFER_LENGTH);
        TEST_ASSERT_EQUAL(-EINVAL, lRead);

        lRead = iot_uart_read_sync(xUartHandle, cpBufferRead, 0);
        TEST_ASSERT_EQUAL(-EINVAL, lRead);
    }

    lClose = iot_uart_close(xUartHandle);
    TEST_ASSERT_EQUAL(0, lClose);
}
/*-----------------------------------------------------------*/

/*-----------------------------------------------------------*/

/* @brief Test Function to fuzz iot_uart_ioctl
 *-----------------------------------------------------------*/
TEST(TEST_IOT_UART, AFQP_IotUARTIoctlFuzzing)
{
    IotUARTHandle_t xUartHandle;
    int32_t lIoctl, lClose;
    IotUARTConfig_t xUartConfigTest;

    /* Call iot_uart_ioctl with NULL handle. Expect -EINVAL */
    lIoctl = iot_uart_ioctl(NULL, eUartSetConfig, &xUartConfigTest);
    TEST_ASSERT_EQUAL(-EINVAL, lIoctl);

    xUartHandle = iot_uart_open(uctestIotUartPort);
    TEST_ASSERT_NOT_EQUAL(NULL, xUartHandle);

    if ( TEST_PROTECT() )
    {
        /* Call iot_uart_ioctl with valid handle, but with invalid enum (-1) request.Expect -EINVAL */
        lIoctl = iot_uart_ioctl(xUartHandle, -1, &xUartConfigTest);
        TEST_ASSERT_EQUAL(-EINVAL, lIoctl);

        /* Call iot_uart_ioctl with enum eUartSetConfig, and NULL buffer.Expect -EINVAL */
        lIoctl = iot_uart_ioctl(xUartHandle, eUartSetConfig, NULL);
        TEST_ASSERT_EQUAL(-EINVAL, lIoctl);

        /* Call iot_uart_ioctl with enum eUartGetConfig, and NULL buffer.Expect -EINVAL */
        lIoctl = iot_uart_ioctl(xUartHandle, eUartGetConfig, NULL);
        TEST_ASSERT_EQUAL(-EINVAL, lIoctl);

        /* Call iot_uart_ioctl with enum eGetTxNoOfbytes, and NULL buffer.Expect -EINVAL */
        lIoctl = iot_uart_ioctl(xUartHandle, eGetTxNoOfbytes, NULL);
        TEST_ASSERT_EQUAL(-EINVAL, lIoctl);

        /* Call iot_uart_ioctl with enum eGetRxNoOfbytes, and NULL buffer.Expect -EINVAL */
        lIoctl = iot_uart_ioctl(xUartHandle, eGetRxNoOfbytes, NULL);
        TEST_ASSERT_EQUAL(-EINVAL, lIoctl);
    }

    lClose = iot_uart_close(xUartHandle);
    TEST_ASSERT_EQUAL(0, lClose);
}
/*-----------------------------------------------------------*/
/*-----------------------------------------------------------*/

/**
 * @brief Test Function to check if UART can be configured when an operation is ongoing.
 * If any asynchronous operation is in the process in UART, iot_uart_ioctl should fail.
 */
TEST(TEST_IOT_UART, AFQP_IotUARTIoctlWhenBusy)
{
    IotUARTHandle_t xUartHandle;
    int32_t lIoctl, lWrite, lClose;
    uint8_t cpBuffer[ testIotUARTBUFFERSIZE ] =
    { 0 };

    memcpy(cpBuffer, testIotUART_BUFFER_STRING, testIotUART_BUFFER_LENGTH);
    IotUARTConfig_t xUartConfig =
    { .ulBaudrate = testIotUART_TEST_BAUD_RATE_DEFAULT };

    xUartHandle = iot_uart_open(uctestIotUartPort);
    TEST_ASSERT_NOT_EQUAL(NULL, xUartHandle);

    if ( TEST_PROTECT() )
    {
        lWrite = iot_uart_write_async(xUartHandle, cpBuffer,
        testIotUART_BUFFER_LENGTH);
        TEST_ASSERT_EQUAL(0, lWrite);

        lIoctl = iot_uart_ioctl(xUartHandle, eUartSetConfig, &xUartConfig);
        TEST_ASSERT_EQUAL(-EBUSY, lIoctl);
    }

    lClose = iot_uart_close(xUartHandle);
    TEST_ASSERT_EQUAL(0, lClose);
}
/*-----------------------------------------------------------*/
/*-----------------------------------------------------------*/

/* @brief Test Function to fuzz iot_uart_open and iot_uart_close
 *-----------------------------------------------------------*/
TEST(TEST_IOT_UART, AFQP_IotUARTOpenCloseCancelFuzzing)
{
    IotUARTHandle_t xUartHandle_1, xUartHandle_2, xUartHandle_3;
    int32_t lClose, lCancel;

    xUartHandle_1 = iot_uart_open(-1);
    TEST_ASSERT_EQUAL(NULL, xUartHandle_1);

    lClose = iot_uart_close(NULL);
    TEST_ASSERT_EQUAL(-EINVAL, lClose);

    xUartHandle_2 = iot_uart_open(uctestIotUartPort);
    TEST_ASSERT_NOT_EQUAL(NULL, xUartHandle_2);

    if ( TEST_PROTECT() )
    {
        xUartHandle_3 = iot_uart_open(uctestIotUartPort);
        TEST_ASSERT_EQUAL(NULL, xUartHandle_3);

        lCancel = iot_uart_cancel(NULL);
        TEST_ASSERT_EQUAL(-EINVAL, lCancel);
    }

    lClose = iot_uart_close(xUartHandle_2);
    TEST_ASSERT_EQUAL(0, lClose);

    lClose = iot_uart_close(xUartHandle_2);
    TEST_ASSERT_EQUAL(-EINVAL, lClose);
}
/*-----------------------------------------------------------*/
