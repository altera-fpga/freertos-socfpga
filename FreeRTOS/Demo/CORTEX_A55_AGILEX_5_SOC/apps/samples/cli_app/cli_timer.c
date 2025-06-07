/*
 * SPDX-FileCopyrightText: Copyright (C) 2025 Altera Corporation
 *
 * SPDX-License-Identifier: MIT-0
 *
 * Implementation of CLI commands for timer
 */

/**
 * @defgroup cli_timer Timer
 * @ingroup cli
 *
 * Perform operations on timer
 *
 * @details
 * It supports the following commands:
 * - timer config <instance> <mode> <period>
 * - timer start
 * - timer stop
 * - timer close
 * - timer ticks
 * - timer help
 *
 * Typical usage:
 * - Use 'timer config' to configure the timer instance
 * - Use 'timer start'  to start the timer
 * - Use 'timer ticks'  to get the current timer ticks while timer is running
 * - Use 'timer stop'   to stop the timer
 * - Use 'timer close'  to close the timer instance
 *
 * @subsection commands Commands
 * @subsubsection timer_config timer config
 * Configure timer instance  <br>
 *
 * Usage: <br>
 *   timer config <instance> <mode> <period>  <br>
 *
 * It requires the following arguments:
 * - instance   The instance of the timer. Valid values: 0 to 3.
 * - mode       The mode of the timer. 1 = one-shot, 2 = free-running.
 * - period     The period in microseconds for one-shot mode. Hex value 0x0 to 0xFFFFFFFF.
 *
 * @subsubsection timer_start timer start
 * Start timer instance  <br>
 *
 * Usage: <br>
 *   timer start  <br>
 *
 * @subsubsection timer_ticks timer ticks
 * Get the current counter value of the timer instance  <br>
 *
 * Usage: <br>
 *   timer ticks  <br>
 *
 * @subsubsection timer_stop timer stop
 * Stop timer instance  <br>
 *
 * Usage: <br>
 *   timer stop  <br>
 *
 * @subsubsection timer_close timer close
 * Close timer instance  <br>
 *
 * Usage: <br>
 *   timer close  <br>
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <FreeRTOS.h>
#include <task.h>
#include <socfpga_uart.h>
#include <socfpga_timer.h>
#include "FreeRTOS_CLI.h"
#include "cli_app.h"
#include "semphr.h"
#include <socfpga_gpio.h>

#define ONE_SHOT 1
#define FREE_RUNNING 2
#define FREE_RUNNING_PERIOD 0xFFFFFFFFU

uint8_t target;
extern SemaphoreHandle_t xPrintSemaphore;
/* timer (cmd_timer) command parameters starts */
Timer_Handle_t pxTimerHandle;
uint8_t isTimerOpen = 0, isTimerRunning = 0;

void vTimerCallback( void *ulBuff )
{
    Timer_Handle_t pxTimerHandleLoc = (Timer_Handle_t) ulBuff;
    timer_stop(pxTimerHandleLoc);
    timer_close(pxTimerHandleLoc);
    isTimerRunning = 0;
    isTimerOpen = 0;
    pxTimerHandleLoc = NULL;
    xSemaphoreGiveFromISR(xPrintSemaphore,NULL);
}
void vTimerCallbackFree( void *ulBuff )
{
    Timer_Handle_t pxTimerHandleLoc = (Timer_Handle_t) ulBuff;
    timer_stop(pxTimerHandleLoc);
    xSemaphoreGiveFromISR(xPrintSemaphore,NULL);
}

void PrintStatus(void *data)
{
    uint32_t* pulCount = (uint32_t*)data;
    timer_set_callback(pxTimerHandle, vTimerCallback, pxTimerHandle);
    timer_set_period_us(pxTimerHandle, *pulCount);
    osal_semaphore_wait(xPrintSemaphore,portMAX_DELAY);
    printf("\r\nOne shot mode completed for %d ms", *pulCount);
    printf("\r\nClosed timer instance.\r\n");
    vTaskDelete(NULL);
}

