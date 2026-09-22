/*
 * Common IO - basic V1.0.0
 * Copyright (C) 2020 Amazon.com, Inc. or its affiliates. All Rights Reserved.
 * Copyright (C) 2025-2026 Altera Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is furnished to
 * do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * http://aws.amazon.com/freertos
 * http://www.FreeRTOS.org
 *
 * Modified for SoC FPGA
 */

#ifndef __SOCFPGA_SPI_H__
#define __SOCFPGA_SPI_H__

/*
    File: SPI Driver

    SoC FPGA SPI HAL driver

    APIs for SoC FPGA SPI driver.

    This is the SPI driver implementation for SoC FPGA. It supports master and
    slave mode operation. It provides APIs for data transfer with an SPI peer.
    The APIs are designed to be used in both synchronous and asynchronous
    modes. To see example usage, see SPI sample application.
*/

/* Standard includes. */
#include <stdint.h>
#include <stddef.h>
#include <errno.h>
#include "socfpga_dma.h"

/*
    Group: Enumerations

    SPI Specific Enumerations
*/

/*
    Enum: spi_xfer_status_t

    The SPI return status from Async operations.
*/
typedef enum
{
    SPI_SUCCESS,        /* SPI operation completed successfully. */
    SPI_WR_ERROR,       /* Write operation error. */
    SPI_RD_ERROR,       /* Read operation error. */
    SPI_XFER_ERROR,     /* Full-duplex transfer error. */
    SPI_RX_OVERFLOW,    /* Slave RX FIFO overflow; bytes were lost (RXOI). */
} spi_xfer_status_t;

/*
    Enum: spi_mode_t

    The SPI Modes denoting the clock polarity and clock phase.
*/
typedef enum
{
    SPI_MODE0, /* CPOL = 0 and CPHA = 0 */
    SPI_MODE1, /* CPOL = 0 and CPHA = 1 */
    SPI_MODE2, /* CPOL = 1 and CPHA = 0 */
    SPI_MODE3, /* CPOL = 1 and CPHA = 1 */
} spi_mode_t;

/*
    Enum: spi_role_t

    SPI controller role (master/slave).
*/
typedef enum
{
    SPI_ROLE_MASTER = 0,
    SPI_ROLE_SLAVE = 1,
} spi_role_t;

/*
    Enum: spi_ioctl_t

    Ioctl request for SPI HAL.
*/
typedef enum
{
    SPI_SET_CONFIG,     /* Configure SPI using spi_cfg_t (input). */
    SPI_GET_CONFIG,     /* Read current config into spi_cfg_t (output). */
    SPI_GET_TX_NBYTES,  /* Get last TX byte count (uint32_t*, output). */
    SPI_GET_RX_NBYTES,  /* Get last RX byte count (uint32_t*, output). */
    SPI_ENABLE_DMA,     /* Enable DMA using spi_dma_config_t (input). */
    SPI_DISABLE_DMA,  /* Disable DMA mode; buf is ignored (pass NULL). */
} spi_ioctl_t;

/*
    Group: Structures

    Callback signatures, structures and types for the SPI driver.
*/
/*
    Struct: spi_cfg_t

    The configuration parameters for the SPI controller.

    The application configures a handle using <spi_ioctl> with
    SPI_SET_CONFIG.
*/
typedef struct
{
    uint32_t clk;      /* SPI frequency set for data transmission in Hz. */
    spi_mode_t mode; /* Mode selected as per enum spi_mode_t. */
    spi_role_t role;  /* SPI role: master or slave. */
} spi_cfg_t;

/*
    Struct: spi_dma_config_t

    DMA configuration for SPI master transfers.

    Used with SPI_ENABLE_DMA. The driver opens and configures two DMA
    channels:
    - TX: memory to SPI (DMAC flow controller)
    - RX: SPI to memory (DMAC flow controller)

    Notes:

        tx_prio/rx_prio are forwarded to the DMA driver as channel priority.
        As of today, the DMA driver does not apply this field when programming
        the
        DMAC registers.
        For SPI, RX DMA is sensitive to burst/threshold programming.
        The driver selects burst length automatically per transfer based on
        nbytes,
        with a maximum of DMA_BURST_LEN_8.
*/
typedef struct
{
    uint32_t tx_instance; /* TX DMA instance (DMA_INSTANCE0/DMA_INSTANCE1). */
    uint32_t tx_channel;  /* DMA channel for TX (DMA_CH1..DMA_CH4). */
    uint8_t tx_prio;      /* TX DMA channel priority. */
    uint32_t rx_instance; /* RX DMA instance (DMA_INSTANCE0/DMA_INSTANCE1). */
    uint32_t rx_channel;  /* DMA channel for RX (DMA_CH1..DMA_CH4). */
    uint8_t rx_prio;      /* RX DMA channel priority. */
} spi_dma_config_t;

