/*
 * SPDX-FileCopyrightText: Copyright (C) 2025 Altera Corporation
 *
 * SPDX-License-Identifier: MIT-0
 *
 * Sample application for bridge driver
 */

/*
    File: Bridge Sample Implementation

    Sample application for the HPS-FPGA bridges.
    Group: Description

    This is a sample application to demonstrate the hps-fpga bridge driver. In
    this sample application, only the hps2fpga and lwhps2fpga bridges are
    demonstrated. Remaining bridges can also be used in the same way. The
    sample can be divided into two sections as below:
         - hps2fpga bridge
    The HPS2FPGA bridges extends the HPS peripherals to the FPGA. Any
    additional IPs implemented in the FPGA fabric can be used from the HPS
    subsystem. In the sample application, basic memory validation test is
    performed. Data is written to the start and end address of the first 1G
    memory area of the HPS2FPGA bridge and is read back and compared with
    initial buffers.

         - lwhps2fpga bridge
    The LWHPS2FPGA bridge has almost the same functionality as the HPS2FPGA
    bridge. This is specifically meant for narrower use case involving simple
    peripherals on the fpga with strongly ordered single transactions. The
    sample for lwhps2fpga bridge has the following features:
              - Read the sysid
              - Toggle an LED connected through the lwhpsfpga bridge, where the LED is mapped to LED D16 on the PDK.

    Group: Prerequisites

    - All the four HPS-FPGA bridges require its own rbf files to be loaded before running the sample. Ensure that
    the sd card has been prepared with the required rbf files, and the name of
    the rbf file is properly updated in this sample app. The sample
    implementation is based on the GHRD design.

    Group: How to Run

    - Follow the common README for build and flashing instructions.
    - Copy all the required rbf file to the SD card, and run the sample.

    Group: Expected Results

    - The SYSTEM ID of all the individual fpga bridge will be displayed in the console.
    - If the bitstream has been loaded successfully, the ID will displayed.
    - The buffer comparison status will be displayed.
*/

#include "bridge_sample.h"
#include "socfpga_bridge.h"
#include "osal_log.h"
#include "socfpga_mmc.h"
#include "socfpga_fpga_manager.h"

#define FPGA_RBF_FILE    "/core.rbf"

/*
    Function: load_bitstream

    Calls <mmc_read_file> and <load_fpga_bitstream>.
*/
static int load_bitstream(const char *rbf)
{
    uint8_t *rbf_ptr;
    uint32_t file_size = 0U;

    rbf_ptr = mmc_read_file(SOURCE_SDMMC, rbf, &file_size);
    if (rbf_ptr == NULL)
    {
        ERROR("Unable to read bitstream from memory !!!");
        return 0;
    }

    if (load_fpga_bitstream(rbf_ptr, file_size) != 0)
    {
        ERROR("Failed to load bitstream !!!");
        return 0;
    }

    PRINT("bitstream configuration successful");
    vPortFree(rbf_ptr);

    return 1;
}

/*
    Function: run_hps2fpga_sample

    Calls <disable_hps2fpga_bridge>, <enable_hps2fpga_bridge> and
    hps2fpga_bridge_sample.
*/
static void run_hps2fpga_sample(void)
{
    if (disable_hps2fpga_bridge() != 0)
    {
        ERROR("Failed to disable the HPS2FPGA bridge !!!");
        return;
    }

    if (enable_hps2fpga_bridge() != 0)
    {
        ERROR("Failed to enable the HPS2FPGA bridge !!!");
        return;
    }

    if (hps2fpga_bridge_sample() != 0)
    {
        ERROR("HPS2FPGA bridge sample failed");
    }
    else
    {
        PRINT("HPS2FPGA sample completed successfully");
    }
}

/*
    Function: run_lwhps2fpga_sample

    Calls <disable_lwhps2fpga_bridge>, <enable_lwhps2fpga_bridge> and
    lwhps2fpga_bridge_sample.
*/
static void run_lwhps2fpga_sample(void)
{
    if (disable_lwhps2fpga_bridge() != 0)
    {
        ERROR("Failed to disable the LWHPS2FPGA bridge !!!");
        return;
    }

    if (enable_lwhps2fpga_bridge() != 0)
    {
        ERROR("Failed to enable the LWHPS2FPGA bridge !!!");
        return;
    }

    if (lwhps2fpga_bridge_sample() != 0)
    {
        ERROR("LWHPS2FPGA bridge sample failed");
    }
    else
    {
        PRINT("LWHPS2FPGA sample completed successfully");
    }
}

/*
    Function: fpga_bridge_task

    Calls <load_bitstream>, <run_hps2fpga_sample> and <run_lwhps2fpga_sample>.
*/
void fpga_bridge_task(void)
{

    PRINT("Starting the bridge sample application");

    /* Load the core.rbf to configure the fpga */
    if (load_bitstream(FPGA_RBF_FILE) == 0)
    {
        ERROR("Bitstream loading failed");
        return;
    }

    PRINT("Bitstream loaded successfully");

    PRINT("Starting h2f sample");

    run_hps2fpga_sample();

    PRINT("Completed h2f sample");

    PRINT("Starting lwh2f sample");

    run_lwhps2fpga_sample();

    PRINT("Completed lwh2f sample");

    PRINT("Bridge sample completed");
}
