/*
 * Common IO - basic V1.0.0
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

/*
 * SPDX-FileCopyrightText: Copyright (C) 2025 Altera Corporation
 *
 * SPDX-License-Identifier: MIT-0
 */

#ifndef __SOCFPGA_TIMER_H__
#define __SOCFPGA_TIMER_H__

/**
 * @file socfpga_timer.h
 * @brief Header file for timer HAL driver
 */

#include "socfpga_defines.h"

/**
 * @defgroup timer Timer
 * @ingroup drivers
 * @{
 */

/**
 * @defgroup timer_fns Functions
 * @ingroup timer
 * Timer HAL APIs
 */

/**
 * @defgroup timer_structs Structures 
 * @ingroup timer
 * Timer Specific Structures
 */

/**
 * @defgroup timer_enums Enumerations
 * @ingroup timer
 * Timer Specific Enumerations
 */

/**
 * @defgroup timer_macros Macros
 * @ingroup timer
 * Timer Specific Macros 
 */

/**
 * @addtogroup timer_macros
 * @{
 */
#define TIMER_MAX_INSTANCE         4 /*!<Number of instances for timer*/
/**
 * @}
 */

/**
 * @brief The timer descriptor type defined in the source file.
 * @ingroup timer_structs
 */
struct TimerDescriptor;

/**
 * @brief Timer handle returned by calling timer_open()
 * @ingroup timer_structs
 */
typedef struct TimerDescriptor *Timer_Handle_t;

/**
 * @brief Function pointer for user callback
 * @ingroup timer_fns
 */
typedef void (*TimerCallback_t)( void *ulBuff );

/**
 * @brief enum for timer instances
 * @ingroup timer_enums
 */
typedef enum
{
    eSocfpgaOsc1Timer0 = 0, /*!<Peripheral ID for SYS_TIMER instance 0*/
    eSocfpgaOsc1Timer1, /*!<Peripheral ID for SYS_TIMER instance 1*/
    eSocfpgaSpTimer0, /*!<Peripheral ID for TIMER instance 0*/
    eSocfpgaSpTimer1, /*!<Peripheral ID for TIMER instance 1*/
    eSocfpgaMaxTimerInstance /*!<Number of timer instances*/
} TimerInstance_t;

/**
 * @addtogroup timer_fns
 * @{
 */

/**
 * @brief  Used to initialize the timer
 *     Once a instance is opened, it needs to be closed before invoking open again.
 * 
 *
 * @param[in]  xTimerInstance - The Timer instance to open.
 *
 * @return
 * - Timer handle on success
 * - NULL if
 *      - Invalid timer instance
 *      - If same instance already opened
 *      - Failed to obtain clock
 *      - Failed to enable interrupt
 */
Timer_Handle_t timer_open( TimerInstance_t xTimerInstance );

/**
 * @brief   Read the current counter value register
 *
 * @param[in]  pxTimerPeripheral - Timer handle returned by open API
 * @param[out] ulCounterVal - Counter value at given instance
 *
 * @return
 * - 0 on success
 * - -EINVAL if timer handle is NULL
 * - -EPERM if timer is not running
 */
int32_t timer_get_value_raw( Timer_Handle_t const pxTimerPeripheral,
        uint32_t *ulCounterVal );

/**
 * @brief   Start the timer instance.
 *
 * @param[in]  pxTimerPeripheral - Timer handle returned by open API
 *
 * @return
 * - 0 on success
 * - -EINVAL if timer handle is NULL
 * - -EBUSY if timer is already running
 */
int32_t timer_start( Timer_Handle_t const pxTimerPeripheral );

/**
 * @brief   Stop the timer instance.
 *
 * @param[in]  pxTimerPeripheral - Timer handle returned by open API
 *
 * @return
 * - 0 on success
 * - -EINVAL if timer handle is NULL
 * - -EPERM if timer is not running
 * - -EFAULT if failed to disable interrupt
 */
int32_t timer_stop( Timer_Handle_t const pxTimerPeripheral );

/**
 * @brief   Close the timer instance.
 *
 * @param[in]  pxTimerPeripheral - Timer handle returned by open API
 *
 * @return
 * - 0 on success
 * - -EINVAL if timer handle is NULL
 */
int32_t timer_close( Timer_Handle_t const pxTimerPeripheral );

/**
 * @brief    Configure the timer instance in free running or in user mode.
 *
 * @param[in]  pxTimerPeripheral - Timer handle returned by open API
 *
 * @return
 * - 0 on success
 * - -EINVAL if timer handle is NULL or period is too long
 * - -EBUSY if timer already running
 */
int32_t timer_set_period_us( Timer_Handle_t const pxTimerPeripheral,
        uint32_t period );

/**
 * @brief    Get remaining time in microseconds.
 *
 * In delay mode (user defined mode) this is the remaining time for next
 * interrupt. In free-running mode, this is the remaining time for next
 * roll over.
 *
 * @param[in]  pxTimerPeripheral - Timer handle returned by open API
 * @param[out] ulMicroSeconds - Remaining time for timer overflow
 * @return
 * - 0 on success
 * - -EINVAL if timer handle is NULL or ulMicroSeconds is NULL
 * - -EPERM if timer is not running
 */
int32_t timer_get_value_us( Timer_Handle_t const pxTimerPeripheral,
        uint32_t *ulMicroSeconds );

/**
 * @brief   Set the callback function
 *
 * Sets the callback function that is invoked on timer expiry. This
 * will be invoked periodically until the timer is stopped.
 *
 * @param[in]  pxTimerPeripheral - Timer handle returned by open API
 * @param[in] TimerCallback_t - Call back function pointer
 * @param[in] pvUserContext - User defined context variable.
 *
 * @return
 * - 0 on success
 * - -EINVAL if timer handle is NULL
 */
int32_t timer_set_callback( Timer_Handle_t const pxTimerPeripheral,
        TimerCallback_t pxCallback, void *pvUserContext );

/**
 * @}
 */
/* end of group timer_fns */
/**
 * @}
 */
/* end of group timer */
#endif
