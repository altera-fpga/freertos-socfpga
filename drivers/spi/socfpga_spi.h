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

#ifndef __SOCFPGA_SPI_H__
#define __SOCFPGA_SPI_H__

/**
 * @file socfpga_spi.h
 * @brief SoC FPGA spi HAL driver
 */

/* Standard includes. */
#include <stdint.h>
#include <stddef.h>
#include <errno.h>

/**
 * @defgroup spi SPI
 * @ingroup drivers
 * @{
 */

/**
 * @defgroup spi_fns Functions
 * @ingroup SPI
 * SPI HAL APIs
 */

/**
 * @defgroup spi_structs Structures
 * @ingroup SPI
 * SPI Specific Structures
 */

/**
 * @defgroup spi_enums Enumerations
 * @ingroup SPI
 * SPI Specific Enumerations
 */

/**
 * @brief The SPI return status from Async operations.
 * @ingroup spi_enums
 */
typedef enum
{
    eSPISuccess = 0,              /*!< SPI operation completed successfully. */
    eSPIWriteError = -EIO,      /*!< SPI driver returns error when performing write operation. */
    eSPIReadError = -EIO,        /*!< SPI driver returns error when performing read operation. */
    eSPITransferError = -EIO, /*!< SPI driver returns error when performing transfer. */
} SpiTransactionStatus_t;

/**
 * @brief The SPI Modes denoting the clock polarity
 * and clock phase.
 * @ingroup spi_enums
 */
typedef enum
{
    eSPIMode0, /*!< CPOL = 0 and CPHA = 0 */
    eSPIMode1, /*!< CPOL = 0 and CPHA = 1 */
    eSPIMode2, /*!< CPOL = 1 and CPHA = 0 */
    eSPIMode3, /*!<CPOL = 1 and CPHA = 1 */
} SpiMode_t;

/**
 * @brief Ioctl request for SPI HAL.
 * @ingroup spi_enums
 */
typedef enum
{
    eSPISetMasterConfig, /*!< Sets the configuration of the SPI master and the data type is SpiMasterConfig_t. */
    eSPIGetMasterConfig, /*!< Gets the configuration of the SPI master and the data type is SpiMasterConfig_t. */
    eSPIGetTxNoOfbytes,  /*!< Get the number of bytes sent in write operation and the data type is uint16_t. */
    eSPIGetRxNoOfbytes,  /*!< Get the number of bytes received in read operation and the data type is uint16_t. */
    eSPIActivateSlave,   /*!< Activates the slave select line of already selected slave. */
    eSPIDeactivateSlave  /*!< Deactivates the slave select line of already selected slave. */
} SpiIoctlRequest_t;

/**
 * @addtogroup spi_structs
 * @{
 */
/**
 * @brief The configuration parameters for SPI Master.
 *
 * @details The application will set the SPI master interface using the Ioctl
 * eSPISetMasterConfig and send this structure.
 */
typedef struct
{
    uint32_t ulFreq;      /*!< SPI frequency set for data transmission in Hz. */
    SpiMode_t eMode; /*!< Mode selected as per enum SpiMode_t. */
} SpiMasterConfig_t;

/**
 * @brief The SPI descriptor type defined in the source file.
 */
struct SpiDescriptor;

/**
 * @brief SpiHandle_t is the handle type returned by calling spi_open().
 *        This is initialized in open and returned to caller. The caller must pass
 *        this pointer to the rest of APIs.
 */
typedef struct SpiDescriptor *SpiHandle_t;
/**
 * @}
 */

/**
 * @brief The callback function for completion of SPI operation.
 * @ingroup spi_fns
 */
typedef void (*SpiCallback_t)(SpiTransactionStatus_t xStatus,
        void *pvSPIparam);

/**
 * @addtogroup spi_fns
 * @{
 */
/**
 * @brief Initializes SPI peripheral with default configuration.
 *
 * @warning Once opened, the same SPI instance must be closed before calling open again.
 *
 * @param[in] lSPIInstance The instance of the SPI driver to initialize.
 *
 * @return
 * - 'the handle to the SPI port (not NULL)', on success.
 * - 'NULL', if
 *     - invalid instance number
 *     - open same instance more than once before closing it
 */
