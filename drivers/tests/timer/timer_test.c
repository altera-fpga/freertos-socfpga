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
 * @file test_iot_timer.c
 * @brief Functional Unit Test - TIMER
 *******************************************************************************
 */

/* Test includes */
#include <errno.h>
#include "timer_test.h"
#include "unity.h"
#include "unity_fixture.h"

/* Driver includes */
#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"

#define testIotTIMER_DEFAULT_DELAY_US 5000
/* use 10 RTOS ticks delay for timer measurements */
#define testIotTIMER_SLEEP_DELAY_TICKS 10
#define testIotTIMER_SLEEP_DELAY_MS (testIotTIMER_SLEEP_DELAY_TICKS * 1000 / configTICK_RATE_HZ)


/*
 * @brief timeout for the semaphore to be invoked by the iot_timer. 4 times larger to account
 * for timer resolution variances.
 */
#define testIotTIMER_CALLBACK_DELAY_TICKS (testIotTIMER_DEFAULT_DELAY_US * 4 * configTICK_RATE_HZ / (1000 * 1000))

/*-----------------------------------------------------------*/

/* Globals values which can be overwritten by the test
 * framework invoking these tests */
/*-----------------------------------------------------------*/
IotTimerHandle_t gIotTimerHandle;
int32_t ltestIotTimerInstance = 0;

/*-----------------------------------------------------------*/
/** Static globals */
/*-----------------------------------------------------------*/
static SemaphoreHandle_t xtestIotTimerSemaphore = NULL;

/*-----------------------------------------------------------*/

/* Define Test Group. */
TEST_GROUP(TEST_IOT_TIMER);

/*-----------------------------------------------------------*/

/**
 * @brief Setup function called before each test in this group is executed.
 */
TEST_SETUP( TEST_IOT_TIMER )
{
}

/*-----------------------------------------------------------*/

/**
 * @brief Tear down function called after each test in this group is executed.
 */
TEST_TEAR_DOWN( TEST_IOT_TIMER )
{
}

/*-----------------------------------------------------------*/
static void prvTimerCallback( void *pvUserContext )
{
    BaseType_t xHigherPriorityTaskWoken;
    (void) pvUserContext;

    xSemaphoreGiveFromISR(xtestIotTimerSemaphore, &xHigherPriorityTaskWoken);
}

/*-----------------------------------------------------------*/


/**
 * @brief Function to define which tests to execute as part of this group.
 */
TEST_GROUP_RUNNER( TEST_IOT_TIMER )
{
    xtestIotTimerSemaphore = xSemaphoreCreateBinary();
    TEST_ASSERT_NOT_EQUAL(NULL, xtestIotTimerSemaphore);

    RUN_TEST_CASE(TEST_IOT_TIMER, AFWP_IotTimer_OpenClose);
    RUN_TEST_CASE(TEST_IOT_TIMER, AFWP_IotTimer_Running);
    RUN_TEST_CASE(TEST_IOT_TIMER, AFWP_IotTimer_Stop);
    RUN_TEST_CASE(TEST_IOT_TIMER, AFWP_IotTimer_Callback);
    RUN_TEST_CASE(TEST_IOT_TIMER, AFWP_IotTimer_NoCallback);
    RUN_TEST_CASE(TEST_IOT_TIMER, AFWP_IotTimer_StartAgain);
//    RUN_TEST_CASE(TEST_IOT_TIMER, AFWP_IotTimer_Cancel);
    RUN_TEST_CASE(TEST_IOT_TIMER, AFWP_IotTimer_OpenFuzzing);
    RUN_TEST_CASE(TEST_IOT_TIMER, AFWP_IotTimer_CloseFuzzing);
    RUN_TEST_CASE(TEST_IOT_TIMER, AFWP_IotTimer_StartFuzzing);
    RUN_TEST_CASE(TEST_IOT_TIMER, AFWP_IotTimer_GetFuzzing);
    RUN_TEST_CASE(TEST_IOT_TIMER, AFWP_IotTimer_StopFuzzing);
    RUN_TEST_CASE(TEST_IOT_TIMER, AFWP_IotTimer_CallbackFuzzing);
    RUN_TEST_CASE(TEST_IOT_TIMER, AFWP_IotTimer_CancelFuzzing);
    RUN_TEST_CASE(TEST_IOT_TIMER, AFWP_IotTimer_DelayFuzzing);
    RUN_TEST_CASE(TEST_IOT_TIMER, AFWP_IotTimer_RepeatedCallback);
    RUN_TEST_CASE(TEST_IOT_TIMER, AFWP_IotTimer_FreeRunning);
    RUN_TEST_CASE(TEST_IOT_TIMER, AFWP_IotTimer_AllInstance);
}

