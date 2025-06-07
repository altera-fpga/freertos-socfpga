/*
 * SPDX-FileCopyrightText: Copyright (C) 2025 Altera Corporation
 *
 * SPDX-License-Identifier: MIT-0
 *
 * HAL driver implementation for Clock manager
 */

#include <errno.h>
#include "socfpga_clk_mngr_reg.h"
#include "socfpga_sys_mngr_reg.h"
#include "socfpga_clk_mngr.h"
#include "socfpga_defines.h"
#include "osal_log.h"

#define CLK_SUCCESS    0U
#define CLK_ERROR      1U
/**
 * @brief Read Source VCO clock for given block
 */
static uint32_t prvClkMngrGetSrcRefClk( uint32_t ulPllGlobRegVal )
{
    uint32_t ulSrcClkValue = 0U, ulRefClkValue = 0U, ulArefClkDiv;
    uint32_t xSrcPllValue;
    xSrcPllValue = (ulPllGlobRegVal & CLK_MNGR_PLLGLOB_PSRC_MASK)
            >> CLK_MNGR_PLLGLOB_PSRC_POS;
    /*The clock reference values were updated in Boot scratch register by ATF.*/
    switch (xSrcPllValue)
    {
    case PSRC_EOSC1_CLK:
        ulSrcClkValue = RD_REG32(
                SYS_MNGR_BASE_ADDR + SYS_MNGR_BOOT_SCRATCH_COLD1);
        break;

    case PSRC_CBINTOSC_CLK:
        ulSrcClkValue = CLKMGR_INTOSC_CLOCK_RATE_HZ;
        break;

    case PSRC_F2S_FREE_CLK:
        ulSrcClkValue = RD_REG32(
                SYS_MNGR_BASE_ADDR + SYS_MNGR_BOOT_SCRATCH_COLD2);
        break;

    default:
        ERROR(" Not a valid block to get clock frequency")
        ;
        break;

    }
    if (ulSrcClkValue != 0U)
    {
        ulArefClkDiv = (ulPllGlobRegVal & CLK_MNGR_PLLGLOB_AREFCLKDIV_MASK)
                >> CLK_MNGR_PLLGLOB_AREFCLKDIV_POS;
        if (ulArefClkDiv == 0U)
        {
            ERROR(" ulArefClkDiv is 0");
            return ulRefClkValue;
        }
        ulRefClkValue = ulSrcClkValue / ulArefClkDiv;
    }
    return ulRefClkValue;
}

/**
 * @brief Read Reference clock value of given PLL source
 */
static uint32_t prvClkMngrGetClk( uint32_t ulClkSrcReg,
        uint32_t ulPllChannelConfigRegMain, uint32_t ulPllChannelConfigRegPeri )
{
    uint32_t xSrcClk;
    uint32_t ulClkSrc, ulClkMdiv, ulRefClkVal = 0U;
    uint32_t ulClkDivCtrlRegVal = 0U, ulPllChannelConfigRegVal = 0U,
            ulPllGlobRegVal = 0U, ulPllClkSliceDiv;
    ulClkSrc = RD_REG32(ulClkSrcReg);
    xSrcClk = (uint32_t) ((ulClkSrc & CLK_MNGR_NOCCLK_SRC_MASK)
            >> CLK_MNGR_NOCCLK_SRC_POS);
    switch (xSrcClk)
    {
    case MAIN_PLL_CLK_SRC:
        ulClkDivCtrlRegVal = RD_REG32(
                CLK_MNGR_BASE_ADDR + MAINPLL_CLK_MNGR_PLLM);
        ulPllChannelConfigRegVal = RD_REG32(ulPllChannelConfigRegMain);
        ulPllGlobRegVal = RD_REG32(
                CLK_MNGR_BASE_ADDR + MAINPLL_CLK_MNGR_PLLGLOB);
        break;

    case PERI_PLL_CLK_SRC:
        ulClkDivCtrlRegVal = RD_REG32(
                CLK_MNGR_BASE_ADDR + PERPLL_CLK_MNGR_PLLM);
        ulPllChannelConfigRegVal = RD_REG32(ulPllChannelConfigRegPeri);
        ulPllGlobRegVal = RD_REG32(
                CLK_MNGR_BASE_ADDR + PERPLL_CLK_MNGR_PLLGLOB);
        break;

    default:
        ERROR(" Not a valid block to get clock frequency")
        ;
        break;
    }
    ulRefClkVal = prvClkMngrGetSrcRefClk(ulPllGlobRegVal);
    if (ulRefClkVal != 0U)
    {

        ulClkMdiv = (ulClkDivCtrlRegVal & CLK_MNGR_PLLM_MDIV_MASK);
        ulRefClkVal = ulRefClkVal * ulClkMdiv;

        ulPllClkSliceDiv = (ulPllChannelConfigRegVal & CLK_MNGR_PLLC0_DIV_MASK);
        if (ulPllClkSliceDiv == 0U)
        {
            ERROR(" PLL Clock Slice Div is 0 ");
            return ulRefClkVal;
        }
        ulRefClkVal = ulRefClkVal / ulPllClkSliceDiv;
    }
    return ulRefClkVal;
}

