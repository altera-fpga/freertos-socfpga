/*
 * SPDX-FileCopyrightText: Copyright (C) 2025 Altera Corporation
 *
 * SPDX-License-Identifier: MIT-0
 *
 * Sample application for wdt
 */

#include "FreeRTOS.h"
#include "FreeRTOSConfig.h"
#include "osal_log.h"
#include "task.h"
#include "semphr.h"
#include "socfpga_watchdog.h"
#include "osal.h"

/** 
 * @defgroup wdt_sample Watchdog Timer
 * @ingroup samples
 * 
 * Sample application for Watchdog Timer
 * @details
 * @subsubsection wdt_desc Description
 * This sample application demonstrates the use of watchdog timer APIs.
 * The sample application will use four parameters. Bark time, bite time,
 * service period and number of services. It will service (restart) the WDT
 * every service period from a thread context. But stops doing it after the
 * given number of services. The bite behavior will be set to interrupt.
 * We will get an interrupt after bark time expiry. Since we stop restarting
 * the timer, it will then reboot the system after the bark time expiry.
 * @note This program will reboot the system.
 * 
 * @subsubsection wdt_param Configurable Parameters
 * - Bark time and bite time can be configured using the list of timeouts given by the driver.
 * - Service time can be configured in the sample structure @c xOpts .
 * - Instance can be configured by changing the value in @c WDT_INSTANCE macro.
 * 
 * @subsubsection wdt_how_to How to Run
 * 1. Follow the common README instructions to build and flash the application.
 * 2. Run the application on the board.
 * 3. Monitor UART terminal output for WDT service messages, bark interrupt notification,
 *    and system reboot.
 * 
 * @subsubsection wdt_result Expected Results
 * - The watchdog timer is periodically serviced for the given number of times.
 * - A bark interrupt is received after the final service and bark timeout.
 * - The system is rebooted once the bite time expires.
 */
typedef struct
{
    uint32_t service;
    uint32_t nservice;
    uint32_t bark;
    uint32_t bite;
} Wdtsample_t;

WdtHandle_t xHandle;
volatile int xCallbackCnt = 0;
SemaphoreHandle_t xCallbackSem;

/* test configuration */
#define TASK_PRIORITY    (configMAX_PRIORITIES - 2)

/*number of instances from 0-4*/
#define WDT_INSTANCE     0

Wdtsample_t xOpts =
{
    /* Use approximately 5 seconds time out */
    .bark = WDT_TIMEOUT_5120MSEC,
    .bite = WDT_TIMEOUT_5120MSEC,

    /* Service every 2 seconds */
    .service = 2,
    /* Service two times */
    .nservice = 2
};

void wdtCallback( void *arg )
{
    xHandle = (WdtHandle_t) arg;

    /*
     * When we get the callback first time we will service it so that
     * we get time to print out a message from the thread
     */
    if (xCallbackCnt == 0)
    {
        xCallbackCnt++;
        wdt_restart(xHandle);
        osal_semaphore_post(xCallbackSem);
    }
    else
    {
        /*
         * When the callback hits again we will hang here.
         * The system will reset after bite time expiry
         */
        while (1)
            ;
    }
}

void wdt_task()
{
    int retval = 0;

    xCallbackSem = osal_semaphore_create(NULL);
    WdtBiteConfig_t Biteconfig = eWdtBiteTimerInterrupt;

    INFO("WDT sample application");

    INFO("Configuring the WDT ...");

    xHandle = wdt_open(0);
    if (xHandle == NULL)
    {
        ERROR("Opening WDT instance failed");
        return;
    }

    retval = wdt_ioctl(xHandle, eSetWatchdogBiteBehaviour,
            &Biteconfig);
    if (retval != 0)
    {
        ERROR("Configuring bite behaviour failed");
        wdt_close(xHandle);
        return;
    }

    retval = wdt_ioctl(xHandle, eSetWatchdogBarkTime, &xOpts.bark);
    if (retval != 0)
    {
        ERROR("Configuring bark time failed");
        wdt_close(xHandle);
        return;
    }

    retval = wdt_ioctl(xHandle, eSetWatchdogBiteTime, &xOpts.bite);
    if (retval != 0)
    {
        ERROR("Configuring bite time failed");
        wdt_close(xHandle);
        return;
    }

    wdt_set_callback(xHandle, wdtCallback, xHandle);

    INFO("WDT configuration done.");

    retval = wdt_start(xHandle);

    if (retval != 0)
    {
        ERROR("WDT start failed");
        wdt_close(xHandle);
        return;
    }

    INFO("WDT started.");

    for (uint32_t numService = 0; numService < xOpts.nservice; numService++)
    {
        osal_task_delay((xOpts.service * 1000));
        wdt_restart(xHandle);
        INFO("Serviced WDT.");
    }

    INFO("WDT service stopped");
    /*
     * At this point we have stopped servicing the interrupt.
     * We will wait for the interrupt on the bark time expiry.
     *
     * We will keep a 25 seconds timeout for receiving the interrupt
     */

    retval = osal_semaphore_wait(xCallbackSem, pdMS_TO_TICKS(25 * 1000));
    if (retval == pdTRUE)
    {
        INFO("WDT bark timer expiry interrupt occurred");
        /*
         * At this point we will suspend this task.
         * After the next bark time expiry, the callback will hit again.
         * In the callback function we will just hang. And on bite time
         * expiry the system reset will occur
         */
        vTaskSuspend(NULL);
    }
    else
    {
        ERROR("WDT bark timer expiry interrupt did not occur");
    }

    vTaskSuspend(NULL);
}