/*-----------------------------------------------------------*/

/*Main function to invoke the tests*/
int timer_test()
{
    UNITY_BEGIN();
    TEST_SETUP(TEST_IOT_TIMER);
    RUN_TEST_GROUP(TEST_IOT_TIMER);
    //RUN_TEST_CASE(TEST_IOT_TIMER, AFWP_IotTimer_OpenClose);
    return UNITY_END();
}

/**
 * @brief Test Function to test iot_timer_open and iot_timer_close.
 *
 */
TEST( TEST_IOT_TIMER, AFWP_IotTimer_OpenClose )
{
    IotTimerHandle_t xTimerHandle;
    int32_t lRetVal;

    /* Open timer to initialize hardware. */
    xTimerHandle = iot_timer_open(ltestIotTimerInstance);
    TEST_ASSERT_NOT_EQUAL(NULL, xTimerHandle);

    lRetVal = iot_timer_close(xTimerHandle);
    TEST_ASSERT_EQUAL(0, lRetVal);

    /* Set the global TimerHandle to NULL as we closed the handle */
    gIotTimerHandle = NULL;
}

/*-----------------------------------------------------------*/

/**
 * @brief Test Function to test delay mode
 *
 */
TEST( TEST_IOT_TIMER, AFWP_IotTimer_Running )
{
    IotTimerHandle_t xTimerHandle;
    int32_t lRetVal;
    uint32_t ulMicroSeconds1 = 0;
    uint32_t ulMicroSeconds2 = 0;
    uint32_t ulTaskDelayTicks = testIotTIMER_SLEEP_DELAY_TICKS;
    uint32_t ulTaskDelayMs = testIotTIMER_SLEEP_DELAY_MS;
    uint32_t const ulTimerConfigMs = 5 * 1000;

    /* Open timer to initialize hardware. */
    xTimerHandle = iot_timer_open(ltestIotTimerInstance);
    TEST_ASSERT_NOT_EQUAL(NULL, xTimerHandle);

    if ( TEST_PROTECT() )
    {
        /* Set up the callback */
        iot_timer_set_callback(xTimerHandle, prvTimerCallback, NULL);

        /* Set up the timer for a delay of a few seconds  */
        lRetVal = iot_timer_delay(xTimerHandle, ulTimerConfigMs * 1000);

        /* Start the timer */
        lRetVal = iot_timer_start(xTimerHandle);
        TEST_ASSERT_EQUAL(0, lRetVal);

        /* Task delay for a few OS ticks */
        vTaskDelay(ulTaskDelayTicks);

        /* Get the time in micro seconds */
        lRetVal = iot_timer_get_value(xTimerHandle, &ulMicroSeconds1);
        TEST_ASSERT_EQUAL(0, lRetVal);

        /* Make sure MicroSeconds returned is not '0' */
        TEST_ASSERT_NOT_EQUAL(0, ulMicroSeconds1);

        /* Task delay for a few OS ticks. */
        vTaskDelay(ulTaskDelayTicks);

        /* Get the time in micro seconds */
        lRetVal = iot_timer_get_value(xTimerHandle, &ulMicroSeconds2);
        TEST_ASSERT_EQUAL(0, lRetVal);

        /*
         * the get_value API returns the remaining time, convert it to
         * elapsed time
         */
        ulMicroSeconds1 = (ulTimerConfigMs * 1000) - ulMicroSeconds1;
        ulMicroSeconds2 = (ulTimerConfigMs * 1000) - ulMicroSeconds2;


        /* Make sure second value is greater than origin + delay within 2msec
         * range (worst case scenario with 1ms resolution and two sleeps)  */
        TEST_ASSERT_GREATER_THAN_UINT32(
                (ulMicroSeconds1 + ((ulTaskDelayMs - 2) * 1000)), ulMicroSeconds2);
        TEST_ASSERT_GREATER_THAN_UINT32(ulMicroSeconds2,
                (ulMicroSeconds1 + ((ulTaskDelayMs + 2) * 1000)));

        /*
         * we are not waiting for the callback function since
         * the timer is setup for 5 seconds and do not expect
         * to fire the callback yet
         */

        /* Stop the timer */
        lRetVal = iot_timer_stop(xTimerHandle);

        if ( lRetVal != -ENOSYS )
        {
            TEST_ASSERT_EQUAL(0, lRetVal);
        }
    }

    lRetVal = iot_timer_close(xTimerHandle);
    TEST_ASSERT_EQUAL(0, lRetVal);
}