BaseType_t cmd_timer( char *pcWriteBuffer, size_t xWriteBufferLen,
        const char *pcCommandString )
{
    const char *pcParameter1, *pcParameter2, *pcParameter3, *pcParameter4;
    uint32_t ulMode, ulGetCount;
    static uint32_t ulCount;
    uint32_t ulStatus;
    BaseType_t xParameter1StringLength, xParameter2StringLength,
            xParameter3StringLength, xParameter4StringLength, xRet;
    xRet = pdFALSE;
    pcParameter1 = FreeRTOS_CLIGetParameter(pcCommandString, 1,
            &xParameter1StringLength);

    if(pcParameter1 == NULL)
    {
        xRet = pdTRUE;
    }
    else if ( !strncmp(pcParameter1, "config", strlen("config")) )
    {
        pcParameter2 = FreeRTOS_CLIGetParameter(pcCommandString, 2,
                &xParameter2StringLength);
        pcParameter3 = FreeRTOS_CLIGetParameter(pcCommandString, 3,
                &xParameter3StringLength);
        pcParameter4 = FreeRTOS_CLIGetParameter(pcCommandString, 4,
                &xParameter4StringLength);
        if (!strncmp(pcParameter2, "help", strlen("help")) )
        {
            printf("\r\nConfigure timer instance"
                   "\r\n\nUsage:"
                   "\r\n  timer config <instance> <mode> <period>"
                   "\r\n\nIt requires the following arguments:"
                   "\r\n  instance    The instance of the timer. Valid values range from 0 to 3."
                   "\r\n  mode        The mode of the timer. Valid values are 1 for one shot and 2 for free running mode."
                   "\r\n  period      The period in microseconds for one shot mode. It is a hexadecimal value in the range 0x0 to 0xFFFFFFFF.");
        }
        else if ( (pcParameter2 != NULL) || (pcParameter3 != NULL)
                || (pcParameter4 != NULL) )
        {
            TimerInstance_t xTimerID = (TimerInstance_t) (strtol(pcParameter2,
            NULL, 10));
            ulMode = strtol(pcParameter3, NULL, 10);
            ulCount = strtol(pcParameter4, NULL, 10);
            Timer_Handle_t pxTimerHandleTemp;
            /* set mode to free running if user entered value is freerunning period*/
            if(ulCount == FREE_RUNNING_PERIOD)
            {
                ulMode = 2;
            }
            if (isTimerOpen != 0)
            {
                printf("\r\nERROR: Timer already configured."
                       "\r\nOnly one instance supported at a time. Please run 'timer close' before a new configuration.");
                return pdFAIL;
            }
            pxTimerHandleTemp = timer_open(xTimerID);
            if ( pxTimerHandleTemp != NULL )
            {
                isTimerOpen = 1;
                isTimerRunning = 0;
                pxTimerHandle = pxTimerHandleTemp;
                switch (ulMode)
                {
                case ONE_SHOT:
                    xTaskCreate(PrintStatus,"PrintStatus",configMINIMAL_STACK_SIZE,&ulCount,tskIDLE_PRIORITY+1,NULL);
                    printf("\r\nConfigured timer in one shot mode.");
                    break;
                case FREE_RUNNING:
                    timer_set_callback(pxTimerHandle, NULL, NULL);
                    timer_set_period_us(pxTimerHandle, 0xFFFFFFFF);
                    printf("\r\nConfigured timer in free running mode.");
                    break;
                default:
                    printf("\r\n ERROR: Invalid arguments.");
                    xRet = pdTRUE;
                    break;
                }
            }
        }
        else
        {
            xRet = pdTRUE;
        }
    }
    else if ( !strncmp(pcParameter1, "start", strlen("start")) )
    {
        pcParameter2 = FreeRTOS_CLIGetParameter(pcCommandString, 2,
                &xParameter2StringLength);
        if ( !strncmp(pcParameter2, "help", strlen("help")) )
        {
            printf("\r\nStart the timer instance"
                   "\r\n\nUsage:"
                   "\r\n  timer start");

        }
        else
        {
            if (isTimerOpen == 0)
            {
                printf("\r\nERROR: Timer not configured.");
                return pdFAIL;
            }
            isTimerRunning = 1;
            timer_start(pxTimerHandle);
        }
    }
    else if ( !strncmp(pcParameter1, "stop", strlen("stop")) )
    {
        pcParameter2 = FreeRTOS_CLIGetParameter(pcCommandString, 2,
                &xParameter2StringLength);
        if ( !strncmp(pcParameter2, "help", strlen("help")) )
        {
            printf("\r\nStop the timer instance"
                   "\r\n\nUsage:"
                   "\r\n  timer stop");
        }
        else
        {
            if (isTimerOpen == 0)
            {
                printf("\r\nERROR: Timer not configured.");
                return pdFAIL;
            }
            timer_stop(pxTimerHandle);
            isTimerRunning = 0;
            printf("\r\nTimer stopped.");
        }
    }
    else if ( !strncmp(pcParameter1, "close", strlen("close")) )
    {
        pcParameter2 = FreeRTOS_CLIGetParameter(pcCommandString, 2,
                &xParameter2StringLength);
        if ( !strncmp(pcParameter2, "help", strlen("help")) )
        {
            printf("\r\nClose the timer instance"
                   "\r\n\nUsage:"
                   "\r\n  timer close");
        }
        else
        {
            timer_close(pxTimerHandle);
            isTimerOpen = 0;
            pxTimerHandle = NULL;
            printf("\r\nClosed timer instance");
        }
    }
    else if ( !strncmp(pcParameter1, "ticks", strlen("ticks")) )
    {
        pcParameter2 = FreeRTOS_CLIGetParameter(pcCommandString, 2,
                &xParameter2StringLength);
        if ( !strncmp(pcParameter2, "help", strlen("help")) )
        {
            printf("\r\nGet the current counter value of the timer instance"
                   "\r\n\nUsage:"
                   "\r\n  timer ticks");
        }
        else
        {
            if (isTimerOpen == 0)
            {
                printf("\r\nERROR: Timer not configured.");
                return pdFAIL;
            }
            ulStatus = timer_get_value_raw(pxTimerHandle, &ulGetCount);
	        if(ulStatus == 0)
	        {
	            printf("\r\n Current counter value: %u ticks", ulGetCount);
	        }
	        else
            {
                printf("\r\n Timer get value raw error:  %d ", ulStatus);
            }
        }
    }
    else if ( !strncmp(pcParameter1, "help", strlen("help")) )
    {
        printf("\rPerform operations on timer"
               "\r\n\nIt supports the following commands:"
               "\r\n  timer config <instance> <mode> <period>"
               "\r\n  timer start"
               "\r\n  timer stop"
               "\r\n  timer close"
               "\r\n  timer ticks"
               "\r\n  timer help"
               "\r\n\nTypical usage:"
               "\r\n  Use 'timer config' to configure the timer instance"
               "\r\n  Use 'timer start' to start the timer"
               "\r\n  Use 'timer ticks' to get the current timer ticks while timer is running"
               "\r\n  Use 'timer stop' to stop the timer"
               "\r\n  Use 'timer close' to close the timer instance"
               "\r\n\nFor command specific help, try:"
               "\r\n  timer <command> help");
    }

    else
    {
        printf("ERROR: Invalid arguments.");
        xRet = pdTRUE;
    }
    (void) pcWriteBuffer;
    (void) xWriteBufferLen;

    if ( xRet != pdFALSE )
    {
        printf("Use 'timer help' for more information.\r\n"
               "For command specific help, try:\r\n"
               "  timer <command> help\r\n");
        xRet = pdFALSE;
    }
    return xRet;
}
