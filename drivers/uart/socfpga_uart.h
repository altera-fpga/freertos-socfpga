/*
 * Common IO - basic V1.0.0
 * Copyright (C) 2020 Amazon.com, Inc. or its affiliates. All Rights Reserved.
 * Copyright (C) 2025-2026 Altera Corporation.
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
 *
 * Modified function names and other definitions for SoC FPGA
 */

#ifndef __SOCFPGA_UART_H__
#define __SOCFPGA_UART_H__

/*
    File: UART Driver

    Public HAL API for the SoC FPGA UART controller.

    The driver supports synchronous, asynchronous (interrupt) and polling
    transfers. Typical usage is: open a port with <uart_open>, optionally
    tune the line settings with <uart_ioctl>, transfer data with the
    read/write functions, then release it with <uart_close>.

    Layer: drivers
*/

#include "socfpga_defines.h"

/*
    Group: Macros

    Callback signatures, types and enumerated values for the UART driver.
*/

/*
    Macro: UART_BAUD_RATE_DEFAULT

    The default baud rate for a given UART port.
*/
#define UART_BAUD_RATE_DEFAULT    (115200U)
/* end of group uart_macros */

/*
    Enum: uart_op_status_t

    UART read/write operation status values.
*/
typedef enum
{
    UART_WR_DONE, /* write completed successfully. */
    UART_RD_DONE, /* read completed successfully. */
    UART_LAST_WR_FAILED, /* error while performing write operation. */
    UART_LAST_RD_FAILED, /* error while performing read operation. */
} uart_op_status_t;

/*
    Enum: uart_partity_t

    UART parity mode.
*/
typedef enum
{
    UART_PARITY_NONE, /* No parity. */
    UART_PARITY_ODD, /* Odd parity. */
    UART_PARITY_EVEN, /* Even parity. */
} uart_partity_t;

/*
    Enum: uart_stop_bits_t

    UART stop bits.
*/
typedef enum
{
    UART_STOP_BITS_1, /* One stop bit. */
    UART_STOP_BITS_2, /* Two stop bits. */
} uart_stop_bits_t;

/*
    Callback: uart_callback_t

    The callback function for completion of UART operation.

    Parameters:

        status - UART asynchronous operation status.
        param - User Context passed when setting the callback. This is not
            used or modified by the driver. The context is provided by the
            caller when setting the callback, and is passed back to the caller
            in the callback.
*/
typedef void (*uart_callback_t)(uart_op_status_t status, void *param);

/*
    Variable: uart_descriptor

    The UART descriptor type defined in the source file.
*/
struct uart_descriptor;

/*
    Type: uart_handle_t

    uart_handle_t is the handle type returned by uart_open(). This is
    initialized in open and returned to caller. The caller must pass this
    pointer to the rest of APIs.
*/
typedef struct uart_descriptor *uart_handle_t;

/*
    Enum: uart_ioctl_t

    Ioctl requests for UART HAL.
*/
typedef enum
{
    UART_SET_CONFIG, /** Sets the UART configuration according to uart_config_t. */
    UART_GET_CONFIG, /** Gets the UART configuration according to uart_config_t. */
    UART_GET_TX_NBYTES, /** Get the number of bytes sent in write operation. */
    UART_GET_RX_NBYTES, /** Get the number of bytes received in read operation. */
    UART_GET_TX_STATE, /** Get the state of Tx UART peripheral*/
    UART_GET_RX_STATE /** Get the state of Rx UART peripheral*/
} uart_ioctl_t;

/*
    Group: Structures

    Structures for the UART driver.
*/
/*
    Struct: uart_config_t

    Configuration parameters for the UART.

    The application will send the user configuration in the form of the
    following structure in ioctl.
*/
typedef struct
{
    uint32_t baud; /* Desired baud rate. */
    uart_partity_t parity; /* Desired parity option. Defined in uart_partity_t. */
    uart_stop_bits_t stop_bits; /* Desired stop bits options. Defined in uart_stop_bits_t. */
    uint32_t wlen; /* Desired word length. Valid values are from 5 to 8 */
} uart_config_t;
/* end of group uart_structs */