/**
 * @brief Read L3 Main Free clock
 */
static uint32_t prvClkMngrGetL3MainFreeClk( void )
{
    uint32_t ulL3MainFreeClk = 0U;
    ulL3MainFreeClk = prvClkMngrGetClk(
            CLK_MNGR_BASE_ADDR + MAINPLL_CLK_MNGR_NOCCLK,
            CLK_MNGR_BASE_ADDR + MAINPLL_CLK_MNGR_PLLC3,
            CLK_MNGR_BASE_ADDR + PERPLL_CLK_MNGR_PLLC1);
    return ulL3MainFreeClk;
}

/**
 * @brief Read L4 SP clock
 */
static uint32_t prvClkMngrGetL4SpClk( void )
{
    uint32_t ulL4SpClk, ulL4SpClkDiv;
    ulL4SpClk = prvClkMngrGetL3MainFreeClk();
    if (ulL4SpClk != 0U)
    {
        ulL4SpClkDiv = RD_REG32(CLK_MNGR_BASE_ADDR + MAINPLL_CLK_MNGR_NOCDIV);
        ulL4SpClkDiv = (ulL4SpClkDiv & CLK_MNGR_NOCDIV_L4SPCLK_MASK)
                >> CLK_MNGR_NOCDIV_L4SPCLK_POS;
        ulL4SpClk = ulL4SpClk >> ulL4SpClkDiv;
    }
    return ulL4SpClk;
}

/**
 * @brief Read L4 Main clock
 */
static uint32_t prvClkMngrGetSpTimerClk( void )
{
    return prvClkMngrGetL4SpClk();
}

/**
 * @brief Read L4 Sys Free clock
 */
static uint32_t prvClkMngrGetUartClk( void )
{
    return prvClkMngrGetL4SpClk();
}

/**
 * @brief Read L4 Sys Free clock
 */
static uint32_t prvClkMngrGetI2cClk( void )
{
    return prvClkMngrGetL4SpClk();
}

/**
 * @brief Read L4 Sys Free clock
 */
static uint32_t prvClkMngrGetL4SysFreeClk( void )
{
    uint32_t ulL4SysFreeClk, ulL4SysFreeClkDiv;
    ulL4SysFreeClk = prvClkMngrGetL3MainFreeClk();
    if (ulL4SysFreeClk != 0U)
    {
        ulL4SysFreeClkDiv = RD_REG32(
                CLK_MNGR_BASE_ADDR + MAINPLL_CLK_MNGR_NOCDIV);
        ulL4SysFreeClkDiv =
                (ulL4SysFreeClkDiv & CLK_MNGR_NOCDIV_L4SYSFREECLK_MASK)
                >> CLK_MNGR_NOCDIV_L4SYSFREECLK_POS;
        ulL4SysFreeClk = ulL4SysFreeClk >> ulL4SysFreeClkDiv;
    }
    return ulL4SysFreeClk;
}