SpiHandle_t spi_open(uint32_t lSPIinstance);

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
 * @param[in] pxSPIPeripheral The SPI peripheral handle returned in the open() call.
 * @param[in] xCallback The callback function to be called on completion of operation.
 * @param[in] pvUserContext The user context to be passed back when callback is called.
 *
 * @return
 * - SPI_SUCCESS, on success
 * - SPI_INVALID_VALUE, if
 *     - pxSPIPeripheral is NULL
 *     - pxSPIPeripheral is not opened yet
 *     - pucBuffer is NULL with requests which needs buffer
 * - SPI_BUS_BUSY, if the bus is busy
 */
int32_t spi_set_callback(SpiHandle_t const pxSPIPeripheral,
        SpiCallback_t xCallback,
        void *pvUserContext);

/**
 * @brief Configures the SPI port with user configuration.
 *
 *
 * @note eSPISetMasterConfig sets the configurations for master.
 * This request expects the buffer with size of SpiMasterConfig_t.
 *
 * @note eSPIGetMasterConfig gets the current configuration for SPI master.
 * This request expects the buffer with size of SpiMasterConfig_t.
 *
 * @note eSPIGetTxNoOfbytes returns the number of written bytes in last operation.
 * This is supposed to be called in the caller task or application callback, right after last operation completes.
 * This request expects 2 bytes buffer (uint16_t).
 *
 * - If the last operation only did write, this returns the actual number of written bytes which might be smaller than the requested number (partial write).
 * - If the last operation only did read, this returns 0.
 * - If the last operation did both write and read, this returns the number of written bytes.
 *
 * @note eSPIGetRxNoOfbytes returns the number of read bytes in last operation.
 * This is supposed to be called in the caller task or application callback, right after last operation completes.
 * This request expects 2 bytes buffer (uint16_t).
 *
 * - If the last operation only did read, this returns the actual number of read bytes which might be smaller than the requested number (partial read).
 * - If the last operation only did write, this returns 0.
 * - If the last operation did both write and read, this returns the number of read bytes.
 *
 * @note eSPIActivateSlave activates the slave select line of the selected slave.
 * This request does not require a buffer, and hence NULL can be passed instead of a buffer.
 *
 * @note eSPIDeactivateSlave deactivates the slave select line of the selected slave.
 * This request does not require a buffer, and hence NULL can be passed instead of a buffer.
 *
 * @param[in] pxSPIPeripheral The SPI peripheral handle returned in open() call.
 * @param[in] xSPIRequest The configuration request from one of the
 * SpiIoctlRequest_t.
 * @param[in,out] pvBuffer The configuration values for the SPI port.
 *
 *
 * @return
 * - SPI_SUCCESS, on success
 * - SPI_INVALID_VALUE, if
 *     - pxSPIPeripheral is NULL
 *     - pxSPIPeripheral is not opened yet
 *     - pucBuffer is NULL with requests which needs buffer
 * - SPI_BUS_BUSY, if the bus is busy for only following requests:
 *     - eSPISetMasterConfig
 */
int32_t spi_ioctl(SpiHandle_t const pxSPIPeripheral,
        SpiIoctlRequest_t xSPIRequest,
        void *const pvBuffer);

/**
 * @brief The SPI master starts reading from the slave synchronously.
 *
 * This function attempts to read certain number of bytes from slave device to a pre-allocated buffer, in synchronous way.
 * This function does not return on partial read, unless there is an error.
 * And the number of bytes that have been actually read can be obtained by calling spi_ioctl.
 *
 * @warning None of other read or write functions shall be called during this function.
 *
 * @param[in] pxSPIPeripheral The SPI peripheral handle returned in open() call.
 * @param[out] pvBuffer The buffer to store the received data.
 * @param[in] xBytes The number of bytes to read.
 *
 * @return
 * - SPI_SUCCESS, on success (all the requested bytes have been read)
 * - SPI_INVALID_VALUE, if
 *     - pxSPIPeripheral is NULL
 *     - pxSPIPeripheral is not opened yet
 *     - pucBuffer is NULL
 *     - xBytes is 0
 * - SPI_READ_ERROR, if there is some unknown driver error.
 * - SPI_BUSY, if the bus is busy which means there is an ongoing operation.
 */
int32_t spi_read_sync(SpiHandle_t const pxSPIPeripheral,
        uint8_t *const pvBuffer,
        uint16_t xBytes);

