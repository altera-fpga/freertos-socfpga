/*
 * SPDX-FileCopyrightText: Copyright (C) 2025-2026 Altera Corporation
 *
 * SPDX-License-Identifier: MIT-0
 *
 * Header file for DWC3 driver
 */

#ifndef __SOCFPGA_DWC3_H__
#define __SOCFPGA_DWC3_H__

#include <errno.h>
#include "socfpga_usb3_reg.h"
#include "socfpga_defines.h"

/*
    File: DWC3 Driver

    Declares the DWC3 driver interface: 1 function.
*/

/*
    Function: dwc3_init

    Initialize the DWC3 controller.

    Initializes the USB2/USB3 PHY and sets the USB mode to host.

    Returns:

        0 - on success
        errno - on failure
*/
int dwc3_init(void);

#endif /* _SOCFPGA_DWC3_H_ */
