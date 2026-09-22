/*
 * SPDX-FileCopyrightText: Copyright (C) 2025-2026 Altera Corporation
 *
 * SPDX-License-Identifier: MIT-0
 *
 * Header file for freeze bridge HAL driver
 */

#ifndef __SOCFPGA_FREEZE_IP_H__
#define __SOCFPGA_FREEZE_IP_H__

/*
    File: FPGA Manager Freeze IP Driver

    SoC FPGA freeze IP HAL driver
*/

/*
    Group: fpga_manager_fns

    Entry points for the FPGA Manager Freeze IP driver.
*/

/*
    Function: do_freeze_pr_region

    freeze the pr region using the freeze ip before initiating the partial
    reconfiguration

    Returns:

        - 0:         if the freeze operation is successful
        - -ETIMEDOUT: if the freeze operation gets timed out
*/
int do_freeze_pr_region(void);

/*
    Function: do_unfreeze_pr_region

    unfreeze the pr region using the freeze ip after completing the partial
    reconfiguration

    Returns:

        - 0:         if the unfreeze operation is successful,
        - -ETIMEDOUT: if the unfreeze operation gets timed out
*/
int do_unfreeze_pr_region(void);
/* end of group fpga_manager_fns */
/* end of group fpga_manager */

#endif /* __SOCFPGA_FREEZE_IP_H__ */
