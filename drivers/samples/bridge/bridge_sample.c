/*
 * SPDX-FileCopyrightText: Copyright (C) 2025 Altera Corporation
 *
 * SPDX-License-Identifier: MIT-0
 *
 * Sample application for bridge driver
 */

/**
 * @file bridge_sample.c
 * @brief Sample Application for hps-fpga bridges.
 */

/**
 * @defgroup bridge_sample HPS-FPGA Bridge Sample Applications
 * @ingroup samples
 *
 * Sample Application for HPS-FPGA bridges
 *
 * @details
 * @subsubsection description Description
 * This is a sample application to demonstrate the hps-fpga bridge driver. In the sample application, buffers are transferred accross the corresponding bridges, and compared with the original buffer to ensure t  * he movement of data happened as expeceted. The sample consits of four bridges which are :
 *      - @b hps2fpga @b bridge <br>
 *           The HPS2FPGA bridges extends the HPS peripherals to the FPGA. Any additional IPs implemented in the FPGA fabric can be used from the HPS subsystem.
 *
 *      - @b lwhps2fpga @b bridge <br>
 *           The LWHPS2FPGA bridge has almost the same funcitonality as the HPS2FPGA bridge. This is specifically meant for narrower use case invloving simple peripherals on the fpga with strongly ordered 
 *           single transactions.
 *
 *      - @b fpga2hps @b bridge <br>
 *           The FPGA2HPS bridge allows the initiators in the fpga fabric to read/write into the address map of the HPS.
 *
 *      - @b fpga2sdram @b bridge <br>
 *           The FPGA2SDRAM bridge allows the logic in the fpga facric to perform non-coherent access to sdram in the HPS.
 *
 * @subsubsection prerequisites Prerequisites
 * - All the four HPS-FPGA bridges require its own rbf files to be loaded before running the sample. Ensure that the sd card has been prepared with the required rbf files, and the name of the rbf file is <br> 
 * properly updated in this sample app. The sample application is tested using quartus 24.3.1. Ensure the ATF/SPL is built using the same quartus version.
 *
 * @subsubsection how_to_run How to Run
 * 1. Follow the common README for build and flashing instructions.
 * 2. Copy all the required rbf file to the SD card, and run the sample.
 *
 * @subsubsection expected_results Expected Results
 * - The SYSTEM ID of all the individual fpga bridge will be displayed in the console.If the bitstream has been loaded successfully, the ID will displayed.
 * - The buffer comparison status will be displayed.
 *
 * @note Due to a fatfs limitation, all the rbf file names should be limited to 8 characters.
 * @{
 */
/** @} */

#include "bridge_sample.h"
#include "socfpga_defines.h"
#include "socfpga_bridge.h"
#include "osal_log.h"
#include "socfpga_mmc.h"
#include "socfpga_fpga_manager.h"
#include "socfpga_smmu.h"

static int load_bitstream(const char *rbf)
{
    uint8_t *rbf_ptr;
    uint32_t file_size;

    rbf_ptr = mmc_read_rbf(SOURCE_SDMMC, rbf, &file_size);
    if (rbf_ptr == NULL)
    {
        ERROR("Unable to read bitstream from memory !!!");
        return 0;
    }

    if (load_fpga_bitstream(rbf_ptr, file_size) == -1)
    {
        ERROR("Failed to load bitstream !!!");
        return 0;
    }

    INFO("bitstream configuration successful");
    vPortFree(rbf_ptr);

    return 1;
}

void run_hps2fpga_sample(void)
{
    if (load_bitstream("/hps2fpga.rbf") == 0)
    {
        ERROR("Bitstream loading failed");
        return;
    }

    if (disable_hps2fpga_bridge() == -1)
    {
        ERROR("Failed to disable the HPS2FPGA bridge !!!");
        return;
    }

    if (enable_hps2fpga_bridge() == -1)
    {
        ERROR("Failed to enable the HPS2FPGA bridge !!!");
        return;
    }

    if (hps2fpga_bridge_sample() == 0)
    {
        ERROR("HPS2FPGA bridge sample failed");
    }
    else
    {
        INFO("FPGS2FPGA sample conpleted successfully");
    }
}

void run_lwhps2fpga_sample(void)
{
    if (load_bitstream("/lwhps2fpga.rbf") == 0)
    {
        ERROR("Bitstream loading failed");
        return;
    }

    if (disable_lwhps2fpga_bridge() == -1)
    {
        ERROR("Failed to disable the LWHPS2FPGA bridge !!!");
        return;
    }

    if (enable_lwhps2fpga_bridge() == -1)
    {
        ERROR("Failed to enable the LWHPS2FPGA bridge !!!");
        return;
    }

    if (lwhps2fpga_bridge_sample() == 0)
    {
        ERROR("LWHPS2FPGA bridge sample failed");
    }
    else
    {
        INFO("LWHPS2FPGA sample completed successfully");
    }

}

void run_f2dsram_sample(void)
{
    if (load_bitstream("/fpga2sdram.rbf") == 0)
    {
        ERROR("Bitstream loading failed");
        return;
    }

    if (disable_fpga2sdram_bridge() == -1)
    {
        ERROR("Failed to disable the FPGA2SDRAM bridge !!!");
        return;
    }

    if (enable_fpga2sdram_bridge() == -1)
    {
        ERROR("Failed to enable the FPGA2SDRAM bridge !!!");
        return;
    }

    if (f2sdram_bridge_sample() == 0)
    {
        ERROR("FPGA2SDRAM bridge sample failed");
    }
    else
    {
        INFO("FPGA2SDRAM sample conpleted successfully");
    }
}

void run_f2h_sample(void)
{
    if (load_bitstream("/fpga2hps.rbf") == 0)
    {
        ERROR("Bitstream loading failed");
        return;
    }

    if (disable_fpga2hps_bridge() == -1)
    {
        ERROR("Failed to disable the FPGA2HPS bridge !!!");
        return;
    }

    if (enable_fpga2hps_bridge() == -1)
    {
        ERROR("Failed to enable the FPGA2HPS bridge !!!");
        return;
    }

    if (fpga2hps_bridge_sample() == 0)
    {
        ERROR("FPGA2HPS bridge sample failed");
    }
    else
    {
        INFO("FPGA2HPS sample conpleted successfully");
    }
}

void fpga_bridge_task(void)
{

#if 0
    run_hps2fpga_sample();
#else
    run_f2h_sample();

    run_lwhps2fpga_sample();

    run_f2dsram_sample();
#endif
}
