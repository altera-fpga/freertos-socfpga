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
 * @file iot_test_watchdog.c
 * @brief Functional Unit Test - Watchdog
 *******************************************************************************
 */

#include <errno.h>
/* Test includes */
#include "unity.h"
#include "unity_fixture.h"

/* Driver includes */
#include "wdt_test.h"

//#include "../gic/interrupt.h"
#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"

/*-----------------------------------------------------------*/
/*Timeout values are selected based on Index 0-15
 * 15 = WDT_TIMEOUT_20000MSEC
 * 14 = WDT_TIMEOUT_10000MSEC
 * 13 = WDT_TIMEOUT_5120MSEC so on...*/

#define testIotWATCHDOG_DEFAULT_BARK_TIME          WDT_TIMEOUT_5120MSEC
#define testIotWATCHDOG_DEFAULT_BITE_TIME          WDT_TIMEOUT_20000MSEC

#define testIotWATCHDOG_DEFAULT_SEMAPHORE_DELAY    100000U

/*Maximum watchdog instances*/
#define MAX_WATCHDOG_INSTANCES                     5U
#define MAX_BUFFER_SIZE                            1024
#define MIN_BUFFER_SIZE                            sizeof(uint32_t)
/*-----------------------------------------------------------*/

/*-----------------------------------------------------------*/

/* Globals values which can be overwritten by the test
 * framework invoking these tests */
/*-----------------------------------------------------------*/
uint8_t ucTestIotWatchdogInstance = 2; /** Default Flash instance for testing */
typedef struct
{
    uint32_t service;
    uint32_t nservice;
    uint32_t bark;
    uint32_t bite;
} WdtTest_t;

/*-----------------------------------------------------------*/
/** Static globals */
/*-----------------------------------------------------------*/
static SemaphoreHandle_t xtestIotWatchdogBarkSemaphore = NULL;
static SemaphoreHandle_t xtestIotWatchdogBiteSemaphore = NULL;

uint32_t callback_flag = 0;
uint32_t callback_flag_1 = 0;
uint32_t callback_flag_2 = 0;
uint32_t callback_flag_3 = 0;
uint32_t callback_flag_4 = 0;
uint32_t callback_flag_5 = 0;
static volatile bool bWdtExpired = false;
/*-----------------------------------------------------------*/

/* Define Test Group. */
TEST_GROUP(TEST_IOT_WATCHDOG);

/*-----------------------------------------------------------*/

/**
 * @brief Setup function called before each test in this group is executed.
 */
TEST_SETUP( TEST_IOT_WATCHDOG )
{
    xtestIotWatchdogBarkSemaphore = xSemaphoreCreateBinary();
    TEST_ASSERT_NOT_EQUAL(NULL, xtestIotWatchdogBarkSemaphore);
    xtestIotWatchdogBiteSemaphore = xSemaphoreCreateBinary();
    TEST_ASSERT_NOT_EQUAL(NULL, xtestIotWatchdogBiteSemaphore);
}

/*-----------------------------------------------------------*/

/**
 * @brief Tear down function called after each test in this group is executed.
 */
TEST_TEAR_DOWN( TEST_IOT_WATCHDOG )
{
    vSemaphoreDelete(xtestIotWatchdogBarkSemaphore);
    vSemaphoreDelete(xtestIotWatchdogBiteSemaphore);
}

/*-----------------------------------------------------------*/

/**
 * @brief Function to define which tests to execute as part of this group.
 */
TEST_GROUP_RUNNER( TEST_IOT_WATCHDOG )
{
     /*   Board resetting tests start
      *   Run the following test cases when running wdt test alone
      */

//    RUN_TEST_CASE(TEST_IOT_WATCHDOG, AFQP_IotWatchdogValidateBarkTimer);
//    RUN_TEST_CASE(TEST_IOT_WATCHDOG, test_wdt_start_valid_handle);
//    RUN_TEST_CASE(TEST_IOT_WATCHDOG, test_wdt_start_null_handle);
//    RUN_TEST_CASE(TEST_IOT_WATCHDOG, test_wdt_start_bite_time_not_set);
//    RUN_TEST_CASE(TEST_IOT_WATCHDOG, test_wdt_start_with_invalid_handle);
//    RUN_TEST_CASE(TEST_IOT_WATCHDOG, test_wdt_restart_valid_handle);
//    RUN_TEST_CASE(TEST_IOT_WATCHDOG, test_wdt_restart_null_handle);
//    RUN_TEST_CASE(TEST_IOT_WATCHDOG, test_wdt_restart_time_not_set);
//    RUN_TEST_CASE(TEST_IOT_WATCHDOG, test_wdt_restart_after_wdt_close);
//    RUN_TEST_CASE(TEST_IOT_WATCHDOG, test_wdt_restart_with_invalid_handle);
//    RUN_TEST_CASE(TEST_IOT_WATCHDOG, test_wdt_start_simultaneous_requests);
//    RUN_TEST_CASE(TEST_IOT_WATCHDOG, test_wdt_stop_simultaneous_requests);
//    RUN_TEST_CASE(TEST_IOT_WATCHDOG, test_wdt_restart_simultaneous_requests);

    /*   Board resetting tests end   */

    RUN_TEST_CASE(TEST_IOT_WATCHDOG, test_wdt_open_valid_instance);
    RUN_TEST_CASE(TEST_IOT_WATCHDOG, AFQP_IotWatchdogOpenClose);
    RUN_TEST_CASE(TEST_IOT_WATCHDOG, AFQP_IotWatchdogOpenOpenClose);
    RUN_TEST_CASE(TEST_IOT_WATCHDOG, AFQP_Fuzz_IotWatchdogClose);
    RUN_TEST_CASE(TEST_IOT_WATCHDOG, AFQP_Fuzz_IotWatchdogStartStop);
    RUN_TEST_CASE(TEST_IOT_WATCHDOG, AFQP_Fuzz_IotWatchdog_ioctl);
    RUN_TEST_CASE(TEST_IOT_WATCHDOG, AFQP_IotWatchdogOpenCloseClose);
    RUN_TEST_CASE(TEST_IOT_WATCHDOG, AFQP_IotWatchdogStartNoTimerSet);
    RUN_TEST_CASE(TEST_IOT_WATCHDOG, AFQP_IotWatchdogRestartNoStart);
    RUN_TEST_CASE(TEST_IOT_WATCHDOG, test_wdt_open_invalid_instance);
    RUN_TEST_CASE(TEST_IOT_WATCHDOG, test_wdt_open_already_open_instance);
    RUN_TEST_CASE(TEST_IOT_WATCHDOG, test_wdt_open_boundary_values);
    RUN_TEST_CASE(TEST_IOT_WATCHDOG, test_wdt_open_multiple_instances);
    RUN_TEST_CASE(TEST_IOT_WATCHDOG, test_wdt_stop_valid_handle);
    RUN_TEST_CASE(TEST_IOT_WATCHDOG, test_wdt_stop_null_handle);
    RUN_TEST_CASE(TEST_IOT_WATCHDOG, test_wdt_stop_handle_already_closed);
    RUN_TEST_CASE(TEST_IOT_WATCHDOG, test_wdt_ioctl_valid_handle_valid_request_valid_buffer);
    RUN_TEST_CASE(TEST_IOT_WATCHDOG, test_wdt_ioctl_null_handle);
    RUN_TEST_CASE(TEST_IOT_WATCHDOG, test_wdt_ioctl_invalid_request);
    RUN_TEST_CASE(TEST_IOT_WATCHDOG, test_wdt_ioctl_null_buffer);
    RUN_TEST_CASE(TEST_IOT_WATCHDOG, test_wdt_ioctl_boundary_timeout);
    RUN_TEST_CASE(TEST_IOT_WATCHDOG, test_wdt_ioctl_maximum_buffer_size);
    RUN_TEST_CASE(TEST_IOT_WATCHDOG, test_wdt_ioctl_minimum_buffer_size);
    RUN_TEST_CASE(TEST_IOT_WATCHDOG, test_wdt_ioctl_simultaneous_requests);
    RUN_TEST_CASE(TEST_IOT_WATCHDOG, test_wdt_close_valid_handle);
    RUN_TEST_CASE(TEST_IOT_WATCHDOG, test_wdt_close_null_handle);
    RUN_TEST_CASE(TEST_IOT_WATCHDOG, test_wdt_close_handle_not_open);
    RUN_TEST_CASE(TEST_IOT_WATCHDOG, test_wdt_close_already_closed_handle);
    RUN_TEST_CASE(TEST_IOT_WATCHDOG, test_wdt_open_simultaneous_requests);
    RUN_TEST_CASE(TEST_IOT_WATCHDOG, test_wdt_close_simultaneous_requests);
}

