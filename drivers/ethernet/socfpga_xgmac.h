/*
 * SPDX-FileCopyrightText: Copyright (C) 2025 Altera Corporation
 *
 * SPDX-License-Identifier: MIT-0
 *
 * Header file for XGMAC HAL driver
 */

#ifndef __SOCFPGA_XGMAC_H__
#define __SOCFPGA_XGMAC_H__

/**
 * @file socfpga_xgmac.h
 * @brief Ethernet HAL driver header file
 * 
 */

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>

#include "FreeRTOS_IP.h"

#include "socfpga_xgmac_reg.h"

/**
 * @defgroup enet Ethernet
 * @ingroup drivers
 * @{
 */

/**
 * @defgroup enet_fns Functions
 * @ingroup enet
 * Ethernet HAL APIs
 */

/**
 * @defgroup enet_structs Structures 
 * @ingroup enet
 * Ethernet Specific Structures
 */

/**
 * @defgroup enet_enums Enumerations
 * @ingroup enet
 * Ethernet Specific Enumerations
 */

/**
 * @defgroup enet_macros Macros
 * @ingroup enet
 * Ethernet Specific Macros 
 */

/**
 * @addtogroup enet_macros
 * @{
 */

#define XGMAC_MAX_INSTANCE                   (3)     /*!< Maximum number of XGMAC instances */

#define SOCFPGA_XGMAC_EMACINDEX0            0       /*!< EMAC index 0 */
#define SOCFPGA_XGMAC_EMACINDEX1            1       /*!< EMAC index 1 */
#define SOCFPGA_XGMAC_EMACINDEX2            2       /*!< EMAC index 2 */

#define SOCFPGA_XGMAC_IOTYPE_HPS            0       /*!< IO type HPS */
#define SOCFPGA_XGMAC_IOTYPE_FPGA           1       /*!< IO type FPGA */

#define SOCFPGA_XGMAC_PHY_TYPE_GMII         0       /*!< PHY type GMII */
#define SOCFPGA_XGMAC_PHY_TYPE_RGMII        1       /*!< PHY type RGMII */
#define SOCFPGA_XGMAC_PHY_TYPE_RMII         2       /*!< PHY type RMII */
#define SOCFPGA_XGMAC_PHY_TYPE_SGMII        3       /*!< PHY type SGMII */
#define SOCFPGA_XGMAC_PHY_TYPE_SGMII_PLUS   4       /*!< PHY type SGMII+ */

#define SOCFPGA_XGMAC_AUTONEG_ENABLE        1       /*!< Enable auto-negotiation */
#define SOCFPGA_XGMAC_AUTONEG_DISABLE       0       /*!< Disable auto-negotiation */

#define SOCFPGA_XGMAC_PHY_SPEED_10M         1       /*!< PHY speed 10 Mbps */
#define SOCFPGA_XGMAC_PHY_SPEED_100M        2       /*!< PHY speed 100 Mbps */
#define SOCFPGA_XGMAC_PHY_SPEED_1G          3       /*!< PHY speed 1 Gbps */
#define SOCFPGA_XGMAC_PHY_SPEED_2POINT5G    4       /*!< PHY speed 2.5 Gbps */

#define SOCFPGA_XGMAC_DUPLEXMODE_HALF       1       /*!< Half duplex mode */
#define SOCFPGA_XGMAC_DUPLEXMODE_FULL       2       /*!< Full duplex mode */

#define SOCFPGA_XGMAC_CHECKSUM_BY_HARDWARE  1       /*!< Use hardware checksum */
#define SOCFPGA_XGMAC_CHECKSUM_BY_SOFTWARE  2       /*!< Use software checksum */

#define SOCFPGA_XGMAC_EMAC_STARTED          0x1     /*!< EMAC started flag */
#define SOCFPGA_XGMAC_EMAC_STOPPED          0x0     /*!< EMAC stopped flag */

/*!< Descriptor configuration */
#define XGMAC_NOF_DESCRIPTORS_TX            512     /*!< Number of TX descriptors */
#define XGMAC_NOF_DESCRIPTORS_RX            512     /*!< Number of RX descriptors */

#define XGMAC_PACKET_SIZE                   1536    /*!< Standard Ethernet packet size */
#define XGMAC_DMA_ALIGN_BYTES               64      /*!< DMA alignment size in bytes */
#define XGMAC_MAX_PACKET_SIZE               (XGMAC_PACKET_SIZE + XGMAC_DMA_ALIGN_BYTES) /*!< Max packet size including alignment */

