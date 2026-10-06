/*
 * SPDX-FileCopyrightText: Copyright (C) 2025-2026 Altera Corporation
 *
 * SPDX-License-Identifier: MIT-0
 *
 * Header file for reboot manager HAL driver
 */


#ifndef __SOCFPGA_REBOOT_MNGR_H__
#define __SOCFPGA_REBOOT_MNGR_H__

/*
    File: Reboot Manager Driver

    This file contains all the Reboot Manager HAL API definitions

    APIs for SoC FPGA Reboot Manager driver.

    This is the Reboot Manager driver implementation for SoC FPGA. It provides
    APIs for initiating warm and cold reboots. For example usage, refer reboot
    manager sample application.
*/

#include <errno.h>

#define REBOOT_WARM    1U        /* Command to initiate warm reboot */
#define REBOOT_COLD    0U        /* Command to initiate cold reboot */

/*
    Group: Functions

    Entry points and callback signatures for the Reboot Manager driver.
*/
/*
    Callback: reboot_callback_t

    Callback function to be invoked before reboot

    Parameters:

        param - Pointer to user param that can be passed to the callback
*/
typedef void (*reboot_callback_t)(void *param);

/*
    Function: reboot_mngr_set_callback

    Set the callback function

    Sets the callback function that is invoked before initiating cold /warm
    reboot.

    Parameters:

        callback - Call back function pointer
        event - Callback for warm/cold reboot.

    Returns:

        - 0: on success
        - -EINVAL: if any argument is invalid
*/
int32_t reboot_mngr_set_callback(reboot_callback_t callback, uint32_t event);

/*
    Function: reboot_mngr_do_reboot

    Initiate a warm or cold reboot.

    Parameters:

        event - Callback for warm/cold reboot.

    Returns:

        - 0: on success
        - -ENOTSUP: if operation is not supported
        - -EINVAL:  if invalid argument is passed
*/
int32_t reboot_mngr_do_reboot(uint32_t event);

#endif
