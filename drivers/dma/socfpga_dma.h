/*
 * SPDX-FileCopyrightText: Copyright (C) 2025 Altera Corporation
 *
 * SPDX-License-Identifier: MIT-0
 *
 * Header file for DMA HAL driver
 */

#ifndef __SOCFPGA_DMA_H__
#define __SOCFPGA_DMA_H__

/**
 * @file socfpga_dma.h
 * @brief DMA HAL driver header file
 *
 * This driver provides methods to perform dma operations.
 *
 * The driver supports memory to memory DMA transfer and memory to 
 * peripheral dma transfer.
 *
 *
 */

/**
 * @defgroup dma DMA
 * @ingroup drivers
 * @{
 */

/**
 * @defgroup dma_fns Functions
 * @ingroup dma
 * DMA HAL APIs
 */

/**
 * @defgroup dma_structs Structures 
 * @ingroup dma
 * DMA Specific Structures
 */

/**
 * @defgroup dma_enums Enumerations
 * @ingroup dma
 * DMA Specific Enumerations
 */

/**
 * @defgroup dma_macros Macros
 * @ingroup dma
 * DMA Specific Macros 
 */

/**
 * @addtogroup dma_macros 
 * @{
 */

/**
 * @brief DMA Supported Instances
 */
#define DMA_INSTANCE_0    0U /*!<DMA Instance 0*/
#define DMA_INSTANCE_1    1U /*!<DMA Instance 1*/

/**
 * @brief DMA Channel IDs
 */
#define DMA_CHANNEL_1     0U /*!<DMA Channel 1*/
#define DMA_CHANNEL_2     1U /*!<DMA Channel 2*/
#define DMA_CHANNEL_3     2U /*!<DMA Channel 3*/
#define DMA_CHANNEL_4     3U /*!<DMA Channel 4*/
/**
 * @}
 */

/**
 * Function pointer for user callback
 * @ingroup dma_fns
 */

typedef void (*DmaCallback_t)( uint32_t *ulBuff );

/**
 * @addtogroup dma_structs
 * @{
 */
struct DmaChnlCntxt;

/**
 * @brief The handle type returned by calling dma_open().
 *        This represents a DMA channel.
 *        This is initialized in open() and returned to caller.
 *        The caller must pass this pointer to the rest of APIs.
 */
typedef struct DmaChnlCntxt *xDmaHandle_t;
/**
 * @}
 */

 /**
  * @addtogroup dma_enums
  * @{
  */
/**
 * @brief DMA Supported Burst Transfer length for Source and destination
 */
typedef enum
{
    eDmaBurstTransactionLen1 = 0, /*!<1 byte burst transfer*/
    eDmaBurstTransactionLen4, /*!<4 byte burst transfer*/
    eDmaBurstTransactionLen8, /*!<8 byte burst transfer*/
    eDmaBurstTransactionLen16, /*!<16 byte burst transfer*/
    eDmaBurstTransactionLen32, /*!<32 byte burst transfer*/
    eDmaBurstTransactionLen64, /*!<64 byte burst transfer*/
    eDmaBurstTransactionLen128, /*!<128 byte burst transfer*/
    eDmaBurstTransactionLen256, /*!<256 byte burst transfer*/
    eDmaBurstTransactionLen512, /*!<512 byte burst transfer*/
    eDmaBurstTransactionLen1024 = 9, /*!<1024 byte burst transfer*/
    eDmaBurstTransactionLenMax = 9 /*!<Maximum burst transfer length*/
} BurstLength_t;

/**
 * @brief DMA Supported Transfer width for Source and destination
 */
typedef enum
{
    eDmaTransferWidth8 = 0, /*Transfer width of 8 bits*/
    eDmaTransferWidth16, /*!<Transfer width of 16 bits*/
    eDmaTransferWidth32, /*!<Transfer width of 32 bits*/
    eDmaTransferWidth64, /*!<Transfer width of 64 bits*/
    eDmaTransferWidth128, /*!<Transfer width of 128 bits*/
    eDmaTransferWidth256, /*!<Transfer width of 256 bits*/
    eDmaTransferWidth512 = 6, /*!<Transfer width of 512 bits*/
    eDmaTransferWidthMax = 6 /*!<Maximum transfer width*/
} TransferWidth_t;

