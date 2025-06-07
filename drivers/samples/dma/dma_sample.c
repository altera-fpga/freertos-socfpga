/*
 * SPDX-FileCopyrightText: Copyright (C) 2025 Altera Corporation
 *
 * SPDX-License-Identifier: MIT-0
 *
 * Sample application for SoC FPGA DMA
 */


#include <stdlib.h>
#include <string.h>
#include "osal.h"
#include "osal_log.h"
#include <task.h>
#include "socfpga_dma.h"
#include "dma_sample.h"
#include "socfpga_cache.h"

/**
 * @defgroup dma_sample DMA
 * @ingroup samples
 * 
 * Sample Application for DMA
 * 
 * @details
 * @subsubsection dma_desc Description
 * This sample application demonstrates the use of the DMA driver to perform
 * a memory-to-memory data transfer. It transfers a predefined block of data
 * from a source buffer to a destination buffer and verifies that the transfer
 * completed successfully by comparing the contents.
 * 
 * @subsubsection dma_param Configurable Parameters
 * - Transfer size can be configured by changing the value of @c TRANSFER_SIZE macro.
 * - Linked list count can be configured by changing the value of @c LLI_COUNT macro.
 * - Instance and channel can be configured by changing the values in respective APIs.
 * 
 * @subsubsection dma_how_to How to Run
 * 1. Follow the common README instructions to build and flash the application.
 * 2. Run the application on the board.
 * 3. Output can be observed in the UART terminal.
 * 
 * @subsubsection dma_result Expected Results
 * - The DMA controller initiates and completes a memory-to-memory transfer.
 * - The destination buffer contents match the source buffer.
 * - A message confirming successful transfer and verification is printed to the terminal.
 */

/* Test configurations */

#define TRANSFER_SIZE    1024
#define LLI_COUNT        2


static uint16_t tx_buf[ LLI_COUNT ][ TRANSFER_SIZE ];
static uint16_t rx_buf[ LLI_COUNT ][ TRANSFER_SIZE ];

osal_semaphore_def_t xDoneSemMem;
osal_semaphore_t xDoneSem;

/*
 * @brief DMA done callback function
 */

void vDmaDoneCallback( uint32_t *arg )
{
    (void) arg;
    osal_semaphore_post(xDoneSem);
}

void vDmaTask()
{

    xDoneSem = osal_semaphore_create(&xDoneSemMem);
    DmaConfig_t dma1channel1config;
    DmaBlkTrnsfrCfg_t BlkTrnsfrList[ LLI_COUNT ] = { 0 };
    BaseType_t xRetVal;
    xDmaHandle_t pxDmaHandle;

    INFO("DMA data transfer sample application");

    INFO("Preparing the data buffers ...");
    for (int j = 0; j < LLI_COUNT; j++)
    {
        for (int i = 0; i < TRANSFER_SIZE; i++)
        {
            tx_buf[ j ][ i ] = (uint16_t) i;
            rx_buf[ j ][ i ] = 0x0;
        }
    }
    cache_force_write_back((void*) tx_buf, (LLI_COUNT * TRANSFER_SIZE * 2));
    cache_force_write_back((void*) rx_buf, (LLI_COUNT * TRANSFER_SIZE * 2));

    INFO("Done.");

    INFO("Preparing the Linked List Items ...");

    for (int i = 0; i < LLI_COUNT; i++)
    {
        BlkTrnsfrList[ i ].ulSrcAddr = (uint64_t) (tx_buf[ i ]);
        BlkTrnsfrList[ i ].ulDestAddr = (uint64_t) (rx_buf[ i ]);
        BlkTrnsfrList[ i ].ulBlockSize = TRANSFER_SIZE * 2;
        /* Update next LLI address if current LLI is not last in list*/
        if (i < (LLI_COUNT - 1))
        {
            BlkTrnsfrList[ i ].xNextTrnsfrCfg = &BlkTrnsfrList[ i + 1 ];
        }
    }

    INFO("Done.");

    INFO("Configuring and setting up the DMA channel ...");
    pxDmaHandle = dma_open(DMA_INSTANCE_0, DMA_CHANNEL_1);
    if (pxDmaHandle == NULL)
    {
        ERROR("Opening DMA channel failed");
        return;
    }

    dma1channel1config.uDmaChannelDirection = eDmaMemToMemDmac;
    dma1channel1config.uDmaChannelPriority = 0;
    dma1channel1config.ulDmaLinkedListCount = LLI_COUNT;
    dma1channel1config.pxDmaCallback = vDmaDoneCallback;

    xRetVal = dma_config(pxDmaHandle, &dma1channel1config);
    if (xRetVal != 0)
    {
        INFO("Configuring DMA channel failed");
        dma_close(pxDmaHandle);
        return;
    }
    xRetVal = dma_setup_transfer(pxDmaHandle, &BlkTrnsfrList[ 0 ]);

    if (xRetVal != 0)
    {
        INFO("Setting up DMA channel failed");
        dma_close(pxDmaHandle);
        return;
    }

    INFO("Done.");

    INFO("Triggering the data transfer ...");
    dma_start_tranfer(pxDmaHandle);

    xRetVal = osal_semaphore_wait(xDoneSem, 1000);

    if (xRetVal == pdTRUE)
    {
        INFO("Data transfer done.");

        INFO("Verifying data ...");

        cache_force_invalidate((void*) rx_buf, sizeof(rx_buf));

        if (!memcmp((void*) tx_buf, (void*) rx_buf,
                (TRANSFER_SIZE * LLI_COUNT)))
        {
            INFO("Verification PASSED");
        }
        else
        {
            ERROR("Verification FAILED");
        }
    }
    else
    {
        ERROR("Failed to get DMA callback");
    }

    osal_semaphore_delete(xDoneSem);

    dma_close(pxDmaHandle);

    INFO("DMA data transfer sample completed.");

}

