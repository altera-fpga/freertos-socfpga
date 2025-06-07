/*
 * FreeRTOS+TCP V3.1.0
 * Copyright (C) 2022 Amazon.com, Inc. or its affiliates.  All Rights Reserved.
 *
 * SPDX-License-Identifier: MIT
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
 *
 * HAL driver implementation for XGMAC
 */

/*
* Ethernet Driver - Features
*
* - Supports 10/100 Mbps Ethernet
* - Full-duplex and half-duplex operation
* - Auto-negotiation enabled
* - Link status detection and monitoring
* - IPv4 and IPv6 compatible
* - DMA-based data transfer support
* - MAC and PHY interface support
* - CRC and checksum offloading 
*/

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include "socfpga_xgmac.h"
#include "socfpga_xgmac_phy_ll.h"
#include "socfpga_xgmac_ll.h"
#include "socfpga_interrupt.h"
#include "socfpga_xgmac_configs.h"
#include "osal.h"
#include "osal_log.h"

SocfpgaXGMACDescriptor_t xgmac_descriptors[ XGMAC_MAX_INSTANCE ];

/* Static functions */
static Basetype_t DMA_SoftReset(XGMAC_BaseAddr_t *pxEmacBaseAddr);
static Basetype_t DMA_Set_Descriptors(SocfpgaXGMACHandle_t pXGMACHandle);
static void DMA_Channel_Init(SocfpgaXGMACHandle_t pXGMACHandle, \
        const XGMACDevConfigStr_t *xgmacDevConfig);
static void DMA_Setup_TxDescriptor_List(SocfpgaXGMACHandle_t pXGMACHandle );
static void DMA_Setup_RxDescriptor_List(SocfpgaXGMACHandle_t pXGMACHandle );
static Basetype_t DMA_Enable_Interrupts(SocfpgaXGMACHandle_t pXGMACHandle,
        const XGMACDevConfigStr_t *xgmacDevConfig);
static Basetype_t DMA_Disable_Interrupts(SocfpgaXGMACHandle_t pXGMACHandle,
        const XGMACDevConfigStr_t *xgmacDevConfig);
static Basetype_t DMA_RegisterISR(SocfpgaXGMACHandle_t pXGMACHandle);
static Basetype_t DMA_UnRegisterISR(SocfpgaXGMACHandle_t pXGMACHandle);
static void socfpga_xgmac_dma_isr( SocfpgaXGMACHandle_t pxEmacPeripheral);

SocfpgaXGMACHandle_t SocfpgaXGMAC_EMACInit(XGMACConfig_t *pXGMACConfig)
{
    int32_t lXgmacInstance = pXGMACConfig->lEMACIndex;
    SocfpgaXGMACHandle_t pxHandle;

    /* Check if Handle is valid */
    if ((lXgmacInstance >= XGMAC_MAX_INSTANCE) || (lXgmacInstance < 0))
    {
        return NULL;
    }

    pxHandle = (SocfpgaXGMACHandle_t)&(xgmac_descriptors[ lXgmacInstance ]);

    /* Check if Handle is already initialized */
    if (pxHandle->cIsInitialized == TRUE)
    {
        return NULL;
    }

    /* Initialize to all 0's */
    memset(pxHandle, 0, sizeof(SocfpgaXGMACDescriptor_t));
    pxHandle->lInstance = lXgmacInstance;

    /* Update the EMAC Instance Core Base Address */
    pxHandle->pXGMACInstBaseAddr =
            (XGMAC_BaseAddr_t *)(uintptr_t)XGMAC_GET_BASE_ADDRESS(lXgmacInstance);

    /* Update the EMAC Instance DMA Base Address */
    pxHandle->pXGMACInstDMABaseAddr =
            (XGMAC_BaseAddr_t *)(uintptr_t)XGMAC_GET_DMA_BASE_ADDRESS(
            lXgmacInstance);

    /* Initialize the XGMAC Instance Parameters */
    pxHandle->ucPhyType = pXGMACConfig->ucPhyType;

    /* Enable Hardware checksum */
    pxHandle->ucChecksumMode = SOCFPGA_XGMAC_CHECKSUM_BY_HARDWARE;

    /* Initialize Tx and Rx BD Ring to zero */
    pxHandle->pTxBdRing  = NULL;
    pxHandle->pRxBdRing  = NULL;

    /* Initialize TransmitSemaphore and TransmitMutex to NULL */
    pxHandle->xTransmitSyncSem = NULL;
    pxHandle->xTransmiSyncMutex = NULL;

    pxHandle->cIsInitialized = TRUE;

    /* Return the initialized handle */
    return pxHandle;
}

