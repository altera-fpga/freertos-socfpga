/*
 * SPDX-FileCopyrightText: Copyright (C) 2025 Altera Corporation
 *
 * SPDX-License-Identifier: MIT-0
 *
 * Sample application for SoC FPGA reboot manager
 */

/*
    File: Reboot Manager Sample

    Sample Application for Reboot Manager
*/

#include <stdlib.h>
#include <string.h>
#include "osal.h"
#include "osal_log.h"
#include <task.h>
#include "socfpga_reboot_mngr.h"
#include "socfpga_defines.h"

/*
    Group: Reboot Manager

    Sample Application for Reboot Manager

    Description:

    This sample application demonstrates the use of the reboot manager driver
    to perform cold or warm reboot. It sets a user-defined callback function
    that is invoked before the reboot occurs.

    Configurable Parameters:

    The reboot type can be configured using the DO_COLD_BOOT macro. If set to
    1, a cold reboot is performed; otherwise, a warm reboot.

    How to Run:

    1. Follow the common README instructions to build and flash the application.
    2. Run the application on the board.
    3. Output can be observed in the UART terminal.

    Expected Results:

    - The application will print a message indicating the type of reboot
    being performed.
    - The system will reboot, either performing a cold or warm reboot based
    on the configuration.
    - The user-defined callback function will be invoked before the reboot.

    Notes:

        The sample application will reboot the system.
*/

/*
    Constant: DO_COLD_BOOT

    Set the value 1 for cold reboot and 0 for warm reboot
*/
#define DO_COLD_BOOT    1

/*
    Function: warm_reboot_callback

    Callback function for handing off before warm boot
*/
void warm_reboot_callback(void *buff)
{
    (void)buff;
    PRINT("Warm reboot");
}

/*
    Function: cold_reboot_callback

    Callback function for handing off before cold boot
*/

void cold_reboot_callback(void *buff)
{
    (void)buff;
    PRINT("Cold reboot");
}

/*
    Function: reboot_task

    Calls <reboot_mngr_set_callback> and <reboot_mngr_do_reboot>.
*/
void reboot_task(void)
{
    int status;
#if (DO_COLD_BOOT == 1)
    PRINT("Cold reboot example");
    PRINT("Set user callback for cold boot");
    reboot_mngr_set_callback(cold_reboot_callback, REBOOT_COLD);
    PRINT("Triggering cold boot ...");
    status = reboot_mngr_do_reboot(REBOOT_COLD);
#else
    PRINT("Warm reboot example");
    PRINT("Set user callback for warm boot");
    reboot_mngr_set_callback(warm_reboot_callback, REBOOT_WARM);
    PRINT("Triggering warm boot ...");
    status = reboot_mngr_do_reboot(REBOOT_WARM);
#endif
    if (status != 0)
    {
        ERROR("Reboot request failed with error code  %d", status);
    }
}