/*
    Group: Lifecycle and Configuration

    Entry points for the UART driver's lifecycle and configuration.
*/

/*
    Function: uart_open

    Initializes a UART port and reserves it for the caller.

    Once opened, the same instance must be closed with <uart_close> before it
    can be opened again.

    Parameters:
        instance - Zero-based index of the UART port to initialize.

    Returns:
        A valid <uart_handle_t> on success, or NULL if the instance number is
        invalid or the instance is already open.
*/
uart_handle_t uart_open(uint32_t instance);

/*
    Function: uart_set_callback

    Registers a per-handle completion callback for asynchronous transfers.

    The callback is invoked when an asynchronous operation completes (success
    or failure). It is not called for synchronous transfers, and a single
    callback is shared by <uart_read_async> and <uart_write_async>.

    Parameters:
        huart    - Handle returned by <uart_open>.
        callback - Function to invoke on completion (may be NULL to clear it).
        param    - Opaque user context passed back to the callback.

    Returns:
        UART_SUCCESS on success, -EINVAL if huart is NULL, or -EBUSY if a
        transfer is in progress.
*/
int32_t uart_set_callback(uart_handle_t const huart, uart_callback_t callback,
        void *param);

/*
    Group: Data Transfer

    Entry points for the UART driver's data transfer.
*/

/*
    Function: uart_read_sync

    Reads up to nbytes from the UART, blocking until done. A partial read may
    occur; use <uart_ioctl> with UART_GET_RX_NBYTES for the actual count.

    Parameters:
        huart  - Handle returned by <uart_open>.
        buf    - Caller-allocated buffer that receives the data.
        nbytes - Number of bytes to read.

    Returns:
        UART_SUCCESS on success, -EINVAL for bad arguments, -EIO on driver
        error, or -EBUSY if another read is in progress.
*/
int32_t uart_read_sync(uart_handle_t const huart, uint8_t *const buf,
        uint32_t nbytes);

/*
    Function: uart_write_sync

    Writes up to nbytes to the UART, blocking until done. A partial write may
    occur; use <uart_ioctl> with UART_GET_TX_NBYTES for the actual count.

    Parameters:
        huart  - Handle returned by <uart_open>.
        buf    - Buffer holding the data to transmit.
        nbytes - Number of bytes to send.

    Returns:
        UART_SUCCESS on success, -EINVAL for bad arguments, -EIO on driver
        error, or -EBUSY if another write is in progress.
*/
int32_t uart_write_sync(uart_handle_t const huart, uint8_t *const buf,
        uint32_t nbytes);

/*
    Function: uart_write_polling

    Transmit data through UART using polling.

    This function is not thread safe, use UART sync/async write functions for
    thread safety. This function attempts to write certain number of bytes
    from a pre-allocated buffer to a receiver device, using polling.

    Parameters:

        huart - The peripheral handle returned in the open() call.
        buf - The buffer with data to transmit.
        nbytes - The number of bytes to send.

    Returns:

        - UART_SUCCESS: on success
        - -EINVAL: if
            - huart is NULL
            - huart is not opened yet
            - buf is NULL
            - nbytes is 0
*/
int32_t uart_write_polling(uart_handle_t const huart, uint8_t *const buf,
        uint32_t nbytes);

/*
    Function: uart_read_async

    Start receiving data from UART asynchronously.

    This function attempts to read certain number of bytes from a
    pre-allocated buffer, in asynchronous way. It returns immediately when the
    operation is started and the status can be checked by calling uart_ioctl.
    Once the operation completes, successful or not, the user callback will be
    invoked.

    Partial read might happen. And the number of bytes that have been actually
    read can be obtained by calling uart_ioctl.

    Parameters:

        huart - The peripheral handle returned in the open() call.
        buf - The buffer to store the received data.
        nbytes - The number of bytes to read.

    Returns:

        - UART_SUCCESS: on success
        - -EINVAL: if
            - huart is NULL
            - huart is not opened yet
            - buf is NULL
            - nbytes is 0
        - -EIO:    if there is unknown driver error

    Notes:

        In order to get notification when the asynchronous call is completed,
        uart_set_callback must be called prior to this.
        Warning: buf must be valid before callback is invoked.
*/
int32_t uart_read_async(uart_handle_t const huart, uint8_t *const buf,
        uint32_t nbytes);

