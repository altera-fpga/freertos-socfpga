/*
 * SPDX-FileCopyrightText: Copyright (C) 2025 Altera Corporation
 *
 * SPDX-License-Identifier: MIT-0
 *
 * HAL driver implementation for DMA
 */
#include <errno.h>
#include "socfpga_defines.h"
#include "socfpga_dma.h"
#include "socfpga_dma_reg.h"
#include "socfpga_cache.h"
#include "socfpga_interrupt.h"
#include "socfpga_rst_mngr.h"
#include "osal_log.h"


#define MULTI_BLK_LLI_MODE_ENABLED    1
#define DMA_MAX_INSTANCE              (2U)
#define MAX_CHANNEL_NUM               (4U)
#define MAX_LLI_PER_CHANNEL           10U
/* Max block size available is 2^21 */
#define MAX_BLOCK_SIZE                0x1FFFFFU

/* Linked list structure dma controller*/
struct DmaChannelRegList
{
    uint64_t ullSar;
    uint64_t ullDar;
    uint64_t ullBlockTs; /*Only 0-21 bits are used in this and other bits are reserved bits*/
    uint64_t ullLlp; /*Only 5:63 bits are used; 0-5 bits are reserved*/
    uint64_t ullCtl;
    uint32_t ulChnSrcStat;
    uint32_t ulChnDstStat;
    uint64_t ullChnLlpStatus;
    uint64_t ullReserved;
} __attribute__((packed, aligned(64)));
static uint32_t ullInstBaseAddr[ DMA_MAX_INSTANCE ] = { 0x10DB0000U, 0x10DC0000U };
static uint32_t ulChnlOffsetAddr[ MAX_CHANNEL_NUM ] = { 0x00000100U, 0x00000200U, 0x00000300U, 0x00000400U };
static SocfpgaHpuInterrupt_t xInterruptId[ DMA_MAX_INSTANCE ][ MAX_CHANNEL_NUM ] =
{
    { eDmaIrq0, eDmaIrq1, eDmaIrq2, eDmaIrq3 },
    { eDma1Irq0, eDma1Irq1, eDma1Irq2, eDma1Irq3 }
};
struct DmaChnlCntxt
{
    BaseType_t xIsOpen;
    uint32_t ulBaseAddress;
    uint32_t ulChOffset;
    uint32_t ulChannelNum;
    DmaChannelState ChannelState;
    SocfpgaHpuInterrupt_t xIntrId;
    /* DmaChannelRegList descriptor base pointer */
    struct DmaChannelRegList *LinkedListBase;
    /* DMA transfer direction */
    DmaTransferType ulDirection;
    /* LinkedList Descriptors count*/
    uint32_t LinkedListCount;
    /* DMA channel config register  */
    uint64_t ullConfig;
    /* Interrupt Pin config  */
    uint64_t ullInterruptEn;
    /* Callback function for Dma interrupts */
    DmaCallback_t xpDmaCallback;
};
static struct DmaChnlCntxt pxDmaDefaultHandle[ DMA_MAX_INSTANCE ][
    MAX_CHANNEL_NUM ];
/*Set array size as 16 as each channel can have maximum of 2 descriptors*/
static struct DmaChannelRegList pxLinkedListChain[ DMA_MAX_INSTANCE ][
    MAX_CHANNEL_NUM * MAX_LLI_PER_CHANNEL ] __attribute__ ((aligned (64)));
void prvDmaIrqHandler( void *xData );
/**
 * @brief Ger the current status of the DMA channel
 */
