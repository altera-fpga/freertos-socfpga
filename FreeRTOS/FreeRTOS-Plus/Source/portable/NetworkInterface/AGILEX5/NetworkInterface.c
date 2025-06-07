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

/*****************************************************************************
* Note: This file is Not! to be used as is. The purpose of this file is to provide
* a template for writing a network interface. Each network interface will have to provide
* concrete implementations of the functions in this file.
*
* See the following URL for an explanation of this file and its functions:
* https://freertos.org/FreeRTOS-Plus/FreeRTOS_Plus_TCP/Embedded_Ethernet_Porting.html
*
*****************************************************************************/

/* Standard includes. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

/* FreeRTOS includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

/* FreeRTOS+TCP includes. */
#include "FreeRTOS_IP.h"
#include "FreeRTOS_Sockets.h"
#include "FreeRTOS_IP_Private.h"
#include "FreeRTOS_ARP.h"
#include "NetworkBufferManagement.h"
#include "NetworkInterface.h"
#include "FreeRTOS_Routing.h"
#include <FreeRTOSIPConfig.h>

/* XGMAC HAL and LL Driver Includes.  */
#include "socfpga_xgmac.h"
#include "socfpga_xgmac_phy.h"

/* If ipconfigETHERNET_DRIVER_FILTERS_FRAME_TYPES is set to 1, then the Ethernet
 * driver will filter incoming packets and only pass the stack those packets it
 * considers need processing. */
#if (ipconfigETHERNET_DRIVER_FILTERS_FRAME_TYPES == 0)
#define ipCONSIDER_FRAME_FOR_PROCESSING( pucEthernetBuffer )    eProcessBuffer
#else
#define ipCONSIDER_FRAME_FOR_PROCESSING( pucEthernetBuffer )    \
    eConsiderFrameForProcessing( \
        (pucEthernetBuffer))
#endif

#define EMAC_MAX_INSTANCE    3U

#ifndef niEMAC_HANDLER_TASK_PRIORITY
/* Define the priority of the task prvEMACHandlerTask(). */
#define niEMAC_HANDLER_TASK_PRIORITY    configMAX_PRIORITIES - 1
#endif

#define niBMSR_LINK_STATUS              0x0004uL

#ifndef PHY_LS_HIGH_CHECK_TIME_MS

/* Check if the Link Status in the PHY is still high after 15 seconds of not
 * receiving packets. */
#define PHY_LS_HIGH_CHECK_TIME_MS    15000
#endif

#ifndef PHY_LS_LOW_CHECK_TIME_MS
/* Check if the LinkSStatus in the PHY is still low every second. */
#define PHY_LS_LOW_CHECK_TIME_MS    1000
#endif


/* Default the size of the stack used by the XGMAC deferred handler task to 4 times
 * the size of the stack used by the idle task. */
#ifndef configEMAC_TASK_STACK_SIZE
#define configEMAC_TASK_STACK_SIZE    (4 * configMINIMAL_STACK_SIZE)
#endif

#if (ipconfigDRIVER_INCLUDED_RX_IP_CHECKSUM == 0 || \
    ipconfigDRIVER_INCLUDED_TX_IP_CHECKSUM == 0)
#warning \
    Please define both 'ipconfigDRIVER_INCLUDED_RX_IP_CHECKSUM' and 'ipconfigDRIVER_INCLUDED_TX_IP_CHECKSUM' as 1 to enable CRC offloading
#endif

/* Interrupt events to process.  Currently only the Rx event is processed
 * although code for other events is included to allow for possible future
 * expansion. */
#define XGMAC_IF_RX_EVENT     1UL
#define XGMAC_IF_TX_EVENT     2UL
#define XGMAC_IF_ERR_EVENT    4UL
#define XGMAC_IF_ALL_EVENT    (XGMAC_IF_RX_EVENT | XGMAC_IF_TX_EVENT | \
    XGMAC_IF_ERR_EVENT)

#define TX_BUFFER_COUNT       XGMAC_NOF_DESCRIPTORS_TX
#define TX_BUFFER_SIZE        XGMAC_MAX_PACKET_SIZE

#define RX_BUFFER_COUNT       XGMAC_NOF_DESCRIPTORS_RX
#define RX_BUFFER_SIZE        XGMAC_MAX_PACKET_SIZE


static NetworkInterface_t *AgxInterface = NULL;

/* Structure with TX data output buffer details*/
typedef struct
{
    uint8_t *pTxBuffer[TX_BUFFER_COUNT];
    uint16_t usHeadIndex;
    uint16_t usTailIndex;
    uint16_t usBufUsedCnt;
    uint8_t ucIsInitiazed;
}TxBufferPool_t;

#if (!(ipconfigZERO_COPY_TX_DRIVER))
TxBufferPool_t TxBufferPool = {NULL};
static TxBufferPool_t  *pTxBufferPool = &TxBufferPool;
#endif

static SemaphoreHandle_t xTxBufSynchSem = NULL;

/* Structure with RX data output buffer details*/
typedef struct
{
    uint8_t *pRxBuffer[RX_BUFFER_COUNT];
    uint16_t usHeadIndex;
    uint16_t usTailIndex;
    uint16_t usBufUsedCnt;
    uint8_t ucIsInitiazed;
}RxBufferPool_t;

#if (!(ipconfigZERO_COPY_RX_DRIVER))
RxBufferPool_t RxBufferPool = {NULL};
static RxBufferPool_t  *pRxBufferPool = &RxBufferPool;
#endif

static SemaphoreHandle_t xRxBufSynchSem = NULL;

BaseType_t prvUpdateRxDMADescriptors( uint8_t  *pucRxBuffer,
        NetworkInterface_t*pxInterface );
/*
 * Create a Pool of DMA Tx and Rx Buffers in case Zero Copy disabled
 */
BaseType_t prvCreateTxBufferPool(TxBufferPool_t *pTxBufferPool);
BaseType_t prvCreateRxBufferPool(RxBufferPool_t *pRxBufferPool);

/*
 * Get Tx and Rx Buffers
 */
uint8_t *pucGetTXBuffer( TxBufferPool_t *pTxBufferPool, size_t xWantedSize );
uint8_t *pucGetRXBuffer( RxBufferPool_t *pRxBufferPool, size_t xWantedSize );

/*
 * Release Tx and Rx Buffers
 */
BaseType_t pucReleaseTXBuffer( TxBufferPool_t *pTxBufferPool, void *pucBuffer );
BaseType_t pucReleaseRXBuffer( RxBufferPool_t *pRxBufferPool, void *pucBuffer );