/*
    Function: uart_write_async

    Start transmitting data from UART asynchronously.

    This function attempts to write certain number of bytes from a
    pre-allocated buffer to a receiver device, in asynchronous way. It returns
    immediately when the operation is started and the status can be checked by
    calling uart_ioctl. Once the operation completes, successful or not, the
    user callback will be invoked.

    Partial write might happen. And the number of bytes that have been
    actually written can be obtained by calling uart_ioctl.

    Parameters:

        huart - The peripheral handle returned in the open() call.
        buf - The buffer with data to transmit.
        nbytes - The number of bytes to send.

    Returns:

        - UART_SUCCESS: on success
        - -EINVAL: if
            - huart is NULL
            - huart is not opened yet
            - buf is NULL
            - nbytes is 0
        - -EIO:   if there is unknown driver error

    Notes:

        In order to get notification when the asynchronous call is completed,
        uart_set_callback must be called prior to this.
*/
int32_t uart_write_async(uart_handle_t const huart, uint8_t *const buf,
        uint32_t nbytes);

/*
    Function: uart_ioctl

    Configure the UART port with user configuration.

    Parameters:

        huart - The peripheral handle returned in the open() call.
        cmd - The configuration request. Should be one of the values from
            uart_ioctl_t.
        buf - The configuration values for the UART port.

    Returns:

        - UART_SUCCESS: on success
        - -EBUSY:  if the UART tx or rx transfer is in progress.
        - -EINVAL: if
            - huart is not opened yet
            - buf is NULL
            - cmd is invalid

    Notes:

        UART_SET_CONFIG sets the UART configuration.
        This request expects the buffer with size of uart_config_t.
        UART_GET_CONFIG gets the current UART configuration.
        This request expects the buffer with size of uart_config_t.
        UART_GET_TX_NBYTES returns the number of written bytes in last
        operation.
        This is supposed to be called in the caller task or application
        callback, right after last operation completes.
        This request expects 2 bytes buffer (uint16_t).
        - If the last operation was write, this returns the actual number of
        written bytes which might be smaller than the requested number
        (partial write).
        - If the last operation was read, this returns 0.
        UART_GET_RX_NBYTES returns the number of read bytes in last operation.
        This is supposed to be called in the caller task or application
        callback, right after last operation completes.
        This request expects 2 bytes buffer (uint16_t).
        - If the last operation was read, this returns the actual number of
        read bytes which might be smaller than the requested number (partial
        read).
        - If the last operation was write, this returns 0.
*/
int32_t uart_ioctl(uart_handle_t const huart, uart_ioctl_t cmd,
        void *const buf);

/*
    Function: uart_cancel

    Abort the operation on the UART port if any underlying driver allows
    cancellation of the operation.

    The application should call this function to stop the ongoing operation.

    Parameters:

        huart - The peripheral handle returned in the open() call.

    Returns:

        - -EINVAL: if
            - huart is NULL
            - huart is not opened yet
        - -ENOSYS: if this board doesn't support this operation.
*/
int32_t uart_cancel(uart_handle_t const huart);

/*
    Group: Teardown

    Entry points for the UART driver's teardown.
*/

/*
    Function: uart_close

    Stops any ongoing operation and de-initializes the UART port, releasing the
    instance so it can be opened again.

    Parameters:
        huart - Handle returned by <uart_open>.

    Returns:
        UART_SUCCESS on success, or -EINVAL if the handle is NULL or the port
        is not open.
*/
int32_t uart_close(uart_handle_t const huart);
/* end of group uart_fns */
/* end of group uart */
#endif /* __SOCFPGA_UART_H__ */