int32_t SocfpgaXGMAC_EMACStart(SocfpgaXGMACHandle_t pXGMACHandle)
{
    int32_t lXgmacInstance = pXGMACHandle->lInstance;
    XGMAC_BaseAddr_t *pxEmacBaseAddr;
    XGMAC_BaseAddr_t *pxMTLBaseAddr;

    uint8_t ucBytes[ 24 ]= {0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                 0x08, 0x00,( uint8_t ) 0x45,0x00,0x00, 0x00,
                 0x00, 0x00,0x00, 0x00,128,17,0x00, 0x00,0x00, 0x00,0x00, 0x00};
                 

    pxEmacBaseAddr = pXGMACHandle->pXGMACInstBaseAddr;
    pxMTLBaseAddr = (XGMAC_BaseAddr_t *)(uintptr_t)XGMAC_GET_MTL_BASE_ADDRESS(
            lXgmacInstance);

    /* Program MAC register configurations */
    xgmac_mac_init(pxEmacBaseAddr, &xXGMACDevConfigStr);

    /* Program MTL configuration registers for Tx and Rx */
    xgmac_mtl_init(pxMTLBaseAddr, &xXGMACDevConfigStr);

    /* Setup the MAC Address in MAC High and MAC Low Registers */
    xgmac_set_macaddress(pxEmacBaseAddr, (void *)ucBytes, 
            MAC_ADRRESS_INDEX1);

    /* Start the MAC Tx and Rx */
    xgmac_dev_start(pxEmacBaseAddr);

    pXGMACHandle->cIsStarted = SOCFPGA_XGMAC_EMAC_STARTED;

    return 0;
}

int32_t SocfpgaXGMAC_DMAInitialize(
        SocfpgaXGMACHandle_t pXGMACHandle)
{
    XGMAC_BaseAddr_t *pxEmacBaseAddr;
    XGMAC_BaseAddr_t *pxDMABaseAddr;
    Basetype_t xRetVal;

    pxEmacBaseAddr = pXGMACHandle->pXGMACInstBaseAddr;
    pxDMABaseAddr = pXGMACHandle->pXGMACInstDMABaseAddr;

    /* Wait for completion of software reset */
    if (DMA_SoftReset(pxEmacBaseAddr) != true)
    {
        ERROR("SOCFPGA_XGMAC: XGMAC DMA Reset Failed.");
        return -EIO;
    }

    /* Program fields of DMA_SysBus_Mode Register */
    xgmac_dma_init(pxEmacBaseAddr,&xXGMACDevConfigStr);

    /* Set DMA Tx/Rx Descriptors */
    xRetVal = DMA_Set_Descriptors(pXGMACHandle);
    if (xRetVal != true)
    {
        ERROR("SOCFPGA_XGMAC: XGMAC DMA Set Descriptors Failed.");
        return -EIO;
    }


    /* Initialize DMA channel */
    DMA_Channel_Init(pXGMACHandle, &xXGMACDevConfigStr);

    /* Start Receive and Transmit DMA */
    xgmac_start_dma_dev(pxDMABaseAddr,&xXGMACDevConfigStr);

    /* Program  DMA Interrupt Enable Register */
    xRetVal = DMA_Enable_Interrupts(pXGMACHandle, &xXGMACDevConfigStr);
    if (xRetVal != true)
    {
        ERROR("SOCFPGA_XGMAC: XGMAC DMA Enable Interrupt Failed.");
        return -EIO;
    }

    return 0;
}