/*
 * Reinit Tx and Rx Buffers in case Zero Copy disabled
 */
BaseType_t prvReinitTxBufferPool(TxBufferPool_t *pTxBufferPool);
BaseType_t prvReinitRxBufferPool(RxBufferPool_t *pRxBufferPool);
/*
 * A deferred interrupt handler for XGMAC DMA interrupt sources.
 */
static void prvEMACHandlerTask( NetworkInterface_t *pxInterface );

static void prvHandleErrorEvents(uint8_t ucErrStatus, uint8_t ucErrChnlNum,
        NetworkInterface_t*pxInterface);

/* Holds the handle of the task used as a deferred interrupt processor */
TaskHandle_t xEMACTaskHandle = NULL;

static void prvEMACIRQHanlderCallback(XGMACInterruptStatus_t xIntrStatus,
        void *pvIrqData );

/*
 * NetworkInterfaceInput function to receive a new packet and send it to IP_task
 */
BaseType_t prvNetworkInterfaceInput( NetworkInterface_t*pxInterface);

/*
 * prvNetworkInterfaceDown function to stop EMAC and DMA for recovery from error
 */
BaseType_t prvNetworkInterfaceOutDone( NetworkInterface_t*pxInterface );

/*
 * NetworkInterfaceInput function to receive a new packet and send it to IP_task
 */
void prvNetworkInterfaceDown(NetworkInterface_t *pxInterface);

static void prvPassEthMessages( NetworkBufferDescriptor_t *pxDescriptor );

static inline unsigned long prvReadMDIO(unsigned ulReg,
        NetworkInterface_t*pxInterface);
BaseType_t GetPhyLinkStatus();
BaseType_t prvPhyCheckLinkStatus(TickType_t xMaxTimeTicks,
        NetworkInterface_t*pxInterface);

/* A copy of PHY register 1: 'COPPER_STATUS_REG' */
static uint32_t ulPHYLinkStatus = 0uL;
uint32_t PhyLinkSpeed;

/* The function xNetworkInterfaceInitialise() will be called as
 * long as it returns the value pdFAIL.
 * It will go through several stages as described in 'eEMACState'.
 */
typedef enum XGMAC_STATE
{
    XGMAC_EMACInit,
    XGMAC_PHYInit,
    XGMAC_DMAInit,
    XGMAC_EMACStart,
    XGMAC_PHYWait,
    XGMAC_Ready,
    XGMAC_Failed,
} XGMACState_t;

/**
 * @brief  Configuration parameters for XGMAC Device Config
 */

static XGMACConfig_t xEmacConfig[EMAC_MAX_INSTANCE];
SemaphoreHandle_t xSemaphoreCounterTx;

/* Initialize PHY parameters */
XGMACPhyConfig_t xPhyDev = {

    .phy_address = 0,
    .phy_identifier = 0,
    .phy_interface = PHY_IF_SELECT_RGMII,
    .enable_autonegotiation = ENABLE_AUTONEG,
    .speed_mbps = ETH_SPEED_1000_MBPS,
    .duplex = PHY_FULL_DUPLEX,
    .advertise = ADVERTISE_ALL,
    .link_status = 0,
};

static XGMACState_t eXGMACState = XGMAC_EMACInit;

