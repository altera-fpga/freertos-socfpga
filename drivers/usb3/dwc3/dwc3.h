/*
 * SPDX-FileCopyrightText: Copyright (C) 2025 Altera Corporation
 *
 * SPDX-License-Identifier: MIT-0
 *
 * Header file for DWC3 driver
 */

#ifndef __SOCFPGA_DWC3_H__
#define __SOCFPGA_DWC3_H__

#include "socfpga_usb3_reg.h"
#include "socfpga_defines.h"

#define DWC3_LOG_LEVEL (0U)
#define DWC3_LOG(...) DWC_DEBUG_LOG(DWC3_LOG_LEVEL, __VA_ARGS__)
#define DWC_DEBUG_LOG(n, ...) \
    if( (n) > 1U ) \
    { printf(__VA_ARGS__); }

/*
 * @func  : dwc3_init
 * @brief : initialize the dwc3 controller
 *          initializes the usb2, usb3 phy and sets the usb mode to host
 * @return 
 *  1 - on success
 *  errno    - on failiure
 */
int dwc3_init(void);

#endif /* _SOCFPGA_DWC3_H_ */
