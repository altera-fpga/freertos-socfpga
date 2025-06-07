/*
 * SPDX-FileCopyrightText: Copyright (C) 2025 Altera Corporation
 *
 * SPDX-License-Identifier: MIT-0
 *
 * Sample application for SoC FPGA sdmmc
 */

/**
 * @file sdmmc_sample.c
 * @brief Sample Application for sdmmc operations.
 */

/**
 * @defgroup sdmmc_rw_sample SD/MMC Read-Write Sample
 * @ingroup samples
 *
 * Sample Application for SD/MMC Block Read/Write Validation
 *
 * @details
 * @subsubsection description Description
 * This sample demonstrates the use of the SD/MMC driver to perform block-level
 * read and write operations. It writes a pattern of data to a specified logical
 * block address (LBA), reads it back from the same address, and verifies the
 * correctness of the data.
 *
 * @subsubsection prerequisites Prerequisites
 * - Proper initialization of the SD/MMC interface.
 * - A formatted and mounted SD/MMC card.
 * - Ensure read/write permissions are available for the target address.
 *
 * @subsubsection how_to_run How to Run
 * 1. Follow the common README for build and flashing instructions.
 * 2. Ensure an SD or eMMC card is inserted and properly detected.
 * 3. Run the sample binary. It will:
 *    - Fill a buffer with known data (e.g., incrementing pattern or fixed value).
 *    - Write the buffer to a predefined block address.
 *    - Read back into a second buffer from the same address.
 *    - Compare both buffers for validation.
 *
 * @subsubsection expected_results Expected Results
 * - Data write to SD/MMC is successful.
 * - Data read back matches the originally written pattern.
 * - Confirms working SD/MMC driver and interface for block operations.
 *
 * @{
 */
/** @} */


#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "FreeRTOSConfig.h"
#include "FreeRTOS.h"
#include "semphr.h"
#include "osal.h"
#include "socfpga_sdmmc.h"
#include "socfpga_cache.h"
#include "sdmmc_sample.h"

#define BLK_SIZE    512
#define BLK_NUM     8192
#define BUFF_ADD    0x8e900000
#define BLK_CNT     1
#define SYMBOL      0x88

#define USE_SYNC    1

static int32_t xfer_resp;
static osal_semaphore_t xfer_lock;
static osal_semaphore_def_t osal_def_xfer;

static uint8_t writeBuffer[ BLK_SIZE * BLK_CNT ];
static uint8_t readBuffer[ BLK_SIZE * BLK_CNT ];

#if !USE_SYNC
static void sdmmc_cb( int32_t state )
{
    xfer_resp = state;
    osal_semaphore_post(xfer_lock);
}

static void wait_xfer_done()
{
    osal_semaphore_wait(xfer_lock, OSAL_TIMEOUT_WAIT_FOREVER);
}
#endif


void sdmmc_task( void )
{
    int32_t status;
    uint64_t sectorCount;
    uint64_t writeAddress = BLK_SIZE * BLK_NUM;
    uint64_t readAddress = BLK_SIZE * BLK_NUM;
    uint32_t blockSize = BLK_SIZE;
    uint32_t numberOfBlocks  = BLK_CNT;

    xfer_lock = osal_semaphore_create(&osal_def_xfer);

    status = sdmmc_init_card(&sectorCount);
    if (status != 0)
    {
        printf("\rCard initialization failed with status: %u\n\r", status);
        return;
    }
    printf("\rCard initialized successfully.\n");
    printf("\rSector Count -> %lu.\n",sectorCount);

    memset(writeBuffer, SYMBOL, 512 * BLK_CNT);
    cache_force_write_back((uint64_t*) writeBuffer,blockSize * numberOfBlocks);

#if USE_SYNC
    xfer_resp = sdmmc_write_block_sync((uint64_t*) writeBuffer, writeAddress,
            blockSize,
            numberOfBlocks);
#else
    status = sdmmc_write_block_async((uint64_t*) writeBuffer, writeAddress,
            blockSize,
            numberOfBlocks,sdmmc_cb);
    if (status != 0)
    {
        printf("Card write failed with status: %d\n\r", status);
        return;
    }
    wait_xfer_done();
#endif

    if (xfer_resp != 0)
    {
        printf("Card write failed with status: %d\n\r", xfer_resp);
        return;
    }

    cache_force_write_back((uint64_t*) readBuffer,blockSize * numberOfBlocks);
    cache_force_invalidate((uint64_t*) readBuffer,blockSize * numberOfBlocks);
#if USE_SYNC
    xfer_resp = sdmmc_read_block_sync((uint64_t*) readBuffer, readAddress,
            blockSize,
            numberOfBlocks);
#else
    status = sdmmc_read_block_async((uint64_t*) readBuffer, readAddress,
            blockSize,
            numberOfBlocks,sdmmc_cb);

    if (status != 0)
    {
        printf("\rCard read failed with status: %d\n\r", status);
        return;
    }
    wait_xfer_done();
#endif

    if (xfer_resp != 0)
    {
        printf("\rCard read failed with status: %d\n\r", xfer_resp);
        return;
    }
    printf("\rData read from Card successfully.\n");
    cache_force_invalidate((uint64_t*) readBuffer,blockSize * numberOfBlocks);

    if ((memcmp(writeBuffer, readBuffer,blockSize * numberOfBlocks )) == 0)
    {
        printf("\rData verification successful.\n");
    }
    printf("\r SD/MMC driver write - read example completed \n");
}