BaseType_t xNetworkInterfaceInitialise(  NetworkInterface_t *pxInterface )
{
    int instance = (int)((uintptr_t)pxInterface->pvArgument);
    BaseType_t xReturn = pdFAIL;
    BaseType_t xRetVal;
    int32_t xStatus;
    const TickType_t xWaitLinkDelay = pdMS_TO_TICKS( 1000U );
    SocfpgaXGMACHandle_t pXGMACHandle;
    uint8_t *pucBufferPool = NULL;
    eXGMACState = XGMAC_EMACInit;

    xSemaphoreCounterTx = xSemaphoreCreateCounting(512, 0);
    configASSERT(xSemaphoreCounterTx != NULL);

    AgxInterface = pxInterface;

    switch (eXGMACState)
    {
    case XGMAC_EMACInit:
        pXGMACHandle = SocfpgaXGMAC_EMACInit((XGMACConfig_t*)(&(xEmacConfig[instance])));
        xEmacConfig[instance].pXGMACHandle = pXGMACHandle;

        if (pXGMACHandle == NULL)
        {
            FreeRTOS_printf(("SOCFPGA_XGMAC: EMAC Initialization Failed....\n"));
            eXGMACState = XGMAC_Failed;
            break;
        }

        pXGMACHandle->pvUserContext = (void *) &pXGMACHandle->xXGMACIntData;
        xStatus = SocfpgaXGMAC_Set_Callback(pXGMACHandle, \
                prvEMACIRQHanlderCallback, pXGMACHandle->pvUserContext);
        if( xStatus != 0 )
        {
            FreeRTOS_printf((
                        "SOCFPGA_XGMAC: IRQ Callback Registration Failed....\n"));
            eXGMACState = XGMAC_Failed;
            break;
        }

        /* Transition to PHY Init */
        eXGMACState = XGMAC_PHYInit;

    /* Fall through. */
    case XGMAC_PHYInit:

        /* Detect the PHY */
        xStatus = SocfpgaXGMAC_PhyDiscover(pXGMACHandle, &xPhyDev);
        if( xStatus != 0 )
        {
            FreeRTOS_printf(("SOCFPGA_XGMAC: PHY Detection Failed....\n"));
            eXGMACState = XGMAC_Failed;
            break;
        }

        /* Initialize PHY */
        xStatus = SocfpgaXGMAC_PhyInitialize(pXGMACHandle, &xPhyDev);

        if( xStatus != 0 )
        {
            FreeRTOS_printf(("SOCFPGA_XGMAC: PHY Initialization Failed....\n"));
            eXGMACState = XGMAC_Failed;
            break;
        }

        /* Transition to DMA Init */
        eXGMACState = XGMAC_DMAInit;

    /* Fall through. */
    /* Initialize DMA and enable DMA interrupts */
    case XGMAC_DMAInit:
        /* Create DMA Buffer Pool for Tx */
#if (!(ipconfigZERO_COPY_TX_DRIVER != 0))
        if (!(pTxBufferPool->ucIsInitiazed))
        {
            xRetVal = prvCreateTxBufferPool(pTxBufferPool);
            if (xRetVal != pdPASS)
            {
                FreeRTOS_printf((
                            "SOCFPGA_XGMAC: Tx DMA Buffer Allocation Failed....\n"));
                eXGMACState = XGMAC_Failed;
                break;
            }
        }

#endif
        /* Create DMA Buffer Pool for Rx */
#if (!(ipconfigZERO_COPY_RX_DRIVER != 0))
        if (!(pRxBufferPool->ucIsInitiazed))
        {
            xRetVal = prvCreateRxBufferPool(pRxBufferPool);
            if (xRetVal != pdPASS)
            {
                FreeRTOS_printf((
                            "SOCFPGA_XGMAC: Rx DMA Buffer Allocation Failed....\n"));
                eXGMACState = XGMAC_Failed;
                break;
            }
        }

        pucBufferPool = (uint8_t *)pRxBufferPool;

#endif
        xStatus = SocfpgaXGMAC_DMAInitialize(pXGMACHandle);
        if( xStatus != 0 )
        {
            FreeRTOS_printf(("SOCFPGA_XGMAC: DMA Initialization Failed....\n"));
            eXGMACState = XGMAC_Failed;
            break;
        }

        /* ReFill DMA Rx Descriptors with Rx DMA Buffer Addresses */
        xRetVal = prvUpdateRxDMADescriptors( pucBufferPool, pxInterface );
        if (xRetVal != pdPASS)
        {
            FreeRTOS_printf(("SOCFPGA_XGMAC: Update Rx Descriptors Failed....\n"));
            eXGMACState = XGMAC_Failed;
            break;
        }

        /* Transition to EMAC Start */
        eXGMACState = XGMAC_EMACStart;

    /* Fall through. */
    /* Configure and start the EMAC */
    case XGMAC_EMACStart:
        xStatus = SocfpgaXGMAC_CfgSpeedMode(pXGMACHandle, &xPhyDev);
        if( xStatus != 0 )
        {
            FreeRTOS_printf((
                        "SOCFPGA_XGMAC: Set EMAC operating Speed Failed....\n"));
            eXGMACState = XGMAC_Failed;
            break;
        }

        xStatus = SocfpgaXGMAC_EMACStart(pXGMACHandle);
        if( xStatus != 0 )
        {
            FreeRTOS_printf(("SOCFPGA_XGMAC: EMAC Start Failed....\n"));
            eXGMACState = XGMAC_Failed;
            break;
        }

        /* Transition to Wait for PHY */
        eXGMACState = XGMAC_PHYWait;

    /* Fall through. */
    case XGMAC_PHYWait:
        prvPhyCheckLinkStatus(xWaitLinkDelay,pxInterface);
        if (GetPhyLinkStatus() == pdFALSE)
        {
            /* Link status is not yet high, stay in XGMAC_WaitPHY */
            break;
        }

        if (xEMACTaskHandle == NULL)
        {
            /* create task for deferred interrupt handler and initialize it's handle */
            xTaskCreate((TaskFunction_t)prvEMACHandlerTask, "EMAC",
                    configEMAC_TASK_STACK_SIZE,pxInterface,
                    niEMAC_HANDLER_TASK_PRIORITY, &xEMACTaskHandle );

            if (xEMACTaskHandle == NULL)
            {
                eXGMACState = XGMAC_Failed;
                break;
            }
        }

        /* Transition to EMAC Ready */
        eXGMACState = XGMAC_Ready;

    /* Fall through. */

    case XGMAC_Ready:
        /* The network driver is operational. */
        xReturn = pdPASS;
        break;

    case XGMAC_Failed:
        /* A fatal error has occurred, and the driver
         * can not start. */
        break;
    }

    return xReturn;
}

void prvNetworkInterfaceDown(NetworkInterface_t *pxInterface)
{
    int instance = (int)((uintptr_t)pxInterface->pvArgument);
    int32_t eth_state;
    SocfpgaXGMACHandle_t pXGMACHandle = (SocfpgaXGMACHandle_t)xEmacConfig[instance].pXGMACHandle;

    /* Stop the EMAC */
    SocfpgaXGMAC_EMACStop(pXGMACHandle);

    /* De-init the DMA */
    eth_state = SocfpgaXGMAC_DMADeInitialize(pXGMACHandle);
    if(eth_state != 0)
    {
        FreeRTOS_printf(("DMA De-Initialization failed \n"));
    }
        
    /* De-init the EMAC */
    eth_state = SocfpgaXGMAC_EMACDeInit(pXGMACHandle);
    if(eth_state != 0)
    {
        FreeRTOS_printf(("EMAC De-Initialization failed \n"));
    }

    /* Reset DMA Buffer Pool */
#if (ipconfigZERO_COPY_TX_DRIVER == 0)
    prvReinitTxBufferPool( pTxBufferPool );
#endif

#if (ipconfigZERO_COPY_RX_DRIVER == 0)
    prvReinitRxBufferPool( pRxBufferPool );
#endif
}

BaseType_t xNetworkInterfaceOutput( NetworkInterface_t *pxInterface,
        NetworkBufferDescriptor_t *const pxNetworkBuffer,
        BaseType_t bReleaseAfterSend )
{
    int instance = (int)((uintptr_t)pxInterface->pvArgument);
    SocfpgaXGMACHandle_t pXGMACHandle = (SocfpgaXGMACHandle_t)xEmacConfig[instance].pXGMACHandle;
    int32_t xStatus;

    XGMACDMATxBufOut_t xDMATxBuffer;
    uint8_t *pucBuffer;
    uint32_t ulDataLength = 0;

    /* Check Link Status and Call XGMAC transmit function */
    if ((ulPHYLinkStatus & niBMSR_LINK_STATUS) != 0UL)
    {
        iptraceNETWORK_INTERFACE_TRANSMIT();

        /* Check Buffer size and adjust to Max packet size */
        ulDataLength = pxNetworkBuffer->xDataLength;

        if (ulDataLength > XGMAC_MAX_PACKET_SIZE)
        {
            ulDataLength = XGMAC_MAX_PACKET_SIZE;
        }

#if (ipconfigZERO_COPY_TX_DRIVER == 0)
        /* Get Tx Buffer Index from DMA Tx Buffer Pool */
        pucBuffer = pucGetTXBuffer(pTxBufferPool, XGMAC_MAX_PACKET_SIZE);
        configASSERT( pucBuffer != NULL );

        /* Copy the bytes from NW buffer to XGMAC Tx Buffer  */
        memcpy(pucBuffer, pxNetworkBuffer->pucEthernetBuffer, ulDataLength );
#else
        /* Pass the address of Network Buffer as-is */
        pucBuffer = (uint8_t *)pxNetworkBuffer->pucEthernetBuffer;
        bReleaseAfterSend = pdFALSE;
#endif

        xDMATxBuffer.pucBuffer = pucBuffer;
        xDMATxBuffer.ulSize = ulDataLength;

        /* The TCP/IP buffer should be released back to stack after tx done
         * hence set the flag. This will be used in DMA TRansmit Done */
        xDMATxBuffer.ucReleaseBuffer = 1;

        /* XGMAC transmit function */
        xStatus = SocfpgaXGMAC_DMA_Transmit( pXGMACHandle, &(xDMATxBuffer));
        if(xStatus != 0)
        {
            bReleaseAfterSend = pdTRUE;
        }
    }

    if (bReleaseAfterSend != pdFALSE)
    {
        /* Release NW buffer if No link or failure in Transmit */
        vReleaseNetworkBufferAndDescriptor( pxNetworkBuffer );
    }

    return pdTRUE;
}

