/*
 * SPDX-FileCopyrightText: Copyright (C) 2025-2026 Altera Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 * Header file for XGMAC HAL driver
 */

#ifndef __SOCFPGA_XGMAC_H__
#define __SOCFPGA_XGMAC_H__

/*
    File: Ethernet XGMAC Driver

    Ethernet HAL driver header file

    APIs for the SoC FPGA Ethernet driver

    This is XGMAC Ethernet controller driver. This is designed to work with
    tcp/ip stack like the FreeRTOS+TCP stack.

    This driver supports the following features

    - Supports 10/100 Mbps Ethernet
    - Link status detection and monitoring
    - IPv4 and IPv6 compatible

    The driver is normally not used directly. It is used indirectly via the
    tcp/ip stack.

    Refer sample applications for usage:
    - ICMP Server ping example
    - TCP server echo example
    - TCP server implementation
    - UDP server implementation
*/

#ifdef __cplusplus
extern "C"
{
#endif

#include "osal.h"

#include "socfpga_xgmac_reg.h"

#define XGMAC_MAX_INSTANCE      (3)            /* Maximum number of XGMAC instances */
#define XGMAC_PHY_TYPE_RGMII    1     /* PHY type - RGMII */

/* Descriptor configuration */
#define XGMAC_NUM_RX_DESC        512     /* Number of RX descriptors */
#define XGMAC_NUM_TX_DESC        512     /* Number of TX descriptors */
#define XGMAC_PACKET_SIZE        1536U            /* Standard Ethernet packet size */
#define XGMAC_DMA_ALIGN_BYTES    64U             /* DMA alignment size in bytes */
#define XGMAC_MAX_PACKET_SIZE    (XGMAC_PACKET_SIZE + XGMAC_DMA_ALIGN_BYTES)            /* Max packet size including alignment */
#define XGMAC_DMA_CH0            0U         /* DMA channel 0 */

/* Receive Descriptor RDES3 Bitmasks */
#define XGMAC_RDES3_OWN          BIT(31)            /* Ownership bit */
#define XGMAC_RDES3_IOC          BIT(30)            /* Interrupt on Completion */
#define XGMAC_RX_PACKET_ERROR    (RDES3_NORM_WR_LD_MASK | RDES3_NORM_WR_ES_MASK)       /* RX packet error flags */

#define DELAY_MS(ms)    osal_task_delay((ms))  /* Macro to set delay */

typedef int32_t xgmac_base_addr_t;/* XGMAC base address type */

/*
    Group: Enumerations

    Enumerated values for the Ethernet XGMAC driver.
*/
/*
    Enum: xgmac_int_status_t

    XGMAC Interrupt Status values
*/
typedef enum
{
    XGMAC_TX_DONE_EVENT, /* Transmission completed successfully */
    XGMAC_RX_EVENT,          /* Packet received */
    XGMAC_ERR_EVENT             /* Error occurred during transmit/receive */
} xgmac_int_status_t;


/*
    Enum: xgmac_err_t

    XGMAC Error Types
*/
typedef enum
{
    XGMAC_ERR_TX_STOPPED,         /* Transmit Stopped */
    XGMAC_ERR_TX_BUF_UNAVAILABLE,     /* Transmit Buffer Unavailable. */
    XGMAC_ERR_RX_BUF_UNAVAILABLE,     /* Receive Buffer Unavailable */
    XGMAC_ERR_RX_STOPPED,         /* Receive Stopped */
    XGMAC_ERR_DESC_DEFINE,          /* Descriptor Definition Error */
    XGMAC_ERR_FATAL_BUS,                /* Fatal Bus Error */
    XGMAC_ERR_CNTXT_DESC,             /* Context Descriptor Error */
    XGMAC_ERR_UNHANDLED,               /* Unhandled Interrupt */
} xgmac_err_t;

/*
    Group: Structures

    Callback signatures and structures for the Ethernet XGMAC driver.
*/
typedef struct
{
    uint8_t err_type;       /* Error Code for Error Handling and Reporting */
    uint8_t err_ch; /* DMA Error channel Number */

} xgmac_err_info_t;

/*
    Struct: xgmac_buf_desc_t

    XGMAC Buffer Descriptor Ring Structure
*/
typedef struct
{
    uint32_t des0;   /* Descriptor word 0 */
    uint32_t des1;   /* Descriptor word 1 */
    uint32_t des2;   /* Descriptor word 2 */
    uint32_t des3;   /* Descriptor word 3 */
} xgmac_buf_desc_t;

typedef struct
{
    uint8_t *buf;       /* Pointer to transmit buffer */
    uint32_t size;          /* Size of the buffer in bytes */
    uint8_t release_buf;  /* Flag to release buffer after transmit */
} xgmac_tx_buf_t;

typedef struct
{
    uint8_t *buf;       /* Pointer to receive buffer */
    uint32_t size;          /* Size of received data in bytes */
    uint32_t packet_status;  /* Status of the received packet */
} xgmac_rx_buf_t;
/*
    Callback: xgmac_callback_t

    Function pointer for user callback
*/
typedef void (*xgmac_callback_t)(xgmac_int_status_t int_status, void *irq_data);

/*
    Group: Structures

    Structures, types and variables for the Ethernet XGMAC driver.
*/
/*
    Variable: xgmac_desc_t

    The XGMAC descriptor type defined in the source file.
*/
struct xgmac_desc_t;

/*
    Type: xgmac_handle_t

    xgmac_handle_t is the handle type returned by calling SocfpgaXGMAC_open().
    This is initialized in open and returned to caller. The caller must pass
    this pointer to the rest of APIs.
*/
typedef struct xgmac_desc_t *xgmac_handle_t;

/*
    Struct: xgmac_config_t

    Configuration structure for XGMAC Instance Parameters
*/
typedef struct
{
    int32_t instance;           /* EMAC index */
    uint8_t phy_type;            /* PHY type */
    xgmac_handle_t hxgmac; /* Pointer to XGMAC handle */
} xgmac_config_t;


/*
    Group: Functions

    Entry points for the Ethernet XGMAC driver.
*/
/*
    Function: xgmac_emac_init

    Initialize the XGMAC.

    The application should call this function to initialize the desired XGMAC.

    Parameters:

        cfg - The configuration structure for XGMAC.

    Returns:

        - 'Handle' on XGMAC successful operation.
        - 'NULL' if the XGMAC instance number is invalid or XGMAC already initialized.
*/
xgmac_handle_t xgmac_emac_init(xgmac_config_t *cfg);

/*
    Function: xgmac_emac_deinit

    Deinitialize the XGMAC.

    The application should call this function to deinitialize the desired
    XGMAC.

    Parameters:

        hxgmac - The handle of XGMAC instance.

    Returns:

        -  0:      if XGMAC successful deinitialized.
        - -EINVAL: if the XGMAC handle passed is null or not initialized already.
*/
int32_t xgmac_emac_deinit(xgmac_handle_t hxgmac);


/*
    Function: xgmac_emac_start

    start the XGMAC initialization.

    The application should call this function to initialize the desired XGMAC
    and start the tx & rx.

    Parameters:

        hxgmac - The instance of the XGMAC to start the XGMAC.

    Returns:

        - 0: on XGMAC successful configuration and start operation.
*/
int32_t xgmac_emac_start(xgmac_handle_t hxgmac);

/*
    Function: xgmac_emac_stop

    stop the XGMAC.

    The application should call this function to stop the XGMAC.

    Parameters:

        hxgmac - The instance of the XGMAC to stop.
*/
void xgmac_emac_stop(xgmac_handle_t hxgmac);

/*
    Function: xgmac_set_callback

    set the callback function to be called on completion of an operation.

    The application should call this function to set the callback is
    guaranteed to be invoked when an interrupt occurs.

    Parameters:

        hxgmac - The instance of the XGMAC to set the callback for.
        callback - The callback function to be called on completion of an
            operation.
        pcntxt - The user context pointer to be passed to the callback
            function

    Returns:

        -  0:      on XGMAC successfully stop operation.
        - -EINVAL: if hxgmac is NULL
        - -EBUSY:  if XGMAC has already been started.
*/
int32_t xgmac_set_callback(xgmac_handle_t hxgmac, xgmac_callback_t callback, void *pcntxt);

/*
    Function: xgmac_get_inst_base_addr

    Get the base address of the XGMAC instance.

    Parameters:

        hxgmac - The instance of the XGMAC to get the base address.

    Returns:

        - 0: If hxgmac is NULL.
        - Base address of the XGMAC instance.
*/

int32_t xgmac_get_inst_base_addr(xgmac_handle_t hxgmac);
/*
    Function: xgmac_get_err_info

    Get the error information of the XGMAC instance.

    Parameters:

        hxgmac - The instance of the XGMAC to get the error information.

    Returns:

        - NULL: If hxgmac is NULL.
        - Pointer to the error information structure of the XGMAC instance.
*/

xgmac_err_info_t *xgmac_get_err_info(xgmac_handle_t hxgmac);
/*
    Function: xgmac_dma_initialize

    Initialize the XGMAC DMA.

    The application should call this function to initialize the desired XGMAC
    DMA.

    Parameters:

        hxgmac - The instance of the XGMAC to initialize the DMA.

    Returns:

        -  0:   on XGMAC DMA successful initialization.
        - -EIO: if DMA failed to initialize.
*/
int32_t xgmac_dma_initialize(xgmac_handle_t hxgmac);

/*
    Function: xgmac_dma_deinitialize

    Deinitialize the XGMAC DMA.

    The application should call this function to deinitialize the desired
    XGMAC DMA.

    Parameters:

        hxgmac - The instance of the XGMAC to deinitialize the DMA.

    Returns:

        -  0:   on XGMAC DMA successful deinitialization.
        - -EIO: if DMA failed to deinitialize.
*/
int32_t xgmac_dma_deinitialize(xgmac_handle_t hxgmac);

/*
    Function: xgmac_dma_transmit

    Initiate the transmit out of buffer via DMA.

    The application should call this once the data is ready to be transmitted.

    Parameters:

        hxgmac - The instance of the XGMAC to stop.
        dma_tx_buf - The structure to buffer descriptor. It contains the
            pointer to the buffer, size of buffer and the flag to notify the
            dma driver regarding releasing the buffer for the stack.

    Returns:

        - 0:    if DMA successfully transmit the buffer.
        - -EIO: if DMA failed to transmit the buffer
*/
int32_t xgmac_dma_transmit(xgmac_handle_t hxgmac, xgmac_tx_buf_t *dma_tx_buf);

/*
    Function: xgmac_dma_tx_done

    Check if the transmit is done to release the buffer.

    The application should call this once it gets an event notification after
    a dma transmit.

    Parameters:

        hxgmac - The instance of the XGMAC to stop.
        release_buffer - The released buffer which will be used for next
            transmit.

    Returns:

        -  0:      if successfully obtained the status of buffer transmission.
        - -EIO:    if failed to obtain the status of buffer transmission.
        - -EAGAIN: if application failed to obtain the status of dma descriptor.
*/
int32_t xgmac_dma_tx_done(xgmac_handle_t hxgmac, uint8_t **release_buffer);

/*
    Function: xgmac_dma_receive

    Initiate the receive of the buffer via DMA.

    The application should call this once the data is ready to be received in
    the dma fifo.

    Parameters:

        hxgmac - The instance of the XGMAC to stop.
        dma_rx_buf - The structure to buffer descriptor. It contains the
            pointer to the buffer, size of buffer and the packet status which
            will be validated by the stack to consider or discard the buffer.

    Returns:

        - 0:       if DMA successfully receive the buffer.
        - -EAGAIN: if DMA failed to receive the buffer
*/
int32_t xgmac_dma_receive(xgmac_handle_t hxgmac, xgmac_rx_buf_t *dma_rx_buf);

/*
    Function: xgmac_refill_rx_descriptor

    Refill a receive descriptor with a new buffer.

    This function updates the RX descriptor to point to a fresh buffer so the
    DMA can receive new incoming packets.

    Parameters:

        hxgmac - The instance of the XGMAC to refill the descriptor for.
        buf - Pointer to the new buffer to assign to the RX descriptor.

    Returns:

        - 0: if the descriptor was successfully refilled.
*/
int32_t xgmac_refill_rx_descriptor(xgmac_handle_t hxgmac, uint8_t *buf);

/*
    Function: xgmac_dma_flush_buffers

    Flush the DMA buffers.

    Parameters:

        hxgmac - The instance of the XGMAC to flush the buffers.
*/
void xgmac_dma_flush_buffers(xgmac_handle_t hxgmac);

#ifdef __cplusplus
}
#endif

#endif /* ifndef __SOCFPGA_XGMAC_H__ */