/*-----------------------------------------------------------*/

/**
 * @brief Test Function to test freerunning mode
 *
 */
TEST( TEST_IOT_TIMER, AFWP_IotTimer_FreeRunning )
{
    IotTimerHandle_t xTimerHandle;
    int32_t lRetVal;
    uint32_t ulMicroSeconds1 = 0;
    uint32_t ulMicroSeconds2 = 0;
    uint32_t ulTaskDelayTicks = testIotTIMER_SLEEP_DELAY_TICKS;
    uint32_t FreeRunningPeriod = 0xFFFFFFFF;

    /* Open timer to initialize hardware. */
    xTimerHandle = iot_timer_open(ltestIotTimerInstance);
    TEST_ASSERT_NOT_EQUAL(NULL, xTimerHandle);

    if ( TEST_PROTECT() )
    {
        /* Set up the callback */
        iot_timer_set_callback(xTimerHandle, prvTimerCallback, NULL);

        /* Set up the timer for a delay of a few seconds  */
        lRetVal = iot_timer_delay(xTimerHandle, FreeRunningPeriod);

        /* Start the timer */
        lRetVal = iot_timer_start(xTimerHandle);
        TEST_ASSERT_EQUAL(0, lRetVal);

        /* Task delay for a few OS ticks */
        osal_delay_ms(pdMS_TO_TICKS(ulTaskDelayTicks));

        /* Get the time in micro seconds */
        lRetVal = iot_timer_get_value(xTimerHandle, &ulMicroSeconds1);
        TEST_ASSERT_EQUAL(0, lRetVal);

        /* Make sure MicroSeconds returned is not '0' */
        TEST_ASSERT_NOT_EQUAL(0, ulMicroSeconds1);

        /* Task delay for a few OS ticks. */
        osal_delay_ms(pdMS_TO_TICKS(ulTaskDelayTicks));

        /* Get the time in micro seconds */
        lRetVal = iot_timer_get_value(xTimerHandle, &ulMicroSeconds2);
        TEST_ASSERT_EQUAL(0, lRetVal);

        /*
         * the get_value API returns the remaining time
         * Make sure first value is greater than the second value
         */
        TEST_ASSERT_GREATER_THAN_UINT32(ulMicroSeconds2, ulMicroSeconds1);

        /* Stop the timer */
        lRetVal = iot_timer_stop(xTimerHandle);
        TEST_ASSERT_EQUAL(0, lRetVal);
    }

    lRetVal = iot_timer_close(xTimerHandle);
    TEST_ASSERT_EQUAL(0, lRetVal);
}

/*-----------------------------------------------------------*/

/**
 * @brief Test Function to test all timer instance running in delay mode.
 *
 */