#define XGMAC_RX_BUFFER_SIZE                (XGMAC_NOF_DESCRIPTORS_RX * XGMAC_MAX_PACKET_SIZE) /*!< RX buffer size */
#define XGMAC_TX_BUFFER_SIZE                (XGMAC_NOF_DESCRIPTORS_TX * XGMAC_MAX_PACKET_SIZE) /*!< TX buffer size */

#define XGMAC_AXI_WIDTH_64                  8       /*!< AXI width of 64 bits (8 bytes) */
#define XGMAC_DMA_DESC_SIZE                 16      /*!< DMA descriptor size in bytes */

#define XGMAC_DMA_CHANNEL0                  0       /*!< DMA channel 0 */
#define XGMAC_DMA_CHANNEL1                  1       /*!< DMA channel 1 */
#define XGMAC_DMA_CHANNEL2                  2       /*!< DMA channel 2 */
#define XGMAC_DMA_CHANNEL3                  3       /*!< DMA channel 3 */
#define XGMAC_DMA_CHANNEL4                  4       /*!< DMA channel 4 */
#define XGMAC_DMA_CHANNEL5                  5       /*!< DMA channel 5 */
#define XGMAC_DMA_CHANNEL6                  6       /*!< DMA channel 6 */
#define XGMAC_DMA_CHANNEL7                  7       /*!< DMA channel 7 */

#define XGMAC_MTL_NUM_TX_QUEUE1             1       /*!< Number of TX queues: 1 */
#define XGMAC_MTL_NUM_TX_QUEUE2             2       /*!< Number of TX queues: 2 */
#define XGMAC_MTL_NUM_TX_QUEUE3             3       /*!< Number of TX queues: 3 */
#define XGMAC_MTL_NUM_TX_QUEUE4             4       /*!< Number of TX queues: 4 */
#define XGMAC_MTL_NUM_TX_QUEUE5             5       /*!< Number of TX queues: 5 */
#define XGMAC_MTL_NUM_TX_QUEUE6             6       /*!< Number of TX queues: 6 */
#define XGMAC_MTL_NUM_TX_QUEUE7             7       /*!< Number of TX queues: 7 */
#define XGMAC_MTL_NUM_TX_QUEUE8             8       /*!< Number of TX queues: 8 */

#define XGMAC_MTL_NUM_RX_QUEUE1             1       /*!< Number of RX queues: 1 */
#define XGMAC_MTL_NUM_RX_QUEUE2             2       /*!< Number of RX queues: 2 */
#define XGMAC_MTL_NUM_RX_QUEUE3             3       /*!< Number of RX queues: 3 */
#define XGMAC_MTL_NUM_RX_QUEUE4             4       /*!< Number of RX queues: 4 */
#define XGMAC_MTL_NUM_RX_QUEUE5             5       /*!< Number of RX queues: 5 */
#define XGMAC_MTL_NUM_RX_QUEUE6             6       /*!< Number of RX queues: 6 */
#define XGMAC_MTL_NUM_RX_QUEUE7             7       /*!< Number of RX queues: 7 */
#define XGMAC_MTL_NUM_RX_QUEUE8             8       /*!< Number of RX queues: 8 */

/*!< Transmit Descriptor TDES2 Bitmasks */
#define XGMAC_TDES2_IC                      BIT(31) /*!< Interrupt on Completion */
#define XGMAC_TDES2_B1L                     0x00001FFF /*!< Buffer 1 Length */

/*!< Transmit Descriptor TDES3 Bitmasks */
#define XGMAC_TDES3_OWN                     BIT(31) /*!< Ownership bit */
#define XGMAC_TDES3_CONTEXT_TYPE            BIT(31) /*!< Context descriptor type */
#define XGMAC_TDES3_FD                      BIT(29) /*!< First descriptor */
#define XGMAC_TDES3_LD                      BIT(28) /*!< Last descriptor */

#define XGMAC_Tx_DESC_CIC_DISABLE           (0 << 16) /*!< Checksum Insertion Control: Disabled */
#define XGMAC_Tx_DESC_CIC_IPHEADER_ONLY     (1 << 16) /*!< Checksum on IP header only */
#define XGMAC_Tx_DESC_CIC_IPHEADER_PAYLOAD  (2 << 16) /*!< Checksum on IP header and payload */
#define XGMAC_Tx_DESC_CIC_FULL_HEADER       (3 << 16) /*!< Checksum on entire packet */

#define XGMAC_Tx_DESC_CIC_DISABLE_MASK      0x00018000 /*!< Mask to disable checksum insertion */

