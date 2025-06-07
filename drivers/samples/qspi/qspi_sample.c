/*
 * SPDX-FileCopyrightText: Copyright (C) 2025 Altera Corporation
 *
 * SPDX-License-Identifier: MIT-0
 *
 * Sample application for QSPI
 */

/**
 * @file spi_sample.c
 * @brief Sample Application for SPI
 */

#include "FreeRTOS.h"
#include "FreeRTOSConfig.h"
#include "task.h"
#include "semphr.h"
#include <stdio.h>
#include <string.h>
#include "socfpga_flash.h"
#include "socfpga_cache.h"
#include "osal_log.h"

/**
 * @defgroup qspi_sample QSPI Flash
 * @ingroup samples
 *
 * Sample Application for QSPI Flash
 *
 * @details
 * @subsubsection qspi_desc Description
 * This is a sample application to demonstrate the usage of QSPI driver for flash access.
 * It writes data to an flash  using QSPI and then reads it back.
 * The written data is compared with the read data to verify the operation.
 *
 * @subsubsection qspi_pre Prerequisites
 * - Test daughter card must be connected to the board.
 * - A flash with QSPI interface  be present  to the QSPI bus.
 *
 * @subsubsection qspi_param Configurable Parameters
 * - The memory address to write and read back can be configured in @c START_ADDRESS macro.
 * - The number of bytes to transfer can be configured in @c TEST_DATA_SIZE macro.
 * - This sample is tested with Micron MT25QU02GCBB Serial NOR Flash Memory.
 *
 * @subsubsection qspi_how_to How to Run
 * 1. Follow the common README instructions to build and flash the application.
 * 2. Run the application on the board.
 * 3. Observe the UART terminal for status messages.
 *
 * @subsubsection qspi_result Expected Results
 * - The application sends the Write Enable and Write commands to store data in Flash.
 * - It then reads the data back using the Read command.
 * - The read and written data are compared, and a success message is printed on match.
 */

#define TEST_DATA_SIZE     ((4 * 1024 * 1024))
#define START_ADDRESS      0x05000000
#define FLASH_OK           0

uint8_t wBuf[TEST_DATA_SIZE];

uint8_t rBuf[TEST_DATA_SIZE];

/*
 * @brief Initialize test data
 */
void init_test_data() {
    for (int i = 0; i < TEST_DATA_SIZE; i++)
    {
        wBuf[i] = i % 256;
    }
}

int qspi_task() {
    INFO("QSPI flash driver example.");
    INFO("Estimated Time for Completion: 50 Seconds.");
    int status;
    FlashHandle_t flash_handle;

    /*Populate the write data*/
    init_test_data();
    cache_force_write_back((void*) wBuf, sizeof(wBuf));

    /*Initialise the flash driver*/
    flash_handle = flash_open(QSPI_DEV0);

    INFO("Erase in progess...");
    status = flash_erase_sectors(flash_handle, START_ADDRESS, TEST_DATA_SIZE);
    if (status < FLASH_OK)
    {
        ERROR("Erase failed with error code %d", status);
        return -1;
    }
    INFO("Erased %d sectors from start address : 0x%x", status,
            START_ADDRESS);

    INFO("Done.");

    INFO("Write in progress...");
    status =
            flash_write_sync(flash_handle, START_ADDRESS, wBuf, TEST_DATA_SIZE);
    if (status != FLASH_OK)
    {
        ERROR("Write failed with error code %d\r\n", status);
        return -1;
    }
    INFO("Done.\r\n");

    for (int i = 0; i < TEST_DATA_SIZE; i++)
    {
        rBuf[i] = 0x00;
    }
    cache_force_write_back((void*) rBuf, sizeof(rBuf));

    INFO("Read in progress...");
    status = flash_read_sync(flash_handle, START_ADDRESS, rBuf, TEST_DATA_SIZE);
    if (status != FLASH_OK)
    {
        ERROR("Read failed with error code %d", status);
        return -1;
    }
    INFO("Done.");
    cache_force_invalidate((void*) rBuf, sizeof(rBuf));

    INFO("Validating read values...");
    if (memcmp(rBuf, wBuf, TEST_DATA_SIZE) == 0) {
        INFO("Done.");
    }
    else
    {
        ERROR("Data Mismatch.");
    }
    flash_close(flash_handle);

    INFO("QSPI Flash driver example completed");

    return 0;
}