BaseType_t prvPhyCheckLinkStatus(TickType_t xMaxTimeTicks,
        NetworkInterface_t*pxInterface)
{
    TickType_t xStartTime, xEndTime;
    const TickType_t xShortDelay = pdMS_TO_TICKS( 20UL );
    BaseType_t xReturn;

    xStartTime = xTaskGetTickCount();

    for ( ; ;)
    {
        xEndTime = xTaskGetTickCount();

        if (xEndTime - xStartTime > xMaxTimeTicks)
        {
            xReturn = pdFALSE;
            break;
        }

        ulPHYLinkStatus = prvReadMDIO( COPPER_STATUS_REG,pxInterface );

        if ((ulPHYLinkStatus & niBMSR_LINK_STATUS) != 0uL)
        {
            xReturn = pdTRUE;
            break;
        }
        vTaskDelay( xShortDelay );
    }
    return xReturn;
}

static void prvPassEthMessages( NetworkBufferDescriptor_t *pxDescriptor )
{
    IPStackEvent_t xRxEvent;

    xRxEvent.eEventType = eNetworkRxEvent;
    xRxEvent.pvData = ( void * ) pxDescriptor;

    if (xSendEventStructToIPTask( &xRxEvent, ( TickType_t ) 1000U ) != pdPASS)
    {
        /* The buffer could not be sent to the stack so must be released again.
         * This is a deferred handler task, not a real interrupt, so it is ok to
         * use the task level function here. */
#if (ipconfigUSE_LINKED_RX_MESSAGES != 0)
        {
            do
            {
                NetworkBufferDescriptor_t *pxNext = pxDescriptor->pxNextBuffer;
                vReleaseNetworkBufferAndDescriptor( pxDescriptor );
                pxDescriptor = pxNext;
            } while(pxDescriptor != NULL);
        }
#else
        {
            vReleaseNetworkBufferAndDescriptor( pxDescriptor );
        }
#endif /* ipconfigUSE_LINKED_RX_MESSAGES */

        iptraceETHERNET_RX_EVENT_LOST();
        FreeRTOS_printf(("prvPassEthMessages: Can not queue return packet!\n"));
    }
    else
    {
        iptraceNETWORK_INTERFACE_RECEIVE();
    }
}

BaseType_t prvNetworkInterfaceInput( NetworkInterface_t *pxInterface )
{
#if (ipconfigUSE_LINKED_RX_MESSAGES != 0)
    NetworkBufferDescriptor_t *pxFirstDescriptor = NULL;
    NetworkBufferDescriptor_t *pxLastDescriptor = NULL;
#endif
    int instance = (int)((uintptr_t)pxInterface->pvArgument);
    SocfpgaXGMACHandle_t pXGMACHandle = (SocfpgaXGMACHandle_t)xEmacConfig[instance].pXGMACHandle;
    BaseType_t xReceivedPacketLength;
    int32_t xStatus;
    XGMACDMARxBufIn_t xDMARxBufferIn;
    uint8_t *pucEthernetBuffer;
    uint8_t *pucRefillRxBuffer;
    uint32_t ulPacketStatus;
    volatile int msgCount = 0;

    /*Adjust head pointer*/
    SocfpgaXGMAC_DMA_AdjustHead(pXGMACHandle);

    do
    {
        NetworkBufferDescriptor_t *pxCurrentBufferDesc, *pxNewBufferDesc = NULL;
        BaseType_t xSendPacket = pdTRUE;

        /* XGMAC receive function */
        xStatus = SocfpgaXGMAC_DMA_Receive( pXGMACHandle, &(xDMARxBufferIn));
        if(xStatus != 0)
        {
            break;
        }

        pucEthernetBuffer = xDMARxBufferIn.pucBuffer;
        xReceivedPacketLength = xDMARxBufferIn.ulSize;
        pucRefillRxBuffer = pucEthernetBuffer;

        /* Check the validity of the received packet and send only if valid else discard here */
        /* Check if packet has errors and discard if true */
        ulPacketStatus = xDMARxBufferIn.ulPacketStatus;
        if ((ulPacketStatus &
                (RDES3_NORM_WR_LD_MASK | RDES3_NORM_WR_ES_MASK)) ==
                XGMAC_RECEIVE_PACKET_ERROR)
        {
            xSendPacket = pdFALSE;
        }
        else if (eConsiderFrameForProcessing( pucEthernetBuffer ) !=
                eProcessBuffer)
        {
            xSendPacket = pdFALSE;
        }
        else
        {
            pxNewBufferDesc = pxGetNetworkBufferWithDescriptor(
                    XGMAC_MAX_PACKET_SIZE, ( TickType_t ) 0 );

            if (pxNewBufferDesc == NULL)
            {
                /* A packet has been received, but there is no replacement buffer and hence packet
                 * will be dropped
                 */
                FreeRTOS_printf(("Unable to allocate a Network Buffer\n"));
                xSendPacket = pdFALSE;
            }
        }

        if (xSendPacket != pdFALSE)
        {
#if (ipconfigZERO_COPY_RX_DRIVER != 0)
            {
                pxCurrentBufferDesc = pxPacketBuffer_to_NetworkBuffer(
                        pucEthernetBuffer );
                configASSERT( pxCurrentBufferDesc != NULL );
                

                pucRefillRxBuffer = pxNewBufferDesc->pucEthernetBuffer;
            }
#else
            {
                /* In zero copy mode both descriptors are the same. */
                pxCurrentBufferDesc = pxNewBufferDesc;

                if (pxNewBufferDesc != NULL)
                {
                    /* Update the Buf1 address in Desc0 field as this replaces old buffer  */
                    memcpy( pxNewBufferDesc->pucEthernetBuffer,
                            pucEthernetBuffer,
                            xReceivedPacketLength);
                }
            }
#endif
            pxCurrentBufferDesc->xDataLength = xReceivedPacketLength - 4;  /*Subtract 4 bytes of CRC from recieved packet*/
            pxCurrentBufferDesc->pxInterface = AgxInterface;
            pxCurrentBufferDesc->pxEndPoint = FreeRTOS_MatchingEndpoint(
                    pxCurrentBufferDesc->pxInterface,
                    pxCurrentBufferDesc->pucEthernetBuffer);


#if (ipconfigUSE_LINKED_RX_MESSAGES != 0)
            {
                pxCurrentBufferDesc->pxNextBuffer = NULL;

                if (pxFirstDescriptor == NULL)
                {
                    /* Becomes the first message */
                    pxFirstDescriptor = pxCurrentBufferDesc;
                }
                else if (pxLastDescriptor != NULL)
                {
                    /* Add to the tail */
                    pxLastDescriptor->pxNextBuffer = pxCurrentBufferDesc;
                }

                pxLastDescriptor = pxCurrentBufferDesc;
            }
#else /* if ( ipconfigUSE_LINKED_RX_MESSAGES != 0 ) */
            {
                prvPassEthMessages( pxCurrentBufferDesc );
            }
#endif /* if ( ipconfigUSE_LINKED_RX_MESSAGES != 0 ) */
            msgCount++;
        }

        /* Update the descriptor with new address and rest other fields */
        SocfpgaXGMAC_ReFill_RxDescriptor(pXGMACHandle, pucRefillRxBuffer);
    } while(1);

#if (ipconfigUSE_LINKED_RX_MESSAGES != 0)
    {
        if (pxFirstDescriptor != NULL)
        {
            prvPassEthMessages( pxFirstDescriptor );
        }
    }
#endif /* ipconfigUSE_LINKED_RX_MESSAGES */

    return msgCount;
}