/**
 * @brief DMA Multi-block transfer type
 */
typedef enum
{
    eDmaMultiBlkContiguous, /*!<Multi-block contiguous transfer*/
    eDmaMultiBlkReload, /*!<Multi-block reload transfer*/
    eDmaMultiBlkShadowReg, /*!<Multi-block Shadow Register transfer*/
    eDmaMultiBlkLinkedList = 3, /*!<Multi-block linked list transfer*/
    eDmaMultiBlkInvalidType = 3 /*!<Invalid multi-block transfer type*/
} MultiBlkTransferType;

/**
 * @brief DMA supported transfer direction and flow controller options
 */
typedef enum
{
    eDmaMemToMemDmac = 0, /*!<Memory to memory transfer, DMAC as flow controller*/
    eDmaMemToPeripheralDmac, /*!<Memory to peripheral transfer, DMAC as flow controller*/
    eDmaPeripheralToMemDmac, /*!<Peripheral to memory transfer, DMAC as flow controller*/
    eDmaPeripheralToPeripheralDmac, /*!<Peripheral to peripheral transfer, DMAC as flow controller*/
    eDmaPeripheralToMemSrc, /*!<Peripheral to memory transfer, source peripheral as flow controller*/
    eDmaPeripheralToPeripheralSrc, /*!<Peripheral to peripheral transfer, source peripheral as flow controller*/
    eDmaMemToPeripheralDst, /*!<Memory to peripheral transfer, destination peripheral as flow controller*/
    eDmaPeripheralToPeripheralDst = 7, /*!<Peripheral to peripheral transfer, destination peripheral as flow controller*/
    eDmaInvalidTransferType = 7 /*!<Invalid transfer type*/
} DmaTransferType;

/**
 * @brief DMA Channel state
 */
typedef enum
{
    eDmaChannelIdle, /*!<Channel is idle*/
    eDmaChannelSuspended, /*!<Channel is suspended*/
    eDmaChannelAbort, /*!<Channel is aborted*/
    eDmaChannelActive, /*!<Channel is active*/
} DmaChannelState;

/**
 * @brief DMA peripheral ID list
 */