/**
 * @brief Read WDT clock
 */
static uint32_t prvClkMngrGetWdtClk( void )
{
    return prvClkMngrGetL4SysFreeClk();
}

/**
 * @brief Read Osc1 Timer clock
 */
static uint32_t prvClkMngrGetOsc1TimerClk( void )
{
    return prvClkMngrGetL4SysFreeClk();
}

/**
 * @brief Read QSPI clock
 */
static uint32_t prvClkMngrGetQspiClk( void )
{
    uint32_t ulQspiClk;
    /*The clock reference values were updated in Boot scratch0 register by ATF.*/
    ulQspiClk = RD_REG32(SYS_MNGR_BASE_ADDR + SYS_MNGR_BOOT_SCRATCH_COLD0);
    ulQspiClk = ulQspiClk * QSPI_CLOCK_MULTIPLIER;
    return ulQspiClk;
}

/**
 * @brief Read L4 Main clock
 */
static uint32_t prvClkMngrGetL4MainClk( void )
{
    uint32_t ulL4MainClk;
    ulL4MainClk = prvClkMngrGetL3MainFreeClk();
    return ulL4MainClk;
}

/**
 * @brief Read SSPI clock
 */
static uint32_t prvClkMngrGetSspiClk( void )
{
    uint32_t ulSspiClk;
    ulSspiClk = prvClkMngrGetL4MainClk();
    return ulSspiClk;
}

/**
 * @brief Read L4 MP clock
 */
static uint32_t prvClkMngrGetL4MpClk( void )
{
    uint32_t ulL4MpClk, ulL4MpClkDiv;
    ulL4MpClk = prvClkMngrGetL3MainFreeClk();
    if (ulL4MpClk != 0U)
    {
        ulL4MpClkDiv = RD_REG32(CLK_MNGR_BASE_ADDR + MAINPLL_CLK_MNGR_NOCDIV);
        ulL4MpClkDiv = (ulL4MpClkDiv & CLK_MNGR_NOCDIV_L4MPCLK_MASK)
                >> CLK_MNGR_EN_L4MPCLKEN_POS;
        ulL4MpClk = ulL4MpClk >> ulL4MpClkDiv;
    }
    return ulL4MpClk;
}

/**
 * @brief Get SDMMC clock
 */
static uint32_t prvClkMngrGetSdmmcClk( void )
{
    uint32_t ulSdmmcClk;
    ulSdmmcClk = prvClkMngrGetL4MpClk();
    return ulSdmmcClk;
}

/**
 * @brief Get MPU clock
 */
static uint32_t prvClkMngrGetMpuClk( void )
{
    uint32_t ulMpuClk = 0U;
    ulMpuClk = prvClkMngrGetClk(CLK_MNGR_BASE_ADDR + CLK_MNGR_DSUCTR,
            CLK_MNGR_BASE_ADDR + MAINPLL_CLK_MNGR_PLLC2,
            CLK_MNGR_BASE_ADDR + PERPLL_CLK_MNGR_PLLC0);
    return ulMpuClk;
}

/**
 * @brief Set L4 Sys Free clock divisor
 */
static uint32_t prvClkMngrSetL4SysFreeClkDiv( uint8_t divisor )
{
    uint32_t reg_val = RD_REG32(CLK_MNGR_BASE_ADDR + MAINPLL_CLK_MNGR_NOCDIV);
    reg_val &= ~CLK_MNGR_NOCDIV_L4SYSFREECLK_MASK;
    if (divisor == 1U)
    {
        reg_val |= (0U << CLK_MNGR_NOCDIV_L4SYSFREECLK_POS);
    }
    else if (divisor == 2U)
    {
        reg_val |= 1U << CLK_MNGR_NOCDIV_L4SYSFREECLK_POS;
    }
    else if (divisor == 4U)
    {
        reg_val |= 2U << CLK_MNGR_NOCDIV_L4SYSFREECLK_POS;
    }
    else
    {
        ERROR("Not a valid divisor for the block");
        return -EINVAL;
    }
    WR_REG32(CLK_MNGR_BASE_ADDR + MAINPLL_CLK_MNGR_NOCDIV, reg_val);
    return 0;
}