int32_t SocfpgaXGMAC_DMA_Transmit(
        SocfpgaXGMACHandle_t pXGMACHandle, \
        XGMACDMATxBufOut_t *pxDmaTxBufferOut)
{
    TickType_t xBlockTimeTicks = pdMS_TO_TICKS( 5000U );
    SocfpgaXGMACBufferDesc_t *pxDmaTxDesc;
    int32_t lHeadindx = pXGMACHandle->lTxDescHead;
    uint32_t ulDataLength = 0;
    uint32_t ulLastTxDesc;
    XGMAC_BaseAddr_t *pxDMABaseAddr;
    uint8_t ucCkecksumInsertion;
    uint32_t ulLowAddr, ulHighAddr;
    uintptr_t xBufferAddr;
    int32_t xRetStatus = 0;

    pxDMABaseAddr = pXGMACHandle->pXGMACInstDMABaseAddr;

    /* Do while once and break in case of error */
    do
    {
        if (pXGMACHandle->xTransmitSyncSem == NULL)
        {
            xRetStatus = -EIO;
            break;
        }

        if (osal_semaphore_wait( pXGMACHandle->xTransmitSyncSem,
                xBlockTimeTicks ) != pdPASS)
        {
            ERROR("SocfpgaXGMAC_DMA_Transmit: Time-out TX buffer not available.");
            xRetStatus = -EIO;
            break;
        }

        if (osal_semaphore_wait( pXGMACHandle->xTransmiSyncMutex,
                xBlockTimeTicks ) != pdFAIL)
        {

            pxDmaTxDesc = &(pXGMACHandle->pTxBdRing[lHeadindx]);

            /* Assign NW Buffer address to Desc0 address */
            xBufferAddr = (uintptr_t)pxDmaTxBufferOut->pucBuffer;
            ulLowAddr = ( uint32_t )xBufferAddr;
            ulHighAddr =( uint32_t )(xBufferAddr >> 32);

            /* The Application will set this flag if it wants to release buffers after Tx done.
             * Hence copy the buffer address later to be used in TX Done function */
            if (pxDmaTxBufferOut->ucReleaseBuffer)
            {
                pXGMACHandle->pTxDMABuf1AP[lHeadindx] = (uint8_t *)xBufferAddr;
            }
            else
            {
                pXGMACHandle->pTxDMABuf1AP[lHeadindx] = NULL;
            }

            /* Assign BufferAP address to Desc0 and Desc1  */
            pxDmaTxDesc->ulDes0 = ulLowAddr;
            pxDmaTxDesc->ulDes1 = ulHighAddr;

            ulDataLength = pxDmaTxBufferOut->ulSize;
            xgmac_flush_buffer(( void *)(uintptr_t)xBufferAddr, ulDataLength);

            /* Set Buffer-1 Length */
            pxDmaTxDesc->ulDes2 = (ulDataLength & TDES2_NORM_RD_HL_B1L_MASL);

            /* Enable Interrupt On Completion */
            pxDmaTxDesc->ulDes2 |= TDES2_NORM_RD_IOC_MASK;

            /* Prepare transmit descriptors to give to DMA. */

            /* Set the IPv4 checksum */
            ucCkecksumInsertion = pXGMACHandle->ucChecksumMode;
            if (ucCkecksumInsertion == SOCFPGA_XGMAC_CHECKSUM_BY_HARDWARE)
            {
                pxDmaTxDesc->ulDes3 |= TDES3_NORM_RD_CIC_TPL_MASK;
            }

            /* Set first and last descriptor mask */
            /* Set Own bit of the Tx descriptor to give the buffer back to DMA */
            pxDmaTxDesc->ulDes3 |= TDES3_NORM_RD_FD_MASK |
                    TDES3_NORM_RD_LD_MASK | TDES3_NORM_RD_OWN_MASK;

            /* Issue synchronization barrier instruction */
            __asm volatile ("DSB SY");

            xgmac_flush_descriptor(pxDmaTxDesc, XGMAC_DMA_DESC_SIZE);

            /* Point to next descriptor */
            lHeadindx++;
            if (lHeadindx == XGMAC_NOF_DESCRIPTORS_TX)
            {
                lHeadindx = 0;
            }

            /* Update the TX-head index */
            pXGMACHandle->lTxDescHead = lHeadindx;

            /* Program the  Tx Tail Pointer Register */
            ulLastTxDesc =
                    (uint32_t)(uintptr_t)&(pXGMACHandle->pTxBdRing[lHeadindx]);
            WR_DMA_CHNL_REG32(pxDMABaseAddr,XGMAC_DMA_CHANNEL0,
                    XGMAC_DMA_CH_TXDESC_TAIL_LPOINTER, ulLastTxDesc);

            /* Release the Mutex. */
            osal_semaphore_post( pXGMACHandle->xTransmiSyncMutex );
        }
        else
        {
            xRetStatus = -EIO;
            break;
        }
    } while(0);

    return xRetStatus;
}

void SocfpgaXGMAC_DMA_FlushBuffers(SocfpgaXGMACHandle_t pXGMACHandle)
{
    int lIdx;
    XGMAC_BaseAddr_t *pxDMABaseAddr;
    SocfpgaXGMACBufferDesc_t *pxDmaRxDesc;

    pxDMABaseAddr = pXGMACHandle->pXGMACInstDMABaseAddr;

    for (lIdx = 0; lIdx < XGMAC_NOF_DESCRIPTORS_RX; lIdx++)
    {
        pxDmaRxDesc = &(pXGMACHandle->pRxBdRing[lIdx]);
        pxDmaRxDesc->ulDes3 = XGMAC_RDES3_OWN | XGMAC_RDES3_IOC;
        xgmac_flush_descriptor(pxDmaRxDesc, XGMAC_DMA_DESC_SIZE);
    }
    pXGMACHandle->lRxDescHead = 0;
    WR_DMA_CHNL_REG32(pxDMABaseAddr,XGMAC_DMA_CHANNEL0,
            XGMAC_DMA_CH_RXDESC_TAIL_LPOINTER,
            XGMAC_NOF_DESCRIPTORS_RX - 1);
}