static void prvWdogCallback( void *pvUserContext )
{
    int32_t lWdogStatus = 0;

    /* Read out status */
    iot_watchdog_ioctl((IotWatchdogHandle_t) pvUserContext, eGetWatchdogStatus,
            (void* const ) &lWdogStatus);
    if ( lWdogStatus == eWdtTimerBarkExpired )
    {
        callback_flag = 1;
//        xSemaphoreGiveFromISR(xtestIotWatchdogBarkSemaphore,
//                &xHigherPriorityTaskWoken);
    }
    else
    {
        /* For bites, we should restart the timer when the callback is received.
         * This as some HW has the interrupt bound to serving the watchdog which
         * means the callback will keep firing until the watchdog is served or
         * until the device resets. */
        /* Restart the watchdog timer. */

        iot_watchdog_restart((IotWatchdogHandle_t) pvUserContext);

//        xSemaphoreGiveFromISR(xtestIotWatchdogBiteSemaphore,
//                &xHigherPriorityTaskWoken);
    }
}


/*-----------------------------------------------------------*/

/*Main function to invoke the tests*/
void wdt_test()
{
    UNITY_BEGIN();

    RUN_TEST_GROUP(TEST_IOT_WATCHDOG);

    UNITY_END();
}

/**
 * @brief Test Function to test iot_watchdog_open and iot_watchdog_close.
 *
 */
TEST( TEST_IOT_WATCHDOG, AFQP_IotWatchdogOpenClose )
{
    IotWatchdogHandle_t xWdogHandle;
    int32_t lRetVal;

    /* Open watchdog to initialize watchdog hardware. */
    xWdogHandle = iot_watchdog_open(ucTestIotWatchdogInstance);
    TEST_ASSERT_NOT_EQUAL(NULL, xWdogHandle);

    lRetVal = iot_watchdog_close(xWdogHandle);
    TEST_ASSERT_EQUAL(0, lRetVal);
}

/*-----------------------------------------------------------*/

/**
 * @brief Test Function to setup bark and bite time.
 * Bite is configured as reset, and Bark will generate interrupt.
 * Validate that bark timer will generate interrupt and callback is called.
 * and watchdog can be restarted.
 *
 */
TEST( TEST_IOT_WATCHDOG, AFQP_IotWatchdogValidateBarkTimer )
{
    IotWatchdogHandle_t xWdogHandle;
    int32_t lRetVal;
    int32_t lBarkTime;
    int32_t lBiteTime;
    int32_t lWdogStatus = 0;
    int32_t lBiteConfig;

    /* Open watchdog to initialize watchdog hardware.*/
    xWdogHandle = iot_watchdog_open(ucTestIotWatchdogInstance);
    TEST_ASSERT_NOT_EQUAL(NULL, xWdogHandle);

    if ( TEST_PROTECT() )
    {
        /* Configure bite as interrupt. */
        lBiteConfig = eWdtBiteTimerInterrupt;
        lRetVal = iot_watchdog_ioctl(xWdogHandle, eSetWatchdogBiteBehaviour,
                (void* const ) &lBiteConfig);

        /* Set the bark time to default value */
        lBarkTime = WDT_TIMEOUT_2P6MSEC;
        lRetVal = iot_watchdog_ioctl(xWdogHandle, eSetWatchdogBarkTime,
                (void* const ) &lBarkTime);

        if ( lRetVal != -ENOSYS )
        {
            printf("\n");
            printf("Bark Time set successfully");
            TEST_ASSERT_EQUAL(0, lRetVal);

            /* Set the bite time to default bite value.*/
            lBiteTime = WDT_TIMEOUT_10000MSEC;
            lRetVal = iot_watchdog_ioctl(xWdogHandle, eSetWatchdogBiteTime,
                    (void* const ) &lBiteTime);
            TEST_ASSERT_EQUAL(0, lRetVal);
            printf("\n");
            printf("Bite Time set successfully");

            /* Read back the bark and bite time */
            lBarkTime = 0;
            lRetVal = iot_watchdog_ioctl(xWdogHandle, eGetWatchdogBarkTime,
                    (void* const ) &lBarkTime);
            TEST_ASSERT_EQUAL(0, lRetVal);
            TEST_ASSERT_EQUAL(WDT_TIMEOUT_2P6MSEC, lBarkTime);

            /* Get the bite time. */
            lBiteTime = 0;
            lRetVal = iot_watchdog_ioctl(xWdogHandle, eGetWatchdogBiteTime,
                    (void* const ) &lBiteTime);
            TEST_ASSERT_EQUAL(0, lRetVal);
            TEST_ASSERT_EQUAL(WDT_TIMEOUT_10000MSEC, lBiteTime);

            /* Set up the callback  */
            iot_watchdog_set_callback(xWdogHandle, prvWdogCallback,
                    xWdogHandle);

            /* Start the watchdog timer. */
            lRetVal = iot_watchdog_start(xWdogHandle);
            TEST_ASSERT_EQUAL(0, lRetVal);

            /* Check the watchdog timer status and ensure that it's running */
            lRetVal = iot_watchdog_ioctl(xWdogHandle, eGetWatchdogStatus,
                    (void* const ) &lWdogStatus);
            TEST_ASSERT_EQUAL(0, lRetVal);
            TEST_ASSERT_EQUAL(eWdtTimerRunning, lWdogStatus);

            while ( callback_flag == 0 )
                ;
            xSemaphoreGive(xtestIotWatchdogBarkSemaphore);

            /*Wait for the watchdog timer bark time to expire. */
            lRetVal = xSemaphoreTake(xtestIotWatchdogBarkSemaphore,
                    testIotWATCHDOG_DEFAULT_SEMAPHORE_DELAY);
            TEST_ASSERT_EQUAL(pdTRUE, lRetVal);

            /* Stop the watchdog timer. */
            printf("\n");
            printf("Stopping watchdog");
            lRetVal = iot_watchdog_stop(xWdogHandle);

            if ( lRetVal != -ENOSYS )
            {
                TEST_ASSERT_EQUAL(0, lRetVal);
            }

            /*Check the watchdog timer status and ensure that it is stopped */
            lRetVal = iot_watchdog_ioctl(xWdogHandle, eGetWatchdogStatus,
                    (void* const ) &lWdogStatus);

            TEST_ASSERT_EQUAL(0, lRetVal);
            TEST_ASSERT_EQUAL(eWdtTimerBarkExpired,
                       lWdogStatus);
        }
    }

    lRetVal = iot_watchdog_close(xWdogHandle);
    TEST_ASSERT_EQUAL(0, lRetVal);
}

