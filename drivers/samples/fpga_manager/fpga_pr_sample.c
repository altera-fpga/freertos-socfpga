/*
 * SPDX-FileCopyrightText: Copyright (C) 2025 Altera Corporation
 *
 * SPDX-License-Identifier: MIT-0
 *
 * Sample application for fpga partial reconfiguration
 */

/* 
 * This sample uses three different .rbf files.
 *      -> ghrd.core.rbf ( fpga base configuration, needs to be loaded before persona overlays)
 *      -> persona0.rbf  ( bitstream for persona0 overlay)
 *      -> persona1.rbf  ( bitstream for persona1 overlay)
 * 
 * Sample initially loads  the core.rbf and configures the FPGA. Initially, the PR region contains persona0
 * Then it freezes the particular PR region, and loads the person1.rbf. Now the PR region contains persona1.
 * The same is confirmed by reading the sysid located at address 0x20020000
 */

#include "socfpga_defines.h"
#include "osal_log.h"
#include "socfpga_bridge.h"
#include "socfpga_mmc.h"
#include "socfpga_fpga_manager.h"
#include "fpga_pr_sample.h"
#include "socfpga_freeze_ip.h"

static int load_bitstream(const char *rbf)
{
    uint8_t *rbf_ptr;
    uint32_t file_size;

    rbf_ptr = mmc_read_file(SOURCE_SDMMC, rbf, &file_size);
    if( rbf_ptr == NULL )
    {
        ERROR("Unable to read bitstream from memory !!!");
        return 0;
    }

    if( load_fpga_bitstream(rbf_ptr, file_size) == -1 )
    {
        ERROR("Failed to load bitstream !!!");
        return 0;
    }

    vPortFree(rbf_ptr);
    INFO("bitstream configuration successful");

    return 1;
}

static int do_partial_reconfiguration(const char *rbf)
{
    if( do_freeze_pr_region() != BRIDGE_OP_SUCCESS )
    {
        return -1;
    }

    /* Load the bitstream */
    if( load_bitstream(rbf) == 0 )
    {
        ERROR("Bitstream loading failed");
        return -1;
    }
    osal_task_delay(100);

    if( do_unfreeze_pr_region() != BRIDGE_OP_SUCCESS )
    {
        return -1;
    }

    return 1;
}

void partial_reconfiguration_sample(void)
{
    uint32_t sysid, freeze_reg_version;

    /* Load the core.rbf with persona0 instantiated */
    if( load_bitstream("/ghrd.core.rbf") == 0 )
    {
        ERROR("Bitstream loading failed");
        return;
    }

    /* Enable the lwhps2fpga bridge */
    if(enable_lwhps2fpga_bridge() == -1 )
    {
        ERROR("Failed to enable the FPGA2HPS bridge !!!");
        return;
    }

    /* By default, person0 sysid is present in the core.rbf */
    sysid = RD_REG32(SYSID_REG);
    if( sysid != PERSONA0_SYSID )
    {
        ERROR("Incorrect sysid");
        return;
    }

    INFO("SYS ID 0 : %x", sysid);

    freeze_reg_version = RD_REG32(PR_FREEZE_BASE+FREEZE_REG_VERSION_OFF);
    if(freeze_reg_version != FREEZE_REG_VERSION)
    {
        ERROR("Freeze IP version does not match");
        return;
    }
    INFO("Freeze IP Version : %x", freeze_reg_version);

    /* Procees for Partial Reconfiguration */
    if( do_partial_reconfiguration(PERSONA1_RBF) == 1)
    {
        INFO("PR configuration done");
    }
    else
    {
        ERROR("PR Configuration failed");
        return;
    }

    sysid = RD_REG32(SYSID_REG);
    if( sysid != PERSONA1_SYSID )
    {
        ERROR("Incorrect sysid");
        return;
    }
    INFO("SYS ID 1 : %x", sysid);

    INFO("PR sample completed ");
}