static DmaChannelState ulDmaGetChannelStatus( xDmaHandle_t pxDmaPeripheral )
{
    uint64_t ullVal;
    DmaChannelState ret;
    ullVal = RD_REG64(pxDmaPeripheral->ulBaseAddress + DMA_DMAC_CHENREG);
    /* Active channel */
    if ( ((ullVal >> (CHENREG_CH_EN_POS + pxDmaPeripheral->ulChannelNum)) & 1U)
            == 1U )
    {
        ret = eDmaChannelActive;
    }
    /* Suspended channel*/
    else if ( ((ullVal >>
            (CHENREG_CH_SUSP_POS + pxDmaPeripheral->ulChannelNum)) & 1U) == 1U )
    {
        ret = eDmaChannelSuspended;
    }
    /* Abort channel*/
    else if ( ((ullVal >>
            (CHENREG_CH_ABORT_POS + pxDmaPeripheral->ulChannelNum)) & 1U) == 1U )
    {
        ret = eDmaChannelAbort;
    }
    /* Idle Channel*/
    else
    {
        ret = eDmaChannelIdle;
    }
    return ret;
}
xDmaHandle_t dma_open( uint32_t xDmaControllerID, uint32_t xDmaChannelID )
{
    int32_t status;
    uint8_t reset_status;
    xDmaHandle_t pxHandle = NULL;
    SocfpgaInterruptErr_t xIntRet;
    if ( (xDmaControllerID >= DMA_MAX_INSTANCE)
            || (xDmaChannelID >= MAX_CHANNEL_NUM) )
    {
        ERROR("Not a valid DMAC Instance or Channel ");
        return NULL;
    }
    pxHandle = &pxDmaDefaultHandle[ xDmaControllerID ][ xDmaChannelID ];
    if ( pxHandle->xIsOpen == 1 )
    {
        ERROR(
                "DMAC channel already opened please close it before re-opening \n");
        return NULL;
    }
    if ( pxHandle->ChannelState != eDmaChannelIdle )
    {
        ERROR("DMAC Channel instance is in use ");
        return NULL;
    }

    status = rstmgr_get_reset_status(RST_DMA, &reset_status);
    if ( status != RSTMGR_SUCCESS )
    {
        ERROR("DMAC block get reset status failed. ");
        return NULL;
    }
    if ( reset_status == 1U )
    {
        status = rstmgr_toggle_reset(RST_DMA);
        if ( status != RSTMGR_SUCCESS )
        {
            ERROR("Failed to reset release DMAC block. ");
            return NULL;
        }
    }
    pxHandle = &pxDmaDefaultHandle[ xDmaControllerID ][ xDmaChannelID ];
    pxHandle->ulBaseAddress = ullInstBaseAddr[ xDmaControllerID ];
    pxHandle->ulChOffset = ullInstBaseAddr[ xDmaControllerID ] +
            ulChnlOffsetAddr[ xDmaChannelID ];
    pxHandle->xIntrId = xInterruptId[ xDmaControllerID ][ xDmaChannelID ];
    pxHandle->ulChannelNum = xDmaChannelID;
    pxHandle->LinkedListBase =
            &pxLinkedListChain[ xDmaControllerID ][ (xDmaChannelID *
                    MAX_LLI_PER_CHANNEL) ];
    pxHandle->xIsOpen = 1;
    /*Setup and enable interrupts in GIC*/
    xIntRet = interrupt_register_isr(pxHandle->xIntrId, prvDmaIrqHandler,
            pxHandle);
    if ( xIntRet != eErrOk )
    {
        return NULL;
    }
    xIntRet = interrupt_enable(pxHandle->xIntrId, GIC_INTERRUPT_PRIORITY_DMA);
    if ( xIntRet != eErrOk )
    {
        return NULL;
    }
    return pxHandle;
}
/**
 * @brief Get the DMA transfer width
 */
static void vDmaGetTransferWidth( xDmaHandle_t const pxDmaPeripheral,
        TransferWidth_t *xSrcTransferWidth, TransferWidth_t *xDstTransferWidth )
{
    if ( pxDmaPeripheral->ulDirection == eDmaMemToMemDmac )
    {
        *xSrcTransferWidth = eDmaTransferWidth32; /*4 bytes*/
        *xDstTransferWidth = eDmaTransferWidth32;
    }
    else
    {
        *xSrcTransferWidth = eDmaTransferWidth32; /*4 bytes*/
        *xDstTransferWidth = eDmaTransferWidth32;
    }
}
/** 
 * @brief Get the DMA burst length
 */