/*
    Variable: spi_handle

    The SPI descriptor type defined in the source file.
*/
struct spi_handle;

/*
    Type: spi_handle_t

    spi_handle_t is the handle type returned by calling spi_open(). This is
    initialized in open and returned to caller. The caller must pass this
    pointer to the rest of APIs.
*/
typedef struct spi_handle *spi_handle_t;

/*
    Callback: spi_callback_t

    The callback function for completion of SPI operation.

    Parameters:

        status - The status of the SPI operation.
        pparam - The user context passed when setting the callback.
*/
typedef void (*spi_callback_t)(spi_xfer_status_t status, void *pparam);

/*
    Group: Functions

    Entry points for the SPI driver.
*/
/*
    Function: spi_open

    Initializes SPI master peripheral with default configuration.

    Parameters:

        instance - The instance of the SPI driver to initialize.

    Returns:

        - 'the handle to the SPI port (not NULL)', on success.
        - 'NULL', if
            - invalid instance number
            - open same instance more than once before closing it

    Notes:

        Warning: Once opened, the same SPI instance must be closed before
        calling
        open again.
*/
spi_handle_t spi_open(uint32_t instance);

/*
    Function: spi_slave_open

    Initializes SPI slave peripheral.

    Parameters:

        instance - The instance of the SPI driver to initialize.

    Returns:

        - 'the handle to the SPI port (not NULL)', on success.
        - 'NULL', if
            - invalid instance number
            - open same instance more than once before closing it

    Notes:

        Warning: Once opened, the same SPI instance must be closed before
        calling
        open again.
*/
spi_handle_t spi_slave_open(uint32_t instance);

/*
    Function: spi_set_callback

    Sets the application callback to be called on completion of an operation.

    The callback is invoked for asynchronous transfers started by
    <spi_xfer_async>.

    In slave mode, the callback may also be invoked to report RX overflow
    (SPI_RX_OVERFLOW).

    Parameters:

        hspi - The SPI peripheral handle returned in the open() call.
        callback - The callback to call on completion of operation.
        pcntxt - The user context to be passed back when callback is
            called.

    Returns:

        - 0:       on success
        - -EINVAL: if
            - hspi is NULL
        - -EBUSY:  if the bus is busy

    Notes:

        This callback will not be invoked when synchronous operation
        completes.
        This callback is per handle. Each instance has its own callback.
        Newly set callback overrides the one previously set.
        Callback is invoked from interrupt context (SPI ISR / DMA ISR path).
        The callback must not call blocking APIs and must use only ISR-safe
        OSAL APIs.
*/
int32_t spi_set_callback(spi_handle_t const hspi, spi_callback_t callback,
        void *pcntxt);

/*
    Function: spi_ioctl

    Issues an ioctl request on the SPI handle.

    Parameters:

        hspi - The SPI peripheral handle returned in open() call.
        cmd - The configuration request from one of the spi_ioctl_t.
        buf - The configuration values for the SPI port.

    Returns:

        - 0:       on success
        - -EINVAL: if
            - hspi is NULL
            - hspi is not opened yet
            - buf is NULL for requests which require a buffer
        - -EBUSY:  if the bus is busy for requests which require an idle bus
        (SPI_SET_CONFIG/SPI_ENABLE_DMA/SPI_DISABLE_DMA)
        - -EBUSY:  if DMA is already enabled for SPI_ENABLE_DMA

    Notes:

        SPI_SET_CONFIG: Set the SPI controller configuration.
        buf points to a spi_cfg_t provided by the caller (driver reads it).
        SPI_GET_CONFIG: Get the current SPI controller configuration.
        buf points to a spi_cfg_t provided by the caller (driver populates
        it).
        SPI_GET_TX_NBYTES: Get the number of bytes written by the last
        operation. Call this in the caller task (sync) or in the application
        callback (async), right after the last operation completes.
        buf points to a uint32_t provided by the caller (driver populates it).
        - If the last operation did not use a TX buffer (txbuf was NULL), this
        returns 0.
        - Otherwise, this returns the number of bytes transmitted.
        SPI_GET_RX_NBYTES: Get the number of bytes read by the last
        operation. Call this in the caller task (sync) or in the application
        callback (async), right after the last operation completes.
        buf points to a uint32_t provided by the caller (driver populates it).
        - If the last operation did not use an RX buffer (rxbuf was NULL),
        this
        returns 0.
        - Otherwise, this returns the number of bytes received.
        SPI_ENABLE_DMA: Enable DMA transfer mode.
        buf points to a spi_dma_config_t provided by the caller (driver reads
        it).
        This request is valid only when there is no ongoing transfer.
        SPI_DISABLE_DMA: Disable DMA transfer mode.
        buf is ignored (pass NULL).
        This request is valid only when there is no ongoing transfer.
*/
int32_t spi_ioctl(spi_handle_t const hspi, spi_ioctl_t cmd, void *const buf);