void SocfpgaXGMAC_DMA_AdjustHead(SocfpgaXGMACHandle_t pXGMACHandle)
{
    int idx;
    int head = pXGMACHandle->lRxDescHead;
    int update_head = 0;
    SocfpgaXGMACBufferDesc_t *pxDmaRxDesc;

    for (idx = 0; idx < XGMAC_NOF_DESCRIPTORS_RX; idx++)
    {
        head += idx;
        head = head % XGMAC_NOF_DESCRIPTORS_RX;
        pxDmaRxDesc = &(pXGMACHandle->pRxBdRing[head]);
        xgmac_invalidate_descriptor(pxDmaRxDesc, XGMAC_DMA_DESC_SIZE);
        if ((pxDmaRxDesc->ulDes3 & XGMAC_RDES3_OWN) == 0)
        {
            break;
        }
        else
        {
            update_head = 1;
        }
    }
    if (update_head)
    {
        if ((pxDmaRxDesc->ulDes3 & XGMAC_RDES3_OWN) == 0)
        {
            pXGMACHandle->lRxDescHead = head;
        }
    }
}

int32_t SocfpgaXGMAC_DMA_Receive(SocfpgaXGMACHandle_t pXGMACHandle, \
        XGMACDMARxBufIn_t *pxDmaRxBufferIn)
{
    SocfpgaXGMACBufferDesc_t *pxDmaRxDesc;
    int32_t lHeadindx = pXGMACHandle->lRxDescHead;
    uint8_t *pucEthernetBuffer;
    BaseType_t xReceivedPacketLength;
    int32_t xRetStatus;

    xRetStatus = 0;

    pxDmaRxDesc = &(pXGMACHandle->pRxBdRing[lHeadindx]);

    xgmac_invalidate_descriptor(pxDmaRxDesc, XGMAC_DMA_DESC_SIZE);

    if ((pxDmaRxDesc->ulDes3 & XGMAC_RDES3_OWN) == 0u)
    {
        /* Parse the buffer address from RxBufAP Array  */
        pucEthernetBuffer = pXGMACHandle->pRxDMABuf1AP[lHeadindx];
        xReceivedPacketLength =  pxDmaRxDesc->ulDes3 & RDES3_NORM_WR_PL_MASK;
        xgmac_invalidate_buffer(pucEthernetBuffer,xReceivedPacketLength);

        pxDmaRxBufferIn->pucBuffer = pucEthernetBuffer;
        pxDmaRxBufferIn->ulSize =  xReceivedPacketLength;
        pxDmaRxBufferIn->ulPacketStatus = pxDmaRxDesc->ulDes3;

    }
    else
    {
        xRetStatus = -EAGAIN;
    }

    return xRetStatus;
}

int32_t SocfpgaXGMAC_ReFill_RxDescriptor(
        SocfpgaXGMACHandle_t pXGMACHandle, \
        uint8_t *pucBuffer)
{
    SocfpgaXGMACBufferDesc_t *pxDmaRxDesc;
    int32_t lHeadindx = pXGMACHandle->lRxDescHead;
    XGMAC_BaseAddr_t *pxDMABaseAddr;
    uint32_t ulLastRxDesc;

    pxDMABaseAddr = pXGMACHandle->pXGMACInstDMABaseAddr;
    pxDmaRxDesc = &(pXGMACHandle->pRxBdRing[lHeadindx]);
    if (pucBuffer != NULL)
    {
        pxDmaRxDesc->ulDes0 = ( uint32_t ) (uintptr_t)pucBuffer;
        pxDmaRxDesc->ulDes1 = ( uint32_t )((uintptr_t)pucBuffer >> 32);

        pXGMACHandle->pRxDMABuf1AP[lHeadindx] = (uint8_t *)pucBuffer;
    }

    /* Release descriptors to DMA */
    pxDmaRxDesc->ulDes2 = 0;
    pxDmaRxDesc->ulDes3 = 0;

    /* Set Own bit and IOC bit of of the Rx descriptor Status */
    pxDmaRxDesc->ulDes3 = XGMAC_RDES3_OWN | XGMAC_RDES3_IOC;
    xgmac_flush_descriptor(pxDmaRxDesc, XGMAC_DMA_DESC_SIZE);

    lHeadindx = (lHeadindx + 1) % XGMAC_NOF_DESCRIPTORS_RX;

    pXGMACHandle->lRxDescHead = lHeadindx;

    /* Update the tail pointer register */
    /*
     * Tail pointer should be always ahead of current pointer
     * Setting the tail to recently processed descriptor gives better change
     * to avoid current catching up to tail (since it is a ring buffer)
     */
    ulLastRxDesc = (uint32_t)(uintptr_t)pxDmaRxDesc;
    WR_DMA_CHNL_REG32(pxDMABaseAddr,XGMAC_DMA_CHANNEL0,
            XGMAC_DMA_CH_RXDESC_TAIL_LPOINTER, ulLastRxDesc);

    return 0;
}


int32_t SocfpgaXGMAC_EMACDeInit(SocfpgaXGMACHandle_t pXGMACHandle)
{
    /* Check if Handle is valid */
    if ((pXGMACHandle == NULL) || (pXGMACHandle->cIsInitialized == FALSE))
    {
        return -EINVAL;
    }
    pXGMACHandle->cIsInitialized = FALSE;

    return 0;
}