/*-----------------------------------------------------------*/

/**
 * @brief Test Function to setup bite time, and validate
 * if bite time expire generates an interrupt and callback is called.
 *
 */
TEST( TEST_IOT_WATCHDOG, AFQP_IotWatchdogValidateBiteInterrupt )
{
    IotWatchdogHandle_t xWdogHandle;
    int32_t lRetVal;
    int32_t lBiteTime[5] = {WDT_TIMEOUT_20000MSEC, WDT_TIMEOUT_5120MSEC,
            WDT_TIMEOUT_1280MSEC, WDT_TIMEOUT_320MSEC, WDT_TIMEOUT_80MSEC};
    int32_t lBiteTimeRec;
    int32_t lBiteConfig;
    int i;
    /* Open watchdog to initialize watchdog hardware. */
    xWdogHandle = iot_watchdog_open(ucTestIotWatchdogInstance);
    TEST_ASSERT_NOT_EQUAL(NULL, xWdogHandle);

    if ( TEST_PROTECT() )
    {
        for(i=0;i<5;i++)
        {
            /* Set the bite time to default */
            lRetVal = iot_watchdog_ioctl(xWdogHandle, eSetWatchdogBiteTime,
                    (void* const ) &lBiteTime[i]);
            TEST_ASSERT_EQUAL(0, lRetVal);

            /* Get the bite time. */
            lBiteTimeRec = 0;
            lRetVal = iot_watchdog_ioctl(xWdogHandle, eGetWatchdogBiteTime,
                    (void* const ) &lBiteTimeRec);
            TEST_ASSERT_EQUAL(0, lRetVal);
            TEST_ASSERT_EQUAL(lBiteTime[i], lBiteTimeRec);

            /* Set up the callback */
            iot_watchdog_set_callback(xWdogHandle, prvWdogCallback, xWdogHandle);

            /* Configure bite as interrupt. */
            lBiteConfig = eWdtBiteTimerInterrupt;
            lRetVal = iot_watchdog_ioctl(xWdogHandle, eSetWatchdogBiteBehaviour,
                    (void* const ) &lBiteConfig);

            if ( lRetVal != -ENOSYS )
            {
                TEST_ASSERT_EQUAL(0, lRetVal);

                /* Start the watchdog timer. */
                lRetVal = iot_watchdog_start(xWdogHandle);
                TEST_ASSERT_EQUAL(0, lRetVal);

                vTaskDelay(20000 - 1000);
                TEST_ASSERT_EQUAL(callback_flag, 0);

                vTaskDelay(pdMS_TO_TICKS(1200));
                TEST_ASSERT_EQUAL(callback_flag, 1);

                //while ( callback_flag == 0 )
                    ;
                xSemaphoreGive(xtestIotWatchdogBiteSemaphore);

                /* Wait for the watchdog timer bark time to expire. */
                lRetVal = xSemaphoreTake(xtestIotWatchdogBiteSemaphore,
                        testIotWATCHDOG_DEFAULT_SEMAPHORE_DELAY);
                TEST_ASSERT_EQUAL(pdTRUE, lRetVal);

                /* Stop the watchdog timer. */
                lRetVal = iot_watchdog_stop(xWdogHandle);

                if ( lRetVal != -ENOSYS )
                {
                    TEST_ASSERT_EQUAL(0, lRetVal);
                }
            }
        }
    }

    lRetVal = iot_watchdog_close(xWdogHandle);
    TEST_ASSERT_EQUAL(0, lRetVal);
}

/*-----------------------------------------------------------*/

/**
 * @brief Test Function to start the watchdog without setting the timer first.
 * verify that watchdog returns proper error.
 *
 */
TEST( TEST_IOT_WATCHDOG, AFQP_IotWatchdogStartNoTimerSet )
{
    IotWatchdogHandle_t xWdogHandle;
    int32_t lRetVal;

    /* Open watchdog to initialize watchdog hardware. */
    xWdogHandle = iot_watchdog_open(ucTestIotWatchdogInstance);
    TEST_ASSERT_NOT_EQUAL(NULL, xWdogHandle);

    if ( TEST_PROTECT() )
    {
        /* Start the watchdog timer. */
        lRetVal = iot_watchdog_start(xWdogHandle);
        TEST_ASSERT_EQUAL(-ENODATA, lRetVal);
    }

    lRetVal = iot_watchdog_close(xWdogHandle);
    TEST_ASSERT_EQUAL(0, lRetVal);
}

/**
 * @brief Test Function to start the watchdog without setting the timer first.
 * verify that watchdog returns proper error.
 *
 */
TEST( TEST_IOT_WATCHDOG, AFQP_IotWatchdogRestartNoStart )
{
    IotWatchdogHandle_t xWdogHandle;
    int32_t lRetVal;

    /* Open watchdog to initialize watchdog hardware. */
    xWdogHandle = iot_watchdog_open(ucTestIotWatchdogInstance);
    TEST_ASSERT_NOT_EQUAL(NULL, xWdogHandle);

    if ( TEST_PROTECT() )
    {
        /* Restart the watchdog timer. */
        lRetVal = iot_watchdog_restart(xWdogHandle);
        TEST_ASSERT_EQUAL(-ENODATA, lRetVal);
    }

    lRetVal = iot_watchdog_close(xWdogHandle);
    TEST_ASSERT_EQUAL(0, lRetVal);
}