TEST( TEST_IOT_TIMER, AFWP_IotTimer_AllInstance )
{
    IotTimerHandle_t xTimerHandle;
    int32_t lRetVal;
    uint32_t ulMicroSeconds1 = 0;
    uint32_t ulMicroSeconds2 = 0;
    uint32_t ulTaskDelayTicks = testIotTIMER_SLEEP_DELAY_TICKS;
    uint32_t const ulTimerConfigMs = 5 * 1000;
    uint32_t i;
    uint32_t ulTaskDelayMs = testIotTIMER_SLEEP_DELAY_MS;
    if ( TEST_PROTECT() )
    {
        for(i=0;i<(sizeof(TimerInstance_t)/sizeof(int));i++)
        {
            /* Open timer to initialize hardware. */
            xTimerHandle = iot_timer_open(i);
            TEST_ASSERT_NOT_EQUAL(NULL, xTimerHandle);

            /* Set up the callback */
            iot_timer_set_callback(xTimerHandle, prvTimerCallback, NULL);

            /* Set up the timer for a delay of a few seconds  */
            lRetVal = iot_timer_delay(xTimerHandle, ulTimerConfigMs * 1000);

            /* Start the timer */
            lRetVal = iot_timer_start(xTimerHandle);
            TEST_ASSERT_EQUAL(0, lRetVal);

            /* Task delay for a few OS ticks */
            vTaskDelay(ulTaskDelayTicks);

            /* Get the time in micro seconds */
            lRetVal = iot_timer_get_value(xTimerHandle, &ulMicroSeconds1);
            TEST_ASSERT_EQUAL(0, lRetVal);

            /* Make sure MicroSeconds returned is not '0' */
            TEST_ASSERT_NOT_EQUAL(0, ulMicroSeconds1);

            /* Task delay for a few OS ticks. */
            vTaskDelay(ulTaskDelayTicks);

            /* Get the time in micro seconds */
            lRetVal = iot_timer_get_value(xTimerHandle, &ulMicroSeconds2);
            TEST_ASSERT_EQUAL(0, lRetVal);

            /*
             * the get_value API returns the remaining time, convert it to
             * elapsed time
             */
            ulMicroSeconds1 = (ulTimerConfigMs * 1000) - ulMicroSeconds1;
            ulMicroSeconds2 = (ulTimerConfigMs * 1000) - ulMicroSeconds2;


            /* Make sure second value is greater than origin + delay within 2msec
             * range (worst case scenario with 1ms resolution and two sleeps)  */
            TEST_ASSERT_GREATER_THAN_UINT32(
                    (ulMicroSeconds1 + ((ulTaskDelayMs - 2) * 1000)), ulMicroSeconds2);
            TEST_ASSERT_GREATER_THAN_UINT32(ulMicroSeconds2,
                    (ulMicroSeconds1 + ((ulTaskDelayMs + 2) * 1000)));

            /*
             * we are not waiting for the callback function since
             * the timer is setup for 5 seconds and do not expect
             * to fire the callback yet
             */

            /* Stop the timer */
            lRetVal = iot_timer_stop(xTimerHandle);

            if ( lRetVal != -ENOSYS )
            {
                TEST_ASSERT_EQUAL(0, lRetVal);
            }

            lRetVal = iot_timer_close(xTimerHandle);
            TEST_ASSERT_EQUAL(0, lRetVal);
        }
    }

}

/*-----------------------------------------------------------*/

/**
 * @brief Test Function to test stopping timer
 *
 */
TEST( TEST_IOT_TIMER, AFWP_IotTimer_Stop )
{
    IotTimerHandle_t xTimerHandle;
    int32_t lRetVal;
    uint32_t ulMicroSeconds1 = 0;
    uint32_t ulMicroSeconds2 = 0;
    uint32_t ulTaskDelayTicks = testIotTIMER_SLEEP_DELAY_TICKS;
    uint32_t ulTaskDelayMs = testIotTIMER_SLEEP_DELAY_MS;
    uint32_t const ulTimerConfigMs = 5 * 1000;

    /* Open timer to initialize hardware. */
    xTimerHandle = iot_timer_open(ltestIotTimerInstance);
    TEST_ASSERT_NOT_EQUAL(NULL, xTimerHandle);

    if ( TEST_PROTECT() )
    {
        /* Set up the callback */
        iot_timer_set_callback(xTimerHandle, prvTimerCallback, NULL);

        /* Set up the timer for a delay of a few seconds  */
        lRetVal = iot_timer_delay(xTimerHandle, ulTimerConfigMs * 1000);

        /* Start the timer */
        lRetVal = iot_timer_start(xTimerHandle);
        TEST_ASSERT_EQUAL(0, lRetVal);

        /* Delay for a few  msec. */
        vTaskDelay(ulTaskDelayTicks);

        /* Get the time in micro seconds */
        lRetVal = iot_timer_get_value(xTimerHandle, &ulMicroSeconds1);
        TEST_ASSERT_EQUAL(0, lRetVal);

        /* Make sure MicroSeconds returned is not '0' */
        TEST_ASSERT_NOT_EQUAL(0, ulMicroSeconds1);

        /* Delay for few msec. */
        vTaskDelay(ulTaskDelayTicks);

        /* Get the time in micro seconds */
        lRetVal = iot_timer_get_value(xTimerHandle, &ulMicroSeconds2);
        TEST_ASSERT_EQUAL(0, lRetVal);

        /*
         * the get_value API returns the remaining time, convert it to
         * elapsed time
         */
        ulMicroSeconds1 = (ulTimerConfigMs * 1000) - ulMicroSeconds1;
        ulMicroSeconds2 = (ulTimerConfigMs * 1000) - ulMicroSeconds2;

        /* Make sure second value is greater than origin + delay within 2msec
         * range (worst case scenario with 1ms resolution and two sleeps)  */
        TEST_ASSERT_GREATER_THAN_UINT32(
                (ulMicroSeconds1 + ((ulTaskDelayMs  - 2) * 1000)), ulMicroSeconds2);
        TEST_ASSERT_GREATER_THAN_UINT32(ulMicroSeconds2,
                (ulMicroSeconds1 + ((ulTaskDelayMs + 2) * 1000)));

        /* Stop the timer */
        lRetVal = iot_timer_stop(xTimerHandle);

        if ( lRetVal != -ENOSYS )
        {
            TEST_ASSERT_EQUAL(0, lRetVal);

            /* Get the time in micro seconds */
            lRetVal = iot_timer_get_value(xTimerHandle, &ulMicroSeconds1);
            TEST_ASSERT_EQUAL(-EPERM, lRetVal);
        }
    }

    lRetVal = iot_timer_close(xTimerHandle);
    TEST_ASSERT_EQUAL(0, lRetVal);
}
/*-----------------------------------------------------------*/

