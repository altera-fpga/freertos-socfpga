/*
 * SPDX-FileCopyrightText: Copyright (C) 2025 Altera Corporation
 *
 * SPDX-License-Identifier: MIT-0
 *
 * HAL driver implementation for reset manager
 */

#include "socfpga_rst_mngr_reg.h"
#include "socfpga_rst_mngr.h"
#include "socfpga_defines.h"
#include "osal.h"

int32_t rstmgr_assert_reset( ResetPeriphrl_t per )
{
    uint8_t bitPos;
    uint32_t val;
    uint32_t baseAddr;
    if ((per < RST_PERIPHERAL_START) || (per > RST_PERIPHERAL_END))
    {
        return RSTMGR_INVALID_VALUE;
    }
    baseAddr = RSTMGR_GET_BASE_ADDR(per);
    bitPos = RSTMGR_GET_BIT_POS(per);
    /* set specified bit in the register */
    val = RD_REG32(baseAddr);
    val |= (1 << bitPos);
    WR_REG32(baseAddr, val);
    return RSTMGR_SUCCESS;
}

int32_t rstmgr_deassert_reset( ResetPeriphrl_t per )
{
    uint8_t bitPos;
    uint32_t val;
    uint32_t baseAddr;
    if ((per < RST_PERIPHERAL_START) || (per > RST_PERIPHERAL_END))
    {
        return RSTMGR_INVALID_VALUE;
    }
    baseAddr = RSTMGR_GET_BASE_ADDR(per);
    bitPos = RSTMGR_GET_BIT_POS(per);
    /* clear specified bit in the register */
    val = RD_REG32(baseAddr);
    val &= ~(1 << bitPos);
    WR_REG32(baseAddr, val);
    return RSTMGR_SUCCESS;
}

int32_t rstmgr_toggle_reset( ResetPeriphrl_t per )
{
    if ((per < RST_PERIPHERAL_START) || (per > RST_PERIPHERAL_END))
    {
        return RSTMGR_INVALID_VALUE;
    }
    rstmgr_assert_reset(per);
    osal_task_delay(1);
    rstmgr_deassert_reset(per);
    return RSTMGR_SUCCESS;
}

int32_t rstmgr_get_reset_status( ResetPeriphrl_t per, uint8_t *stat )
{
    uint8_t bitPos;
    uint32_t baseAddr;
    if ((per < RST_PERIPHERAL_START) || (per > RST_PERIPHERAL_END))
    {
        return RSTMGR_INVALID_VALUE;
    }
    baseAddr = RSTMGR_GET_BASE_ADDR(per);
    bitPos = RSTMGR_GET_BIT_POS(per);
    /* get specified bit from the register */
    *stat = (RD_REG32(baseAddr) >> bitPos) & 1U;
    return RSTMGR_SUCCESS;
}