/**
 * @brief Set L4 MP clock divisor
 */
static uint32_t prvClkMngrSetL4MpClkDiv( uint8_t divisor )
{
    uint32_t reg_val = RD_REG32(CLK_MNGR_BASE_ADDR + MAINPLL_CLK_MNGR_NOCDIV);
    reg_val &= ~CLK_MNGR_NOCDIV_L4MPCLK_MASK;
    if (divisor == 1U)
    {
        reg_val |= (0U << CLK_MNGR_NOCDIV_L4MPCLK_POS);
    }
    else if (divisor == 2U)
    {
        reg_val |= 1U << CLK_MNGR_NOCDIV_L4MPCLK_POS;
    }
    else
    {
        ERROR("Not a valid divisor for the block");
        return -EINVAL;
    }
    WR_REG32(CLK_MNGR_BASE_ADDR + MAINPLL_CLK_MNGR_NOCDIV, reg_val);
    return 0;
}

/**
 * @brief Set L4 SP clock divisor
 */
static uint32_t prvClkMngrSetL4SpClkDiv( uint8_t divisor )
{
    uint32_t reg_val = RD_REG32(CLK_MNGR_BASE_ADDR + MAINPLL_CLK_MNGR_NOCDIV);
    reg_val &= ~CLK_MNGR_NOCDIV_L4SPCLK_MASK;
    if (divisor == 1U)
    {
        reg_val |= (0U << CLK_MNGR_NOCDIV_L4SPCLK_POS);
    }
    else if (divisor == 2U)
    {
        reg_val |= 1U << CLK_MNGR_NOCDIV_L4SPCLK_POS;
    }
    else if (divisor == 4U)
    {
        reg_val |= 2U << CLK_MNGR_NOCDIV_L4SPCLK_POS;
    }
    else
    {
        ERROR("Not a valid divisor for the block");
        return -EINVAL;
    }
    WR_REG32(CLK_MNGR_BASE_ADDR + MAINPLL_CLK_MNGR_NOCDIV, reg_val);
    return 0;
}

/**
 * @brief Set Soft PHY clock divisor
 */
static uint32_t prvClkMngrSetSoftPhyDiv( uint8_t divisor )
{
    uint32_t reg_val = RD_REG32(CLK_MNGR_BASE_ADDR + MAINPLL_CLK_MNGR_NOCDIV);
    reg_val &= ~CLK_MNGR_NOCDIV_SOFTPHYDIV_MASK;
    if (divisor == 1U)
    {
        reg_val |= ((uint32_t) 0 << CLK_MNGR_NOCDIV_SOFTPHYDIV_POS);
    }
    else if (divisor == 2U)
    {
        reg_val |= (uint32_t) 1 << CLK_MNGR_NOCDIV_SOFTPHYDIV_POS;
    }
    else if (divisor == 4U)
    {
        reg_val |= (uint32_t) 2 << CLK_MNGR_NOCDIV_SOFTPHYDIV_POS;
    }
    else
    {
        ERROR("Not a valid divisor for the block");
        return -EINVAL;
    }
    WR_REG32(CLK_MNGR_BASE_ADDR + MAINPLL_CLK_MNGR_NOCDIV, reg_val);
    return 0;
}

/**
 * @brief Set CCU clock divisor
 */