static void prvEMACHandlerTask( NetworkInterface_t *pxInterface )
{
    TimeOut_t xPhyTime;
    int instance = (int)((uintptr_t)pxInterface->pvArgument);
    TickType_t xPhyRemTime;
    BaseType_t xResult;
    uint32_t xStatus;
    const TickType_t ulMaxBlockTime = pdMS_TO_TICKS( 100UL );
    uint32_t ulISREvents = 0U;
    SocfpgaXGMACHandle_t pXGMACHandle = (SocfpgaXGMACHandle_t)xEmacConfig[instance].pXGMACHandle;

    /* Remove compiler warnings about unused parameters. */

    vTaskSetTimeOutState( &xPhyTime );
    xPhyRemTime = pdMS_TO_TICKS( PHY_LS_LOW_CHECK_TIME_MS );

    for ( ; ;)
    {
        xResult = 0;

#ifdef ENABLE_PRINTF
#if (ipconfigHAS_PRINTF != 0)
        {
            /* Call a function that monitors resources: the amount of free network
             * buffers and the amount of free space on the heap.  See FreeRTOS_IP.c
             * for more detailed comments. */
            vPrintResourceStats();
        }
#endif /* ( ipconfigHAS_PRINTF != 0 ) */
#endif

        /* Wait for a new event or a time-out. */
        xTaskNotifyWait( 0U,                        /* ulBitsToClearOnEntry */
                XGMAC_IF_ALL_EVENT,                 /* ulBitsToClearOnExit */
                &(ulISREvents),                    /* pulNotificationValue */
                ulMaxBlockTime );
        if ((ulISREvents & XGMAC_IF_RX_EVENT) != 0)
        {
            prvNetworkInterfaceInput(pxInterface);
        }

        if ((ulISREvents & XGMAC_IF_TX_EVENT) != 0)
        {
            prvNetworkInterfaceOutDone(pxInterface);
        }

        if ((ulISREvents & XGMAC_IF_ERR_EVENT) != 0)
        {
            uint8_t ucErrType;
            uint8_t ucErrDMAChnlNum;

            /* Notify prvEMACHandlerTask of Error event with Error Type and Error Data */
            ucErrType = ulISREvents >> 24;
            ucErrDMAChnlNum = ulISREvents >> 16 & 0xFFFF;
            prvHandleErrorEvents(ucErrType, ucErrDMAChnlNum,pxInterface);
        }

        if (xResult > 0)
        {
            /* A packet was received. No need to check for the PHY status now,
             * but set a timer to check it later on. */
            vTaskSetTimeOutState( &xPhyTime );
            xPhyRemTime = pdMS_TO_TICKS( PHY_LS_HIGH_CHECK_TIME_MS );
            xResult = 0;

            if ((ulPHYLinkStatus & niBMSR_LINK_STATUS) == 0uL)
            {
                /* Indicate that the Link Status is high, so that
                 * xNetworkInterfaceOutput() can send packets. */
                ulPHYLinkStatus |= niBMSR_LINK_STATUS;
                FreeRTOS_printf(("prvEMACHandlerTask: PHY LS assume 1\n"));
            }
        }
        else if (xTaskCheckForTimeOut( &xPhyTime, &xPhyRemTime ) != pdFALSE)
        {
            xStatus = prvReadMDIO( COPPER_STATUS_REG,pxInterface );

            if ((ulPHYLinkStatus & niBMSR_LINK_STATUS) !=
                    (xStatus & niBMSR_LINK_STATUS))
            {
                /* PHY link status has been changed need to update XGMAC Configurations */
                if( SocfpgaXGMAC_UpdateXGMACSpeedMode(pXGMACHandle,&xPhyDev ) != 0 )
                {
                    FreeRTOS_printf((
                                "SOCFPGA_XGMAC: Updating Configurations failed....\n"));
                }
                xStatus = prvReadMDIO( COPPER_STATUS_REG, pxInterface );
                if ((ulPHYLinkStatus & niBMSR_LINK_STATUS) !=
                        (xStatus & niBMSR_LINK_STATUS))
                {
                    ulPHYLinkStatus = xStatus;
                    FreeRTOS_printf(("prvEMACHandlerTask: PHY LS now %d\n",
                            (ulPHYLinkStatus & niBMSR_LINK_STATUS) != 0uL));
                }
            }

            vTaskSetTimeOutState( &xPhyTime );

            if ((ulPHYLinkStatus & niBMSR_LINK_STATUS) != 0uL)
            {
                xPhyRemTime = pdMS_TO_TICKS( PHY_LS_HIGH_CHECK_TIME_MS );
            }
            else
            {
                xPhyRemTime = pdMS_TO_TICKS( PHY_LS_LOW_CHECK_TIME_MS );
            }
        }
    }
}