int32_t SocfpgaXGMAC_DMADeInitialize(
        SocfpgaXGMACHandle_t pXGMACHandle)
{
    Basetype_t xRetVal;
    XGMAC_BaseAddr_t *pxDMABaseAddr;

    pxDMABaseAddr = pXGMACHandle->pXGMACInstDMABaseAddr;

    /* Disable DMA interrupts */
    xRetVal = DMA_Disable_Interrupts(pXGMACHandle, &xXGMACDevConfigStr);
    if (xRetVal != true)
    {
        ERROR("SOCFPGA_XGMAC: Failed to disable XGMAC DMA interrupts.");
        return -EIO;
    }

    /* Stop DMA */
    xgmac_stop_dma_dev(pxDMABaseAddr, &xXGMACDevConfigStr);

    /* Delete the Tx Buffer Descriptor Semaphore  */
    osal_semaphore_delete(pXGMACHandle->xTransmitSyncSem);
    pXGMACHandle->xTransmitSyncSem = NULL;

    /* Delete the Tx Buffer Descriptor Mutex  */
    osal_semaphore_delete(pXGMACHandle->xTransmiSyncMutex);
    pXGMACHandle->xTransmiSyncMutex = NULL;

    return 0;
}

void SocfpgaXGMAC_EMACStop(SocfpgaXGMACHandle_t pXGMACHandle)
{
    XGMAC_BaseAddr_t *pxEmacBaseAddr;

    pxEmacBaseAddr = pXGMACHandle->pXGMACInstBaseAddr;

    /* Stop the MAC Tx and Rx */
    xgmac_dev_stop(pxEmacBaseAddr);

    pXGMACHandle->cIsStarted = SOCFPGA_XGMAC_EMAC_STOPPED;
}

int32_t SocfpgaXGMAC_Set_Callback(
        SocfpgaXGMACHandle_t pXGMACHandle,
        SocfpgaXGMAClback_t xCallback, void *pvUserContext )
{
    if (pXGMACHandle == NULL)
    {
        return -EINVAL;
    }

    if (pXGMACHandle->cIsStarted)
    {
        return -EBUSY;
    }

    pXGMACHandle->xCallbackFunc = xCallback;
    pXGMACHandle->pvUserContext = pvUserContext;

    return 0;
}

int32_t SocfpgaXGMAC_DMA_Transmit_Done(
        SocfpgaXGMACHandle_t pXGMACHandle, \
        uint8_t **pucReleaseBuffer)
{
    int lTailindx = pXGMACHandle->lTxDescTail;
    int lHeadindx = pXGMACHandle->lTxDescHead;
    TickType_t xBlockTimeTicks = pdMS_TO_TICKS( 5000U );
    int32_t xRetStatus;

    xRetStatus = 0;

    SocfpgaXGMACBufferDesc_t *pxDmaTxDesc;

    size_t uxCount = (( UBaseType_t ) XGMAC_NOF_DESCRIPTORS_TX) -
            uxSemaphoreGetCount( pXGMACHandle->xTransmitSyncSem );

    if (osal_semaphore_wait( pXGMACHandle->xTransmiSyncMutex,
            xBlockTimeTicks ) != pdFAIL)
    {
        pxDmaTxDesc = &(pXGMACHandle->pTxBdRing[lTailindx]);

        xgmac_invalidate_descriptor(pxDmaTxDesc, XGMAC_DMA_DESC_SIZE);

        if (uxCount > 0)
        {
            if ((lTailindx == lHeadindx) &&
                    (uxCount != XGMAC_NOF_DESCRIPTORS_TX))
            {
                return -EINVAL;
            }

            *pucReleaseBuffer =
                    (uint8_t *)pXGMACHandle->pTxDMABuf1AP[lTailindx];

            /* Reset all descriptor values */
            pxDmaTxDesc->ulDes0 = 0;
            pxDmaTxDesc->ulDes1 = 0;
            pxDmaTxDesc->ulDes2 = 0;
            pxDmaTxDesc->ulDes3 = 0;

            /* Issue synchronization barrier instruction */
            __asm volatile ("DSB SY");

            uxCount--;

            /* Give back counting semaphore */
            osal_semaphore_post( pXGMACHandle->xTransmitSyncSem );

            if (++lTailindx == XGMAC_NOF_DESCRIPTORS_TX)
            {
                lTailindx = 0;
            }

            pXGMACHandle->lTxDescTail = lTailindx;
        }
        else
        {
            xRetStatus = -EAGAIN;
        }

        osal_semaphore_post( pXGMACHandle->xTransmiSyncMutex );
    }
    else
    {
        xRetStatus = -EINVAL;
    }

    return xRetStatus;
}