static uint32_t prvClkMngrSetCCUDiv( uint8_t divisor )
{
    uint32_t reg_val = RD_REG32(CLK_MNGR_BASE_ADDR + MAINPLL_CLK_MNGR_NOCDIV);
    reg_val &= ~CLK_MNGR_NOCDIV_CCUDIV_MASK;
    if (divisor == 1U)
    {
        reg_val |= ((uint32_t) 0 << CLK_MNGR_NOCDIV_CCUDIV_POS);
    }
    else if (divisor == 2U)
    {
        reg_val |= (uint32_t) 1 << CLK_MNGR_NOCDIV_CCUDIV_POS;
    }
    else
    {
        ERROR("Not a valid divisor for the block");
        return -EINVAL;
    }
    WR_REG32(CLK_MNGR_BASE_ADDR + MAINPLL_CLK_MNGR_NOCDIV, reg_val);
    return 0;
}

/**
 * @brief Set MPU Peripheral clock divisor
 */
static uint32_t prvClkMngrSetMpuPeriphDiv( uint8_t divisor )
{
    uint32_t reg_val = RD_REG32(CLK_MNGR_BASE_ADDR + MAINPLL_CLK_MNGR_NOCDIV);
    reg_val &= ~CLK_MNGR_NOCDIV_MPUPERIPHDIV_MASK;
    if (divisor == 1U)
    {
        reg_val |= ((uint32_t) 0 << CLK_MNGR_NOCDIV_MPUPERIPHDIV_POS);
    }
    else if (divisor == 2U)
    {
        reg_val |= (uint32_t) 1 << CLK_MNGR_NOCDIV_MPUPERIPHDIV_POS;
    }
    else if (divisor == 4U)
    {
        reg_val |= (uint32_t) 2 << CLK_MNGR_NOCDIV_MPUPERIPHDIV_POS;
    }
    else
    {
        ERROR("Not a valid divisor for the block");
        return -EINVAL;
    }
    WR_REG32(CLK_MNGR_BASE_ADDR + MAINPLL_CLK_MNGR_NOCDIV, reg_val);
    return 0;
}

/**
 * @brief Set CS clock divisor
 */
static uint32_t prvClkMngrSetCsClkDiv( uint8_t divisor )
{
    uint32_t reg_val = RD_REG32(CLK_MNGR_BASE_ADDR + MAINPLL_CLK_MNGR_NOCDIV);
    reg_val &= ~CLK_MNGR_NOCDIV_CSCLK_MASK;
    if (divisor == 1U)
    {
        reg_val |= ((uint32_t) 0 << CLK_MNGR_NOCDIV_CSCLK_POS);
    }
    else if (divisor == 2U)
    {
        reg_val |= (uint32_t) 1 << CLK_MNGR_NOCDIV_CSCLK_POS;
    }
    else if (divisor == 4U)
    {
        reg_val |= (uint32_t) 2 << CLK_MNGR_NOCDIV_CSCLK_POS;
    }
    else if (divisor == 8U)
    {
        reg_val |= (uint32_t) 3 << CLK_MNGR_NOCDIV_CSCLK_POS;
    }
    else
    {
        ERROR("Not a valid divisor for the block");
        return -EINVAL;
    }
    WR_REG32(CLK_MNGR_BASE_ADDR + MAINPLL_CLK_MNGR_NOCDIV, reg_val);
    return 0;
}

/**
 * @brief Set CS Trace clock divisor
 */
static uint32_t prvClkMngrSetCsTraceClkDiv( uint8_t divisor )
{
    uint32_t reg_val = RD_REG32(CLK_MNGR_BASE_ADDR + MAINPLL_CLK_MNGR_NOCDIV);
    reg_val &= ~CLK_MNGR_NOCDIV_CSTRACECLK_MASK;
    if (divisor == 1U)
    {
        reg_val |= ((uint32_t) 0 << CLK_MNGR_NOCDIV_CSTRACECLK_POS);
    }
    else if (divisor == 2U)
    {
        reg_val |= (uint32_t) 1 << CLK_MNGR_NOCDIV_CSTRACECLK_POS;
    }
    else if (divisor == 4U)
    {
        reg_val |= (uint32_t) 2 << CLK_MNGR_NOCDIV_CSTRACECLK_POS;
    }
    else if (divisor == 8U)
    {
        reg_val |= (uint32_t) 3 << CLK_MNGR_NOCDIV_CSTRACECLK_POS;
    }
    else
    {
        ERROR("Not a valid divisor for the block");
        return -EINVAL;
    }
    WR_REG32(CLK_MNGR_BASE_ADDR + MAINPLL_CLK_MNGR_NOCDIV, reg_val);
    return 0;
}