BaseType_t GetPhyLinkStatus()
{
    BaseType_t xReturn;

    if ((ulPHYLinkStatus & niBMSR_LINK_STATUS) == 0uL)
    {
        xReturn = pdFALSE;
    }
    else
    {
        xReturn = pdTRUE;
    }

    return xReturn;

}

static inline unsigned long prvReadMDIO(unsigned ulReg,
        NetworkInterface_t*pxInterface )
{
    int instance = (int)((uintptr_t)pxInterface->pvArgument); 
    SocfpgaXGMACHandle_t pXGMACHandle = (SocfpgaXGMACHandle_t)xEmacConfig[instance].pXGMACHandle;

    return read_phy_reg( pXGMACHandle->pXGMACInstBaseAddr, xPhyDev.phy_address,
            ulReg);
}



__attribute__((aligned( 64 )))

static uint8_t ucNetworkPackets[ ipconfigNUM_NETWORK_BUFFER_DESCRIPTORS *
        XGMAC_MAX_PACKET_SIZE];

void vNetworkInterfaceAllocateRAMToBuffers(
        NetworkBufferDescriptor_t pxNetworkBuffers[ ipconfigNUM_NETWORK_BUFFER_DESCRIPTORS ] )

{
    uint8_t *ucRAMBuffer = ucNetworkPackets;
    uint32_t ul;

    for (ul = 0; ul < ipconfigNUM_NETWORK_BUFFER_DESCRIPTORS; ul++)
    {
        pxNetworkBuffers[ ul ].pucEthernetBuffer = ucRAMBuffer +
                ipBUFFER_PADDING;

        *(( unsigned * ) ucRAMBuffer) =
                ( unsigned )(uintptr_t) (&(pxNetworkBuffers[ ul ]));

        ucRAMBuffer += XGMAC_MAX_PACKET_SIZE;
    }
}

static void prvEMACIRQHanlderCallback(XGMACInterruptStatus_t xIntrStatus,
        void *pvIrqData )
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    uint32_t ulISREvent = 0U;
    XGMACInterruptData_t *pxIntData = (XGMACInterruptData_t *)pvIrqData;

    if (xEMACTaskHandle != NULL)
    {
        if (xIntrStatus == eXGMACReceiveEvent)
        {
            /* Notify prvEMACHandlerTask of Receive event */
            ulISREvent = XGMAC_IF_RX_EVENT;
        }
        else if (xIntrStatus == eXGMACTransmitDoneEvent)
        {
            /* Notify prvEMACHandlerTask of Transmit completion event */
            ulISREvent = XGMAC_IF_TX_EVENT;
            xSemaphoreGiveFromISR(xSemaphoreCounterTx,NULL);
        }
        else if (xIntrStatus == eXGMACErrorEvent)
        {
            /* Notify prvEMACHandlerTask of Error event with Error Type and Error Data */
            ulISREvent |= pxIntData->ucErrType << 24;
            ulISREvent |= pxIntData->ucErrDMAChnlNum << 16;
            ulISREvent |= XGMAC_IF_ERR_EVENT;
        }

        xTaskNotifyFromISR( xEMACTaskHandle, ulISREvent, eSetBits,
                &(xHigherPriorityTaskWoken));
        portYIELD_FROM_ISR( xHigherPriorityTaskWoken );
    }
}


static void prvHandleErrorEvents(uint8_t ucErrStatus, uint8_t ucErrChnlNum,
        NetworkInterface_t*pxInterface)
{
    BaseType_t ucRemainInErrState = pdFALSE;
    int instance = (int)((uintptr_t)pxInterface->pvArgument); 
    SocfpgaXGMACHandle_t pXGMACHandle = (SocfpgaXGMACHandle_t)xEmacConfig[instance].pXGMACHandle;

    switch (ucErrStatus)
    {
    case eErrFatalBus:
    {
        FreeRTOS_printf(("Fatal Bus Error on DMA Channel %d\n", ucErrChnlNum));
        break;
    }
    case eErrTransmitStopped:
    {
        FreeRTOS_printf((
                    "Transmit Stopped on DMA Channel %d Re-init NetworkInterface\n", \
                    ucErrChnlNum));

        /* Stop DMA and EMAC */
        prvNetworkInterfaceDown(pxInterface);

        /* When Transmit Stopped Re-initialize Network Interface */
        xNetworkInterfaceInitialise(AgxInterface);
        break;
    }

    case eErrtReceiveStopped:
    {
        FreeRTOS_printf((
                    "Receive Stopped on DMA Channel %d Re-init NetworkInterface\n", \
                    ucErrChnlNum));

        /* Stop DMA and EMAC */
        prvNetworkInterfaceDown(pxInterface);

        /* When Receive Stopped Re-initialize Network Interface */
        xNetworkInterfaceInitialise(AgxInterface);
        break;
    }

    case eErrTxBufferUnavailable:
    {
        FreeRTOS_printf(("Transmit Buffer Unavailable Error on DMA Channel %d\n",
                ucErrChnlNum));
        break;
    }

    case eErrRxBufferUnavailable:
    {
        SocfpgaXGMAC_DMA_FlushBuffers(pXGMACHandle);
        FreeRTOS_printf( ( "Receive Buffer Unavailable Error on DMA Channel %d\n", ucErrChnlNum) );
        break;
    }
    case eErrContextDesc:
    {
        FreeRTOS_printf(("Context Descriptor Error on DMA Channel %d\n",
                ucErrChnlNum));
        break;
    }
    case eErrDescDefinition:
    {
        FreeRTOS_printf(("Descriptor Definition Error on DMA Channel %d\n",
                ucErrChnlNum));
        break;
    }
    case eErrUnhandled:
    {
        FreeRTOS_printf(("Unhandled Error on DMA Channel %d\n", ucErrChnlNum));
        break;
    }
    default:
        ucRemainInErrState  = pdTRUE;
        break;

    }

    configASSERT( ucRemainInErrState == pdFALSE );
}

