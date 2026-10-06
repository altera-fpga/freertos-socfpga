/*
 * SPDX-FileCopyrightText: Copyright (C) 2025 Altera Corporation
 *
 * SPDX-License-Identifier: MIT-0
 *
 * Header file for Clock manager HAL driver
 */
#ifndef __SOCFPGA_CLK_MNGR_H__
#define __SOCFPGA_CLK_MNGR_H__

/*
    File: Clock Manager Driver

    Clock manager HAL driver header file

    APIs for the SoC FPGA Clock Manager driver

    This is the Clock Manager driver for SoC FPGA, which provides APIs to
    manage and control clock sources, dividers, and frequencies for various
    peripheral blocks.
*/

#include "socfpga_defines.h"

/*
    Group: Enumerations

    Clock Manager Specific Enumerations
*/

/*
    Enum: clock_block_t

    enum for peripheral blocks
*/
typedef enum
{
    CLOCK_MPU = 0, /* MPU Clock */
    CLOCK_SDMMC, /* SDMMC Clock */
    CLOCK_QSPI, /* QSPI Clock */
    CLOCK_SP_TIMER, /* Peripheral Timer Clock */
    CLOCK_OSC1TIMER, /* OSC1 Timer Clock */
    CLOCK_SSPI, /* SPI Clock */
    CLOCK_L4MAIN, /* L4 Main Clock */
    CLOCK_UART, /* UART Clock */
    CLOCK_WDT,  /* Watchdog Timer Clock */
    CLOCK_I2C, /* I2C Clock */
    CLOCK_INVALID, /* Invalid Clock Block */
} clock_block_t;

/*
    Group: Macros

    Constants for the Clock Manager driver.
*/
/*
    Constant: L4_SYS_FREE_CLK

    Clock types
*/

#define L4_SYS_FREE_CLK    0U /* L4_SYS_FREE_CLK block ID */
#define L4_MP_CLK          1U /* L4_MP_CLK block ID */
#define L4_SP_CLK          2U /* L4_SP_CLK block ID */
#define SOFT_PHY_DIV       3U /* SOFT_PHY_CLK block ID */
#define CCU_DIV            4U /* CCU_CLK block ID */
#define MPU_PERIPH_DIV     5U /* MPU_PERIPH_CLK block ID */
#define CS_CLK             6U /* CS_CLK block ID */
#define CS_TRACE_CLK       7U /* CS_TRACE_CLK block ID */
#define CS_PDBG_CLK        8U /* CS_PDBG_CLK block ID */

/*
    Constant: MAIN_PLL_CLK_SRC

    PLL clock source
*/

#define MAIN_PLL_CLK_SRC    0U            /* MAIN_PLL_CLK source ID */
#define PERI_PLL_CLK_SRC    1U            /* PERI_PLL_CLK source ID */
#define OSC1_CLK_SRC        3U            /* OSC1_CLK source ID */
#define INTOSC_CLK_SRC      4U            /* INTOSC_CLK source ID */
#define FPGA_CLK_SRC        5U            /* FPGA_CLK source ID */
#define INVALID_CLK_SRC     6U            /* Invalid source ID */

/*
    Constant: PSRC_EOSC1_CLK

    VCO Clock Source
*/
#define PSRC_EOSC1_CLK       0U           /* EOSC1_CLK source ID */
#define PSRC_CBINTOSC_CLK    1U           /* CBINTOSC_CLK source ID */
#define PSRC_F2S_FREE_CLK    2U           /* F2S_FREE_CLK source ID */

#define CLKMGR_INTOSC_CLOCK_RATE_HZ    460000000U  /* Internal oscillator clock value in Hz */
#define QSPI_CLOCK_MULTIPLIER          1000U /* QSPI block multiplier value for calculating source value */

/*
    Group: Functions

    Entry points for the Clock Manager driver.
*/
/*
    Function: clk_mngr_get_clk

    SoC FPGA get source clock frequency for a given block

    Parameters:

        clock_block_name - Peripheral block ID
        pclock_rate - Clock rate in Hz.

    Returns:

        - 0:       for a non zero clock  value
        - -EINVAL: if it is zero
*/
int32_t clk_mngr_get_clk(clock_block_t clock_block_name, uint32_t *pclock_rate);

/*
    Function: clk_mngr_set_divisor

    set control clock dividers for a given clock type

    Parameters:

        clock_type - Control clock type
        divisor - Required divisor given by user.

    Returns:

        - 0:       if the divisor is set successfully
        - -EINVAL: if divisor or clock type is invalid
*/
int32_t clk_mngr_set_divisor(uint32_t clock_type, uint8_t divisor);

/*
    Function: clk_mngr_get_divisor

    get the divisor value for each control clock type

    Parameters:

        clock_type - Control clock type
        divisor - Divisor set for the clock type.

    Returns:

        - 0:       for a valid divisor value
        - -EINVAL: if divisor or clock type is invalid
*/
int32_t clk_mngr_get_divisor(uint32_t clock_type, uint8_t *divisor);

#endif /* __SOCFPGA_CLK_MNGR_H__ */