/**
 * @brief Set CS PDBG clock divisor
 */
static uint32_t prvClkMngrSetCsPdbgClkDiv( uint8_t divisor )
{
    uint32_t reg_val = RD_REG32(CLK_MNGR_BASE_ADDR + MAINPLL_CLK_MNGR_NOCDIV);
    reg_val &= ~CLK_MNGR_NOCDIV_CSPDBGCLK_MASK;
    if (divisor == 1U)
    {
        reg_val |= ((uint32_t) 0 << CLK_MNGR_NOCDIV_CSPDBGCLK_POS);
    }
    else if (divisor == 2U)
    {
        reg_val |= (uint32_t) 1 << CLK_MNGR_NOCDIV_CSPDBGCLK_POS;
    }
    else if (divisor == 4U)
    {
        reg_val |= (uint32_t) 2 << CLK_MNGR_NOCDIV_CSPDBGCLK_POS;
    }
    else if (divisor == 8U)
    {
        reg_val |= (uint32_t) 3 << CLK_MNGR_NOCDIV_CSPDBGCLK_POS;
    }
    else
    {
        ERROR("Not a valid divisor for the block");
        return -EINVAL;
    }
    WR_REG32(CLK_MNGR_BASE_ADDR + MAINPLL_CLK_MNGR_NOCDIV, reg_val);
    return 0;
}

/**
 * @brief Get L4 Sys Free clock divisor
 */
static uint8_t prvClkMngrGetL4SysFreeClkDiv( void )
{
    uint32_t reg_val = RD_REG32(CLK_MNGR_BASE_ADDR + MAINPLL_CLK_MNGR_NOCDIV);
    uint32_t div = (reg_val >> CLK_MNGR_NOCDIV_L4SYSFREECLK_POS) & 3U;

    if (div == 2U)
    {
        return 4U;
    }
    if (div == 1U)
    {
        return 2U;
    }
    if (div == 0U)
    {
        return 0U;
    }
    return -EINVAL;
}

/**
 * @brief Get L4 MP clock divisor
 */
static uint8_t prvClkMngrGetL4MpClkDiv( void )
{
    uint32_t reg_val = RD_REG32(CLK_MNGR_BASE_ADDR + MAINPLL_CLK_MNGR_NOCDIV);
    uint32_t div = (reg_val >> CLK_MNGR_NOCDIV_L4MPCLK_POS) & 3U;

    if (div == 1U)
    {
        return 2U;
    }
    if (div == 0U)
    {
        return 0U;
    }
    return -EINVAL;
}

/**
 * @brief Get L4 SP clock divisor
 */
static uint8_t prvClkMngrGetL4SpClkDiv( void )
{
    uint32_t reg_val = RD_REG32(CLK_MNGR_BASE_ADDR + MAINPLL_CLK_MNGR_NOCDIV);
    uint32_t div = (reg_val >> CLK_MNGR_NOCDIV_L4SPCLK_POS) & 3U;

    if (div == 2U)
    {
        return 4U;
    }
    if (div == 1U)
    {
        return 2U;
    }
    if (div == 0U)
    {
        return 0U;
    }
    return -EINVAL;
}

/**
 * @brief Get Soft PHY clock divisor
 */