static void vDmaGetBurstLen( xDmaHandle_t const pxDmaPeripheral,
        BurstLength_t *xSrcBurstLen, BurstLength_t *xDstBurstLen )
{

    if ( pxDmaPeripheral->ulDirection == eDmaMemToMemDmac )
    {
        *xSrcBurstLen = eDmaBurstTransactionLen16; /*4 units*/
        *xDstBurstLen = eDmaBurstTransactionLen16;
    }
    else
    {
        *xSrcBurstLen = eDmaBurstTransactionLen4; /*4 Units*/
        *xDstBurstLen = eDmaBurstTransactionLen4;
    }
}
int32_t dma_config( xDmaHandle_t const pxDmaPeripheral, DmaConfig_t *pxConfig )
{

    if ( pxDmaPeripheral == NULL )
    {
        ERROR("DMAC handle cannot be NULL ");
        return -EINVAL;
    }
    if ( pxDmaPeripheral->xIsOpen != 1 )
    {
        ERROR("DMAC channel should be opened before config \n");
        return -EFAULT;
    }
    if ( pxDmaPeripheral->ChannelState != eDmaChannelIdle )
    {
        ERROR("DMAC Channel is in active state ");
        return -EBUSY;
    }
    pxDmaPeripheral->ullConfig = 0UL;
    pxDmaPeripheral->ullInterruptEn = 0UL;

    pxDmaPeripheral->ulDirection =
            pxConfig->uDmaChannelDirection;
    pxDmaPeripheral->LinkedListCount = pxConfig->ulDmaLinkedListCount;
    pxDmaPeripheral->ullConfig |= ((uint64_t) (pxConfig->uDmaChannelDirection)
            << DMA_CH_CFG2_TT_FC_POS);
    if ( pxConfig->uDmaChannelDirection == eDmaMemToPeripheralDmac )
    {
        pxDmaPeripheral->ullConfig |= ((uint64_t) pxConfig->ulPeripheralId
                << DMA_CH_CFG2_DST_PER_POS);
    }
    if ( pxConfig->uDmaChannelDirection == eDmaPeripheralToMemDmac )
    {
        pxDmaPeripheral->ullConfig |= ((uint64_t) pxConfig->ulPeripheralId
                << DMA_CH_CFG2_SRC_PER_POS);
    }
    pxDmaPeripheral->ullConfig &=
            ~((DMA_CH_CFG2_DST_MULTBLK_TYPE_MASK |
            DMA_CH_CFG2_SRC_MULTBLK_TYPE_MASK));
    /*Multiple block linked list mode configuration.
       Data available in multiple block of memory*/
#ifdef MULTI_BLK_LLI_MODE_ENABLED
    pxDmaPeripheral->ullConfig |= (((uint32_t) eDmaMultiBlkLinkedList
            << DMA_CH_CFG2_DST_MULTBLK_TYPE_POS) |
            ((uint32_t) eDmaMultiBlkLinkedList
            << DMA_CH_CFG2_SRC_MULTBLK_TYPE_POS));
#else
    /*Single linked list mode configuration.
       Data available in a single continuous memory */
    pxDmaPeripheral->ullConfig |=
            ((eDmaMultiBlkContiguous <<
            DMA_CH_CFG2_DST_MULTBLK_TYPE_POS) | (eDmaMultiBlkContiguous
            << DMA_CH_CFG2_SRC_MULTBLK_TYPE_POS));
#endif
    pxDmaPeripheral->xpDmaCallback = pxConfig->pxDmaCallback;
    return 0;
}
int32_t dma_setup_transfer( xDmaHandle_t const pxDmaPeripheral,
        DmaBlkTrnsfrCfg_t *xBlkTrnsfrList )
{
    uint64_t ullVal;
    uint64_t ulTransferSize;
    uint32_t i;
    TransferWidth_t xSrcWidth, xDstWidth;
    BurstLength_t xSrcBurstLen, xDstBurstLen;
    struct DmaChannelRegList *pxLinkedList;
    DmaBlkTrnsfrCfg_t *pxTransferCfg;
    if ( pxDmaPeripheral == NULL )
    {
        ERROR("DMAC handle cannot be NULL ");
        return -EINVAL;
    }
    if ( xBlkTrnsfrList == NULL )
    {
        ERROR("Transfer list cannot be null");
        return -EFAULT;
    }
    if ( pxDmaPeripheral->xIsOpen != 1 )
    {
        ERROR("DMAC channel should be opened before setup transfer \n");
        return -EIO;
    }
    if ( pxDmaPeripheral->ChannelState != eDmaChannelIdle )
    {
        ERROR("DMAC Channel is in active state ");
        return -EBUSY;
    }
    vDmaGetTransferWidth(pxDmaPeripheral, &xSrcWidth, &xDstWidth);
    vDmaGetBurstLen(pxDmaPeripheral, &xSrcBurstLen, &xDstBurstLen);
    (void) memset(pxDmaPeripheral->LinkedListBase, 0,
            (sizeof(struct DmaChannelRegList) * MAX_LLI_PER_CHANNEL));
    pxLinkedList = pxDmaPeripheral->LinkedListBase;
    if ( pxLinkedList == NULL )
    {
        ERROR("Linked list is null");
        return -EFAULT;
    }
    pxTransferCfg = xBlkTrnsfrList;
    if ( pxTransferCfg == NULL )
    {
        ERROR("Transfer Cfg is NULL");
        return -EFAULT;
    }
    ulTransferSize =
            (((uint64_t) xSrcWidth <<
            DMA_CH_CTL_SRC_TR_WIDTH_POS) |
            ((uint64_t) xDstWidth << DMA_CH_CTL_DST_TR_WIDTH_POS));
    ulTransferSize |=
            (((uint64_t) xSrcBurstLen <<
            DMA_CH_CTL_SRC_MSIZE_POS) |
            ((uint64_t) xDstBurstLen << DMA_CH_CTL_DST_MSIZE_POS));
    ulTransferSize |=
            ((DMA_CH_CTL_DST_STAT_EN_MASK | DMA_CH_CTL_SRC_STAT_EN_MASK) |
            DMA_CH_CTL_IOC_BLKTFR_MASK);
    if ( (1UL << (uint64_t) xSrcWidth) == 0U )
    {
        ERROR("Denominator cannot be 0");
        return -EFAULT;
    }

    for ( i = 0U; i < pxDmaPeripheral->LinkedListCount; i++ )
    {
        pxLinkedList->ullSar = pxTransferCfg->ulSrcAddr;
        pxLinkedList->ullDar = pxTransferCfg->ulDestAddr;
        pxLinkedList->ullCtl = ulTransferSize;
        pxLinkedList->ullBlockTs =
                ((uint64_t) pxTransferCfg->ulBlockSize /
                (1UL << (uint64_t) xSrcWidth)) - 1UL;
        if ( pxLinkedList->ullBlockTs > MAX_BLOCK_SIZE )
        {
            ERROR("Transfer block size exceeding maximum size");
            return -EINVAL;
        }
        /*Set next descriptor address*/
        pxLinkedList->ullLlp = ((uint64_t)(uintptr_t) (pxLinkedList + (uintptr_t) 1U));
#ifdef MULTI_BLK_LLI_MODE_ENABLED
        pxLinkedList->ullCtl |= (1UL << DMA_CH_CTL_SHADOWREG_OR_LLI_VALID_POS);
        /*Last descriptor*/
        if ( (pxDmaPeripheral->LinkedListCount) == (i + 1U) )
        {
            pxLinkedList->ullCtl |= (1UL
                    << DMA_CH_CTL_SHADOWREG_OR_LLI_LAST_POS);
            pxLinkedList->ullLlp = 0UL;
        }
#endif
        /* Next descriptor updates*/
        pxLinkedList++;
        pxTransferCfg = pxTransferCfg->xNextTrnsfrCfg;
    }
    cache_force_write_back((void*) pxDmaPeripheral->LinkedListBase,
            ((MAX_LLI_PER_CHANNEL) *sizeof(pxLinkedList[ 0U ])));
    pxDmaPeripheral->ullInterruptEn = TFR_DONE_MASK;
    ullVal = RD_REG64(pxDmaPeripheral->ulBaseAddress + DMA_DMAC_CFGREG);
    ullVal |= (DMA_DMAC_CFGREG_INT_EN_MASK | DMA_DMAC_CFGREG_DMAC_EN_MASK);
    WR_REG64(pxDmaPeripheral->ulBaseAddress + DMA_DMAC_CFGREG, ullVal);
    (void) RD_REG64(pxDmaPeripheral->ulChOffset + DMA_CH_CFG2);
    WR_REG64((pxDmaPeripheral->ulChOffset + DMA_CH_CFG2),
            pxDmaPeripheral->ullConfig);
    WR_REG64(pxDmaPeripheral->ulChOffset + DMA_CH_INTSTATUS_ENABLEREG,
            pxDmaPeripheral->ullInterruptEn);
    WR_REG64(pxDmaPeripheral->ulChOffset + DMA_CH_INTSIGNAL_ENABLEREG,
            pxDmaPeripheral->ullInterruptEn);
    pxLinkedList = pxDmaPeripheral->LinkedListBase;
    if ( pxLinkedList == NULL )
    {
        ERROR("Linked list is null");
        return -EFAULT;
    }
#ifdef MULTI_BLK_LLI_MODE_ENABLED
    (void) RD_REG64(pxDmaPeripheral->ulChOffset + DMA_CH_LLP);
    WR_REG64(pxDmaPeripheral->ulChOffset + DMA_CH_LLP,
            ((uint64_t )pxLinkedList));
    (void) RD_REG64(pxDmaPeripheral->ulChOffset + DMA_CH_LLP);
#else
    WR_REG64((pxDmaPeripheral->ulChOffset + DMA_CH_SAR), pxLinkedList->ullSar);
    WR_REG64((pxDmaPeripheral->ulChOffset + DMA_CH_DAR), pxLinkedList->ullDar);
    WR_REG64((pxDmaPeripheral->ulChOffset + DMA_CH_BLOCK_TS),
            (pxLinkedList->ullBlockTs & DMA_CH_BLOCK_TS_BLOCK_TS_MASK));
    WR_REG64((pxDmaPeripheral->ulChOffset + DMA_CH_CTL), pxLinkedList->ullCtl);
#endif
    return 0;
}
int32_t dma_start_tranfer( xDmaHandle_t const pxDmaPeripheral )
{
    uint64_t ullVal;
    if ( pxDmaPeripheral == NULL )
    {
        ERROR("DMAC handle cannot be NULL ");
        return -EINVAL;
    }
    if ( pxDmaPeripheral->xIsOpen != 1 )
    {
        ERROR("DMAC channel should be opened before start transfer \n");
        return -EIO;
    }

    if ( pxDmaPeripheral->ChannelState != eDmaChannelIdle )
    {
        ERROR("DMAC Channel is in active state ");
        return -EBUSY;
    }
    INFO("Starting the DMA transfer on channel %d", pxDmaPeripheral->ulChannelNum);
    ullVal = RD_REG64(pxDmaPeripheral->ulBaseAddress + DMA_DMAC_CHENREG);
    ullVal |= (1UL << (pxDmaPeripheral->ulChannelNum + CHENREG_CH_EN_POS));
    ullVal |= (1UL << (pxDmaPeripheral->ulChannelNum + CHENREG_CH_EN_WE_POS));
    /*Start the channel for transfer */
    WR_REG64(pxDmaPeripheral->ulBaseAddress + DMA_DMAC_CHENREG, ullVal);
    /*Set the channel state to active as the transfer is started */
    pxDmaPeripheral->ChannelState = eDmaChannelActive;
    return 0;
}
int32_t dma_stop_transfer( xDmaHandle_t const pxDmaPeripheral )
{
    uint64_t ullVal;
    uint32_t wait_count = 0U;

    if ( pxDmaPeripheral == NULL )
    {
        ERROR("DMAC handle cannot be NULL ");
        return -EINVAL;
    }
    if ( pxDmaPeripheral->ChannelState == eDmaChannelIdle )
    {
        ERROR("DMAC Channel not in active state ");
        return -EIO;
    }
    ullVal = RD_REG64(pxDmaPeripheral->ulBaseAddress + DMA_DMAC_CHENREG);
    ullVal |= (0x1UL
            << (pxDmaPeripheral->ulChannelNum + CHENREG_CH_SUSP_WE_POS)) |
            (0x1UL << (pxDmaPeripheral->ulChannelNum + CHENREG_CH_SUSP_POS));
    WR_REG64(pxDmaPeripheral->ulBaseAddress + DMA_DMAC_CHENREG, ullVal);
    /*Disable channel*/
    ullVal = RD_REG64(pxDmaPeripheral->ulBaseAddress + DMA_DMAC_CHENREG);
    ullVal &= (~(1UL << pxDmaPeripheral->ulChannelNum));
    WR_REG64(pxDmaPeripheral->ulBaseAddress + DMA_DMAC_CHENREG, ullVal);
    ullVal = RD_REG64(pxDmaPeripheral->ulBaseAddress + DMA_DMAC_CHENREG);
    while (((ullVal & (1UL << pxDmaPeripheral->ulChannelNum))
            == (1UL << pxDmaPeripheral->ulChannelNum)) || (wait_count < 100U))
    {
        ullVal = RD_REG64(pxDmaPeripheral->ulBaseAddress + DMA_DMAC_CHENREG);
        wait_count++;
    }
    if ( wait_count == 100U )
    {
        return -EIO;
    }
    /*Set the channel state to idle  as the transfer is stopped */
    pxDmaPeripheral->ChannelState = eDmaChannelIdle;
    return 0;
}
int32_t dma_suspend_transfer( xDmaHandle_t const pxDmaPeripheral )
{
    uint64_t ullVal;

    if ( pxDmaPeripheral == NULL )
    {
        ERROR("DMAC handle cannot be NULL ");
        return -EINVAL;
    }
    if ( pxDmaPeripheral->ChannelState != eDmaChannelActive )
    {
        ERROR("Only Active channel can be suspended ");
        return -EIO;
    }
    ullVal = RD_REG64(pxDmaPeripheral->ulBaseAddress + DMA_DMAC_CHENREG);
    ullVal |= (0x1UL
            << (pxDmaPeripheral->ulChannelNum + CHENREG_CH_SUSP_WE_POS)) |
            (0x1UL << (pxDmaPeripheral->ulChannelNum + CHENREG_CH_SUSP_POS));
    WR_REG64(pxDmaPeripheral->ulBaseAddress + DMA_DMAC_CHENREG, ullVal);
    if ( ulDmaGetChannelStatus(pxDmaPeripheral) != eDmaChannelSuspended )
    {
        ERROR("DMAC Channel is in active state ");
        return -EBUSY;
    }
    pxDmaPeripheral->ChannelState = eDmaChannelSuspended;
    return 0;
}
int32_t dma_resume_transfer( xDmaHandle_t const pxDmaPeripheral )
{
    uint64_t ullVal;
    if ( pxDmaPeripheral == NULL )
    {
        ERROR("DMAC handle cannot be NULL ");
        return -EINVAL;
    }
    if ( pxDmaPeripheral->ChannelState != eDmaChannelSuspended )
    {
        ERROR("Only Suspended DMAC Channel can be resumed.");
        return -EBUSY;
    }
    ullVal = RD_REG64(pxDmaPeripheral->ulBaseAddress + DMA_DMAC_CHENREG);
    ullVal |=
            ((1UL <<
            pxDmaPeripheral->ulChannelNum) |
            ((0x1UL <<
            (pxDmaPeripheral->ulChannelNum + CHENREG_CH_EN_WE_POS))));
    WR_REG64(pxDmaPeripheral->ulBaseAddress + DMA_DMAC_CHENREG, ullVal);
    /*Read the status register to check current status after resume transfer command*/
    if ( ulDmaGetChannelStatus(pxDmaPeripheral) != eDmaChannelActive )
    {
        ERROR("DMAC Channel not in active state ");
        return -EIO;
    }
    pxDmaPeripheral->ChannelState = eDmaChannelActive;
    return 0;
}
int32_t dma_close( xDmaHandle_t const pxDmaPeripheral )
{
    if ( pxDmaPeripheral == NULL )
    {
        ERROR("DMAC handle cannot be NULL ");
        return -EINVAL;
    }
    (void) memset(pxDmaPeripheral, 0, sizeof(struct DmaChnlCntxt));
    return 0;
}
/** 
 * @brief Interrupt handler for DMA
 */
void prvDmaIrqHandler( void *xData )
{
    uint64_t ullVal;
    xDmaHandle_t pxHandle = (xDmaHandle_t) xData;
    ullVal = RD_REG64(pxHandle->ulChOffset + DMA_CH_INTSTATUS);
    if ( (ullVal & TFR_DONE_MASK) == TFR_DONE_MASK )
    {
        WR_REG64((pxHandle->ulChOffset + DMA_CH_INTCLEARREG), TFR_DONE_MASK);
        /* Set the channel state to idle once transfer completed*/
        pxHandle->ChannelState = eDmaChannelIdle;
        pxHandle->xpDmaCallback(xData);
    }
}
