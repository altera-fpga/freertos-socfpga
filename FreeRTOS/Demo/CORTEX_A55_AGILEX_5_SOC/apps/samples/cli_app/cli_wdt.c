/*
 * SPDX-FileCopyrightText: Copyright (C) 2025 Altera Corporation
 *
 * SPDX-License-Identifier: MIT-0
 *
 * Implementation of CLI commands for WDT
 */

 /**
 * @defgroup cli_wdt Watchdog Timer
 * @ingroup cli
 *
 * Perform watchdog timer operations
 *
 * @details
 * It supports the following commands:
 * - wdog test <service_period> <number_of_services> <bark_time> <bite_time>
 * - wdog help
 *
 * Typical usage:
 * - Use 'wdog test' command to perform a watchdog timer test.
 *
 * @subsection commands Commands
 * @subsubsection wdt_test wdog test
 * Perform Watchdog timer test <br>
 *
 * Usage: <br>
 *   wdog test <service_period> <number_of_services> <bark_time> <bite_time> <br>
 *
 * It requires the following arguments:
 * - service_period       The time in seconds to wait before servicing the WDT timer. The range is 0 to 20 seconds.
 * - number_of_services   The number of times to service the WDT timer. It should be a positive number less than 256.
 * - bark_time            Select the bark time period (one of 16 possible values).
 * - bite_time            Select the bite time period (one of 16 possible values).
 *
 * Supported bark and bite time values:
 * - 0  – 0.6 msec
 * - 1  – 1.3 msec
 * - 2  – 2.6 msec
 * - 3  – 5.1 msec
 * - 4  – 10   msec
 * - 5  – 20   msec
 * - 6  – 40   msec
 * - 7  – 80   msec
 * - 8  – 160  msec
 * - 9  – 320  msec
 * - 10 – 640  msec
 * - 11 – 1280 msec
 * - 12 – 2560 msec
 * - 13 – 5120 msec
 * - 14 – 10000 msec
 * - 15 – 20000 msec
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <FreeRTOS.h>
#include <task.h>
#include <socfpga_uart.h>
#include "FreeRTOS_CLI.h"
#include "cli_app.h"
#include <socfpga_watchdog.h>
#include <osal_log.h>

#define MAX_SERVICE_TIME            20
#define MAX_SERVICE_NUM             255

typedef struct
{
    uint32_t service;
    uint32_t nservice;
    uint32_t bark;
    uint32_t bite;
} WdtTest_t;

WdtTest_t xWdtOpts;
WdtHandle_t xWdtHandle;

void wdtTestCallback( void *pvUserContext )
{
    (void) pvUserContext;
    printf(" Woof!! from watchdoog\r\n");
    while ( 1 )
        ;
}

bool isTimeoutValid(uint8_t tOut)
{
    uint8_t i;
    bool ret = false;
    for(i = WDT_TIMEOUT_0P6MSEC;i <= WDT_TIMEOUT_20000MSEC;i++)
    {
        if(tOut == i)
        {
            ret = true;
        }
    }
    return ret;
}

void wdtTest( void *params )
{
    int retval = 0;
    WdtTest_t *pxWdtOpts = (WdtTest_t*) params;
    WdtBiteConfig_t Biteconfig = eWdtBiteTimerInterrupt;

    xWdtHandle = wdt_open(0);
    if ( xWdtHandle == NULL )
    {
        printf("WDT Instance error\r\n");
        goto end;
    }

    retval = wdt_ioctl(xWdtHandle, eSetWatchdogBarkTime,
            &pxWdtOpts->bark);
    if ( retval != 0 )
    {
        printf("Bark time not set\r\n");
        goto end;
    }

    retval = wdt_ioctl(xWdtHandle, eSetWatchdogBiteTime,
            &pxWdtOpts->bite);
    if ( retval != 0 )
    {
        printf("Bite time not set\r\n");
        goto end;
    }

    retval = wdt_ioctl(xWdtHandle, eSetWatchdogBiteBehaviour,
            &Biteconfig);
    if ( retval != 0 )
    {
        printf("Interrupt not set......\r\n");
        goto end;
    }
    wdt_set_callback(xWdtHandle, wdtTestCallback, NULL);

    retval = wdt_start(xWdtHandle);      //start the watchdog timer
    if ( retval != 0 )
    {
        printf("Timer not started\r\n");
        goto end;
    }

    for ( uint32_t numService = 0; numService < pxWdtOpts->nservice;
            numService++ )
    {
        osal_delay_ms((pxWdtOpts->service * 1000));
        wdt_restart(xWdtHandle);
        printf("%d : Servicing WDT timer!\r\n", numService);
    }
    end: vTaskSuspend(NULL);
}

BaseType_t cmd_wdt( char *pcWriteBuffer, size_t xWriteBufferLen,
        const char *pcCommandString )
{
    (void) xWriteBufferLen;
    const char *pcParameter1, *pcParameter2, *pcParameter3, *pcParameter4,
            *pcParameter5;
    BaseType_t xParameter1StringLength, xParameter2StringLength,
            xParameter3StringLength, xParameter4StringLength,
            xParameter5StringLength;

    pcParameter1 = FreeRTOS_CLIGetParameter(pcCommandString, 1,
            &xParameter1StringLength);
    pcParameter2 = FreeRTOS_CLIGetParameter(pcCommandString, 2,
            &xParameter2StringLength);
    pcParameter3 = FreeRTOS_CLIGetParameter(pcCommandString, 3,
            &xParameter3StringLength);
    pcParameter4 = FreeRTOS_CLIGetParameter(pcCommandString, 4,
            &xParameter4StringLength);
    pcParameter5 = FreeRTOS_CLIGetParameter(pcCommandString, 5,
            &xParameter5StringLength);

    xWdtOpts.service = strtol(pcParameter2, NULL, 10);
    xWdtOpts.nservice = strtol(pcParameter3, NULL, 10);
    xWdtOpts.bark = strtol(pcParameter4, NULL, 10);
    xWdtOpts.bite = strtol(pcParameter5, NULL, 10);

    if ( !strncmp(pcParameter1, "test", strlen("test")) )
    {
        if (!strncmp(pcParameter2, "help", strlen("help")) )
        {
            printf("\r\nPerform Watchdog timer test\r\n"
                   "\r\n\nUsage:"
                   "\r\n  wdog test <service_period> <number_of_services> <bark_time> <bite_time>"
                   "\r\n\nIt supports the following arguments:"
                   "\r\n  service_period        The time in seconds to wait before servicing the WDT timer."
                   "\r\n  number_of_services    The number of times to service the WDT timer."
                   "\r\n  bark_time             Select the time period for bark time. This can be selected from the 16 possible values."
                   "\r\n  bite_time             Select the time period for bite time. This can be selected from the 16 possible values."
                   "\r\n\nSupported bark and bite time values are:"
                   "\r\n  0  - 0p6msec"
                   "\r\n  1  - 1p3msec"
                   "\r\n  2  - 2p6msec"
                   "\r\n  3  - 5p1msec"
                   "\r\n  4  - 10msec"
                   "\r\n  5  - 20msec"
                   "\r\n  6  - 40msec"
                   "\r\n  7  - 80msec"
                   "\r\n  8  - 160msec"
                   "\r\n  9  - 320msec"
                   "\r\n  10 - 640msec"
                   "\r\n  11 - 1280msec"
                   "\r\n  12 - 2560msec"
                   "\r\n  13 - 5120msec"
                   "\r\n  14 - 10000msec"
                   "\r\n  15 - 20000msec\r\n");
            return pdFALSE;
        }
        else if(!isTimeoutValid(xWdtOpts.bark) || !isTimeoutValid(xWdtOpts.bite))
        {
            ERROR("Invalid argument. Try 'wdog test help' for more information.");
            return pdFALSE;
        }
        else if(xWdtOpts.service > MAX_SERVICE_TIME)
        {
            ERROR("Invalid argument. Try 'wdog test help' for more information.");
            return pdFALSE;
        }
        else if(xWdtOpts.nservice > MAX_SERVICE_NUM)
        {
            ERROR("Invalid argument. Try 'wdog test help' for more information.");
            return pdFALSE;
        }
        else if ( xTaskCreate(wdtTest, "wdttest_task", configMINIMAL_STACK_SIZE,
                &xWdtOpts, tskIDLE_PRIORITY, NULL) != pdPASS )
        {
            ERROR("task creation failed");
            while ( 1 )
                ;
        }
        else
        {
            printf("test started.\r\n");
        }
    }
    else if ( !strncmp(pcParameter1, "help", strlen("help")) )
    {
        printf("\rPerform Watchdog timer operations"
               "\r\n\nIt supports the following commands:"
               "\r\n  wdog test <service_period> <number_of_services> <bark_time> <bite_time>"
               "\r\n  wdog help"
               "\r\n\nTypical usage:"
               "\r\n  Use the 'wdog test' command to start the watchdog timer test"
               "\r\n\nFor command specific help, try:"
               "\r\n  wdog <command> help\r\n");
    }
    else
    {
        printf("\r\nUnknown WDT command."
               "\r\nUse 'wdog help' for more information.\r\n");
    }
    pcWriteBuffer[ 0 ] = 0;
    return pdFALSE;
}