/**
 * @brief Test Function to set the Bark time first,
 * and set Bark time greater than Bite and verify that it returns proper error code
 *
 */
TEST( TEST_IOT_WATCHDOG,
        AFQP_IotWatchdogSetBarkGreaterThanBiteSetWatchdogBarkFirst )
{
    IotWatchdogHandle_t xWdogHandle;
    int32_t lRetVal;
    int32_t lBarkTime;
    int32_t lBiteTime;

    /* Open watchdog to initialize watchdog hardware. */
    xWdogHandle = iot_watchdog_open(ucTestIotWatchdogInstance);
    TEST_ASSERT_NOT_EQUAL(NULL, xWdogHandle);

    if ( TEST_PROTECT() )
    {

        /* Set the bark time to default value. */
        lBarkTime = testIotWATCHDOG_DEFAULT_BARK_TIME;
        lRetVal = iot_watchdog_ioctl(xWdogHandle, eSetWatchdogBarkTime,
                (void* const ) &lBarkTime);

        if ( lRetVal != -ENOSYS )
        {
            TEST_ASSERT_EQUAL(0, lRetVal);

            /* Set the bite time less than bark. */
            lBiteTime = lBarkTime - 1;
            lRetVal = iot_watchdog_ioctl(xWdogHandle, eSetWatchdogBiteTime,
                    (void* const ) &lBiteTime);
            TEST_ASSERT_EQUAL(-EINVAL, lRetVal);
        }
    }

    lRetVal = iot_watchdog_close(xWdogHandle);
    TEST_ASSERT_EQUAL(0, lRetVal);
}

/**
 * @brief Test Function to set the Bite time first,
 * and set Bark time greater than Bite and verify that it returns proper error code
 *
 */
TEST( TEST_IOT_WATCHDOG,
        AFQP_IotWatchdogSetBarkGreaterThanBiteSetWatchdogBiteFirst )
{
    IotWatchdogHandle_t xWdogHandle;
    int32_t lRetVal;
    int32_t lBarkTime;
    int32_t lBiteTime;

    /* Open watchdog to initialize watchdog hardware. */
    xWdogHandle = iot_watchdog_open(ucTestIotWatchdogInstance);
    TEST_ASSERT_NOT_EQUAL(NULL, xWdogHandle);

    if ( TEST_PROTECT() )
    {
        /* Set the bite time to default bite time. */
        lBiteTime = testIotWATCHDOG_DEFAULT_BITE_TIME;
        lRetVal = iot_watchdog_ioctl(xWdogHandle, eSetWatchdogBiteTime,
                (void* const ) &lBiteTime);
        TEST_ASSERT_EQUAL(0, lRetVal);

        /* Set the bark time to lBiteTime + 1 */
        lBarkTime = lBiteTime + 1;
        lRetVal = iot_watchdog_ioctl(xWdogHandle, eSetWatchdogBarkTime,
                (void* const ) &lBarkTime);

        if ( lRetVal != -ENOSYS )
        {
            TEST_ASSERT_EQUAL(-EINVAL, lRetVal);
        }
    }

    lRetVal = iot_watchdog_close(xWdogHandle);
    TEST_ASSERT_EQUAL(0, lRetVal);
}

/*-----------------------------------------------------------*/

/**
 * @brief Test Function to setup bark and bite time.
 * Bite is configured as reset, and Bark will generate interrupt.
 * Validate that bark timer will generate interrupt and callback is called.
 * and watchdog can be restarted.
 *
 */
TEST( TEST_IOT_WATCHDOG, AFQP_IotWatchdogValidateWdogRestart )
{
    IotWatchdogHandle_t xWdogHandle;
    int32_t lRetVal;
    int32_t lBarkTime;
    int32_t lBiteTime;
    int32_t lWdogStatus;

    /* Open watchdog to initialize watchdog hardware. */
    xWdogHandle = iot_watchdog_open(ucTestIotWatchdogInstance);
    TEST_ASSERT_NOT_EQUAL(NULL, xWdogHandle);

    if ( TEST_PROTECT() )
    {
        /* Set the bark time to default default value. */
        lBarkTime = testIotWATCHDOG_DEFAULT_BARK_TIME;
        lRetVal = iot_watchdog_ioctl(xWdogHandle, eSetWatchdogBarkTime,
                (void* const ) &lBarkTime);

        if ( lRetVal == -ENOSYS )
        {
            lRetVal = iot_watchdog_close(xWdogHandle);
            TEST_ASSERT_EQUAL(0, lRetVal);
            return;
        }

        TEST_ASSERT_EQUAL(0, lRetVal);

        /* Set the bite time to default bite value */
        lBiteTime = testIotWATCHDOG_DEFAULT_BITE_TIME;
        lRetVal = iot_watchdog_ioctl(xWdogHandle, eSetWatchdogBiteTime,
                (void* const ) &lBiteTime);
        TEST_ASSERT_EQUAL(0, lRetVal);

        /* Read back the bark and bite time */
        lBarkTime = 0;
        lRetVal = iot_watchdog_ioctl(xWdogHandle, eGetWatchdogBarkTime,
                (void* const ) &lBarkTime);
        TEST_ASSERT_EQUAL(0, lRetVal);
        TEST_ASSERT_EQUAL(testIotWATCHDOG_DEFAULT_BARK_TIME, lBarkTime);

        /* Get the bite time. */
        lBiteTime = 0;
        lRetVal = iot_watchdog_ioctl(xWdogHandle, eGetWatchdogBiteTime,
                (void* const ) &lBiteTime);
        TEST_ASSERT_EQUAL(0, lRetVal);
        TEST_ASSERT_EQUAL(testIotWATCHDOG_DEFAULT_BITE_TIME, lBiteTime);

        /* Set up the callback */
        iot_watchdog_set_callback(xWdogHandle, prvWdogCallback, xWdogHandle);

        /* Start the watchdog timer. */
        lRetVal = iot_watchdog_start(xWdogHandle);
        TEST_ASSERT_EQUAL(0, lRetVal);

        /* Wait for the watchdog timer bark time to expire. */
        lRetVal = xSemaphoreTake(xtestIotWatchdogBarkSemaphore,
                testIotWATCHDOG_DEFAULT_SEMAPHORE_DELAY);
        TEST_ASSERT_EQUAL(pdTRUE, lRetVal);

        /* Restart the watchdog timer. */
        lRetVal = iot_watchdog_restart(xWdogHandle);
        TEST_ASSERT_EQUAL(-ENODATA, lRetVal);

        /* Wait again for the watchdog timer bark time to expire. */
        lRetVal = xSemaphoreTake(xtestIotWatchdogBarkSemaphore,
                testIotWATCHDOG_DEFAULT_SEMAPHORE_DELAY);
        TEST_ASSERT_EQUAL(pdTRUE, lRetVal);

        /* Check the watchdog timer status */
        lRetVal = iot_watchdog_ioctl(xWdogHandle, eGetWatchdogStatus,
                (void* const ) &lWdogStatus);
        TEST_ASSERT_EQUAL(0, lRetVal);
        TEST_ASSERT_EQUAL(eWdtTimerBarkExpired,
                eWdtTimerBarkExpired & lWdogStatus);

        /* Stop the watchdog timer. */
        lRetVal = iot_watchdog_stop(xWdogHandle);

        if ( lRetVal != -ENOSYS )
        {
            TEST_ASSERT_EQUAL(0, lRetVal);
        }
    }

    lRetVal = iot_watchdog_close(xWdogHandle);
    TEST_ASSERT_EQUAL(0, lRetVal);
}

