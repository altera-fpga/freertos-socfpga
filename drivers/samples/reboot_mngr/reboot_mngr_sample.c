/*
 * SPDX-FileCopyrightText: Copyright (C) 2025 Altera Corporation
 *
 * SPDX-License-Identifier: MIT-0
 *
 * Sample application for SoC FPGA reboot manager
 */


#include <stdlib.h>
#include <string.h>
#include "osal.h"
#include "osal_log.h"
#include <task.h>
#include "socfpga_reboot_mngr.h"
#include "reboot_mngr_sample.h"
#include "socfpga_defines.h"

/*
 * This sample application demonstrate the use of reboot manager driver to perform cold or warm reboot.
 */


#define DO_COLD_BOOT    1
/*
 * @brief callback function for handing off before warm boot
 */

void warm_reboot_callback( void *ulBuff )
{
    (void)ulBuff;
    INFO("Warm reboot");
}

/*
 * @brief callback function for handing off before cold boot
 */

void cold_reboot_callback( void *ulBuff )
{
    (void)ulBuff;
    INFO("Cold reboot");
}

void reboot_task()
{
    int status;
#ifdef DO_COLD_BOOT
    INFO("Cold reboot example");
    INFO("Set user callback for cold boot");
    reboot_mngr_set_callback(cold_reboot_callback, COLD_REBOOT);
    INFO("Triggering cold boot ...");
    status = reboot_mngr_do_reboot(COLD_REBOOT);
#else
    INFO("Warm reboot example");
    INFO("Set user callback for warm boot");
    reboot_mngr_set_callback(warm_reboot_callback, WARM_REBOOT);
    INFO("Triggering warm boot ...");
    status = reboot_mngr_do_reboot(WARM_REBOOT);
#endif
    if (status != REBOOT_SUCCESS)
    {
        INFO("Reboot request failed with error code  %d", status);
    }
}

