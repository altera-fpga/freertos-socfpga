/*
 * SPDX-FileCopyrightText: Copyright (C) 2025 Altera Corporation
 *
 * SPDX-License-Identifier: MIT-0
 *
 * SoC FPGA combophy definitions
 */

/*
    File: SD/MMC Combo PHY Driver

    SoC FPGA combophy definitions.
*/

/*
    Constants: SD/MMC Combo PHY constants

    COMBO_PHY_ADD             - Value: 0x10B92000U.
    PHY_DQ_TIM_REG_OFFSET     - Value: 0x0U.
    PHY_DQS_TIM_REG_OFFSET    - Value: 0x04U.
    PHY_GATE_LPBK_CTL_OFFSET  - Value: 0x08U.
    PHY_DLL_MASTER_CTL_OFFSET - Value: 0xCU.
    PHY_DLL_SLAVE_CTL_OFFSET  - Value: 0x10U.
    PHY_CTL_REG_ADD_OFFSET    - Value: 0x80U.
    PHY_DQ_TIM_REG_ADD        - Expands to (COMBO_PHY_ADD + PHY_DQ_TIM_REG_OFFSET).
    PHY_DQS_TIM_REG_ADD       - Expands to (COMBO_PHY_ADD + PHY_DQS_TIM_REG_OFFSET).
    PHY_GATE_LPBK_CTL_ADD     - Expands to (COMBO_PHY_ADD + PHY_GATE_LPBK_CTL_OFFSET).
    PHY_DLL_MASTER_CTL_ADD    - Expands to (COMBO_PHY_ADD + PHY_DLL_MASTER_CTL_OFFSET).
    PHY_DLL_SLAVE_CTL_ADD     - Expands to (COMBO_PHY_ADD + PHY_DLL_SLAVE_CTL_OFFSET).
    PHY_CTL_REG_ADD           - Expands to (COMBO_PHY_ADD + PHY_CTL_REG_ADD_OFFSET).
    USE_EXT_LPBK_DQS          - Expands to ((uint32_t) 1 << 22U).
    USE_LPBK_DQS              - Expands to ((uint32_t) 1 << 21U).
    USE_PHONY_DQS             - Expands to ((uint32_t) 1 << 20U).
    USE_PHONY_DQS_CMD         - Expands to ((uint32_t) 1 << 19U).
    DQS_SEL_OE_END            - Value: (4U).
    SYNC_METHOD_EN            - Expands to ((uint32_t) 1 << 31U).
    RD_DEL_SEL                - Expands to ((uint32_t) 52 << 19U).
    SET_UNDERRUN_SUPPRESS     - Expands to ((uint32_t) 1 << 18U).
    GATE_CFG_ALWAYS_ON        - Expands to (1U << 6U).
    SW_HALF_CYCLE_SHIFT       - Expands to ((uint32_t) 0 << 28U).
    SEL_DLL_BYPASS_MODE       - Expands to ((uint32_t) 1 << 23U).
    PARAM_DLL_START_POINT     - Expands to (4U << 0U).
    READ_DQS_CMD_DELAY        - Expands to ((uint32_t) 0 << 24U).
    CLK_WRDQS_DELAY           - Expands to ((uint32_t) 0 << 16U).
    CLK_WR_DELAY              - Expands to ((uint32_t) 0 << 8U).
    READ_DQS_DELAY            - Value: (0U).
    IO_MASK_DISABLE           - Expands to ((uint32_t) 0 << 31U).
    IO_MASK_END               - Expands to ((uint32_t) 0 << 27U).
    IO_MASK_START             - Expands to ((uint32_t) 0 << 24U).
    DATA_SEL_OE_END           - Value: (1U).
    RDDATA_EN                 - Expands to (1U << 16U).
    RDCMD_EN                  - Expands to (1U << 15U).
    EXTENDED_RD_MODE          - Expands to (1U << 2U).
    EXTENDED_WR_MODE          - Expands to (1U << 3U).
    HCSD_CLK_ADJ              - Expands to (6U << 16U).
    WRDATA0_SDCLK_DLY         - Expands to (0U << 24U).
    WRCMD0_SDCLK_DLY          - Expands to ((uint32_t) 0 << 16U).
    WRDATA0_DLY               - Expands to (1U << 8U).
    WRCMD0_DLY                - Value: (1U).
    REG_ADD_LSB_MASK          - Value: (0x0000FFFFU).
    PHONY_DQS_DELAY           - Expands to (0x3FU << 4U).
*/

