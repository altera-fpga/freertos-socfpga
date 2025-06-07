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

#ifndef __SOCFPGA_UART_H__
#define __SOCFPGA_UART_H__

/**
 * @file socfpga_uart.h
 * @brief SoC FPGA UART HAL driver 
 */

#include "socfpga_defines.h"

/**
 * @defgroup uart UART
 * @ingroup drivers
 *
 * @{
 */

/**
 * @defgroup uart_fns Functions
 * @ingroup uart
 * UART HAL APIs
 */

/**
 * @defgroup uart_structs Structures 
 * @ingroup uart
 * UART Specific Structures
 */

/**
 * @defgroup uart_enums Enumerations
 * @ingroup uart
 * UART Specific Enumerations
 */

/**
 * @defgroup uart_macros Macros
 * @ingroup uart
 * UART Specific Macros 
 */

/**
 * @addtogroup uart_macros 
 * @{
 */

/**
 * @brief The default baud rate for a given UART port.
 */
#define UART_BAUD_RATE_DEFAULT    (115200U)

/**
 * @}
 */
/* end of group uart_macros */

/**
 * @brief UART read/write operation status values
 * @ingroup uart_enums
 */
typedef enum
{
    eUartWriteCompleted, /*!< write completed successfully. */
    eUartReadCompleted, /*!< read completed successfully. */
    eUartLastWriteFailed, /*!< Error while performing write operation. */
    eUartLastReadFailed, /*!< Error while performing read operation. */
} UartOperationStatus_t;

/**
 * @brief UART parity mode
 * @ingroup uart_enums
 */
typedef enum
{
    eUartParityNone, /*!< No parity. */
    eUartParityOdd, /*!< Odd parity. */
    eUartParityEven, /*!< Even parity. */
} UartParity_t;

/**
 * @brief UART stop bits
 * @ingroup uart_enums
 */
typedef enum
{
    eUartStopBitsOne, /*!< One stop bit. */
    eUartStopBitsTwo, /*!< Two stop bits. */
} UartStopBits_t;

/**
 * @brief The callback function for completion of UART operation.
 * @ingroup uart_fns
 *
 * @param[out] xStatus      UART asynchronous operation status.
 * @param[in] pvUserContext User Context passed when setting the callback.
 *                          This is not used or modified by the driver. The context
 *                          is provided by the caller when setting the callback, and is
 *                          passed back to the caller in the callback.
 */
typedef void (*UartCallback_t)( UartOperationStatus_t xStatus,
        void *pvUserContext );

/**
 * @brief The UART descriptor type defined in the source file.
 * @ingroup uart_structs
 */
struct UartDescriptor;

/**
 * @brief UartHandle_t is the handle type returned by calling uart_open().
 *        This is initialized in open and returned to caller. The caller must pass
 *        this pointer to the rest of APIs.
 * @ingroup uart_structs
 */
typedef struct UartDescriptor *UartHandle_t;

/**
 * @brief Ioctl requests for UART HAL.
 * @ingroup uart_enums
 */
typedef enum
{
    eUartSetConfig, /** Sets the UART configuration according to UartConfig_t. */
    eUartGetConfig, /** Gets the UART configuration according to UartConfig_t. */
    eGetTxNoOfbytes, /** Get the number of bytes sent in write operation. */
    eGetRxNoOfbytes, /** Get the number of bytes received in read operation. */
    eGetTxState, /** Get the state of Tx UART peripheral*/
    eGetRxState /** Get the state of Rx UART peripheral*/
} UartIoctlRequest_t;

/**
 * @addtogroup uart_structs
 * @{
 */
/**
 * @brief Configuration parameters for the UART.
 *
 * The application will send the user configuration in the form of the
 * following structure in ioctl.
 * 
 */
typedef struct
{
    uint32_t ulBaudrate; /*!< Desired baud rate. */
    UartParity_t xParity; /*!< Desired parity option. Defined in UartParity_t. */
    UartStopBits_t xStopbits; /*!< Desired stop bits options. Defined in UartStopBits_t. */
    uint32_t ucWordlength; /*!< Desired word length. Valid values are from 5 to 8 */
} UartConfig_t;

/**
 * @}
 */
/* end of group uart_structs */


/**
 * @addtogroup uart_fns
 * @{
 */

