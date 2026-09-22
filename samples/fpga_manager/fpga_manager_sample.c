/*
 * SPDX-FileCopyrightText: Copyright (C) 2025 Altera Corporation
 *
 * SPDX-License-Identifier: MIT-0
 *
 * Sample application for fpga manager
 */

/*
    File: FPGA Manager Sample

    Sample application for the FPGA Manager: 1 function, local to this file.
    Group: Description

    This is a sample application to demonstrate the use of the fpga manager to
    configure the fpga. The fpga manager loads the rbf bitstream data to the
    fpga fabric via the SDM. It also demonstrates the use of fpga partial
    reconfiguration after completing the fpga configuration.

    Group: Prerequisites

    The required rbf file should be available in the sdmmc before running the
    sample. By default, the name of the rbf file is core.rbf. If a different
    rbf file is used, modify the RBF_FILENAME macro with the new rbf name to
    be used.

    Group: How to Run

    - Follow the common README for build and flashing instructions.
    - Copy all the required rbf file to the SD card, and run the sample.

    Group: Expected Results

    The success/failure logs are displayed in the console.
*/

#include <stdio.h>
#include "socfpga_mmc.h"
#include "socfpga_fpga_manager.h"
#include "osal_log.h"

/* fpga bitstream file */
#define RBF_FILENAME    "/core.rbf"

void partial_reconfiguration_sample(void);

/*
    Function: fpga_manager_task
*/
void fpga_manager_task(void)
{
    uint8_t *rbf_ptr;
    uint32_t file_size = 0U;

    PRINT("Reading the rbf file from sdmmc");
    rbf_ptr = mmc_read_file(SOURCE_SDMMC, RBF_FILENAME, &file_size);
    if (rbf_ptr == NULL)
    {
        ERROR("Unable to read bitstream from memory !!!");
        return;
    }

    PRINT("Read the %s rbf file from sdmmc", RBF_FILENAME);

    PRINT("Starting fpga configuration");
    if (load_fpga_bitstream(rbf_ptr, file_size) != 0)
    {
        ERROR("Failed to load bitstream !!!");
        vPortFree(rbf_ptr);
        return;
    }

    vPortFree(rbf_ptr);

    PRINT("bitstream configuration successful");

    PRINT("Starting fpga partial reconfiguration");

    partial_reconfiguration_sample();
}
