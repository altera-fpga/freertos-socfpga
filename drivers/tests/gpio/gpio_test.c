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
 * @file test_iot_gpio.c
 * @brief Functional Unit Test - GPIO
 *******************************************************************************
 */

/* Test includes */
#include "unity.h"
#include "unity_fixture.h"

/* GPIO driver include */
#include "socfpga_gpio.h"

/* FreeRTOS includes */
#include "FreeRTOS.h"
#include "semphr.h"
#include "gpio_test.h"
#include <errno.h>

/*-----------------------------------------------------------*/
#define testIotGPIO_TEST_PORT_MASK     ( 0x00FF )
#define testIotGPIO_TEST_DIR_BIT       ( 8 )
#define testIotGPIO_TEST_DIR_MASK      ( 1 << testIotGPIO_TEST_DIR_BIT )
#define testIotGPIO_TEST_IRQ_BIT       ( 9 )
#define testIotGPIO_TEST_IRQ_MASK      ( 1 << testIotGPIO_TEST_IRQ_BIT )
#define testIotGPIO_TEST_VALUE_BIT     ( 10 )
#define testIotGPIO_TEST_VALUE_MASK    ( 1 << testIotGPIO_TEST_VALUE_BIT )
#define testIotGPIO_TEST_READ_DELAY    ( 50 )
/*-----------------------------------------------------------*/

/*-----------------------------------------------------------*/

/* Globals values which can be overwritten by the test
 * framework invoking these tests */
/*-----------------------------------------------------------*/

int32_t ltestIotGpioPortA = 0;
int32_t ltestIotGpioPortB = 1;
uint32_t ultestIotGpioFunction = 0;

int32_t ltestIotGpioPortInvalid = 12345; //INT_MAX;

/*
 * ustestIotGpioConfig is used for configuring GPIO test
 * user must set this variable for testing different GPIOs
 *
 * default ustestIotGpioConfig = 0 means reading logic GPIO# 0
 *
 * lower 8 bit for port
 * bit 8 for direction
 * bit 9 for IRQ
 * bit 10 for write value 0 or 1
 */
uint16_t ustestIotGpioConfig = 0;
uint32_t ultestIotGpioWaitTime = 500; /* 0.5s */

uint32_t ultestIotGpioSlowSpeed = 0;
uint32_t ultestIotGpioFastSpeed = 1;
/*-----------------------------------------------------------*/
/* Static Globals */
/*-----------------------------------------------------------*/
static SemaphoreHandle_t xtestIotGpioSemaphore = NULL;
static IotGpioHandle_t xtestIotGpioHandleA, xtestIotGpioHandleB;

/* Define Test Group. */
TEST_GROUP(TEST_IOT_GPIO);
/*-----------------------------------------------------------*/

/**
 * @brief Setup function called before each test in this group is executed.
 */
TEST_SETUP( TEST_IOT_GPIO )
{
    xtestIotGpioSemaphore = xSemaphoreCreateBinary();
    TEST_ASSERT_NOT_EQUAL(NULL, xtestIotGpioSemaphore);
}

/*-----------------------------------------------------------*/

/**
 * @brief Tear down function called after each test in this group is executed.
 */
TEST_TEAR_DOWN( TEST_IOT_GPIO )
{
    /* Make sure resources are freed up */
    iot_gpio_close(xtestIotGpioHandleA);
    iot_gpio_close(xtestIotGpioHandleB);

    vSemaphoreDelete(xtestIotGpioSemaphore);
    xtestIotGpioSemaphore = NULL;
}

/*-----------------------------------------------------------*/

/**
 * @brief Function to define which tests to execute as part of this group.
 */
TEST_GROUP_RUNNER( TEST_IOT_GPIO )
{
    RUN_TEST_CASE(TEST_IOT_GPIO, AFQP_IotGpioOpenClose);
    RUN_TEST_CASE(TEST_IOT_GPIO, AFQP_IotGpioIoctlSetGet);
    RUN_TEST_CASE(TEST_IOT_GPIO, AFQP_IotGpioOpenCloseFuzz);
    RUN_TEST_CASE(TEST_IOT_GPIO, AFQP_IotGpioReadWriteSyncFuzz);
    RUN_TEST_CASE(TEST_IOT_GPIO, AFQP_IotGpioIoctlFuzz);
}

/*-----------------------------------------------------------*/

/*Main function to invoke the tests*/
void gpio_test()
{
    UNITY_BEGIN();

    RUN_TEST_GROUP(TEST_IOT_GPIO);

    UNITY_END();
}

/**
 * @brief Function to test iot_gpio_open --> iot_gpio_close.
 */