static Basetype_t DMA_SoftReset(XGMAC_BaseAddr_t *pxEmacBaseAddr)
{
    uint8_t ucElapsedTime = 0;
    uint8_t ucTimeout = 100;

    if (xgmac_dma_software_reset(pxEmacBaseAddr) != XGMAC_LL_RETVAL_SUCCESS)
    {
        return false;
    }
    /* Poll for reset for completion */
    while (!xgmac_is_dma_reset_done(pxEmacBaseAddr))
    {
        if (ucElapsedTime >= ucTimeout)
        {
            return false;
        }
        /* Wait for some delay */
        DELAY_MS(1);
        ucElapsedTime += 1;
    }
    return true;
}
static Basetype_t DMA_Set_Descriptors(SocfpgaXGMACHandle_t pXGMACHandle)

{
    /* Initialize the Tx and Rx Head and Tail to 0*/
    pXGMACHandle->lTxDescHead = 0;
    pXGMACHandle->lRxDescHead = 0;

    pXGMACHandle->lTxDescTail = 0;
    pXGMACHandle->lRxDescTail = 0;

    /* Create Descriptor list for Tx and Rx */
    if (pXGMACHandle->pTxBdRing == NULL)
    {
        pXGMACHandle->pTxBdRing  = pXGMACHandle->axBufferDescTx;
    }

    if (pXGMACHandle->pRxBdRing == NULL)
    {
        pXGMACHandle->pRxBdRing  = pXGMACHandle->axBufferDescRx;
    }

    /* Set all field values to zero */
    memset( pXGMACHandle->pTxBdRing, '\0', sizeof(SocfpgaXGMACBufferDesc_t));
    memset( pXGMACHandle->pRxBdRing, '\0', sizeof(SocfpgaXGMACBufferDesc_t));

    /* Setup Tx Descriptor Table Parameters */
    DMA_Setup_TxDescriptor_List( pXGMACHandle );

    /* Create the Tx Buffer Descriptor Semaphore  */
    if (pXGMACHandle->xTransmitSyncSem == NULL)
    {
        pXGMACHandle->xTransmitSyncSem = osal_semaphore_counting_create(NULL,
                ( UBaseType_t ) XGMAC_NOF_DESCRIPTORS_TX, \
                ( UBaseType_t ) XGMAC_NOF_DESCRIPTORS_TX );
        configASSERT( pXGMACHandle->xTransmitSyncSem != NULL );
    }

    /* Create the Tx Descriptor Mutex   */
    if (pXGMACHandle->xTransmiSyncMutex == NULL)
    {
        pXGMACHandle->xTransmiSyncMutex = osal_mutex_create(NULL);
        configASSERT( pXGMACHandle->xTransmiSyncMutex != NULL );
    }

    /* Setup Rx Descriptor Table Parameters */
    DMA_Setup_RxDescriptor_List( pXGMACHandle );

    return true;
}

void DMA_Setup_TxDescriptor_List(SocfpgaXGMACHandle_t pXGMACHandle )
{
    SocfpgaXGMACBufferDesc_t *pxDMADescriptor;
    uint16_t usIndex;

    /* Initialize the Tx buffer descriptor pointer */
    pxDMADescriptor = pXGMACHandle->pTxBdRing;

    /* Initialize the Tx buffer descriptor parameters - Desc0, Desc1, Desc2, Desc3 */
    for (usIndex=0; usIndex < XGMAC_NOF_DESCRIPTORS_TX; usIndex++)
    {
        /* Initialize Buffer1 address pointer in Handle TxDMA Buffer Pointer Array to NULL */
        pXGMACHandle->pTxDMABuf1AP[usIndex] = NULL;

        /* Initialize all Descriptors to 0 */
        pxDMADescriptor[usIndex].ulDes0 = 0;
        pxDMADescriptor[usIndex].ulDes1 = 0;
        pxDMADescriptor[usIndex].ulDes2 = 0;
        pxDMADescriptor[usIndex].ulDes3 = 0;

        /* Perform cache flush of buffer descriptor range  */
        xgmac_flush_descriptor(&pxDMADescriptor[usIndex], XGMAC_DMA_DESC_SIZE);
    }
}

