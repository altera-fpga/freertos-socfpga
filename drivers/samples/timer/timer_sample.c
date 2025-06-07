/*
 * SPDX-FileCopyrightText: Copyright (C) 2025 Altera Corporation
 *
 * SPDX-License-Identifier: MIT-0
 *
 * Sample application for SoC FPGA timer
 */

/** 
 * @file timer_sample.c
 * @brief Sample Application for Timer
 */

#include <stdlib.h>
#include <string.h>
#include "osal.h"
#include "osal_log.h"
#include <task.h>
#include "socfpga_timer.h"
#include "timer_sample.h"

/** 
 * @defgroup timer_sample Timer
 * @ingroup samples
 * 
 * Sample Application for Timer
 * @details
 * @subsubsection tim_desc Description
 * This sample application demonstrates the usage of the timer driver
 * in single shot mode. It configures a timer to trigger an interrupt
 * after a specified period and invokes a callback function when the 
 * timer expires. The callback function stops the timer and posts a semaphore
 * to indicate that the timer callback has been executed.
 * 
 * @subsubsection tim_param Configurable Parameters
 * - The timer period can be configured in @c TIMER_PERIOD_US macro.
 * - The timer instance can be changed in @c TIMER_INSTANCE macro.
 * 
 * @subsubsection tim_how_to How to Run
 * 1. Follow the common README instructions to build and flash the application.
 * 2. Run the application on the board.
 * 3. Monitor the UART terminal for the output.
 * 
 * @subsubsection tim_result Expected Results
 * - The timer starts and waits for the configured period.
 * - Once the timer expires, the callback is executed.
 * - The application prints a message indicating that the timer expired and the callback ran successfully.
 */

#define TIMER_INSTANCE    0
#define TIMER_PERIOD_US  (5 * 1000 * 1000) // 5 seconds in microseconds

osal_semaphore_def_t xSemMem;
osal_semaphore_t xSem;
void vTimerCallbackSample( void *arg )
{
    Timer_Handle_t pxTimerHandle = (Timer_Handle_t)arg;
    /* for single shot usage stop the timer after fist interrupt */
    timer_stop(pxTimerHandle);
    osal_semaphore_post(xSem);
}

void vTimerTask()
{
    Timer_Handle_t pxTimerHandle;
    BaseType_t lRetVal;
    xSem = osal_semaphore_create(&xSemMem);

    INFO("Timer single shot usage example");

    pxTimerHandle = timer_open(TIMER_INSTANCE);
    if (pxTimerHandle != NULL)
    {
        timer_set_callback(pxTimerHandle, vTimerCallbackSample, pxTimerHandle);

        INFO("Configuring the timer for 5 seconds");
        timer_set_period_us(pxTimerHandle, TIMER_PERIOD_US);
        timer_start(pxTimerHandle);

    }

    INFO("Waiting for timer callback ...");
    /*
     * Wait for the timer callback to be invoked.
     * Keep a 20 msec margin to receive the callback
     */
    lRetVal = osal_semaphore_wait(xSem, 5 * 1000 + 20);
    if (lRetVal == pdTRUE)
    {
        INFO("Timer callback received");
    }
    else
    {
        ERROR("Failed to get timer callback");
    }

    timer_close(pxTimerHandle);
    INFO("Timer example completed");

}