/**
 * @brief The SPI master starts reading from the slave asynchronously.
 *
 * This function attempts to read certain number of bytes from a pre-allocated buffer, in asynchronous way.
 * It returns immediately when the operation is started and the status can be check by calling spi_ioctl.
 *
 * Once the operation completes successfully, the user callback will be invoked.
 * If the operation encounters an error, the user callback will be invoked.
 * The callback is not invoked on partial read, unless there is an error.
 * And the number of bytes that have been actually read can be obtained by calling spi_ioctl.
 *
 * @note Dummy data will be written to slave while reading. The dummy data value can be configured with spi_ioctl.
 * @note In order to get notification when the asynchronous call is completed, spi_set_callback must be called prior to this.
 *
 * @warning pvBuffer must be valid before callback is invoked.
 * @warning None of other read or write functions shall be called during this function or before user callback.
 *
 * @param[in] pxSPIPeripheral The SPI peripheral handle returned in open() call.
 * @param[out] pvBuffer The buffer to store the received data.
 * @param[in] xBytes The number of bytes to read.
 *
 * @return
 * - SPI_SUCCESS, on success (all the requested bytes have been read)
 * - SPI_INVALID_VALUE, if
 *     - pxSPIPeripheral is NULL
 *     - pxSPIPeripheral is not opened yet
 *     - pucBuffer is NULL
 *     - xBytes is 0
 * - SPI_READ_ERROR, if there is some unknown driver error.
 * - SPI_BUSY, if the bus is busy which means there is an ongoing operation.
 */
int32_t spi_read_async(SpiHandle_t const pxSPIPeripheral,
        uint8_t *const pvBuffer,
        uint16_t xBytes);

/**
 * @brief The SPI master starts transmission of data to the slave synchronously.
 *
 * This function attempts to write certain number of bytes from a pre-allocated buffer to a slave device, in synchronous way.
 * This function does not return on partial write, unless there is an error.
 * And the number of bytes that have been actually written can be obtained by calling spi_ioctl.
 *
 *
 * @warning None of other read or write functions shall be called during this function.
 *
 * @param[in] pxSPIPeripheral The SPI peripheral handle returned in open() call.
 * @param[in] pvBuffer The buffer with data to transmit.
 * @param[in] xBytes The number of bytes to write.
 *
 * @return
 * - SPI_SUCCESS, on success (all the requested bytes have been read)
 * - SPI_INVALID_VALUE, if
 *     - pxSPIPeripheral is NULL
 *     - pxSPIPeripheral is not opened yet
 *     - pucBuffer is NULL
 *     - xBytes is 0
 * - SPI_WRITE_ERROR, if there is some unknown driver error.
 * - SPI_BUSY, if the bus is busy which means there is an ongoing operation.
 */
int32_t spi_write_sync(SpiHandle_t const pxSPIPeripheral,
        uint8_t *const pvBuffer,
        uint16_t xBytes);

/**
 * @brief The SPI master starts transmission of data to the slave asynchronously.
 *
 * This function attempts to read certain number of bytes from a pre-allocated buffer, in asynchronous way.
 * It returns immediately when the operation is started and the status can be check by calling spi_ioctl.
 *
 * Once the operation completes successfully, the user callback will be invoked.
 * If the operation encounters an error, the user callback will be invoked.
 * The callback is not invoked on partial write, unless there is an error.
 * And the number of bytes that have been actually written can be obtained by calling spi_ioctl.
 *
 * @note In order to get notification when the asynchronous call is completed, spi_set_callback must be called prior to this.
 *
 * @warning pvBuffer must be valid before callback is invoked.
 * @warning None of other read or write functions shall be called during this function or before user callback.
 *
 * @param[in] pxSPIPeripheral The SPI peripheral handle returned in open() call.
 * @param[in] pvBuffer The buffer with data to transmit.
 * @param[in] xBytes The number of bytes to write.
 *
 * @return
 * - SPI_SUCCESS, on success (all the requested bytes have been read)
 * - SPI_INVALID_VALUE, if
 *     - pxSPIPeripheral is NULL
 *     - pxSPIPeripheral is not opened yet
 *     - pucBuffer is NULL
 *     - xBytes is 0
 * - SPI_WRITE_ERROR, if there is some unknown driver error.
 * - SPI_BUSY, if the bus is busy which means there is an ongoing operation.
 */