/**
 * @brief Test Function to test iot_timer_delay
 * and verify that callback is called correctly.
 */
TEST( TEST_IOT_TIMER, AFWP_IotTimer_Callback )
{
    IotTimerHandle_t xTimerHandle;
    uint32_t ulMicroSeconds;
    int32_t lRetVal;

    /* Open timer to initialize hardware. */
    xTimerHandle = iot_timer_open(ltestIotTimerInstance);
    TEST_ASSERT_NOT_EQUAL(NULL, xTimerHandle);

    if ( TEST_PROTECT() )
    {
        /* Set up the callback */
        iot_timer_set_callback(xTimerHandle, prvTimerCallback, NULL);

        /* Set up the delay */
        lRetVal = iot_timer_delay(xTimerHandle,
                testIotTIMER_DEFAULT_DELAY_US);

        /* Expect either "Not running" or "Success" */
        if ( lRetVal != -EPERM )
        {
            TEST_ASSERT_EQUAL(0, lRetVal);
        }

        /* Start the timer */
        lRetVal = iot_timer_start(xTimerHandle);
        TEST_ASSERT_EQUAL(0, lRetVal);

        /* Get the time in micro seconds */
        lRetVal = iot_timer_get_value(xTimerHandle, &ulMicroSeconds);
        TEST_ASSERT_EQUAL(0, lRetVal);

        /* Wait for the Delay callback to be called. */
        lRetVal = xSemaphoreTake(xtestIotTimerSemaphore,
                testIotTIMER_CALLBACK_DELAY_TICKS);
        TEST_ASSERT_EQUAL(pdTRUE, lRetVal);

        /* Get the time in micro seconds
         * Make sure the current micro-seconds value is less than the delay
         */
        lRetVal = iot_timer_get_value(xTimerHandle, &ulMicroSeconds);
        TEST_ASSERT_EQUAL(0, lRetVal);
        TEST_ASSERT_LESS_THAN_UINT32(testIotTIMER_DEFAULT_DELAY_US,
                ulMicroSeconds);
    }

    lRetVal = iot_timer_close(xTimerHandle);
    TEST_ASSERT_EQUAL(0, lRetVal);
}

/*-----------------------------------------------------------*/

/**
 * @brief Test timer repeated open-start-stop-close with callback
 */