/*
    Function: spi_xfer_sync

    Perform a synchronous SPI transfer.

    This function reads/writes nbytes bytes synchronously. On success, the
    number of bytes actually transferred can be obtained via <spi_ioctl>
    (see SPI_GET_TX_NBYTES and SPI_GET_RX_NBYTES).

    Parameters:

        hspi - SPI handle returned by spi_open() or spi_slave_open().
        txbuf - Transmit buffer pointer (may be NULL for RX-only transfers
            in slave mode). - Interrupt/PIO mode: treated as uint8_t[nbytes].
            - DMA mode: treated as uint32_t[nbytes] where each 32-bit word
            represents one SPI byte in bits [7:0].
        rxbuf - Receive buffer pointer (may be NULL for TX-only
            transfers). - Interrupt/PIO mode: treated as uint8_t[nbytes]. -
            DMA mode: treated as uint8_t[nbytes].
        nbytes - Number of SPI bytes to clock.

    Returns:

        - 0:       on success (all the requested bytes have been read/written)
        - -EINVAL: if
            - hspi is NULL
            - hspi is not opened yet
            - txbuf is NULL for master transfers
            - txbuf and rxbuf are both NULL
            - nbytes is 0
        - -EIO:    if there is some unknown driver error.
        - -EBUSY:  if the bus is busy which means there is an ongoing operation.

    Notes:

        When DMA is enabled, the caller is responsible for DMA-safe buffer
        properties (for example, alignment and cache maintenance policy).
*/
int32_t spi_xfer_sync(spi_handle_t const hspi, void *const txbuf,
        void *const rxbuf, uint32_t nbytes);

/*
    Function: spi_xfer_async

    Perform an asynchronous SPI transfer.

    This function starts a transfer and returns immediately. Completion is
    reported through the callback set by <spi_set_callback>. Buffer
    interpretation matches <spi_xfer_sync>.

    Parameters:

        hspi - SPI handle returned by spi_open() or spi_slave_open().
        txbuf - Transmit buffer pointer (see <spi_xfer_sync>).
        rxbuf - Receive buffer pointer (see <spi_xfer_sync>).
        nbytes - Number of SPI bytes to clock.

    Returns:

        - 0:       on success (all the requested bytes have been read/written)
        - -EINVAL: if
            - hspi is NULL
            - hspi is not opened yet
            - txbuf is NULL for master transfers
            - txbuf and rxbuf are both NULL
            - nbytes is 0
        - -EIO:    if there is some unknown driver error.
        - -EBUSY:  if the bus is busy which means there is an ongoing operation.

    Notes:

        When DMA is enabled, RX buffers may require cache invalidation after
        completion (depending on platform cache configuration).
*/
int32_t spi_xfer_async(spi_handle_t const hspi, void *const txbuf,
        void *const rxbuf, uint32_t nbytes);

/*
    Function: spi_close

    Closes the SPI instance.

    Parameters:

        hspi - The SPI peripheral handle returned in open() call.

    Returns:

        - 0:       on success
        - -EINVAL: if
            - hspi is NULL
            - hspi is not opened yet
        - -EIO:    if there is an internal driver error.
*/
int32_t spi_close(spi_handle_t const hspi);

/*
    Function: spi_cancel

    Stops an ongoing SPI transfer.

    In DMA mode this function stops active DMA channels after disabling SPI
    DMA requests. In slave DMA RX mode, any bytes already present in SPI RX
    FIFO are drained into the user RX buffer so SPI_GET_RX_NBYTES
    reports the total received bytes captured by DMA and FIFO drain.

    Parameters:

        hspi - The SPI peripheral handle returned in open() call.

    Returns:

        - 0:       on success.
        - -EINVAL: if
            - hspi is NULL
            - hspi is not opened yet
            - SPI instance is idle
        - -EIO:    if an internal stop operation fails.
*/
int32_t spi_cancel(spi_handle_t const hspi);

/*
    Function: spi_select_slave

    This function is used to select spi slave.

    Parameters:

        hspi - The SPI peripheral handle returned in open() call.
        ss - Slave select number.

    Returns:

        - 0:       on success
        - -EINVAL: if
             - ss is invalid
             - hspi is NULL
             - hspi is not opened yet
        - -ENOTSUP: if called on a slave-mode handle
*/
int32_t spi_select_slave(spi_handle_t const hspi, uint32_t ss);
/* end of group spi_fns */
/* end of group spi */

#endif /* __SOCFPGA_SPI_H__ */
