/*
 * SPDX-FileCopyrightText: Copyright (C) 2025-2026 Altera Corporation
 *
 * SPDX-License-Identifier: MIT-0
 *
 * Header file for console driver
 */


#ifndef __SOCFPGA_CONSOLE_H__
#define __SOCFPGA_CONSOLE_H__

#include <errno.h>

/*
    File: Console Driver

    Declares the Console driver interface: 10 functions.
*/

/*
    Function: console_init

    Initialize the console for system prints.

    Parameters:

        id - UART ID to use.
        config_str - UART configuration string. Format: <baudrate>-<bit
            len><parity><number of stop bit> Example: 115200-8N1

    Returns:

        0 on success, -errno on failure.
*/
int console_init(uint32_t id, const char *config_str);

/*
    Function: console_write

    Write data to the console.

    Parameters:

        buf - Pointer to the data to write.
        length - Number of bytes to write.

    Returns:

        Number of bytes written on success, -errno on failure.
*/
int console_write(unsigned char *const buf, int length);

/*
    Function: vLoggingPrintf

    Format a message and queue it for the console.

    The function bypasses stdio, so it is safe with the scheduler suspended.
    Messages longer than the internal buffer are truncated.

    Parameters:

        format - printf style format string.
*/
void vLoggingPrintf(const char *format, ...);

/*
    Function: console_read

    Read data from the console.

    Parameters:

        buf - Pointer to the receive buffer.
        length - Maximum number of bytes to read.

    Returns:

        Number of bytes read on success, -errno on failure.
*/
int console_read(unsigned char *const buf, int length);

/*
    Function: console_fill_buffer

    Fill the console output buffer with provided data.

    The data remains in the buffer until console_signal_fill() is called to
    signal it for write. Use this if the writes need to be deferred and do not
    invoke a context switch

    Parameters:

        buf - Pointer to the data to copy into the read buffer.
        length - Number of bytes to copy.

    Returns:

        Number of bytes copied on success, -errno on failure.
*/
int console_fill_buffer(unsigned char *const buf, int length);

/*
    Function: console_signal_fill

    Signal that the console buffer has been filled.

    This function does nothing if configCONSOLE_MAKE_FLUSH_TASK is false. In
    that case the console flush is the responsibility of the user.
*/
void console_signal_fill(void);

/*
    Function: console_clear_pending

    Clear any pending console events or state.
*/
void console_clear_pending(void);

/*
    Function: console_lock

    Acquire the console write semaphore.
*/
int console_lock(void);

/*
    Function: console_unlock

    Release the console write semaphore.
*/
int console_unlock(void);

/*
    Function: console_deinit

    Deinitialize the console.

    Returns:

        0 on success, -errno on failure.
*/
int console_deinit();

#endif /* __SOCFPGA_CONSOLE_H__ */
