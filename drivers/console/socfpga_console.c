/*
 * SPDX-FileCopyrightText: Copyright (C) 2025 Altera Corporation
 *
 * SPDX-License-Identifier: MIT-0
 *
 * Console driver implementation using UART driver
 */

#include <stdio.h>
#include <socfpga_uart.h>
#include "socfpga_console.h"
#include "osal.h"

#define RETRY_MAX_COUNT 10

#define MAX_PIPE_SIZE 4096
#define MAX_INT_BUFF_SIZE 128

static osal_mutex_t buffer_mutex;
static osal_mutex_def_t buffer_mutex_mem;
static osal_pipe_t buffer_pipe;
static uint8_t intermediate_buffer[MAX_INT_BUFF_SIZE];
UartHandle_t pxConsoleUARTHandle = NULL;
UartConfig_t xConsoleConfig;

int console_init(int id, const char *configString)
{
    int ret = 0;
    int baudrate;
    int wordLength;
    char parity;
    int numStopBits;

    /* example: 115200-8N1 */
    if (sscanf(configString, "%d-%d%c%d", &baudrate, &wordLength, &parity,
            &numStopBits) < 4) {
        ret = -EINVAL;
    } else {
        pxConsoleUARTHandle = uart_open(id);
        if (pxConsoleUARTHandle == NULL) {
            ret = -EBUSY;
        }


        xConsoleConfig.ulBaudrate = baudrate;
        /* Set word length (default value is 8) */
        switch (wordLength) {
        case 5:
            xConsoleConfig.ucWordlength = 5;
            break;
        case 6:
            xConsoleConfig.ucWordlength = 6;
            break;
        case 7:
            xConsoleConfig.ucWordlength = 7;
            break;
        case 8:
            xConsoleConfig.ucWordlength = 8;
            break;
        default:
            xConsoleConfig.ucWordlength = 8;
            break;
        }
        /* Set parity (default is None) */
        switch (parity) {
        case 'n':
        case 'N':
            xConsoleConfig.xParity = eUartParityNone;
            break;
        case 'o':
        case 'O':
            xConsoleConfig.xParity = eUartParityOdd;
            break;
        case 'e':
        case 'E':
            xConsoleConfig.xParity = eUartParityEven;
            break;
        default:
            xConsoleConfig.xParity = eUartParityNone;
            break;
        }
        if (numStopBits > 1)
            xConsoleConfig.xStopbits = eUartStopBitsOne;
        else
            xConsoleConfig.xStopbits = eUartStopBitsTwo;

        ret = uart_ioctl(pxConsoleUARTHandle, eUartSetConfig, &xConsoleConfig);
    }

    if(ret == 0)
    {
        buffer_mutex = osal_mutex_create(&buffer_mutex_mem);
        buffer_pipe = osal_pipe_create(MAX_PIPE_SIZE);
    }

    return ret;
}

int console_write(unsigned char*const buffer, int length)
{
    int ret;
    uint32_t num_bytesin_pipe;
    int32_t consoleState;

    if(osal_get_kernel_state() == OSAL_KERNEL_NOT_RUNNING)
    {
        return 0;
    }


    ret = uart_ioctl(pxConsoleUARTHandle, eGetTxState, &consoleState);
    if(consoleState == -EBUSY) {
        if (osal_mutex_lock(buffer_mutex, 0xFFFFFFFFU)) {
            osal_pipe_send(buffer_pipe, buffer, length);
            osal_mutex_unlock(buffer_mutex);
        }
        return 0;
    }

    if ( !xPortIsInsideInterrupt() ) {
        if (osal_mutex_lock(buffer_mutex, 0xFFFFFFFFU)) {
            num_bytesin_pipe = osal_pipe_bytes_available(buffer_pipe);
            osal_mutex_unlock(buffer_mutex);
        }
        if(num_bytesin_pipe > 0)
        {
            int bytes_read = 0;
            do {
                if (osal_mutex_lock(buffer_mutex, 0xFFFFFFFFU)) {
                    bytes_read = osal_pipe_receive(buffer_pipe,
                            intermediate_buffer, MAX_INT_BUFF_SIZE);
                    osal_mutex_unlock(buffer_mutex);
                }
                ret = uart_write_sync(pxConsoleUARTHandle, intermediate_buffer, bytes_read);
            } while(bytes_read);
        }

        ret = uart_write_sync(pxConsoleUARTHandle, buffer, length);
    } else {
        ret = uart_write_async(pxConsoleUARTHandle, buffer, length);
    }
    return ret;
}

void console_clear_pending()
{
    int32_t consoleState;
    uint32_t num_bytesin_pipe;

    uart_ioctl(pxConsoleUARTHandle, eGetTxState, &consoleState);
    if(consoleState == -EBUSY) {
        return;
    }

    num_bytesin_pipe = osal_pipe_bytes_available(buffer_pipe);
    if(num_bytesin_pipe > 0)
    {
        int bytes_read = 0;
        do {
            /* Check if lock could be aquired.
             * Use non blocking apis only to make this
             * usable from idle tasks.
             * As Idle task should not be blocked/suspended.
             * */
            if (osal_mutex_lock(buffer_mutex, 0)) {
                bytes_read = osal_pipe_receive(buffer_pipe,
                        intermediate_buffer, MAX_INT_BUFF_SIZE);
                osal_mutex_unlock(buffer_mutex);
                uart_write_async(pxConsoleUARTHandle, intermediate_buffer, bytes_read);
            }
        } while(bytes_read);
    }
}

int console_read(unsigned char*const buffer, int length)
{
    return uart_read_sync(pxConsoleUARTHandle, buffer, length);
}

int console_deinit()
{
    int ret = uart_close(pxConsoleUARTHandle);
    if (ret == 0) {
        pxConsoleUARTHandle = NULL;
    }
    return ret;
}