BaseType_t prvUpdateRxDMADescriptors( uint8_t  *pucRxBuffer,
        NetworkInterface_t*pxInterface )
{
    uint8_t *pucBufAddr = NULL;
    int32_t xStatus;
    (void)(pucRxBuffer);
    int instance = (int)((uintptr_t)pxInterface->pvArgument); 
    SocfpgaXGMACHandle_t pXGMACHandle = (SocfpgaXGMACHandle_t)xEmacConfig[instance].pXGMACHandle;

    for (uint16_t usIndex = 0; usIndex < XGMAC_NOF_DESCRIPTORS_RX; usIndex++)
    {
#if (ipconfigZERO_COPY_RX_DRIVER != 0)
        TickType_t uxBlockTimeTicks = pdMS_TO_TICKS( 100UL );
        NetworkBufferDescriptor_t *pxBufferDescriptor;

        pxBufferDescriptor = pxGetNetworkBufferWithDescriptor(
                XGMAC_MAX_PACKET_SIZE, uxBlockTimeTicks );

        if (pxBufferDescriptor != NULL)
        {
            pucBufAddr = pxBufferDescriptor->pucEthernetBuffer;
        }
        else
        {
            return pdFAIL;
        }

#else
        /* Get address of Rx Buffer to assign to Descriptor */
        pucBufAddr = pucGetRXBuffer((RxBufferPool_t *)pucRxBuffer,
                XGMAC_MAX_PACKET_SIZE);
        if (pucBufAddr == NULL)
        {
            return pdFAIL;
        }
#endif
        xStatus = SocfpgaXGMAC_ReFill_RxDescriptor(pXGMACHandle, pucBufAddr);
        if( xStatus != 0 )
        {
            return pdFAIL;
        }
    }
    return pdPASS;
}

BaseType_t prvNetworkInterfaceOutDone( NetworkInterface_t*pxInterface )
{
    uint8_t *pucReleaseBuffer = NULL;
    int32_t xStatus;
    int instance = (int)((uintptr_t)pxInterface->pvArgument);
    SocfpgaXGMACHandle_t pXGMACHandle = (SocfpgaXGMACHandle_t)xEmacConfig[instance].pXGMACHandle;

    while( xSemaphoreTake(xSemaphoreCounterTx, 0) == pdTRUE )
    {
        xStatus = SocfpgaXGMAC_DMA_Transmit_Done(pXGMACHandle, &pucReleaseBuffer);
        if(xStatus != 0)
        {
            break;
        }

        if (pucReleaseBuffer != NULL)
        {
#if (ipconfigZERO_COPY_TX_DRIVER != 0)
            {
                NetworkBufferDescriptor_t *pxBuffer;
                void *pvBuffer;

                pvBuffer = (void *)pucReleaseBuffer;
                if (pvBuffer != NULL)
                {
                    pxBuffer = pxPacketBuffer_to_NetworkBuffer( pvBuffer );

                    if (pxBuffer != NULL)
                    {
                        vReleaseNetworkBufferAndDescriptor( pxBuffer );
                    }
                    else
                    {
                        FreeRTOS_printf((
                                    "Tx Done Get Buff: Can not find network buffer\n"));
                    }
                }
            }
#else
            BaseType_t xReturn;
            xReturn = pucReleaseTXBuffer(pTxBufferPool, pucReleaseBuffer );
            if (xReturn != pdPASS)
            {
                FreeRTOS_printf((
                            "Tx Done Get Buff: Can not release pool buffer  \n"));
            }
#endif /* ipconfigZERO_COPY_TX_DRIVER */
        }
    };

    return pdPASS;
}

BaseType_t prvCreateTxBufferPool( TxBufferPool_t *pTxBufferPool)
{
    uint8_t *pTxBuf;
    BaseType_t xReturn = pdPASS;

    /* Allocate Transmit Buffer Pool */
    pTxBuf = (uint8_t *)pvPortMalloc(TX_BUFFER_COUNT * TX_BUFFER_SIZE);

    /* Return failure if unable to allocate buffer */
    if (pTxBuf == NULL)
    {
        xReturn = pdFAIL;
        return xReturn;
    }

    /* Assign Tx Buffer Addresses to Tx Pool Structure  */
    for (int i = 0; i < TX_BUFFER_COUNT; i++)
    {
        pTxBufferPool->pTxBuffer[i] = (uint8_t *)&(pTxBuf[i * TX_BUFFER_SIZE]);
    }

    /* Assign Head and Tail to start of the location */
    pTxBufferPool->usHeadIndex = 0;
    pTxBufferPool->usTailIndex = 0;

    /* Initial available buffer count */
    pTxBufferPool->usBufUsedCnt = 0;

    pTxBufferPool->ucIsInitiazed = 1;

    xTxBufSynchSem = xSemaphoreCreateMutex();
    if (xTxBufSynchSem == NULL)
    {
        xReturn = pdFAIL;
    }

    return xReturn;
}

BaseType_t prvCreateRxBufferPool( RxBufferPool_t *pRxBufferPool)
{
    uint8_t *pRxBuf;
    BaseType_t xReturn = pdPASS;

    /* Allocate Receive Buffer Pool */
    pRxBuf = (uint8_t *)pvPortMalloc(RX_BUFFER_COUNT * RX_BUFFER_SIZE);

    /* Return failure if unable to allocate buffer */
    if (pRxBuf == NULL)
    {
        xReturn = pdFAIL;
        return xReturn;
    }

    /* Assign Rx Buffer Addresses to Rx Pool Structure  */
    for (int i = 0; i < RX_BUFFER_COUNT; i++)
    {
        pRxBufferPool->pRxBuffer[i] = (uint8_t *)&(pRxBuf[i * RX_BUFFER_SIZE]);
    }

    /* Assign Head and Tail to start of the location */
    pRxBufferPool->usHeadIndex = 0;
    pRxBufferPool->usTailIndex = 0;

    /* Initial available buffer count */
    pRxBufferPool->usBufUsedCnt = 0;

    pRxBufferPool->ucIsInitiazed = 1;

    xRxBufSynchSem = xSemaphoreCreateMutex();
    if (xRxBufSynchSem == NULL)
    {
        xReturn = pdFAIL;
    }

    return xReturn;
}

BaseType_t prvReinitTxBufferPool( TxBufferPool_t *pTxBufferPool)
{

    /* Reset Head, Tail and Count */
    pTxBufferPool->usHeadIndex = 0;
    pTxBufferPool->usTailIndex = 0;
    pTxBufferPool->usBufUsedCnt = 0;

    return pdPASS;
}