typedef enum
{
    eDmaFpgaPeripheral0, /*!<DMA peripheral ID for FPGA Peripheral 0*/
    eDmaFpgaPeripheral1, /*!<DMA peripheral ID for FPGA Peripheral 1*/
    eDmaFpgaPeripheral2, /*!<DMA peripheral ID for FPGA Peripheral 2*/
    eDmaFpgaPeripheral3, /*!<DMA peripheral ID for FPGA Peripheral 3*/
    eDmaFpgaPeripheral4, /*!<DMA peripheral ID for FPGA Peripheral 4*/
    eDmaFpgaPeripheral5, /*!<DMA peripheral ID for FPGA Peripheral 5*/
    eDmaFpgaPeripheral6, /*!<DMA peripheral ID for FPGA Peripheral 6*/
    eDmaFpgaPeripheral7, /*!<DMA peripheral ID for FPGA Peripheral 7*/
    eDmaI2c0Tx, /*!<DMA peripheral ID for I2C instance 0 Tx*/
    eDmaI2c0Rx, /*!<DMA peripheral ID for I2C instance 0 Rx*/
    eDmaI2c1Tx, /*!<DMA peripheral ID for I2C instance 1 Tx*/
    eDmaI2c1Rx, /*!<DMA peripheral ID for I2C instance 1 Rx*/
    eDmaI2cEmac0Tx, /*!<DMA peripheral ID for EMAC instance 0 Tx*/
    eDmaI2cEmac0Rx, /*!<DMA peripheral ID for EMAC instance 0 Rx*/
    eDmaI2cEmac1Tx, /*!<DMA peripheral ID for EMAC instance 1 Tx*/
    eDmaI2cEmac1Rx, /*!<DMA peripheral ID for EMAC instance 1 Rx*/
    eDmaSpi0MasterTx, /*!<DMA peripheral ID for SPI instance 0 Master Tx*/
    eDmaSpi0MasterRx, /*!<DMA peripheral ID for SPI instance 0 Master Rx*/
    eDmaSpi0SlaveTx, /*!<DMA peripheral ID for SPI instance 0 Slave Tx*/
    eDmaSpi0SlaveRx, /*!<DMA peripheral ID for SPI instance 0 Slave Rx*/
    eDmaSpi1MasterTx, /*!<DMA peripheral ID for SPI instance 1 Master Tx*/
    eDmaSpi1MasterRx, /*!<DMA peripheral ID for SPI instance 1 Master Rx*/
    eDmaSpi1SlaveTx, /*!<DMA peripheral ID for SPI instance 1 Slave Tx*/
    eDmaSpi1SlaveRx, /*!<DMA peripheral ID for SPI instance 1 Slave Rx*/
    eDmaSTM = 26, /*!<DMA peripheral ID for System Trace Macrocell*/
    eDmaUart0Tx = 28, /*!<DMA peripheral ID for UART instance 0 Tx*/
    eDmaUart0Rx, /*!<DMA peripheral ID for UART instance 0 Rx*/
    eDmaUart1Tx, /*!<DMA peripheral ID for UART instance 1 Tx*/
    eDmaUart1Rx, /*!<DMA peripheral ID for UART instance 1 Rx*/
    eDmaI2cEmac2Tx, /*!<DMA peripheral ID for EMAC instance 2 Tx*/
    eDmaI2cEmac2Rx, /*!<DMA peripheral ID for EMAC instance 2 Rx*/
    eDmaI3c0Tx = 35, /*!<DMA peripheral ID for I3C instance 0 Tx*/
    eDmaI3c0Rx, /*!<DMA peripheral ID for I3C instance 0 Rx*/
    eDmaI3c1Tx, /*!<DMA peripheral ID for I3C instance 1 Tx*/
    eDmaI3c1Rx, /*!<DMA peripheral ID for I3C instance 1 Rx*/
    eDmaInvalidChnl = 48 /*!<Invalid peripheral ID*/
} PeripheralId_t;
/**
 * @}
 */
/**
 * @addtogroup dma_structs
 * @{
 */
/**
 * @brief Configuration parameters for the DMA.
 *
 * The application will send the user configuration in the form of the
 * following structure in dma_config.
 */
typedef struct DmaConfig
{
    uint8_t ulDmaInstance; /*!< DMA controller instance number */
    DmaTransferType uDmaChannelDirection; /*!< DMA channel transfer direction */
    uint8_t uDmaChannelPriority; /*!< DMA channel priority */
    PeripheralId_t ulPeripheralId; /*!< Peripheral ID for the DMA channel */
    uint32_t ulDmaLinkedListCount; /*!< Number of linked list items for the DMA channel */
    DmaCallback_t pxDmaCallback; /*!< Callback function for DMA interrupts */

} DmaConfig_t;

/**
 * @brief Block transfer configuration parameters for the DMA.
 *
 * The application will send the block transfer configuration in the form of the
 * following structure in dma_setup_transfer.
 */
typedef struct DmaBlkTrnsfrCfg
{
    uint64_t ulSrcAddr; /*!< Source address for the DMA transfer */
    uint64_t ulDestAddr; /*!< Destination address for the DMA transfer */
    uint32_t ulBlockSize; /*!< Size of the block to be transferred in bytes */
    struct DmaBlkTrnsfrCfg *xNextTrnsfrCfg; /*!< Pointer to the next block transfer configuration*/
                                            
} DmaBlkTrnsfrCfg_t;
/**
 * @}
 */

/**
 * @addtogroup dma_fns
 * @{
 */
/**
 * @brief Initialize a DMA channel and return the handle to it
 *
 * The application should call this function to initialize the desired DMA channel.
 * The handle shall be passed while calling all other APIs
 *
 * @param[in] ulDmaControllerID The desired DMA controller instance.
 *                              Select from the DmaInstance_t enum
 * @param[in] ulDmaChannelID   The desired DMA channel instance.
 *                              Select from the DmaChannels_t enum
 *
 * @return
 * - 'the handle to the DMA channel (not NULL)', on success.
 * - 'NULL', if
 *     - invalid instance number
 *     - open same instance more than once before closing it
 *     - failed to enable the interrupt
 */