/*-----------------------------------------------------------*/

/**
 * @brief Test Function to setup bite time, and validate
 * if bite time expire resets the device.
 *
 */
TEST( TEST_IOT_WATCHDOG, AFQP_IotWatchdogValidateWdtReset )
{
    IotWatchdogHandle_t xWdogHandle;
    int32_t lRetVal;
    int32_t lBiteTime;
    int32_t lBiteConfig;

    /* Open watchdog to initialize watchdog hardware. */
    xWdogHandle = iot_watchdog_open(ucTestIotWatchdogInstance);
    TEST_ASSERT_NOT_EQUAL(NULL, xWdogHandle);

    if ( TEST_PROTECT() )
    {
        /* Set the bite time to default */
        lBiteTime = testIotWATCHDOG_DEFAULT_BITE_TIME;
        lRetVal = iot_watchdog_ioctl(xWdogHandle, eSetWatchdogBiteTime,
                (void* const ) &lBiteTime);
        TEST_ASSERT_EQUAL(0, lRetVal);

        /*Get the bite time. */
        lBiteTime = 0;
        lRetVal = iot_watchdog_ioctl(xWdogHandle, eGetWatchdogBiteTime,
                (void* const ) &lBiteTime);
        TEST_ASSERT_EQUAL(0, lRetVal);
        TEST_ASSERT_EQUAL(testIotWATCHDOG_DEFAULT_BITE_TIME, lBiteTime);

        /* Configure bite as interrupt. */
        lBiteConfig = eWdtBiteTimerReset;
        lRetVal = iot_watchdog_ioctl(xWdogHandle, eSetWatchdogBiteBehaviour,
                (void* const ) &lBiteConfig);
        TEST_ASSERT_EQUAL(0, lRetVal);

        /* Start the watchdog timer. */
        lRetVal = iot_watchdog_start(xWdogHandle);
        TEST_ASSERT_EQUAL(0, lRetVal);

        vTaskDelay(lBiteTime * 5);
        /*Target shall reset */
        /*Execution shall not reach here. */
        TEST_ASSERT_EQUAL(0, 1);
    }
}

/*-----------------------------------------------------------*/

TEST( TEST_IOT_WATCHDOG, AFQP_IotWatchdogOpenOpenClose )
{
    IotWatchdogHandle_t xWdogHandle;
    IotWatchdogHandle_t xWdogHandleOpen;
    int32_t lRetVal;

    /* Open watchdog to initialize watchdog hardware. */
    xWdogHandle = iot_watchdog_open(ucTestIotWatchdogInstance);
    TEST_ASSERT_NOT_EQUAL(NULL, xWdogHandle);

    if ( TEST_PROTECT() )
    {
        /* Attempt to Re-Open watchdog to initialize watchdog hardware. */
        xWdogHandleOpen = iot_watchdog_open(ucTestIotWatchdogInstance);
        TEST_ASSERT_EQUAL(NULL, xWdogHandleOpen);
    }

    lRetVal = iot_watchdog_close(xWdogHandle);
    TEST_ASSERT_EQUAL(NULL, lRetVal);
}

/**
 * @brief Test Function to fuzz iot_watchdog_close.
 *
 */
TEST( TEST_IOT_WATCHDOG, AFQP_Fuzz_IotWatchdogClose )
{
    int32_t lRetVal;

    /* Close the watchdog timer. */
    lRetVal = iot_watchdog_close(NULL);
    TEST_ASSERT_EQUAL(-EINVAL, lRetVal);
}

/**
 * @brief Test Function to fuzz iot_watchdog_start, iot_watchdog_stop and iot_watchdog_restart.
 *
 */
TEST( TEST_IOT_WATCHDOG, AFQP_Fuzz_IotWatchdogStartStop )
{
    int32_t lRetVal;

    /* Open watchdog to initialize watchdog hardware. */
    lRetVal = iot_watchdog_start(NULL);
    TEST_ASSERT_EQUAL(-EINVAL, lRetVal);

    lRetVal = iot_watchdog_stop(NULL);

    if ( lRetVal != -ENOSYS )
    {
        TEST_ASSERT_EQUAL(-EINVAL, lRetVal);
    }

    /*
     *  lRetVal = iot_watchdog_restart(NULL);
     *  TEST_ASSERT_EQUAL(-EINVAL, lRetVal);
     */
}

/**
 * @brief Test Function to fuzz iot_watchdog_ioctl.
 *
 */
TEST( TEST_IOT_WATCHDOG, AFQP_Fuzz_IotWatchdog_ioctl )
{
    IotWatchdogHandle_t xWdogHandle;
    int32_t lRetVal;
    int32_t lBiteTime;

    /* Invoke iot_watchdog_ioctl with NULL handle and valid buffer. */
    lBiteTime = 0;
    lRetVal = iot_watchdog_ioctl(NULL, eGetWatchdogBiteTime,
            (void* const ) &lBiteTime);
    TEST_ASSERT_EQUAL(-EINVAL, lRetVal);

    /* Open watchdog to initialize watchdog hardware. */
    xWdogHandle = iot_watchdog_open(ucTestIotWatchdogInstance);
    TEST_ASSERT_NOT_EQUAL(NULL, xWdogHandle);

    if ( TEST_PROTECT() )
    {
        /* Invoke iot_watchdog_ioctl with NULL buffer and valid handle. */
        lRetVal = iot_watchdog_ioctl(xWdogHandle, eSetWatchdogBiteTime, NULL);
        TEST_ASSERT_EQUAL(-EINVAL, lRetVal);

        /* Invoke iot_watchdog_ioctl with NULL buffer and valid handle and invalid enum. */
        lRetVal = iot_watchdog_ioctl(xWdogHandle, -1,
                (void* const ) &lBiteTime);
        TEST_ASSERT_EQUAL(-EINVAL, lRetVal);
    }

    /* Close the watchdog timer. */
    lRetVal = iot_watchdog_close(xWdogHandle);
    TEST_ASSERT_EQUAL(0, lRetVal);
}

/* Test to ensure closing the stale handle is not permitted */