static uint8_t prvClkMngrGetSoftPhyDiv( void )
{
    uint32_t reg_val = RD_REG32(CLK_MNGR_BASE_ADDR + MAINPLL_CLK_MNGR_NOCDIV);
    uint32_t div = (reg_val >> CLK_MNGR_NOCDIV_SOFTPHYDIV_POS) & 3U;

    if (div == 2U)
    {
        return 4U;
    }
    if (div == 1U)
    {
        return 2U;
    }
    if (div == 0U)
    {
        return 0U;
    }
    return -EINVAL;
}

/**
 * @brief Get CCU clock divisor
 */
static uint8_t prvClkMngrGetCcuDiv( void )
{
    uint32_t reg_val = RD_REG32(CLK_MNGR_BASE_ADDR + MAINPLL_CLK_MNGR_NOCDIV);
    uint32_t div = (reg_val >> CLK_MNGR_NOCDIV_CCUDIV_POS) & 3U;

    if (div == 1U)
    {
        return 2U;
    }
    if (div == 0U)
    {
        return 0U;
    }
    return -EINVAL;
}

/**
 * @brief Get MPU Peripheral clock divisor
 */
static uint8_t prvClkMngrGetMpuPeriphDiv( void )
{
    uint32_t reg_val = RD_REG32(CLK_MNGR_BASE_ADDR + MAINPLL_CLK_MNGR_NOCDIV);
    uint32_t div = (reg_val >> CLK_MNGR_NOCDIV_MPUPERIPHDIV_POS) & 3U;

    if (div == 2U)
    {
        return 4U;
    }
    if (div == 1U)
    {
        return 2U;
    }
    if (div == 0U)
    {
        return 0U;
    }
    return -EINVAL;
}

/**
 * @brief Get CS clock divisor
 */
static uint8_t prvClkMngrGetCsClkDiv( void )
{
    uint32_t reg_val = RD_REG32(CLK_MNGR_BASE_ADDR + MAINPLL_CLK_MNGR_NOCDIV);
    uint32_t div = (reg_val >> CLK_MNGR_NOCDIV_CSCLK_POS) & 3U;

    if (div == 3U)
    {
        return 8U;
    }
    if (div == 2U)
    {
        return 4U;
    }
    if (div == 1U)
    {
        return 2U;
    }
    if (div == 0U)
    {
        return 0U;
    }
    return -EINVAL;
}

/**
 * @brief Get CS Trace clock divisor
 */
static uint8_t prvClkMngrGetCsTraceClkDiv( void )
{
    uint32_t reg_val = RD_REG32(CLK_MNGR_BASE_ADDR + MAINPLL_CLK_MNGR_NOCDIV);
    uint32_t div = (reg_val >> CLK_MNGR_NOCDIV_CSTRACECLK_POS) & 3U;

    if (div == 3U)
    {
        return 8U;
    }
    if (div == 2U)
    {
        return 4U;
    }
    if (div == 1U)
    {
        return 2U;
    }
    if (div == 0U)
    {
        return 0U;
    }
    return -EINVAL;
}

/**
 * @brief Get CS PDBG clock divisor
 */
static uint8_t prvClkMngrGetCsPdbgClkDiv( void )
{
    uint32_t reg_val = RD_REG32(CLK_MNGR_BASE_ADDR + MAINPLL_CLK_MNGR_NOCDIV);
    uint32_t div = (reg_val >> CLK_MNGR_NOCDIV_CSPDBGCLK_POS) & 3U;

    if (div == 3U)
    {
        return 8U;
    }
    if (div == 2U)
    {
        return 4U;
    }
    if (div == 1U)
    {
        return 2U;
    }
    if (div == 0U)
    {
        return 0U;
    }
    return -EINVAL;
}

