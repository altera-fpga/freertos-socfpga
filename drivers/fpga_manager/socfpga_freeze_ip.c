/*
 * SPDX-FileCopyrightText: Copyright (C) 2025 Altera Corporation
 *
 * SPDX-License-Identifier: MIT-0
 *
 * Driver functions for freeze IP 
 */

#include "socfpga_defines.h"
#include "socfpga_freeze_ip.h"
#include "socfpga_freeze_ip_ll.h"
#include "osal_log.h"

int do_freeze_pr_region(void)
{
    int ret;

    ret = freeze_pr_region();

    if( ret != 0 )
    {
        ERROR("freeze bridge error %d", ret);
    }

    return ret;
}

int do_unfreeze_pr_region(void)
{
    int ret;

    ret = unfreeze_pr_region();

    if( ret != 0 )
    {
        ERROR("freeze bridge error %d", ret);
    }

    return ret;
}