TEST( TEST_IOT_WATCHDOG, AFQP_IotWatchdogOpenCloseClose )
{
    IotWatchdogHandle_t xWdogHandle;
    int32_t lRetVal;

    /* Open watchdog to initialize watchdog hardware. */
    xWdogHandle = iot_watchdog_open(ucTestIotWatchdogInstance);
    TEST_ASSERT_NOT_EQUAL(NULL, xWdogHandle);

    /* Close the stale watchdog handle. */
    lRetVal = iot_watchdog_close(xWdogHandle);
    TEST_ASSERT_EQUAL(0, lRetVal);

    /* Attempt to Close the stale watchdog handle. */
    lRetVal = iot_watchdog_close(xWdogHandle);
    TEST_ASSERT_EQUAL(-EINVAL, lRetVal);
}

WdtHandle_t validHandle = NULL;
WdtHandle_t validHandle2 = NULL;

TEST(TEST_IOT_WATCHDOG, test_wdt_open_valid_instance) {
    validHandle = wdt_open(ucTestIotWatchdogInstance);
    TEST_ASSERT_NOT_NULL(validHandle);

    // Clean up
    uint32_t result = wdt_close(validHandle);
    TEST_ASSERT_EQUAL(0, result);
    validHandle = NULL;
}

TEST(TEST_IOT_WATCHDOG, test_wdt_open_invalid_instance) {

    int32_t invalidInstance = -1; // Assuming -1 is an invalid instance

    WdtHandle_t handle = wdt_open(invalidInstance);

    TEST_ASSERT_NULL(handle);
}

TEST(TEST_IOT_WATCHDOG, test_wdt_open_already_open_instance) {

    int32_t validInstance = 2; // Assuming 2 is a valid instance // Moved to setup

    validHandle = wdt_open(validInstance);
    TEST_ASSERT_NOT_NULL(validHandle);

    WdtHandle_t secondHandle = wdt_open(validInstance);
    TEST_ASSERT_NULL(secondHandle);

    // Clean up
    uint32_t result = wdt_close(validHandle);
    TEST_ASSERT_EQUAL(0, result);
    validHandle = NULL;
}

TEST(TEST_IOT_WATCHDOG, test_wdt_open_boundary_values) {
    validHandle = wdt_open(MAX_WATCHDOG_INSTANCES);
    TEST_ASSERT_NULL(validHandle);

    validHandle = wdt_open(0);
    TEST_ASSERT_NOT_NULL(validHandle);

    uint32_t result = wdt_close(validHandle);
    TEST_ASSERT_EQUAL(0, result);
    validHandle = NULL;
}

// Test opening multiple instances
TEST(TEST_IOT_WATCHDOG, test_wdt_open_multiple_instances) {
    WdtHandle_t handle1 = wdt_open(0);
    WdtHandle_t handle2 = wdt_open(2);
    TEST_ASSERT_NOT_NULL(handle1);
    TEST_ASSERT_NOT_NULL(handle2);

    int32_t result = wdt_close(handle1);
    TEST_ASSERT_EQUAL(0, result);
    result = wdt_close(handle2);
    TEST_ASSERT_EQUAL(0, result);
}

// Test starting with a valid handle
TEST(TEST_IOT_WATCHDOG, test_wdt_start_valid_handle) {
    validHandle = wdt_open(2);
    int biteTime = 15; // Set bite time to 1000ms

    // Set bite time using IOCTL
    wdt_ioctl(validHandle, eSetWatchdogBiteTime, &biteTime); // Set bite time to 1000ms

    int32_t result = wdt_start(validHandle);

    TEST_ASSERT_EQUAL(0, result);

    // Clean up
    wdt_close(validHandle);
    validHandle = NULL;
}

// Test starting with a null handle
TEST(TEST_IOT_WATCHDOG, test_wdt_start_null_handle) {
    WdtHandle_t nullHandle = NULL; // Moved to global variable

    int32_t result = wdt_start(nullHandle);

    TEST_ASSERT_EQUAL(-EINVAL, result);
}

// Test starting with bite time not set
TEST(TEST_IOT_WATCHDOG, test_wdt_start_bite_time_not_set) {
    validHandle = wdt_open(2);
    TEST_ASSERT_NOT_NULL(validHandle);

    int32_t result = wdt_start(validHandle);
    TEST_ASSERT_EQUAL(-ENODATA, result);

    // Clean up
    result = wdt_close(validHandle);
    TEST_ASSERT_EQUAL(0, result);
    validHandle = NULL;
}

// Test starting with an invalid handle
TEST(TEST_IOT_WATCHDOG, test_wdt_start_with_invalid_handle) {
    validHandle = wdt_open(-1);
    TEST_ASSERT_NULL(validHandle);
    int32_t result = wdt_start(validHandle);
    TEST_ASSERT_EQUAL(-EINVAL, result);
}

//Test stopping with a valid handle
TEST(TEST_IOT_WATCHDOG, test_wdt_stop_valid_handle) {
    validHandle = wdt_open(2);
    TEST_ASSERT_NOT_NULL(validHandle);

    int32_t result = wdt_stop(validHandle);
    TEST_ASSERT_EQUAL(0, result);

    result = wdt_close(validHandle);
    TEST_ASSERT_EQUAL(0, result);
    validHandle = NULL;
}

// Test stopping with a null handle
TEST(TEST_IOT_WATCHDOG, test_wdt_stop_null_handle) {

    int32_t result = wdt_stop(NULL);
    TEST_ASSERT_EQUAL(-EINVAL, result);
}

// Test stopping with a handle that is already closed
TEST(TEST_IOT_WATCHDOG, test_wdt_stop_handle_already_closed) {
    validHandle = wdt_open(2);
    TEST_ASSERT_NOT_NULL(validHandle);
    int32_t result = wdt_close(validHandle);
    TEST_ASSERT_EQUAL(0, result);
    validHandle = NULL;
    result = wdt_stop(validHandle);
    TEST_ASSERT_EQUAL(-EINVAL, result);
}

// Test restarting with a valid handle
TEST(TEST_IOT_WATCHDOG, test_wdt_restart_valid_handle) {
    validHandle = wdt_open(2);
    TEST_ASSERT_NOT_NULL(validHandle);

    int biteTime = 15; // Set bite time to a valid value
    int32_t result = wdt_ioctl(validHandle, eSetWatchdogBiteTime, &biteTime); // Set bite time using IOCTL
    TEST_ASSERT_EQUAL(0, result);

    result = wdt_start(validHandle); // Start the watchdog timer
    TEST_ASSERT_EQUAL(0, result);

    result = wdt_restart(validHandle);
    TEST_ASSERT_EQUAL(0, result);

    result = wdt_close(validHandle);
    TEST_ASSERT_EQUAL(0, result);
    validHandle = NULL;
}