int32_t clk_mngr_get_clk( ClockBlock_t xClockBlockName, uint32_t *pClockRate )
{
    uint32_t ulRet = 0;
    switch (xClockBlockName)
    {
    case eClockMpu:
        *pClockRate = prvClkMngrGetMpuClk();
        break;

    case eClockSdmmc:
        *pClockRate = prvClkMngrGetSdmmcClk();
        break;

    case eClockQspi:
        *pClockRate = prvClkMngrGetQspiClk();
        break;

    case eClockSpTimer:
        *pClockRate = prvClkMngrGetSpTimerClk();
        break;
    case eClockOsc1Timer:
        *pClockRate = prvClkMngrGetOsc1TimerClk();
        break;
    case eClockSspi:
        *pClockRate = prvClkMngrGetSspiClk();
        break;

    case eClockUart:
        *pClockRate = prvClkMngrGetUartClk();
        break;

    case eClockWdt:
        *pClockRate = prvClkMngrGetWdtClk();
        break;

    case eClockI2c:
        *pClockRate = prvClkMngrGetI2cClk();
        break;

    default:
        ERROR(" Not a valid block to get clock frequency");
        *pClockRate = 0;
        break;
    }

    if (*pClockRate == 0U)
    {
        ulRet = -EINVAL;
    }
    return ulRet;
}

int32_t clk_mngr_set_divisor( uint32_t clock_type, uint8_t divisor )
{
    int32_t ret = 1;
    switch (clock_type)
    {
    case L4_SYS_FREE_CLK:
        if (prvClkMngrSetL4SysFreeClkDiv(divisor) != 0)
        {
            ret = -1;
        }
        break;

    case L4_MP_CLK:
        if (prvClkMngrSetL4MpClkDiv(divisor) != 0)
        {
            ret = -1;
        }
        break;

    case L4_SP_CLK:
        if (prvClkMngrSetL4SpClkDiv(divisor) != 0)
        {
            ret = -1;
        }
        break;

    case SOFT_PHY_DIV:
        if (prvClkMngrSetSoftPhyDiv(divisor) != 0)
        {
            ret = -1;
        }
        break;

    case CCU_DIV:
        if (prvClkMngrSetCCUDiv(divisor) != 0)
        {
            ret = -1;
        }
        break;

    case MPU_PERIPH_DIV:
        if (prvClkMngrSetMpuPeriphDiv(divisor) != 0)
        {
            ret = -1;
        }
        break;

    case CS_CLK:
        if (prvClkMngrSetCsClkDiv(divisor) != 0)
        {
            ret = -1;
        }
        break;

    case CS_TRACE_CLK:
        if (prvClkMngrSetCsTraceClkDiv(divisor) != 0)
        {
            ret = -1;
        }
        break;

    case CS_PDBG_CLK:
        if (prvClkMngrSetCsPdbgClkDiv(divisor) != 0)
        {
            ret = -1;
        }
        break;

    default:
        ERROR("Not a valid clock type");
        ret = -1;
        break;
    }

    if (ret == -1)
    {
        return -EINVAL;
    }

    return 0;
}

int32_t clk_mngr_get_divisor( uint32_t clock_type, uint8_t *divisor)
{
    switch (clock_type)
    {
    case L4_SYS_FREE_CLK:
        *divisor = prvClkMngrGetL4SysFreeClkDiv();
        break;

    case L4_MP_CLK:
        *divisor = prvClkMngrGetL4MpClkDiv();
        break;

    case L4_SP_CLK:
        *divisor = prvClkMngrGetL4SpClkDiv();
        break;

    case SOFT_PHY_DIV:
        *divisor = prvClkMngrGetSoftPhyDiv();
        break;

    case CCU_DIV:
        *divisor = prvClkMngrGetCcuDiv();
        break;

    case MPU_PERIPH_DIV:
        *divisor = prvClkMngrGetMpuPeriphDiv();
        break;

    case CS_CLK:
        *divisor = prvClkMngrGetCsClkDiv();
        break;

    case CS_TRACE_CLK:
        *divisor = prvClkMngrGetCsTraceClkDiv();
        break;

    case CS_PDBG_CLK:
        *divisor = prvClkMngrGetCsPdbgClkDiv();
        break;

    default:
        ERROR("Not a valid clock type");
        *divisor = 0;
        break;
    }

    if (*divisor == 0)
    {
        return -EINVAL;
    }
    return 0;
}