/*!< Receive Descriptor RDES3 Bitmasks */
#define XGMAC_RDES3_OWN                     BIT(31) /*!< Ownership bit */
#define XGMAC_RDES3_IOC                     BIT(30) /*!< Interrupt on Completion */
#define XGMAC_RDES3_BUF2AP                  0x3FFFFFFF /*!< Buffer 2 Address Pointer */
#define XGMAC_RDES3_PL                      0x00001FFF /*!< Packet length field */

#define XGMAC_RECEIVE_PACKET_ERROR          (RDES3_NORM_WR_LD_MASK | RDES3_NORM_WR_ES_MASK) /*!< RX packet error flags */



#define XGMAC_BaseAddr_t                      uint32_t

/* Delay */
#define DELAY_MS(ms)     osal_task_delay(ms)
/**
 * @}
 */

/**
 * @addtogroup enet_enums
 * @{
 */
/**
 * @brief XGMAC Interrupt Status values
 */
typedef enum
{
    eXGMACTransmitDoneEvent, /*!< Transmission completed successfully */
    eXGMACReceiveEvent,          /*!< Packet received */
    eXGMACErrorEvent             /*!< Error occurred during transmit/receive */
} XGMACInterruptStatus_t;


typedef enum
{
    eErrTransmitStopped,         /*!< Transmit Stopped */
    eErrTxBufferUnavailable,     /*!< Transmit Buffer Unavailable.*/
    eErrRxBufferUnavailable,     /*!< Receive Buffer Unavailable */
    eErrtReceiveStopped,         /*!< Receive Stopped */
    eErrDescDefinition,          /*!< Descriptor Definition Error  */
    eErrFatalBus,                /*!< Fatal Bus Error  */
    eErrContextDesc,             /*!< Context Descriptor Error  */
    eErrUnhandled,               /*!< Unhandled Interrupt  */
} XgmacErrorType_t;
/**
 * @}
 */

/**
 * @addtogroup enet_structs
 * @{
 */
typedef struct
{
    uint8_t ucErrType;              /*!< Error Code for Error Handling and Reporting */
    uint8_t ucErrDMAChnlNum;        /*!< DMA Error Channel Number */

}XGMACInterruptData_t;

/**
 * @brief  XGMAC Buffer Descriptor Ring Structure
 */
typedef struct
{
    uint32_t ulDes0;   /*!< Descriptor word 0 */
    uint32_t ulDes1;   /*!< Descriptor word 1 */
    uint32_t ulDes2;   /*!< Descriptor word 2 */
    uint32_t ulDes3;   /*!< Descriptor word 3 */
} SocfpgaXGMACBufferDesc_t;

typedef struct
{
    uint8_t *pucBuffer;       /*!< Pointer to transmit buffer */
    uint32_t ulSize;          /*!< Size of the buffer in bytes */
    uint8_t ucReleaseBuffer;  /*!< Flag to release buffer after transmit */
} XGMACDMATxBufOut_t;

typedef struct
{
    uint8_t *pucBuffer;       /*!< Pointer to receive buffer */
    uint32_t ulSize;          /*!< Size of received data in bytes */
    uint32_t ulPacketStatus;  /*!< Status of the received packet */
} XGMACDMARxBufIn_t;

/**
 * @}
 */
/**
 * Function pointer for user callback
 * @ingroup enet_fns
 */
typedef void (*SocfpgaXGMAClback_t)( XGMACInterruptStatus_t xIntrStatus,
        void *pvIrqData );

/**
 * @addtogroup enet_structs
 * @{
 */
/**
 * @brief The XGMAC descriptor type defined in the source file.
 */

