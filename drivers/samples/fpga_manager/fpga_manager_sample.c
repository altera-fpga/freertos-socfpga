/*
 * SPDX-FileCopyrightText: Copyright (C) 2025 Altera Corporation
 *
 * SPDX-License-Identifier: MIT-0
 *
 * Sample application for fpga manager
 */

 /**
  * @file fpga_manager_sample.c
  * @brief Sample Application for fpga manager.
  */

 /**
  * @defgroup fpga_manager_sample FPGA manager sample applications
  * @ingroup samples
  *
  * Sample Application for fpga manager.
  *
  * @details
  * @subsubsection description Description
  * This is a sample application to demonstrate the use of the fpga manager to configure the fpga. The fpga manager loads the rbf bitstream data to the fpga fabric via the SDM. It also demostrates the use of fpg	 * a partial reconfiguration.
  *
  * @subsubsection prerequisites Prerequisites
  * The required rbf file should be available in the sdmmc before running the sample. By default, the name of the rbf file is ghrd.core.rbf. If a different rbf file is used, modify the RBF_FILENAME macro <br> 
  * with the new rbf name in the sample application.
  * @subsubsection how_to_run How to Run
  * 1. Follow the common README for build and flashing instructions.
  * 2. Copy all the required rbf file to the SD card, and run the sample.
  *
  * @subsubsection expected_results Expected Results
  * The success/failure logs are displayed in the console.
  * @{
  */
  /** @} */

#include <stdio.h>
#include "socfpga_mmc.h"
#include "socfpga_fpga_manager.h"
#include "osal_log.h"

/* fpga bitstream file */
#define RBF_FILENAME "/ghrd.core.rbf"

void partial_reconfiguration_sample(void);

void fpga_manager_task(void)
{
     uint8_t *rbf_ptr;
     uint32_t file_size;

     INFO("Reading the rbf file from sdmmc");
     rbf_ptr = mmc_read_file(SOURCE_SDMMC, RBF_FILENAME, &file_size);
     if( rbf_ptr == NULL )
     {
         ERROR("Unable to read bitstream from memory !!!");
         return;
     }

     INFO("Read the %s rbf file from sdmmc", RBF_FILENAME);

     INFO("Starting fpga configuration");
     if( load_fpga_bitstream(rbf_ptr, file_size) == -1 )
     {
         ERROR("Failed to load bitstream !!!");
         vPortFree(rbf_ptr);
         return;
     }

     vPortFree(rbf_ptr);

     INFO("bitstream configuration successful");

     INFO("Starting fpga partial reconfiguration");

     partial_reconfiguration_sample();
}
