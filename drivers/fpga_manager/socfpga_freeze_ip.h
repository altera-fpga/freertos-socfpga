/*
 * SPDX-FileCopyrightText: Copyright (C) 2025 Altera Corporation
 *
 * SPDX-License-Identifier: MIT-0
 *
 * Header file for freeze bridge HAL driver
 */

#ifndef __SOCFPGA_FREEZE_IP_H__
#define __SOCFPGA_FREEZE_IP_H__

#define BRIDGE_OP_SUCCESS (0)

/*
 * @brief freeze the pr region using the freeze ip before doing the partial
 *      reconfiguration
 * @return
 *  - 1, if the operation is successful,
 *  - ETIMEOUT, if the operation gets timed out
 */
int do_freeze_pr_region(void);

/*
 * @brief unfreeze the pr region using the freeze ip after doing the partial
 *      reconfiguration
 * @return
 *  - 1, if the operation is successful,
 *  - ETIMEOUT, if the operation gets timed out
 */
int do_unfreeze_pr_region(void);

#endif /* __SOCFPGA_FREEZE_IP_H__ */