TEST( TEST_IOT_TIMER, AFWP_IotTimer_RepeatedCallback )
{
    IotTimerHandle_t xTimerHandle;
    uint32_t ulMicroSeconds1;
    uint32_t ulMicroSeconds2;
    int32_t lRetVal;
    uint32_t testIotTIMER_DELAY_US = 80000;
    int i,j;

    if ( TEST_PROTECT() )
    {
        for(i=0;i<5;i++)
        {
            /* Open timer to initialize hardware. */
            xTimerHandle = iot_timer_open(ltestIotTimerInstance);
            TEST_ASSERT_NOT_EQUAL(NULL, xTimerHandle);

            for(j=0;j<5;j++)
            {
                /* Set up the callback */
                iot_timer_set_callback(xTimerHandle, prvTimerCallback, NULL);

                /* Set up the delay */
                lRetVal = iot_timer_delay(xTimerHandle,
                        testIotTIMER_DELAY_US);

                /* Expect either "Not running" or "Success" */
                if ( lRetVal != -EPERM )
                {
                    TEST_ASSERT_EQUAL(0, lRetVal);
                }

                /* Start the timer */
                lRetVal = iot_timer_start(xTimerHandle);
                TEST_ASSERT_EQUAL(0, lRetVal);

                /* Get the time in micro seconds */
                lRetVal = iot_timer_get_value(xTimerHandle, &ulMicroSeconds1);
                TEST_ASSERT_EQUAL(0, lRetVal);

                /* Wait for 2ms less than the delay set. */
                osal_delay_ms(((testIotTIMER_DELAY_US/1000)-2));

                /* Poll the semaphore. */
                xSemaphoreTake(xtestIotTimerSemaphore, 0);
                lRetVal = xSemaphoreTake(xtestIotTimerSemaphore, 0);
                TEST_ASSERT_EQUAL(pdFALSE, lRetVal);

                /* Wait for the Delay callback to be called. */
                lRetVal = xSemaphoreTake(xtestIotTimerSemaphore, testIotTIMER_CALLBACK_DELAY_TICKS);
                TEST_ASSERT_EQUAL(pdTRUE, lRetVal);

                /* Get the time in micro seconds
                 * Make sure the current micro-seconds value is less than the delay
                 */
                lRetVal = iot_timer_get_value(xTimerHandle, &ulMicroSeconds2);
                TEST_ASSERT_EQUAL(0, lRetVal);

                /*
                 * the get_value API returns the remaining time, convert it to
                 * elapsed time
                 */
                ulMicroSeconds1 = (testIotTIMER_DELAY_US * 1000) - ulMicroSeconds1;
                ulMicroSeconds2 = (testIotTIMER_DELAY_US * 1000) - ulMicroSeconds2;

                TEST_ASSERT_LESS_THAN_UINT32(testIotTIMER_DELAY_US,
                        ulMicroSeconds2-ulMicroSeconds1);

                /* Stop the timer */
                lRetVal = iot_timer_stop(xTimerHandle);
                TEST_ASSERT_EQUAL(0, lRetVal);
            }

            lRetVal = iot_timer_close(xTimerHandle);
            TEST_ASSERT_EQUAL(0, lRetVal);
            testIotTIMER_DELAY_US += 10000;
        }
    }

}

/**
 * @brief Test Function to test no callback when timer stopped
 */
TEST( TEST_IOT_TIMER, AFWP_IotTimer_NoCallback )
{
    IotTimerHandle_t xTimerHandle;
    uint32_t ulMicroSeconds;
    int32_t lRetVal;

    /* Open timer to initialize hardware. */
    xTimerHandle = iot_timer_open(ltestIotTimerInstance);
    TEST_ASSERT_NOT_EQUAL(NULL, xTimerHandle);

    if ( TEST_PROTECT() )
    {
        /* Set up the delay */
        lRetVal = iot_timer_delay(xTimerHandle,
                testIotTIMER_DEFAULT_DELAY_US);

        /* Expect either "Not running" or "Success" */
        if ( lRetVal != -EPERM )
        {
            TEST_ASSERT_EQUAL(0, lRetVal);
        }

        /* Start the timer */
        lRetVal = iot_timer_start(xTimerHandle);
        TEST_ASSERT_EQUAL(0, lRetVal);

        /* Get the time in micro seconds */
        lRetVal = iot_timer_get_value(xTimerHandle, &ulMicroSeconds);
        TEST_ASSERT_EQUAL(0, lRetVal);

        /* Stop the timer */
        lRetVal = iot_timer_stop(xTimerHandle);

        if ( lRetVal != -EINVAL )
        {
            /* Wait for the Delay.  Make sure callback is not called. */
            lRetVal = xSemaphoreTake(xtestIotTimerSemaphore,
                    testIotTIMER_CALLBACK_DELAY_TICKS);
            TEST_ASSERT_NOT_EQUAL(pdTRUE, lRetVal);
        }
    }

    lRetVal = iot_timer_close(xTimerHandle);
    TEST_ASSERT_EQUAL(0, lRetVal);
}

/**
 * @brief Test Function to test timer stopped, then started again
 */