// Test restarting with a null handle
TEST(TEST_IOT_WATCHDOG, test_wdt_restart_null_handle) {

    int32_t result = wdt_restart(NULL);
    TEST_ASSERT_EQUAL(-EINVAL, result);
}

// Test restarting without setting bite time
TEST(TEST_IOT_WATCHDOG, test_wdt_restart_time_not_set) {
    validHandle = wdt_open(2);
    TEST_ASSERT_NOT_NULL(validHandle);
    // Simulate condition where timers are not set
    // This part depends on the implementation details
    // For example, you might need to mock the behavior of the underlying hardware or driver

    int32_t result = wdt_restart(validHandle);
    TEST_ASSERT_EQUAL(-ENODATA, result);

    result = wdt_close(validHandle);
    TEST_ASSERT_EQUAL(0, result);
    validHandle = NULL;
}

// Test restarting after closing the handle
TEST(TEST_IOT_WATCHDOG, test_wdt_restart_after_wdt_close) {
    validHandle = wdt_open(2);
    TEST_ASSERT_NOT_NULL(validHandle);

    int32_t result = wdt_close(validHandle);
    TEST_ASSERT_EQUAL(0, result);
    validHandle = NULL;

    TEST_ASSERT_EQUAL(-EINVAL, wdt_restart(validHandle));
}

// Test restarting with an invalid handle
TEST(TEST_IOT_WATCHDOG, test_wdt_restart_with_invalid_handle) {
    WdtHandle_t invalid_handle = wdt_open(-1); // Example invalid handle
    TEST_ASSERT_EQUAL(-EINVAL, wdt_restart(invalid_handle));
}

//Test cases for wdt_ioctl
// Test ioctl with a valid handle, valid request, and valid buffer
TEST(TEST_IOT_WATCHDOG, test_wdt_ioctl_valid_handle_valid_request_valid_buffer) {
    validHandle = wdt_open(2);
    TEST_ASSERT_NOT_NULL(validHandle);
    int32_t validRequest = eSetWatchdogBiteTime; // Example valid request
    int32_t validBuffer = 15; // Example valid buffer
    int32_t result = wdt_ioctl(validHandle, validRequest, &validBuffer);
    TEST_ASSERT_EQUAL(0, result);

    result = wdt_close(validHandle);
    TEST_ASSERT_EQUAL(0, result);
    validHandle = NULL;
}

// Test ioctl with a null handle
TEST(TEST_IOT_WATCHDOG, test_wdt_ioctl_null_handle) {
    int32_t validRequest = eSetWatchdogBiteTime; // Example valid request
    int32_t validBuffer = 15; // Example valid buffer
    int32_t result = wdt_ioctl(NULL, validRequest, &validBuffer);
    TEST_ASSERT_EQUAL(-EINVAL, result);
}

// Test ioctl with an invalid request
TEST(TEST_IOT_WATCHDOG, test_wdt_ioctl_invalid_request) {
    validHandle = wdt_open(2);
    TEST_ASSERT_NOT_NULL(validHandle);

    int32_t validBuffer = 0;
    int32_t result = wdt_ioctl(validHandle, 6, &validBuffer);
    TEST_ASSERT_EQUAL(-EINVAL, result);

    result = wdt_close(validHandle);
    TEST_ASSERT_EQUAL(0, result);
    validHandle = NULL;
}

// Test ioctl with a null buffer
TEST(TEST_IOT_WATCHDOG, test_wdt_ioctl_null_buffer) {
    validHandle = wdt_open(0);
    TEST_ASSERT_NOT_NULL(validHandle);
    WdtIoctlRequest_t validRequest = eSetWatchdogBiteTime; // Example valid request
    int32_t result = wdt_ioctl(validHandle, validRequest, NULL);
    TEST_ASSERT_EQUAL(-EINVAL, result);

    result = wdt_close(validHandle);
    TEST_ASSERT_EQUAL(0, result);
    validHandle = NULL;
}

// Test ioctl with boundary timeout values
TEST(TEST_IOT_WATCHDOG, test_wdt_ioctl_boundary_timeout) {
    validHandle = wdt_open(2);
    TEST_ASSERT_NOT_NULL(validHandle);
    WdtIoctlRequest_t validRequest = eSetWatchdogBiteTime; // Example valid request
    uint32_t validBuffer = 15;

    int32_t result = wdt_ioctl(validHandle, validRequest, &validBuffer);
    TEST_ASSERT_EQUAL(0, result);

    result = wdt_close(validHandle);
    TEST_ASSERT_EQUAL(0, result);
    validHandle = NULL;
}

// Test ioctl with maximum buffer size..... Logic for the test seems not correct
TEST(TEST_IOT_WATCHDOG, test_wdt_ioctl_maximum_buffer_size) {
    validHandle = wdt_open(2);
    TEST_ASSERT_NOT_NULL(validHandle);
    WdtIoctlRequest_t validRequest = eGetWatchdogBarkTime; // Initialize with valid request
    void *buffer = malloc(MAX_BUFFER_SIZE); // Initialize with maximum buffer size

    int32_t result = wdt_ioctl(validHandle, validRequest, buffer);
    TEST_ASSERT_EQUAL(0, result);
    free(buffer);

    result = wdt_close(validHandle);
    TEST_ASSERT_EQUAL(0, result);
    validHandle = NULL;
}

// Test ioctl with minimum buffer size
TEST(TEST_IOT_WATCHDOG, test_wdt_ioctl_minimum_buffer_size) {
    validHandle = wdt_open(2);
    TEST_ASSERT_NOT_NULL(validHandle);
    WdtIoctlRequest_t validRequest = eGetWatchdogBarkTime;
    void *buffer = malloc(MIN_BUFFER_SIZE); // Initialize with minimum buffer size

    int32_t result = wdt_ioctl(validHandle, validRequest, buffer);
    TEST_ASSERT_EQUAL(0, result);
    free(buffer);

    result = wdt_close(validHandle);
    TEST_ASSERT_EQUAL(0, result);
    validHandle = NULL;
}

// Test simultaneous ioctl requests
TEST(TEST_IOT_WATCHDOG, test_wdt_ioctl_simultaneous_requests) {
    validHandle = wdt_open(2);
    validHandle2 = wdt_open(0);
    TEST_ASSERT_NOT_NULL(validHandle);
    TEST_ASSERT_NOT_NULL(validHandle2);
    int32_t validRequest = eSetWatchdogBiteTime;
    int32_t validRequest2 = eSetWatchdogBiteTime;
    int32_t validBuffer = 15; // Example valid buffer
    int32_t validBuffer2 = 14; // Example valid buffer

    int32_t result1 = wdt_ioctl(validHandle, validRequest, &validBuffer);
    int32_t result2 = wdt_ioctl(validHandle2, validRequest2, &validBuffer2);

    TEST_ASSERT_EQUAL(0, result1);
    TEST_ASSERT_EQUAL(0, result2);

    result1 = wdt_close(validHandle);
    TEST_ASSERT_EQUAL(0, result1);
    result2 = wdt_close(validHandle2);
    TEST_ASSERT_EQUAL(0, result2);
    validHandle = NULL;
    validHandle2 = NULL;
}