/**
 * @brief Initializes the UART peripheral of the board.
 *
 * The application should call this function to initialize the desired UART port.
 *
 * @warning Once opened, the same UART instance must be closed before calling open again.
 *
 * @param[in] lUartInstance The instance of the UART port to initialize.
 *
 * @return
 * - 'the handle to the UART port (not NULL)', on success.
 * - 'NULL', if
 *     - invalid instance number
 *     - open same instance more than once before closing it
 */
UartHandle_t uart_open( uint32_t lUartInstance );

/**
 * @brief Sets the application callback to be called on completion of an operation.
 *
 * The callback is guaranteed to be invoked when the current asynchronous operation completes, either successful or failed.
 * This simply provides a notification mechanism to user's application. It has no impact if the callback is not set.
 *
 * @note This callback will not be invoked when synchronous operation completes.
 * @note This callback is per handle. Each instance has its own callback.
 * @note Single callback is used for both read_async and write_async. Newly set callback overrides the one previously set.
 * @warning If the input handle is invalid, this function silently takes no action.
 *
 * @param[in] pxUartPeripheral The peripheral handle returned in the open() call.
 * @param[in] xCallback The callback function to be called on completion of transaction (This can be NULL).
 * @param[in] pvUserContext The user context to be passed back when callback is called (This can be NULL).
 *
 * @return
 * - UART_SUCCESS, on success
 * - -EINVAL, if pxUartPeripheral is NULL
 * - -EBUSY, if a transfer is in progress.
 */
int32_t uart_set_callback( UartHandle_t const pxUartPeripheral,
        UartCallback_t xCallback, void *pvUserContext );

/**
 * @brief Starts receiving the data from UART synchronously.
 *
 * This function attempts to read certain number of bytes from transmitter device to a pre-allocated buffer, in synchronous way.
 * Partial read might happen.
 * And the number of bytes that have been actually read can be obtained by calling uart_ioctl.
 *
 * @note If the number of bytes is not known, it is recommended that the application reads one byte at a time.
 *
 * @param[in] pxUartPeripheral The peripheral handle returned in the open() call.
 * @param[out] pvBuffer The buffer to store the received data. Allocated by the caller.
 * @param[in] xBytes The number of bytes to read.
 *
 * @return
 * - UART_SUCCESS, on success (all the requested bytes have been read)
 * - -EINVAL, if
 *     - pxUartPeripheral is NULL
 *     - pxUartPeripheral is not opened yet
 *     - pucBuffer is NULL
 *     - xBytes is 0
 * - -EIO, if there is unknown driver error
 * - -EBUSY, if another read is in progress.
 */
int32_t uart_read_sync( UartHandle_t const pxUartPeripheral,
        uint8_t *const pvBuffer, uint32_t xBytes );

/**
 * @brief Starts the transmission of data from UART synchronously.
 *
 * This function attempts to write certain number of bytes from a pre-allocated buffer to a receiver device, in synchronous way.
 * Partial write might happen.
 * And the number of bytes that have been actually written can be obtained by calling uart_ioctl.
 *
 * @param[in] pxUartPeripheral The peripheral handle returned in the open() call.
 * @param[in] pvBuffer The buffer with data to transmit.
 * @param[in] xBytes The number of bytes to send.
 *
 * @return
 * - UART_SUCCESS, on success (all the requested bytes have been written)
 * - -EINVAL, if
 *     - pxUartPeripheral is NULL
 *     - pxUartPeripheral is not opened yet
 *     - pucBuffer is NULL
 *     - xBytes is 0
 * - -EIO, if there is unknown driver error
 * - -EBUSY, if another write is in progress
 */
int32_t uart_write_sync( UartHandle_t const pxUartPeripheral,
        uint8_t *const pvBuffer, uint32_t xBytes );

/**
 * @brief Starts receiving the data from UART asynchronously.
 *
 * This function attempts to read certain number of bytes from a pre-allocated buffer, in asynchronous way.
 * It returns immediately when the operation is started and the status can be checked by calling uart_ioctl.
 * Once the operation completes, successful or not, the user callback will be invoked.
 *
 * Partial read might happen.
 * And the number of bytes that have been actually read can be obtained by calling uart_ioctl.
 *
 * @note In order to get notification when the asynchronous call is completed, uart_set_callback must be called prior to this.
 * @warning pucBuffer must be valid before callback is invoked.
 *
 * @param[in] pxUartPeripheral The peripheral handle returned in the open() call.
 * @param[out] pvBuffer The buffer to store the received data.
 * @param[in] xBytes The number of bytes to read.
 *
 * @return
 * - UART_SUCCESS, on success
 * - -EINVAL, if
 *     - pxUartPeripheral is NULL
 *     - pxUartPeripheral is not opened yet
 *     - pucBuffer is NULL
 *     - xBytes is 0
 * - -EIO, if there is unknown driver error
 */