typedef struct
{
    XGMAC_BaseAddr_t *pXGMACInstBaseAddr;       /*!< Pointer to XGMAC instance base address */
    XGMAC_BaseAddr_t *pXGMACInstDMABaseAddr;    /*!< Pointer to XGMAC DMA base address */
    int32_t lInstance;                           /*!< Instance number */
    uint8_t ucChecksumMode;                      /*!< Checksum mode */
    uint8_t ucPhyType;                           /*!< PHY type */

    SocfpgaXGMACBufferDesc_t *pTxBdRing;        /*!< Transmit buffer descriptor ring */
    SocfpgaXGMACBufferDesc_t *pRxBdRing;        /*!< Receive buffer descriptor ring */

    uint8_t *pTxDMABuf1AP[XGMAC_NOF_DESCRIPTORS_TX]; /*!< Transmit DMA buffer pointers */
    uint8_t *pRxDMABuf1AP[XGMAC_NOF_DESCRIPTORS_RX]; /*!< Receive DMA buffer pointers */

    SocfpgaXGMACBufferDesc_t axBufferDescTx[XGMAC_NOF_DESCRIPTORS_TX] /*!< Aligned transmit buffer descriptors */
        __attribute__((aligned(XGMAC_DMA_ALIGN_BYTES)));
    SocfpgaXGMACBufferDesc_t axBufferDescRx[XGMAC_NOF_DESCRIPTORS_RX] /*!< Aligned receive buffer descriptors */
        __attribute__((aligned(XGMAC_DMA_ALIGN_BYTES)));

    volatile int32_t lTxDescHead;                /*!< Transmit descriptor head index */
    volatile int32_t lTxDescTail;                /*!< Transmit descriptor tail index */
    volatile int32_t lRxDescHead;                /*!< Receive descriptor head index */
    volatile int32_t lRxDescTail;                /*!< Receive descriptor tail index */

    SocfpgaXGMAClback_t xCallbackFunc;           /*!< Callback function */
    void *pvUserContext;                          /*!< User context pointer */
    XGMACInterruptData_t xXGMACIntData;          /*!< Interrupt error data */

    int8_t cIsInitialized;                        /*!< Indicates if device handle is initialized */
    int8_t cIsStarted;                            /*!< Indicates if device is started */
    int8_t cIsReady;                              /*!< Indicates if device is ready */

    SemaphoreHandle_t xTransmitSyncSem;           /*!< Transmit synchronization semaphore */
    SemaphoreHandle_t xTransmiSyncMutex;           /*!< Transmit synchronization mutex */

} SocfpgaXGMACDescriptor_t;


/**
 * @brief SocfpgaXGMACHandle_t is the handle type returned by calling SocfpgaXGMAC_open().
 *        This is initialized in open and returned to caller. The caller must pass
 *        this pointer to the rest of APIs.
 */
typedef SocfpgaXGMACDescriptor_t *SocfpgaXGMACHandle_t;

/**
 * @brief  Configuration structure for XGMAC Instance Parameters
 */
typedef struct
{
    int32_t lEMACIndex;           /*!< EMAC index */
    uint8_t ucPhyType;            /*!< PHY type */
    SocfpgaXGMACHandle_t pXGMACHandle; /*!< Pointer to XGMAC handle */
} XGMACConfig_t;

/**
 * @}
 */


/**
 * @addtogroup enet_fns
 * @{
 */
/**
 * @brief Initialize the XGMAC.
 *
 * The application should call this function to initialize the desired XGMAC.
 *
 * @param[in] pXGMACConfig The configuration structure for XGMAC.
 *
 * @return
 * - 'Handle' on XGMAC successful operation.
 * - 'NULL' if the XGMAC instance number is invalid or XGMAC already initialized.
 *
 */
SocfpgaXGMACHandle_t SocfpgaXGMAC_EMACInit(XGMACConfig_t *pXGMACConfig);

/**
 * @brief Deinitialize the XGMAC.
 *
 * The application should call this function to deinitialize the desired XGMAC.
 *
 * @param[in] pXGMACHandle The handle of XGMAC instance.
 *
 * @return
 * -  0 if XGMAC successful deinitialized.
 * - -EINVAL if the XGMAC handle passed is null or not initialized already.
 *
 */
int32_t SocfpgaXGMAC_EMACDeInit(SocfpgaXGMACHandle_t pXGMACHandle);


/**
 * @brief start the XGMAC initialization.
 *
 * The application should call this function to initialize the desired XGMAC and start the tx & rx.
 *
 * @param[in] pXGMACHandle The instance of the XGMAC to start the XGMAC.
 *
 * @return
 * - 0 on XGMAC successful configuration and start operation.
 *
 */
int32_t SocfpgaXGMAC_EMACStart(SocfpgaXGMACHandle_t pXGMACHandle);

/**
 * @brief stop the XGMAC.
 *
 * The application should call this function to stop the XGMAC.
 *
 * @param[in] pXGMACHandle The instance of the XGMAC to stop.
 *
 * @return
 * - 0 on XGMAC successfully stop operation.
 *
 */
void SocfpgaXGMAC_EMACStop(SocfpgaXGMACHandle_t pXGMACHandle);

/**
 * @brief set the callback function to be called on completion of an operation.
 *
 * The application should call this function to set the callback is guaranteed to be invoked
 * when an interrupt occurs.
 *
 * @param[in] pXGMACHandle The instance of the XGMAC to stop.
 *
 * @return
 * -  0 on XGMAC successfully stop operation.
 * - -EINVAL, if pXGMACHandle is NULL
 * - -EBUSY, if XGMAC has already been started.
 *
 */