TEST( TEST_IOT_GPIO, AFQP_IotGpioOpenClose )
{
    int32_t lRetVal;

    xtestIotGpioHandleA = iot_gpio_open(ltestIotGpioPortA);
    TEST_ASSERT_NOT_EQUAL(NULL, xtestIotGpioHandleA);

    lRetVal = iot_gpio_close(xtestIotGpioHandleA);
    TEST_ASSERT_EQUAL(0, lRetVal);

    /* Test close NULL handle */
    lRetVal = iot_gpio_close( NULL);
    TEST_ASSERT_EQUAL(-EINVAL, lRetVal);
}

/*-----------------------------------------------------------*/
/**
 * @brief Function to test iot_gpio_ioctl.
 */
TEST( TEST_IOT_GPIO, AFQP_IotGpioIoctlSetGet )
{
    int32_t lRetVal;
    IotGpioDirection_t eGpioDirectionWrite = eGpioDirectionInput;
    IotGpioDirection_t eGpioDirectionRead;
    IotGpioConfig_t eGpioConfig;
    eGpioConfig = eGpioInterruptLow;
    IotGpioConfig_t eGpioInterruptRead;

    xtestIotGpioHandleA = iot_gpio_open(ltestIotGpioPortA);
    TEST_ASSERT_NOT_EQUAL(NULL, xtestIotGpioHandleA);

    if ( TEST_PROTECT() )
    {
        /* Set the GPIO Direction */
        lRetVal = iot_gpio_ioctl(xtestIotGpioHandleA, eSetGpioDirection,
                &eGpioDirectionWrite);
        TEST_ASSERT_EQUAL(0, lRetVal);

        lRetVal = iot_gpio_ioctl(xtestIotGpioHandleA, eGetGpioDirection,
                &eGpioDirectionRead);
        TEST_ASSERT_EQUAL(0, lRetVal);

        /* Set the GPIO Interrupt */
        lRetVal = iot_gpio_ioctl(xtestIotGpioHandleA, eSetGpioInterrupt,
                &eGpioConfig);

        if ( -EINVAL != lRetVal )
        {
            TEST_ASSERT_EQUAL(0, lRetVal);
        }

        lRetVal = iot_gpio_ioctl(xtestIotGpioHandleA, eGetGpioInterrupt,
                &eGpioInterruptRead);

        if ( -EINVAL != lRetVal )
        {
            TEST_ASSERT_EQUAL(0, lRetVal);
        }
    }

    lRetVal = iot_gpio_close(xtestIotGpioHandleA);
    TEST_ASSERT_EQUAL(0, lRetVal);
}

/*-----------------------------------------------------------*/

/* Below are Fuzzing tests */

/*-----------------------------------------------------------*/

/**
 * @brief Function to test iot_gpio_open --> iot_gpio_close.
 */
TEST( TEST_IOT_GPIO, AFQP_IotGpioOpenCloseFuzz )
{
    int32_t lRetValA, lRetValB;

    xtestIotGpioHandleA = iot_gpio_open(ltestIotGpioPortInvalid);
    TEST_ASSERT_EQUAL(NULL, xtestIotGpioHandleA);

    xtestIotGpioHandleA = iot_gpio_open(-ltestIotGpioPortInvalid);
    TEST_ASSERT_EQUAL(NULL, xtestIotGpioHandleA);

    /* Open GPIO port */
    xtestIotGpioHandleA = iot_gpio_open(ltestIotGpioPortA);
    TEST_ASSERT_NOT_EQUAL(NULL, xtestIotGpioHandleA);

    lRetValA = iot_gpio_close(xtestIotGpioHandleA);
    TEST_ASSERT_EQUAL(0, lRetValA);

    /* Test close NULL handle */
    lRetValB = iot_gpio_close( NULL);
    TEST_ASSERT_EQUAL(-EINVAL, lRetValB);
}

/*-----------------------------------------------------------*/

/**
 * Test to fuzz iot_gpio_read_sync().
 */
TEST( TEST_IOT_GPIO, AFQP_IotGpioReadWriteSyncFuzz )
{
    int32_t lRetValA, lRetValB;
    uint8_t ucValue;

    lRetValA = iot_gpio_read_sync( NULL, &ucValue);
    TEST_ASSERT_EQUAL(-EINVAL, lRetValA);

    lRetValB = iot_gpio_write_sync( NULL, ucValue);
    TEST_ASSERT_EQUAL(-EINVAL, lRetValB);
}

/*-----------------------------------------------------------*/

/**
 * Test to fuzz iot_gpio_ioctl().
 */
TEST( TEST_IOT_GPIO, AFQP_IotGpioIoctlFuzz )
{
    int32_t lRetVal;
    uint8_t ucValue;
    uint8_t invalidEnum = 0xFF;
    lRetVal = iot_gpio_ioctl( NULL, invalidEnum, &ucValue);
    TEST_ASSERT_EQUAL(-EINVAL, lRetVal);
}