xDmaHandle_t dma_open( uint32_t xDmaControllerID,
        uint32_t xDmaChannelID );

/**
 * @brief Configure the DMA channel parameters
 *
 * This function configures channel properties that does not change every
 * transfer. That is properties like channel direction, peripheral id etc
 *
 * @param [in] pxDmaPeripheral Handle to the channel returned by the Open()
 *
 * @param [in] pxConfig The channel configuration structure
 * @return
 * - 0, on success
 * - -EINVAL, if pxDmaPeripheral is NULL
 * - -EIO   , if the channel is not opened before invoking dma_config.
 * - -EBUSY , if another transfer is in progress.
 *
 */
int32_t dma_config( xDmaHandle_t const pxDmaPeripheral,
        DmaConfig_t *pxConfig );

/**
 * @brief Setup a DMA data transfer
 *
 * This will setup a data transfer with given transfer parameters. It does
 * not start the data transfer.
 *
 * @param [in] pxDmaPeripheral Handle to the channel returned by the Open()
 *
 * @param [in] xBlkTrnsfrList a linked list with source address, destination
 *                            address and transfer size.
 *
 * @return
 * - 0, on success
 * - -EINVAL, if pxDmaPeripheral is NULL
 * - -EFAULT, if xBlkTrnsfrList is NULL or channel is not opened.
 * - -EBUSY, if another transfer is in progress.
 *
 */
int32_t dma_setup_transfer( xDmaHandle_t const pxDmaPeripheral,
        DmaBlkTrnsfrCfg_t *xBlkTrnsfrList );

/**
 * @brief Start the data transfer
 *
 * This will start the data transfer which is already set up by dma_setup_transfer.
 *
 * @param [in] pxDmaPeripheral Handle to the channel returned by the Open()
 *
 * @return
 * - 0, on success
 * - -EINVAL, if pxDmaPeripheral is NULL
 * - -EIO   , if channel is not open
 * - -EBUSY , if another transfer is in progress.
 *
 */
int32_t dma_start_tranfer( xDmaHandle_t const pxDmaPeripheral );

/**
 * @brief Stop a data transfer in progress
 *
 * This will stop a data transfer which is in progress
 *
 * @param [in] pxDmaPeripheral Handle to the channel returned by the Open()
 *
 * @return
 * - 0, on success
 * - -EINVAL, if pxDmaPeripheral is NULL
 * - -EIO   , if no data trasnfer is in progress
 * - -EIO   , if it fails to stop the transfer
 */
int32_t dma_stop_transfer( xDmaHandle_t const pxDmaPeripheral );

/**
 * @brief Suspend a data transfer in progress
 *
 * This will suspend a data transfer which is in progress
 *
 * @param [in] pxDmaPeripheral Handle to the channel returned by the Open()
 *
 * @return
 * - 0, on success
 * - -EINVAL, if pxDmaPeripheral is NULL
 * - -EIO, if no data trasnfer is in progress
 * - -EBUSY, if it fails to suspend the data transfer
 */
int32_t dma_suspend_transfer( xDmaHandle_t const pxDmaPeripheral );

/**
 * @brief Resume a suspended data transfer
 *
 * This will resume a suspended data transfer
 *
 * @param [in] pxDmaPeripheral Handle to the channel returned by the Open()
 *
 * @return
 * - 0, on success
 * - -EINVAL, if pxDmaPeripheral is NULL
 * - -EIO, if it fails to resume the data transfer
 * - -EBUSY, if it is not in a suspended state
 */
int32_t dma_resume_transfer( xDmaHandle_t const pxDmaPeripheral );

/**
 * @brief Close the dma channel
 *
 * This will close the dma channel
 *
 * @param [in] pxDmaPeripheral Handle to the channel returned by the Open()
 *
 * @return
 * - 0, on success
 * - -EINVAL, if pxDmaPeripheral is NULL
 */
int32_t dma_close( xDmaHandle_t const pxDmaPeripheral );
/**
 * @}
 */
/**
 * @}
 */
#endif /* __SOCFPGA_DMA_H__ */