TEST( TEST_IOT_TIMER, AFWP_IotTimer_StartAgain )
{
    IotTimerHandle_t xTimerHandle;
    uint32_t ulMicroSeconds;
    int32_t lRetVal;

    /* Open timer to initialize hardware. */
    xTimerHandle = iot_timer_open(ltestIotTimerInstance);
    TEST_ASSERT_NOT_EQUAL(NULL, xTimerHandle);

    if ( TEST_PROTECT() )
    {
        /* Set up the callback */
        iot_timer_set_callback(xTimerHandle, prvTimerCallback, NULL);

        /* Set up the delay */
        lRetVal = iot_timer_delay(xTimerHandle,
                testIotTIMER_DEFAULT_DELAY_US);

        /* Expect either "Not running" or "Success" */
        if ( lRetVal != -EPERM )
        {
            TEST_ASSERT_EQUAL(0, lRetVal);
        }

        /* Start the timer */
        lRetVal = iot_timer_start(xTimerHandle);
        TEST_ASSERT_EQUAL(0, lRetVal);

        /* Get the time in micro seconds */
        lRetVal = iot_timer_get_value(xTimerHandle, &ulMicroSeconds);
        TEST_ASSERT_EQUAL(0, lRetVal);

        /* Stop the timer */
        lRetVal = iot_timer_stop(xTimerHandle);

        if ( lRetVal != -EINVAL )
        {
            lRetVal = xSemaphoreTake(xtestIotTimerSemaphore,
                    testIotTIMER_CALLBACK_DELAY_TICKS);
            TEST_ASSERT_EQUAL(pdTRUE, lRetVal);

            /* Set up the delay again - the stop clears the timer
             * configuration */
            lRetVal = iot_timer_delay(xTimerHandle,
                    testIotTIMER_DEFAULT_DELAY_US);

            TEST_ASSERT_EQUAL(0, lRetVal);

            /* Start the timer again*/
            lRetVal = iot_timer_start(xTimerHandle);
            TEST_ASSERT_EQUAL(0, lRetVal);

            /* Wait for the Delay.  Make sure callback is called. */
            lRetVal = xSemaphoreTake(xtestIotTimerSemaphore,
                    testIotTIMER_CALLBACK_DELAY_TICKS);
            TEST_ASSERT_EQUAL(pdTRUE, lRetVal);
        }
    }

    lRetVal = iot_timer_close(xTimerHandle);
    TEST_ASSERT_EQUAL(0, lRetVal);
}

/**
 * @brief Test Function to test timer cancel
 */
TEST( TEST_IOT_TIMER, AFWP_IotTimer_Cancel )
{
    IotTimerHandle_t xTimerHandle;
    int32_t lRetVal;

    /* Open timer to initialize hardware. */
    xTimerHandle = iot_timer_open(ltestIotTimerInstance);
    TEST_ASSERT_NOT_EQUAL(NULL, xTimerHandle);

    if ( TEST_PROTECT() )
    {
        /* Set up the callback */
        iot_timer_set_callback(xTimerHandle, prvTimerCallback, NULL);

        /* Set up the delay */
        lRetVal = iot_timer_delay(xTimerHandle,
        testIotTIMER_DEFAULT_DELAY_US);

        /* Expect either "Not running" or "Success" */
        if ( lRetVal != -EPERM )
        {
            TEST_ASSERT_EQUAL(0, lRetVal);
        }

        /* Start the timer */
        lRetVal = iot_timer_start(xTimerHandle);
        TEST_ASSERT_EQUAL(0, lRetVal);

        /* Cancel the timer */
        lRetVal = iot_timer_cancel(xTimerHandle);

        if ( lRetVal != -ENOSYS )
        {
            TEST_ASSERT_EQUAL(0, lRetVal);

            /* Wait for the Delay.  Make sure callback is not called. */
            lRetVal = xSemaphoreTake(xtestIotTimerSemaphore,
                    testIotTIMER_CALLBACK_DELAY_TICKS);
            TEST_ASSERT_NOT_EQUAL(pdTRUE, lRetVal);
        }
    }

    lRetVal = iot_timer_close(xTimerHandle);
    TEST_ASSERT_EQUAL(0, lRetVal);
}

/**
 * @brief Test Function to test fuzzing of open call
 */
TEST( TEST_IOT_TIMER, AFWP_IotTimer_OpenFuzzing )
{
    IotTimerHandle_t xTimerHandle;

    /* Open timer to initialize hardware with invalid instance. */
    xTimerHandle = iot_timer_open(-1);
    TEST_ASSERT_EQUAL(NULL, xTimerHandle);
}

/**
 * @brief Test Function to test fuzzing of close call
 */