//Test cases for wdt_close

// Test closing with a valid handle
TEST(TEST_IOT_WATCHDOG, test_wdt_close_valid_handle) {
    validHandle = wdt_open(2);
    TEST_ASSERT_NOT_NULL(validHandle);

    int32_t result = wdt_close(validHandle);
    TEST_ASSERT_EQUAL(0, result);
    validHandle = NULL;
}

// Test closing with a null handle
TEST(TEST_IOT_WATCHDOG, test_wdt_close_null_handle) {

    int32_t result = wdt_close(NULL);
    TEST_ASSERT_EQUAL(-EINVAL, result);
}

// Test closing a handle that is not open
TEST(TEST_IOT_WATCHDOG, test_wdt_close_handle_not_open) {
    WdtHandle_t handle = NULL;
    int32_t result = wdt_close(handle);
    TEST_ASSERT_EQUAL(-EINVAL, result);
}

// Test closing a handle that is already closed
TEST(TEST_IOT_WATCHDOG, test_wdt_close_already_closed_handle) {
    validHandle = wdt_open(2);
    TEST_ASSERT_NOT_NULL(validHandle);

    int32_t result = wdt_close(validHandle);
    TEST_ASSERT_EQUAL(0, result);
    result = wdt_close(validHandle);
    TEST_ASSERT_EQUAL(-EINVAL, result);
    validHandle = NULL;
}

// Test cases for simultaneous requests

// Simultaneous requests for wdt_open
TEST(TEST_IOT_WATCHDOG, test_wdt_open_simultaneous_requests) {
    validHandle = wdt_open(2);
    validHandle2 = wdt_open(0);

    TEST_ASSERT_NOT_NULL(validHandle);
    TEST_ASSERT_NOT_NULL(validHandle2);

    // Clean up
    int32_t result = wdt_close(validHandle);
    TEST_ASSERT_EQUAL(0, result);
    validHandle = NULL;
    result = wdt_close(validHandle2);
    TEST_ASSERT_EQUAL(0, result);
    validHandle2 = NULL;
}

// Simultaneous requests for wdt_start
TEST(TEST_IOT_WATCHDOG, test_wdt_start_simultaneous_requests) {
    validHandle = wdt_open(2);
    validHandle2 = wdt_open(0);
    TEST_ASSERT_NOT_NULL(validHandle);
    TEST_ASSERT_NOT_NULL(validHandle2);

    int biteTime = 15; // Set bite time to a valid value
    int32_t result = wdt_ioctl(validHandle, eSetWatchdogBiteTime, &biteTime);
    TEST_ASSERT_EQUAL(0, result);
    result = wdt_ioctl(validHandle2, eSetWatchdogBiteTime, &biteTime);
    TEST_ASSERT_EQUAL(0, result);

    int32_t result1 = wdt_start(validHandle);
    int32_t result2 = wdt_start(validHandle2);

    TEST_ASSERT_EQUAL(0, result1);
    TEST_ASSERT_EQUAL(0, result2);

    result = wdt_close(validHandle);
    TEST_ASSERT_EQUAL(0, result);
    result = wdt_close(validHandle2);
    TEST_ASSERT_EQUAL(0, result);
    validHandle = NULL;
    validHandle2 = NULL;
}

// Simultaneous requests for wdt_stop
TEST(TEST_IOT_WATCHDOG, test_wdt_stop_simultaneous_requests) {
    validHandle = wdt_open(2);
    validHandle2 = wdt_open(0);
    TEST_ASSERT_NOT_NULL(validHandle);
    TEST_ASSERT_NOT_NULL(validHandle2);
    int biteTime = 15; // Set bite time to a valid value
    int32_t result = wdt_ioctl(validHandle, eSetWatchdogBiteTime, &biteTime);
    TEST_ASSERT_EQUAL(0, result);
    result = wdt_ioctl(validHandle2, eSetWatchdogBiteTime, &biteTime);
    TEST_ASSERT_EQUAL(0, result);

    result = wdt_start(validHandle);
    TEST_ASSERT_EQUAL(0, result);
    result = wdt_start(validHandle2);
    TEST_ASSERT_EQUAL(0, result);

    int32_t result1 = wdt_stop(validHandle);
    int32_t result2 = wdt_stop(validHandle2);

    TEST_ASSERT_EQUAL(0, result1);
    TEST_ASSERT_EQUAL(0, result2);

    result = wdt_close(validHandle);
    TEST_ASSERT_EQUAL(0, result);
    result = wdt_close(validHandle2);
    TEST_ASSERT_EQUAL(0, result);
    validHandle = NULL;
    validHandle2 = NULL;
}

// Simultaneous requests for wdt_restart
TEST(TEST_IOT_WATCHDOG, test_wdt_restart_simultaneous_requests) {
    validHandle = wdt_open(2);
    validHandle2 = wdt_open(0);
    TEST_ASSERT_NOT_NULL(validHandle);
    TEST_ASSERT_NOT_NULL(validHandle2);

    int biteTime = 15; // Set bite time to a valid value
    int32_t result = wdt_ioctl(validHandle, eSetWatchdogBiteTime, &biteTime);
    TEST_ASSERT_EQUAL(0, result);
    result = wdt_ioctl(validHandle2, eSetWatchdogBiteTime, &biteTime);
    TEST_ASSERT_EQUAL(0, result);
    result = wdt_start(validHandle);
    TEST_ASSERT_EQUAL(0, result);
    result = wdt_start(validHandle2);
    TEST_ASSERT_EQUAL(0, result);

    int32_t result1 = wdt_restart(validHandle);
    int32_t result2 = wdt_restart(validHandle2);
    TEST_ASSERT_EQUAL(0, result1);
    TEST_ASSERT_EQUAL(0, result2);

    result = wdt_close(validHandle);
    TEST_ASSERT_EQUAL(0, result);
    result = wdt_close(validHandle2);
    TEST_ASSERT_EQUAL(0, result);
    validHandle = NULL;
    validHandle2 = NULL;
}

// Simultaneous requests for wdt_close
TEST(TEST_IOT_WATCHDOG, test_wdt_close_simultaneous_requests) {
    validHandle = wdt_open(2);
    validHandle2 = wdt_open(0);
    TEST_ASSERT_NOT_NULL(validHandle);
    TEST_ASSERT_NOT_NULL(validHandle2);

    int32_t result1 = wdt_close(validHandle);
    int32_t result2 = wdt_close(validHandle2);

    TEST_ASSERT_EQUAL(0, result1);
    TEST_ASSERT_EQUAL(0, result2);
    validHandle = NULL;
    validHandle2 = NULL;
}
