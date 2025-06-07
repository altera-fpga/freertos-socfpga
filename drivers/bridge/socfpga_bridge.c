/*
 * SPDX-FileCopyrightText: Copyright (C) 2025 Altera Corporation
 *
 * SPDX-License-Identifier: MIT-0
 *
 * Low level driver implementation for fpga bridges
 */

#include <stdint.h>
#include <stdio.h>
#include "socfpga_bridge_ll.h"
#include "socfpga_rst_mngr.h"
#include "socfpga_defines.h"
#include "osal_log.h"

int enable_hps2fpga_bridge(void)
{
    if( is_enabled(HPS2FPGA) == RET_SUCCESS )
    {
        return RET_SUCCESS;
    }

    if( configure_hps2fpga_bridge() == RET_FAIL )
    {
        ERROR("HPS2FPGA bridge timeout occured");
        return RET_FAIL;
    }

    if(rstmgr_assert_reset(RST_SOC2FPGA_BRIDGE) != RSTMGR_SUCCESS )
    {
        ERROR("Unable to assdert HPS2FPGA bridge");
        return RET_FAIL;
    }

    osal_delay_ms(10);

    if(rstmgr_deassert_reset(RST_SOC2FPGA_BRIDGE) != RSTMGR_SUCCESS )
    {
        ERROR("Unable to deassdert HPS2FPGA bridge");
        return RET_FAIL;
    }

    return RET_SUCCESS;
}

int disable_hps2fpga_bridge(void)
{
    if( is_disabled(HPS2FPGA) == RET_SUCCESS )
    {
        return RET_SUCCESS;
    }

    if( configure_hps2fpga_bridge() == RET_FAIL )
    {
        ERROR("HPS2FPGA bridge timeout occured");
        return RET_FAIL;
    }

    if(rstmgr_assert_reset(RST_SOC2FPGA_BRIDGE) != RSTMGR_SUCCESS )
    {
        ERROR("Unable to deassdert HPS2FPGA bridge");
        return RET_FAIL;
    }

    osal_delay_ms(10);

    return RET_SUCCESS;
}

int enable_lwhps2fpga_bridge(void)
{
    if( is_enabled(LWHPS2FPGA) == RET_SUCCESS )
    {
        return RET_SUCCESS;
    }

    if( configure_lwhps2fpga_bridge() == RET_FAIL )
    {
        ERROR("LWHPS2FPGA bridge timeout occured");
        return RET_FAIL;
    }

    if(rstmgr_assert_reset(RST_LWSOC2FPGA_BRIDGE) != RSTMGR_SUCCESS )
    {
        ERROR("Unable to deassdert LWHPS2FPGA bridge");
        return RET_FAIL;
    }

    osal_delay_ms(10);

    /* deassert reset */
    if(rstmgr_deassert_reset(RST_LWSOC2FPGA_BRIDGE) != RSTMGR_SUCCESS)
    {
        ERROR("Unable to deassdert LWHPS2FPGA bridge");
        return RET_FAIL;
    }

    return RET_SUCCESS;
}

int disable_lwhps2fpga_bridge(void)
{
    if( is_disabled(LWHPS2FPGA) == RET_SUCCESS )
    {
        return RET_SUCCESS;
    }

    if( configure_lwhps2fpga_bridge() == RET_FAIL )
    {
        ERROR("LWHPS2FPGA bridge timeout occured");
        return RET_FAIL;
    }

    if(rstmgr_assert_reset(RST_LWSOC2FPGA_BRIDGE) != RSTMGR_SUCCESS )
    {
        ERROR("Unable to assert LWHPS2FPGA bridge");
        return RET_FAIL;
    }

    osal_delay_ms(10);

    return RET_SUCCESS;
}

int enable_fpga2hps_bridge(void)
{
    if( is_enabled(FPGA2HPS) == RET_SUCCESS )
    {
        return RET_SUCCESS;
    }

    if(rstmgr_deassert_reset(RST_FPGA2SOC_BRIDGE) != RSTMGR_SUCCESS)
    {
        ERROR("Unable to deassdert FPGA2HPS bridge");
        return RET_FAIL;
    }

    if( fpga2hps_bridge_enable_conf() == RET_FAIL )
    {
        ERROR("FPGA2HPS bridge timeout occured");
        return RET_FAIL;
    }

    return RET_SUCCESS;
}

int disable_fpga2hps_bridge(void)
{
    if( is_disabled(FPGA2HPS) == RET_SUCCESS )
    {
        return RET_SUCCESS;
    }

    if( fpga2hps_bridge_disable_conf() == RET_FAIL )
    {
        ERROR("FPGA2HPS bridge timeout occured");
        return RET_FAIL;
    }

    if(rstmgr_assert_reset(RST_FPGA2SOC_BRIDGE) != RSTMGR_SUCCESS )
    {
        ERROR("Unable to assdert LWHPS2FPGA bridge");
        return RET_FAIL;
    }

    return RET_SUCCESS;
}

int enable_fpga2sdram_bridge(void)
{
    if( is_enabled(FPGA2SDRAM) == RET_SUCCESS )
    {
        return RET_SUCCESS;
    }

    /* deassert reset */
    if(rstmgr_deassert_reset(RST_FPGA2SDRAM_BRIDGE) != RSTMGR_SUCCESS)
    {
        ERROR("Unable to deassdert FPGA2SDRAM bridge");
        return RET_FAIL;
    }

    if( fpga2sdram_bridge_enable_conf() == RET_FAIL)
    {
        ERROR("FPGA2SDRAM bridge timeout occured");
        return RET_FAIL;
    }

    return RET_SUCCESS;
}

int disable_fpga2sdram_bridge(void)
{
    if( is_disabled(FPGA2SDRAM) == RET_SUCCESS )
    {
        return RET_SUCCESS;
    }

    if( fpga2sdram_bridge_disable_conf() == RET_FAIL )
    {
        ERROR("FPGA2SDRAM bridge timeout occured");
        return RET_FAIL;
    }

    if(rstmgr_assert_reset(RST_FPGA2SDRAM_BRIDGE) != RSTMGR_SUCCESS )
    {
        ERROR("Unable to assert FPGA2SDRAM bridge");
        return RET_FAIL;
    }

    return RET_SUCCESS;
}