BaseType_t prvReinitRxBufferPool( RxBufferPool_t *pRxBufferPool)
{

    /* Reset Head, Tail and Count */
    pRxBufferPool->usHeadIndex = 0;
    pRxBufferPool->usTailIndex = 0;
    pRxBufferPool->usBufUsedCnt = 0;

    return pdPASS;
}

uint8_t *pucGetTXBuffer( TxBufferPool_t *pTxBufferPool, size_t xWantedSize)
{
    TickType_t xBlockTimeTicks = pdMS_TO_TICKS( 1000U );
    uint8_t *pucBufAddr = NULL;
    uint16_t usIndex;

    if (xWantedSize > TX_BUFFER_SIZE)
    {
        FreeRTOS_printf(("Tx Buffer Size more than the Poll Buffer size  \n"));
        return NULL;
    }

    if (xSemaphoreTake(xTxBufSynchSem, xBlockTimeTicks ) != pdFAIL)
    {
        if (pTxBufferPool->usBufUsedCnt == TX_BUFFER_COUNT)
        {
            FreeRTOS_printf(("Tx Buffer Pool Fully Used \n"));
            return NULL;
        }

        /*Get the buffer from the head of the buffer pool */
        usIndex = pTxBufferPool->usHeadIndex++;
        pucBufAddr = pTxBufferPool->pTxBuffer[usIndex];

        /* Update Head and Buffer Count in use */
        pTxBufferPool->usHeadIndex %= TX_BUFFER_COUNT;
        pTxBufferPool->usBufUsedCnt++;

        /* Release the Mutex. */
        xSemaphoreGive( xTxBufSynchSem );
    }

    return pucBufAddr;
}

uint8_t *pucGetRXBuffer( RxBufferPool_t *pRxBufferPool, size_t xWantedSize )
{
    TickType_t xBlockTimeTicks = pdMS_TO_TICKS( 1000U );
    uint8_t *pucBufAddr = NULL;
    uint16_t usIndex;

    if (xWantedSize > RX_BUFFER_SIZE)
    {
        FreeRTOS_printf(("Rx Buffer Size more than the Pool Buffer size  \n"));
        return NULL;
    }

    if (xSemaphoreTake(xRxBufSynchSem, xBlockTimeTicks ) != pdFAIL)
    {
        if (pRxBufferPool->usBufUsedCnt == RX_BUFFER_COUNT)
        {
            FreeRTOS_printf(("Rx Buffer Pool Fully Used \n"));
            return NULL;
        }

        /*Get the buffer from the head of the buffer pool */
        usIndex = pRxBufferPool->usHeadIndex++;
        pucBufAddr = pRxBufferPool->pRxBuffer[usIndex];

        /* Update Head and Buffer Count in use */
        pRxBufferPool->usHeadIndex %=  RX_BUFFER_COUNT;
        pRxBufferPool->usBufUsedCnt++;

        /* Release the Mutex. */
        xSemaphoreGive( xRxBufSynchSem );
    }

    return pucBufAddr;
}

BaseType_t pucReleaseTXBuffer( TxBufferPool_t *pTxBufferPool, void *pvBuffer )
{
    TickType_t xBlockTimeTicks = pdMS_TO_TICKS( 1000U );
    BaseType_t xReturn = pdFAIL;
    uint16_t usIndex;

    if (xSemaphoreTake(xTxBufSynchSem, xBlockTimeTicks ) != pdFAIL)
    {
        if (pTxBufferPool->usBufUsedCnt == 0)
        {
            FreeRTOS_printf(("Tx Buffer Pool is Already Empty \n"));
            return xReturn;
        }

        /* Add the buffer to the tail of the pool */
        memset(pvBuffer, '\0', TX_BUFFER_SIZE);
        usIndex = pTxBufferPool->usTailIndex++;
        pTxBufferPool->pTxBuffer[usIndex] = pvBuffer;

        /* Update tail and free count */
        pTxBufferPool->usTailIndex %= TX_BUFFER_COUNT;
        pTxBufferPool->usBufUsedCnt--;

        /* Release the Mutex. */
        xSemaphoreGive( xTxBufSynchSem );

        xReturn = pdPASS;
    }

    return xReturn;
}

BaseType_t pucReleaseRXBuffer( RxBufferPool_t *pRxBufferPool, void *pvBuffer )
{
    TickType_t xBlockTimeTicks = pdMS_TO_TICKS( 1000U );
    BaseType_t xReturn = pdFAIL;
    uint16_t usIndex;

    if (pRxBufferPool->usBufUsedCnt == 0)
    {
        FreeRTOS_printf(("Rx Buffer Pool is Already Empty \n"));
        return xReturn;
    }

    if (xSemaphoreTake(xRxBufSynchSem, xBlockTimeTicks ) != pdFAIL)
    {
        /* Add the buffer to the tail of the pool */
        memset(pvBuffer, '\0', RX_BUFFER_SIZE);
        usIndex = pRxBufferPool->usTailIndex++;
        pRxBufferPool->pRxBuffer[usIndex] = pvBuffer;

        /* Update tail and used count */
        pRxBufferPool->usTailIndex %= RX_BUFFER_COUNT;
        pRxBufferPool->usBufUsedCnt--;

        /* Release the Mutex. */
        xSemaphoreGive( xRxBufSynchSem );

        xReturn = pdPASS;
    }

    return xReturn;
}

NetworkInterface_t *pxFillInterfaceDescriptor( BaseType_t xEMACIndex,
        NetworkInterface_t *pxInterface )
{
    xEmacConfig[xEMACIndex].lEMACIndex = xEMACIndex;
    xEmacConfig[xEMACIndex].ucPhyType = SOCFPGA_XGMAC_PHY_TYPE_RGMII;


    memset( pxInterface,0, sizeof(NetworkInterface_t));
    pxInterface->pcName = "socfpga";
    pxInterface->pvArgument = ( void * ) xEMACIndex;
    pxInterface->pfInitialise =
            (NetworkInterfaceInitialiseFunction_t)xNetworkInterfaceInitialise;
    pxInterface->pfOutput =
            (NetworkInterfaceOutputFunction_t)xNetworkInterfaceOutput;
    pxInterface->pfGetPhyLinkStatus =
            (GetPhyLinkStatusFunction_t)GetPhyLinkStatus;

    FreeRTOS_AddNetworkInterface( pxInterface );

    return pxInterface;
}