void DMA_Setup_RxDescriptor_List(SocfpgaXGMACHandle_t pXGMACHandle )
{
    SocfpgaXGMACBufferDesc_t *pxDMADescriptor;
    uint16_t usIndex;

    /* Initialize the Rx buffer descriptor pointer */
    pxDMADescriptor = pXGMACHandle->pRxBdRing;

    /* Initialize the Rx buffer descriptor parameters - Desc0, Desc1, Desc2, Desc3 */
    for (usIndex = 0; usIndex < XGMAC_NOF_DESCRIPTORS_RX; usIndex++)
    {
        /* Initialize Buffer1 address pointer in Handle RxDMA Buffer Pointer Array to NULL */
        pXGMACHandle->pRxDMABuf1AP[usIndex] = NULL;

        /* Initialize all Descriptors to 0. These will be set in Refill Descriptor */
        pxDMADescriptor[usIndex].ulDes0 = 0;
        pxDMADescriptor[usIndex].ulDes1 = 0;
        pxDMADescriptor[usIndex].ulDes2 = 0;
        pxDMADescriptor[usIndex].ulDes3 = 0;

        /* Perform cache flush operations */
        xgmac_flush_descriptor(&pxDMADescriptor[usIndex], XGMAC_DMA_DESC_SIZE);
    }
}
static void DMA_Channel_Init(SocfpgaXGMACHandle_t pXGMACHandle,
        const XGMACDevConfigStr_t *xgmacDevConfig)
{
    XGMAC_BaseAddr_t *pxDMABaseAddr;
    XgmacDmaDescAddr_t xDmaDescAddrParams;
    SocfpgaXGMACBufferDesc_t *paxBufferDescTx;
    SocfpgaXGMACBufferDesc_t *paxBufferDescRx;
    uint8_t ucDmaChIndex = XGMAC_DMA_CHANNEL0;

    pxDMABaseAddr = pXGMACHandle->pXGMACInstDMABaseAddr;

    /* Program the DMA Channel Descriptor Registers */
    paxBufferDescTx = pXGMACHandle->pTxBdRing;
    paxBufferDescRx = pXGMACHandle->pRxBdRing;

    xDmaDescAddrParams.ulTxRingLen = XGMAC_NOF_DESCRIPTORS_TX - 1;
    xDmaDescAddrParams.ulRxRingLen = XGMAC_NOF_DESCRIPTORS_RX - 1;
    xDmaDescAddrParams.ulTxDescLowAddr =(uint32_t)(uintptr_t)paxBufferDescTx;
    xDmaDescAddrParams.ulTxDescHighAddr =
            (uint32_t)((uintptr_t)paxBufferDescTx >> 32);
    xDmaDescAddrParams.ulRxDescLowAddr = (uint32_t)(uintptr_t)paxBufferDescRx;
    xDmaDescAddrParams.ulRxDescHighAddr =
            (uint32_t)((uintptr_t)paxBufferDescRx >> 32);
    xDmaDescAddrParams.ulTxLastDescAddr =
            (uint32_t)(uintptr_t)&(paxBufferDescTx[XGMAC_NOF_DESCRIPTORS_TX -
            1]);
    xDmaDescAddrParams.ulRxLastDescAddr =
            (uint32_t)(uintptr_t)&(paxBufferDescRx[XGMAC_NOF_DESCRIPTORS_RX -
            1]);

    /* Set DMA Tx/Rx Descriptor Address */
    xgmac_init_dma_channel_desc_reg(pxDMABaseAddr,ucDmaChIndex,
            &xDmaDescAddrParams);

    /* Program  DMA Channel Control Settings */
    xgmac_config_dma_channel_control(pxDMABaseAddr, ucDmaChIndex,
            (const XGMACDMAChanlConfig_t *)xgmacDevConfig->dmaChannelConfig);
}

static Basetype_t DMA_Enable_Interrupts(SocfpgaXGMACHandle_t pXGMACHandle,
        const XGMACDevConfigStr_t *xgmacDevConfig)
{
    XGMAC_BaseAddr_t *pxDMABaseAddr;
    uint8_t ucdmaChnlIndex;
    uint8_t ucdmaNumChnls;

    ucdmaNumChnls = xgmacDevConfig->macDevConfig->nofdmachannels;
    pxDMABaseAddr = pXGMACHandle->pXGMACInstDMABaseAddr;

    for (ucdmaChnlIndex = 0; ucdmaChnlIndex < ucdmaNumChnls; ucdmaChnlIndex++)
    {
        /* Enable Normal Interrupt Summary and associated interrupt mask bits */
        xgmac_enable_dma_interrupt(pxDMABaseAddr, ucdmaChnlIndex,
                eInterruptNIS);

        /* Enable Error Interrupt Summary and associated interrupt mask bits */
        xgmac_enable_dma_interrupt(pxDMABaseAddr, ucdmaChnlIndex,
                eInterruptAIS);
    }

    /* Register IRQ Handler with GIC for XGMAC */
    if (DMA_RegisterISR(pXGMACHandle) != true)
    {
        ERROR("SOCFPGA_XGMAC Init: ISR Registration Failed.");
        return false;
    }
    return true;
}

static Basetype_t DMA_RegisterISR(SocfpgaXGMACHandle_t pXGMACHandle)
{
    int32_t lInstance;
    SocfpgaHpuInterrupt_t xEMACIntID;
    SocfpgaInterruptErr_t xErrRet;

    lInstance = pXGMACHandle->lInstance;
    xEMACIntID = eEmac0Irq + lInstance * 17;

    /* Clear the MAC based interrupts */
    xgmac_disable_interrupt(pXGMACHandle->pXGMACInstBaseAddr);

    /* Register the ISR */
    xErrRet = interrupt_register_isr(xEMACIntID,
            (SocfpgaInterruptCallback_t) socfpga_xgmac_dma_isr,
            pXGMACHandle);
    if (xErrRet != eErrOk)
        return false;

    /* Enable Shared peripheral interrupt */
    xErrRet =  interrupt_enable(xEMACIntID, GIC_INTERRUPT_PRIORITY_ENET);
    if (xErrRet != eErrOk)
        return false;

    return true;
}