int32_t SocfpgaXGMAC_Set_Callback(
    SocfpgaXGMACHandle_t pXGMACHandle,
    SocfpgaXGMAClback_t xCallback, void *pvUserContext );

/**
 * @brief Initialize the XGMAC DMA.
 *
 * The application should call this function to initialize the desired XGMAC DMA.
 *
 * @param[in] pXGMACHandle The instance of the XGMAC to initialize the DMA.
 *
 * @return
 * -  0 on XGMAC DMA successful initialization.
 * - -EIO if DMA failed to initialize.
 *
 */
int32_t SocfpgaXGMAC_DMAInitialize(
    SocfpgaXGMACHandle_t pXGMACHandle);

/**
 * @brief Deinitialize the XGMAC DMA.
 *
 * The application should call this function to deinitialize the desired XGMAC DMA.
 *
 * @param[in] pXGMACHandle The instance of the XGMAC to deinitialize the DMA.
 *
 * @return
 * -  0 on XGMAC DMA successful deinitialization.
 * - -EIO if DMA failed to deinitialize.
 *
 */
int32_t SocfpgaXGMAC_DMADeInitialize(
    SocfpgaXGMACHandle_t pXGMACHandle);

/**
 * @brief Initiate the transmit out of buffer via DMA.
 *
 * The application should call this once the data is ready to be transmitted.
 *
 * @param[in] pXGMACHandle The instance of the XGMAC to stop.
 * @param[in] pxDmaTxBufferOut The structure to buffer descriptor. It contains the
 *            pointer to the buffer, size of buffer and the flag to notify the dma driver
 *            regarding releasing the buffer for the stack.
 * @return
 * - 0 if DMA successfully transmit the buffer.
 * - -EIO, if DMA failed to transmit the buffer
 *
 */
int32_t SocfpgaXGMAC_DMA_Transmit(
    SocfpgaXGMACHandle_t pXGMACHandle, \
    XGMACDMATxBufOut_t *pxDmaTxBufferOut );

/**
 * @brief Check if the transmit is done to release the buffer.
 *
 * The application should call this once it gets an event notification after a dma transmit.
 *
 * @param[in] pXGMACHandle The instance of the XGMAC to stop.
 * @param[out] pucReleaseBuffer The released buffer which will be used for next transmit.
 *
 * @return
 * -  0 if Successfully obtained the status of buffer transmission.
 * - -EIO, if failed to obtain the status of buffer transmission.
 * - -EAGAIN, if application failed to obtain the status of dma descriptor.
 *
 */
int32_t SocfpgaXGMAC_DMA_Transmit_Done(
    SocfpgaXGMACHandle_t pXGMACHandle, \
    uint8_t **pucReleaseBuffer);

/**
 * @brief Initiate the receive of the buffer via DMA.
 *
 * The application should call this once the data is ready to be recevied in the dma fifo.
 *
 * @param[in] pXGMACHandle The instance of the XGMAC to stop.
 * @param[out] pxDmaRxBufferIn The structure to buffer descriptor. It contains the
 *            pointer to the buffer, size of buffer and the packet status which will be
 *            validated by the stack to consider or discard the buffer.
 * @return
 * - 0 if DMA successfully receive the buffer.
 * - -EAGAIN, if DMA failed to receive the buffer
 * 
 */
int32_t SocfpgaXGMAC_DMA_Receive(SocfpgaXGMACHandle_t pXGMACHandle, \
        XGMACDMARxBufIn_t *pxDmaRxBufferIn);

/**
 * @brief Refill a receive descriptor with a new buffer.
 *
 * This function updates the RX descriptor to point to a fresh buffer
 * so the DMA can receive new incoming packets.
 *
 * @param[in] pXGMACHandle The instance of the XGMAC to refill the descriptor for.
 * @param[in] pucBuffer Pointer to the new buffer to assign to the RX descriptor.
 * @return
 * - 0 if the descriptor was successfully refilled.
 */
int32_t SocfpgaXGMAC_ReFill_RxDescriptor(
    SocfpgaXGMACHandle_t pXGMACHandle,
    uint8_t *pucBuffer);


void SocfpgaXGMAC_DMA_AdjustHead(SocfpgaXGMACHandle_t pXGMACHandle);
void SocfpgaXGMAC_DMA_FlushBuffers(SocfpgaXGMACHandle_t pXGMACHandle);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */
/**
 * @}
 */

#endif /* ifndef __SOCFPGA_XGMAC_H__ */
