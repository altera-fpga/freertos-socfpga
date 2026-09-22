/*
 * SPDX-FileCopyrightText: Copyright (C) 2025-2026 Altera Corporation
 *
 * SPDX-License-Identifier: MIT-0
 *
 * Header file for FPGA manager driver
 */
#ifndef __SOCFPGA_FPGA_MANAGER_H__
#define __SOCFPGA_FPGA_MANAGER_H__

/*
    File: FPGA Manager Driver

    SoC FPGA fpga manager HAL driver

    APIs for the SoC FPGA FPGA manager driver

    FPGA manager driver supports both FPGA configuration and partial
    reconfiguration through HPS.

    FPGA configuration : Fpga configuration configures the FPGA fabric via the
    core bitstream via HPS. The HPS sends bitstream to FPGA via SDM.

    Partial Reconfiguration : PR allows the user to configure only a specific
    portion of the fpga (referred as PR region ) fabric. The process of PR is
    same as fpga configuration except that the PR region needs to be frozen
    before loading the bitstream, and unfreeze the PR region after the
    bitstream is configured. The freeze/unfreeze operation can be performed
    using the freeze IP driver. To see example usage, refer
    fpga_manager_sample
*/

#include <stdint.h>
#include <errno.h>


/*
    Group: Functions

    Entry points for the FPGA Manager driver.
*/

/*
    Function: load_fpga_bitstream

    Sends the bitstream data to the fpga and configures the fpga.

    Parameters:

        rbf_ptr - Reference to the bitstream data
        rbf_file_size - Length of bitstream file

    Returns:

        - 0:    if fpga bitstream configuration is success
        - -EIO: if fpga bitstream configuration fails
*/
int load_fpga_bitstream(uint8_t *rbf_ptr, uint32_t rbf_file_size);
/* end of group fpga_manager_fns */
/* end of group fpga_manager */

#endif  /* __SOCFPGA_FPGA_MANAGER_H__ */