static Basetype_t DMA_Disable_Interrupts(SocfpgaXGMACHandle_t pXGMACHandle,
        const XGMACDevConfigStr_t *xgmacDevConfig)
{
    XGMAC_BaseAddr_t *pxDMABaseAddr;
    uint8_t ucdmaChnlIndex;
    uint8_t ucdmaNumChnls;

    ucdmaNumChnls = xgmacDevConfig->macDevConfig->nofdmachannels;
    pxDMABaseAddr = pXGMACHandle->pXGMACInstDMABaseAddr;

    /* Unregister the DMA ISR */
    if (DMA_UnRegisterISR(pXGMACHandle) != true)
    {
        ERROR("SOCFPGA_XGMAC_DMA: ISR UnRegistration Failed.");
        return false;
    }
    for (ucdmaChnlIndex= 0; ucdmaChnlIndex < ucdmaNumChnls; ucdmaChnlIndex++)
    {
        /* Disable Normal Interrupt Summary and associated interrupt mask bits */
        xgmac_disable_dma_interrupt(pxDMABaseAddr,ucdmaChnlIndex,
                eInterruptNIS);

        /* Disable Error Interrupt Summary and associated interrupt mask bits */
        xgmac_disable_dma_interrupt(pxDMABaseAddr,ucdmaChnlIndex,
                eInterruptAIS);
    }

    return true;

}

static Basetype_t DMA_UnRegisterISR(SocfpgaXGMACHandle_t pXGMACHandle)
{
    int32_t lInstance;
    SocfpgaHpuInterrupt_t xEMACIntID;
    SocfpgaInterruptErr_t xErrRet;

    lInstance = pXGMACHandle->lInstance;
    xEMACIntID = eEmac0Irq + lInstance * 17;

    /* Disable interrupts */
    xErrRet =  interrupt_spi_disable(xEMACIntID);
    if (xErrRet != eErrOk)
        return false;

    return true;
}

static void socfpga_xgmac_dma_isr( SocfpgaXGMACHandle_t pXGMACHandle)
{
    XGMAC_BaseAddr_t *pxDMABaseAddr;
    XgmacDmaInterruptID_t id;
    XgmacErrorType_t errType;
    XGMACInterruptStatus_t xIntStatus;
    uint32_t ulBaseDmaChnlAddress;
    uint8_t dmachnum;

    pxDMABaseAddr = pXGMACHandle->pXGMACInstDMABaseAddr;
    dmachnum = XGMAC_DMA_CHANNEL0;
    ulBaseDmaChnlAddress =
            ((uint32_t)(uintptr_t)pxDMABaseAddr + XGMAC_DMA_CHANNEL_BASE +
            (dmachnum * XGMAC_DMA_CHANNEL_INC));

    id = check_and_clear_xgmac_interrupt_status(ulBaseDmaChnlAddress);

    switch (id)
    {
    case eInterruptTI:
        xIntStatus = eXGMACTransmitDoneEvent;
        break;

    case eInterruptRI:
        xIntStatus = eXGMACReceiveEvent;
        break;

    case eInterruptFBE:
    case eInterruptTXS:
    case eInterruptRBU:
    case eInterruptRS:
    case eInterruptDDE:
    case eInterruptUnhandled:
    {
        XGMACInterruptData_t *pIntData =
                (XGMACInterruptData_t *)pXGMACHandle->pvUserContext;
        xIntStatus = eXGMACErrorEvent;

        /* Re-mapping the error type to handle in the Network Interface Layer */
        switch (id)
        {
        case eInterruptFBE:
            errType = eErrFatalBus;
            break;
        case eInterruptTXS:
            errType = eErrTransmitStopped;
            break;
        case eInterruptRBU:
            errType = eErrRxBufferUnavailable;
            break;
        case eInterruptRS:
            errType = eErrtReceiveStopped;
            break;
        case eInterruptDDE:
            errType = eErrDescDefinition;
            break;
        default:
            break;
        }

        pIntData->ucErrDMAChnlNum = dmachnum;
        pIntData->ucErrType = errType;
        break;
    }

    default:
        break;
    }

    if (pXGMACHandle->xCallbackFunc != NULL)
    {
        pXGMACHandle->xCallbackFunc(xIntStatus, pXGMACHandle->pvUserContext);
        check_and_clear_link_interrupt_status((uint32_t)((uintptr_t)pXGMACHandle
                ->pXGMACInstBaseAddr));
    }
}