TEST( TEST_IOT_TIMER, AFWP_IotTimer_CloseFuzzing )
{
    IotTimerHandle_t xTimerHandle;
    int32_t lRetVal;

    /* Close timer with NULL handle */
    lRetVal = iot_timer_close(NULL);
    TEST_ASSERT_EQUAL(-EINVAL, lRetVal);

    /* Open timer to initialize hardware. */
    xTimerHandle = iot_timer_open(ltestIotTimerInstance);
    TEST_ASSERT_NOT_EQUAL(NULL, xTimerHandle);

    /* Close once with valid handle */
    lRetVal = iot_timer_close(xTimerHandle);
    TEST_ASSERT_EQUAL(0, lRetVal);

    /* Close second time with stale handle */
    lRetVal = iot_timer_close(xTimerHandle);
    TEST_ASSERT_EQUAL(-EINVAL, lRetVal);
}

/**
 * @brief Test Function to test fuzzing of start call
 */
TEST( TEST_IOT_TIMER, AFWP_IotTimer_StartFuzzing )
{
    int32_t lRetVal;

    /* Call Start Timer with invalid handle */
    lRetVal = iot_timer_start(NULL);
    TEST_ASSERT_EQUAL(-EINVAL, lRetVal);
}

/**
 * @brief Test Function to test fuzzing of get call
 */
TEST( TEST_IOT_TIMER, AFWP_IotTimer_GetFuzzing )
{
    IotTimerHandle_t xTimerHandle;
    int32_t lRetVal;
    uint32_t ulMicroSeconds1 = 0;

    /* Get the time in micro seconds, with invalid Handle*/
    lRetVal = iot_timer_get_value(NULL, &ulMicroSeconds1);
    TEST_ASSERT_EQUAL(-EINVAL, lRetVal);

    /* Open timer to initialize hardware. */
    xTimerHandle = iot_timer_open(ltestIotTimerInstance);
    TEST_ASSERT_NOT_EQUAL(NULL, xTimerHandle);

    if ( TEST_PROTECT() )
    {
        /* Get the time in micro seconds, with valid handle but NULL buffer */
        lRetVal = iot_timer_get_value(xTimerHandle, NULL);
        TEST_ASSERT_EQUAL(-EINVAL, lRetVal);
    }

    /* Close once with valid handle */
    lRetVal = iot_timer_close(xTimerHandle);
    TEST_ASSERT_EQUAL(0, lRetVal);
}

/**
 * @brief Test Function to test fuzzing of stop call
 */
TEST( TEST_IOT_TIMER, AFWP_IotTimer_StopFuzzing )
{
    int32_t lRetVal;

    /* Call Stop Timer with invalid handle */
    lRetVal = iot_timer_stop(NULL);

    TEST_ASSERT_EQUAL(-EINVAL, lRetVal);
}

/**
 * @brief Test Function to test fuzzing of set_callback call
 */
TEST( TEST_IOT_TIMER, AFWP_IotTimer_CallbackFuzzing )
{
    IotTimerHandle_t xTimerHandle;
    int32_t lRetVal;

    /* Call set_callback with NULL handle */
    iot_timer_set_callback(NULL, prvTimerCallback, NULL);

    /* Open timer to initialize hardware. */
    xTimerHandle = iot_timer_open(ltestIotTimerInstance);
    TEST_ASSERT_NOT_EQUAL(NULL, xTimerHandle);

    /* Call set_callback with NULL handle */
    iot_timer_set_callback(xTimerHandle, NULL, NULL);

    /* Close once with valid handle */
    lRetVal = iot_timer_close(xTimerHandle);
    TEST_ASSERT_EQUAL(0, lRetVal);
}

/**
 * @brief Test Function to test fuzzing of cancel call
 */
TEST( TEST_IOT_TIMER, AFWP_IotTimer_CancelFuzzing )
{
    int32_t lRetVal;

    /* Call Cancel Timer with invalid handle */
    lRetVal = iot_timer_cancel(NULL);

    TEST_ASSERT_EQUAL(-EINVAL, lRetVal);
}

/**
 * @brief Test Function to test fuzzing of delay call
 */
TEST( TEST_IOT_TIMER, AFWP_IotTimer_DelayFuzzing )
{
    int32_t lRetVal;

    /* Call Cancel Timer with invalid handle */
    lRetVal = iot_timer_delay(NULL, testIotTIMER_DEFAULT_DELAY_US);
    TEST_ASSERT_EQUAL(-EINVAL, lRetVal);
}