#ifndef __SOCFPGA_COMBO_PHY_H__
#define __SOCFPGA_COMBO_PHY_H__

#define COMBO_PHY_ADD                0x10B92000U


#define PHY_DQ_TIM_REG_OFFSET        0x0U
#define PHY_DQS_TIM_REG_OFFSET       0x04U
#define PHY_GATE_LPBK_CTL_OFFSET     0x08U
#define PHY_DLL_MASTER_CTL_OFFSET    0xCU
#define PHY_DLL_SLAVE_CTL_OFFSET     0x10U
#define PHY_CTL_REG_ADD_OFFSET       0x80U

#define PHY_DQ_TIM_REG_ADD           (COMBO_PHY_ADD + PHY_DQ_TIM_REG_OFFSET)
#define PHY_DQS_TIM_REG_ADD          (COMBO_PHY_ADD + PHY_DQS_TIM_REG_OFFSET)
#define PHY_GATE_LPBK_CTL_ADD        (COMBO_PHY_ADD + PHY_GATE_LPBK_CTL_OFFSET)
#define PHY_DLL_MASTER_CTL_ADD       (COMBO_PHY_ADD + PHY_DLL_MASTER_CTL_OFFSET)
#define PHY_DLL_SLAVE_CTL_ADD        (COMBO_PHY_ADD + PHY_DLL_SLAVE_CTL_OFFSET)
#define PHY_CTL_REG_ADD              (COMBO_PHY_ADD + PHY_CTL_REG_ADD_OFFSET)


#define USE_EXT_LPBK_DQS             ((uint32_t) 1 << 22U)
#define USE_LPBK_DQS                 ((uint32_t) 1 << 21U)
#define USE_PHONY_DQS                ((uint32_t) 1 << 20U)
#define USE_PHONY_DQS_CMD            ((uint32_t) 1 << 19U)
#define DQS_SEL_OE_END               (4U)
#define SYNC_METHOD_EN               ((uint32_t) 1 << 31U)
#define RD_DEL_SEL                   ((uint32_t) 52 << 19U)
#define SET_UNDERRUN_SUPPRESS        ((uint32_t) 1 << 18U)
#define GATE_CFG_ALWAYS_ON           (1U << 6U)
#define SW_HALF_CYCLE_SHIFT          ((uint32_t) 0 << 28U)
#define SEL_DLL_BYPASS_MODE          ((uint32_t) 1 << 23U)
#define PARAM_DLL_START_POINT        (4U << 0U)
#define READ_DQS_CMD_DELAY           ((uint32_t) 0 << 24U)
#define CLK_WRDQS_DELAY              ((uint32_t) 0 << 16U)
#define CLK_WR_DELAY                 ((uint32_t) 0 << 8U)
#define READ_DQS_DELAY               (0U)
#define IO_MASK_DISABLE              ((uint32_t) 0 << 31U)
#define IO_MASK_END                  ((uint32_t) 0 << 27U)
#define IO_MASK_START                ((uint32_t) 0 << 24U)
#define DATA_SEL_OE_END              (1U)
#define RDDATA_EN                    (1U << 16U)
#define RDCMD_EN                     (1U << 15U)
#define EXTENDED_RD_MODE             (1U << 2U)
#define EXTENDED_WR_MODE             (1U << 3U)
#define HCSD_CLK_ADJ                 (6U << 16U)
#define WRDATA0_SDCLK_DLY            (0U << 24U)
#define WRCMD0_SDCLK_DLY             ((uint32_t) 0 << 16U)
#define WRDATA0_DLY                  (1U << 8U)
#define WRCMD0_DLY                   (1U)
#define REG_ADD_LSB_MASK             (0x0000FFFFU)
#define PHONY_DQS_DELAY              (0x3FU << 4U)


#endif