int32_t spi_write_async(SpiHandle_t const pxSPIPeripheral,
        uint8_t *const pvBuffer,
        uint16_t xBytes);

/**
 * @brief The SPI master starts a synchronous transfer between master and the slave.
 *
 * This function attempts to read/write certain number of bytes from/to two pre-allocated buffers at the same time, in synchronous way.
 * This function does not return on partial read/write, unless there is an error.
 * And the number of bytes that have been actually read or written can be obtained by calling spi_ioctl.
 *
 * @param[in] pxSPIPeripheral The SPI peripheral handle returned in open() call.
 * @param[in] pvTxBuffer The buffer to store the received data.
 * @param[out] pvRxBuffer The buffer with data to transmit.
 * @param[in] xBytes The number of bytes to transfer.
 *
 * @return
 * - SPI_SUCCESS, on success (all the requested bytes have been read/written)
 * - SPI_INVALID_VALUE, if
 *     - pxSPIPeripheral is NULL
 *     - pxSPIPeripheral is not opened yet
 *     - pucBuffer is NULL
 *     - xBytes is 0
 * - SPI_TRANSFER_ERROR, if there is some unknown driver error.
 * - SPI_BUSY, if the bus is busy which means there is an ongoing operation.
 */
int32_t spi_transfer_sync(SpiHandle_t const pxSPIPeripheral,
        uint8_t *const pvTxBuffer,
        uint8_t *const pvRxBuffer,
        uint16_t xBytes);

/**
 * @brief The SPI master starts a asynchronous transfer between master and the slave.
 *
 * This function attempts to read/write certain number of bytes from/to two pre-allocated buffers at the same time, in asynchronous way.
 * It returns immediately when the operation is started and the status can be check by calling spi_ioctl.
 *
 * Once the operation completes successfully, the user callback will be invoked.
 * If the operation encounters an error, the user callback will be invoked.
 * The callback is not invoked on partial read/write, unless there is an error.
 * And the number of bytes that have been actually read/write can be obtained by calling spi_ioctl.
 *
 * @param[in] pxSPIPeripheral The SPI peripheral handle returned in open() call.
 * @param[in] pvTxBuffer The buffer to store the received data.
 * @param[out] pvRxBuffer The buffer with data to transmit.
 * @param[in] xBytes The number of bytes to transfer.
 *
 * - SPI_SUCCESS, on success (all the requested bytes have been read/written)
 * - SPI_INVALID_VALUE, if
 *     - pxSPIPeripheral is NULL
 *     - pxSPIPeripheral is not opened yet
 *     - pucBuffer is NULL
 *     - xBytes is 0
 * - SPI_TRANSFER_ERROR, if there is some unknown driver error.
 * - SPI_BUSY, if the bus is busy which means there is an ongoing operation.
 */
int32_t spi_transfer_async(SpiHandle_t const pxSPIPeripheral,
        uint8_t *const pvTxBuffer,
        uint8_t *const pvRxBuffer,
        uint16_t xBytes);

/**
 * @brief Closes the SPI instance.
 * 
 * @param[in] pxSPIPeripheral The SPI peripheral handle returned in open() call.
 *
 * @return
 * - SPI_SUCCESS, on success
 * - SPI_INVALID_VALUE, if
 *     - pxSPIPeripheral is NULL
 *     - pxSPIPeripheral is not opened yet
 */
int32_t spi_close(SpiHandle_t const pxSPIPeripheral);

/**
 * @brief Stops ongoing operation - Cancel is not supported for this driver.
 *
 * @param[in] pxSPIPeripheral The SPI peripheral handle returned in open() call.
 *
 * @return
 * - SPI_FUNCTION_NOT_SUPPORTED always.
 */
int32_t spi_cancel(SpiHandle_t const pxSPIPeripheral);

/**
 * @brief This function is used to select spi slave.
 *
 * @param[in] lSPIInstance The instance of the SPI driver to initialize.
 * @param[in] lSPISlave Slave select number.
 *
 * @return
 * - SPI_SUCCESS, on success
 * - SPI_INVALID_VALUE, if
 *     - lSPISlave is invalid
 */
int32_t spi_select_slave(SpiHandle_t const pxSPIPeripheral,
        uint32_t lSPISlave);
/**
 * @}
 */
/* end of group spi_fns */

/**
 * @}
 */
/* end of group spi */

#endif /* _SOCFPGA_SPI_H_ */