int32_t uart_read_async( UartHandle_t const pxUartPeripheral,
        uint8_t *const pvBuffer, uint32_t xBytes );

/**
 * @brief Starts the transmission of data from UART asynchronously.
 *
 * This function attempts to write certain number of bytes from a pre-allocated buffer to a receiver device, in asynchronous way.
 * It returns immediately when the operation is started and the status can be checked by calling uart_ioctl.
 * Once the operation completes, successful or not, the user callback will be invoked.
 *
 * Partial write might happen.
 * And the number of bytes that have been actually written can be obtained by calling uart_ioctl.
 *
 * @note In order to get notification when the asynchronous call is completed, uart_set_callback must be called prior to this.
 *
 *
 * @param[in] pxUartPeripheral The peripheral handle returned in the open() call.
 * @param[in] pvBuffer The buffer with data to transmit.
 * @param[in] xBytes The number of bytes to send.
 *
 * @return
 * - UART_SUCCESS, on success
 * - -EINVAL, if
 *     - pxUartPeripheral is NULL
 *     - pxUartPeripheral is not opened yet
 *     - pucBuffer is NULL
 *     - xBytes is 0
 * - -EIO, if there is unknown driver error
 */
int32_t uart_write_async( UartHandle_t const pxUartPeripheral,
        uint8_t *const pvBuffer, uint32_t xBytes );

/**
 * @brief Configures the UART port with user configuration.
 *
 *
 * @note eUartSetConfig sets the UART configuration.
 * This request expects the buffer with size of UartConfig_t.
 *
 * @note eUartGetConfig gets the current UART configuration.
 * This request expects the buffer with size of UartConfig_t.
 *
 * @note eGetTxNoOfbytes returns the number of written bytes in last operation.
 * This is supposed to be called in the caller task or application callback, right after last operation completes.
 * This request expects 2 bytes buffer (uint16_t).
 *
 * - If the last operation was write, this returns the actual number of written bytes which might be smaller than the requested number (partial write).
 * - If the last operation was read, this returns 0.
 *
 * @note eGetRxNoOfbytes returns the number of read bytes in last operation.
 * This is supposed to be called in the caller task or application callback, right after last operation completes.
 * This request expects 2 bytes buffer (uint16_t).
 *
 * - If the last operation was read, this returns the actual number of read bytes which might be smaller than the requested number (partial read).
 * - If the last operation was write, this returns 0.
 *
 * @param[in] pxUartPeripheral The peripheral handle returned in the open() call.
 * @param[in] xUartRequest The configuration request. Should be one of the values
 * from UartIoctlRequest_t.
 * @param[in,out] pvBuffer The configuration values for the UART port.
 *
 * @return
 * - UART_SUCCESS, on success
 * - -EBUSY, if the UART tx or rx transfer is in progress.
 * - -EINVAL, if
 *     - pxUartPeripheral is not opened yet
 *     - pucBuffer is NULL
 *     - xUartRequest is invalid
 */
int32_t uart_ioctl( UartHandle_t const pxUartPeripheral,
        UartIoctlRequest_t xUartRequest, void *const pvBuffer );

/**
 * @brief Aborts the operation on the UART port if any underlying driver allows
 * cancellation of the operation.
 *
 * The application should call this function to stop the ongoing operation.
 *
 * @param[in] pxUartPeripheral The peripheral handle returned in the open() call.
 *
 * @return
 * - -EINVAL, if
 *     - pxUartPeripheral is NULL
 *     - pxUartPeripheral is not opened yet
 * - -ENOSYS, if this board doesn't support this operation.
 */
int32_t uart_cancel( UartHandle_t const pxUartPeripheral );

/**
 * @brief Stops the operation and de-initializes the UART peripheral.
 *
 *
 * @param[in] pxUartPeripheral The peripheral handle returned in the open() call.
 *
 * @return
 * - UART_SUCCESS, on success
 * - -EINVAL, if
 *     - pxUartPeripheral is NULL
 *     - pxUartPeripheral is not opened yet
 */
int32_t uart_close( UartHandle_t const pxUartPeripheral );

/**
 * @} 
 */
/* end of group uart_fns */

/**
 * @}
 */
/* end of group uart */
#endif /* __SOCFPGA_UART_H__ */

