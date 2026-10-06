/*
 * SPDX-FileCopyrightText: Copyright (C) 2025 Altera Corporation
 *
 * SPDX-License-Identifier: MIT-0
 *
 * This contains the register defintions for SoC FPGA timer
 */

/*
    File: Timer Registers

    This contains the register defintions for SoC FPGA timer.
*/

/*
    Constants: Timer constants

    SPTIMER0_BASE_ADDR                               - Value: 0x10C03000U.
    SPTIMER1_BASE_ADDR                               - Value: 0x10C03100U.
    OSC1_TIMER0_BASE_ADDR                            - Value: 0x10D00000U.
    OSC1_TIMER1_BASE_ADDR                            - Value: 0x10D00100U.
    TIMER_TIMER1LOADCOUNT                            - Timer1 Load Count Register.
    TIMER_TIMER1CURRENTVAL                           - Value: 0x0004U.
    TIMER_TIMER1CONTROLREG                           - Timer1 Control Register.
    TIMER_TIMER1EOI                                  - Timer1 End-of-Interrupt Register.
    TIMER_TIMER1INTSTAT                              - Timer1 Interrupt Status Register.
    TIMER_TIMERSINTSTAT                              - Timers Interrupt Status Register.
    TIMER_TIMERSEOI                                  - Timers End-of-Interrupt Register.
    TIMER_TIMERSRAWINTSTAT                           - Timers Raw Interrupt Status Register.
    TIMER_TIMERSCOMPVERSION                          - Timers Component Version.
    TIMER_TIMER1LOADCOUNT_TIMER1LOADCOUNT_MASK       - Value: 0xFFFFFFFFU.
    TIMER_TIMER1LOADCOUNT_TIMER1LOADCOUNT_POS        - Value: 0U.
    TIMER_TIMER1CURRENTVAL_TIMER1CURRENTVAL_MASK     - Value: 0xFFFFFFFFU.
    TIMER_TIMER1CURRENTVAL_TIMER1CURRENTVAL_POS      - Value: 0U.
    TIMER_TIMER1CONTROLREG_TIMER_INTERRUPT_MASK_MASK - Value: 0x00000004U.
    TIMER_TIMER1CONTROLREG_TIMER_INTERRUPT_MASK_POS  - Value: 2U.
    TIMER_TIMER1CONTROLREG_TIMER_MODE_MASK           - Value: 0x00000002U.
    TIMER_TIMER1CONTROLREG_TIMER_MODE_POS            - Value: 1U.
    TIMER_TIMER1CONTROLREG_TIMER_ENABLE_MASK         - Value: 0x00000001U.
    TIMER_TIMER1CONTROLREG_TIMER_ENABLE_POS          - Value: 0U.
    TIMER_TIMER1EOI_TIMER1EOI_MASK                   - Value: 0x00000001U.
    TIMER_TIMER1EOI_TIMER1EOI_POS                    - Value: 0U.
    TIMER_TIMER1INTSTAT_TIMER1INTSTAT_MASK           - Value: 0x00000001U.
    TIMER_TIMER1INTSTAT_TIMER1INTSTAT_POS            - Value: 0U.
    TIMER_TIMERSINTSTAT_TIMERSINTSTAT_MASK           - Value: 0x00000001U.
    TIMER_TIMERSINTSTAT_TIMERSINTSTAT_POS            - Value: 0U.
    TIMER_TIMERSEOI_TIMERSEOI_MASK                   - Value: 0x00000001U.
    TIMER_TIMERSEOI_TIMERSEOI_POS                    - Value: 0U.
    TIMER_TIMERSRAWINTSTAT_TIMERSRAWINTSTAT_MASK     - Value: 0x00000001U.
    TIMER_TIMERSRAWINTSTAT_TIMERSRAWINTSTAT_POS      - Value: 0U.
    TIMER_TIMERSCOMPVERSION_TIMERSCOMPVERSION_MASK   - Value: 0xFFFFFFFFU.
    TIMER_TIMERSCOMPVERSION_TIMERSCOMPVERSION_POS    - Value: 0U.
*/

#ifndef __SOCFPGA_TIMER_REG_H__
#define __SOCFPGA_TIMER_REG_H__

/* Timer Instances Base Address */
#define SPTIMER0_BASE_ADDR                                  0x10C03000U
#define SPTIMER1_BASE_ADDR                                  0x10C03100U
#define OSC1_TIMER0_BASE_ADDR                               0x10D00000U
#define OSC1_TIMER1_BASE_ADDR                               0x10D00100U

/* Timer1 Load Count Register */
#define TIMER_TIMER1LOADCOUNT                               0x0000U
/* TIMER1CURRENTVAL */
#define TIMER_TIMER1CURRENTVAL                              0x0004U
/* Timer1 Control Register */
#define TIMER_TIMER1CONTROLREG                              0x0008U
/* Timer1 End-of-Interrupt Register */
#define TIMER_TIMER1EOI                                     0x000CU
/* Timer1 Interrupt Status Register */
#define TIMER_TIMER1INTSTAT                                 0x0010U
/* Timers Interrupt Status Register */
#define TIMER_TIMERSINTSTAT                                 0x00A0U
/* Timers End-of-Interrupt Register */
#define TIMER_TIMERSEOI                                     0x00A4U
/* Timers Raw Interrupt Status Register */
#define TIMER_TIMERSRAWINTSTAT                              0x00A8U
/* Timers Component Version */
#define TIMER_TIMERSCOMPVERSION                             0x00ACU

/* Bit fields of TIMER1LOADCOUNT register */
#define TIMER_TIMER1LOADCOUNT_TIMER1LOADCOUNT_MASK          0xFFFFFFFFU
#define TIMER_TIMER1LOADCOUNT_TIMER1LOADCOUNT_POS           0U

/* Bit fields of TIMER1CURRENTVAL register */
#define TIMER_TIMER1CURRENTVAL_TIMER1CURRENTVAL_MASK        0xFFFFFFFFU
#define TIMER_TIMER1CURRENTVAL_TIMER1CURRENTVAL_POS         0U

/* Bit fields of TIMER1CONTROLREG register */
#define TIMER_TIMER1CONTROLREG_TIMER_INTERRUPT_MASK_MASK    0x00000004U
#define TIMER_TIMER1CONTROLREG_TIMER_INTERRUPT_MASK_POS     2U
#define TIMER_TIMER1CONTROLREG_TIMER_MODE_MASK              0x00000002U
#define TIMER_TIMER1CONTROLREG_TIMER_MODE_POS               1U
#define TIMER_TIMER1CONTROLREG_TIMER_ENABLE_MASK            0x00000001U
#define TIMER_TIMER1CONTROLREG_TIMER_ENABLE_POS             0U

/* Bit fields of TIMER1EOI register */
#define TIMER_TIMER1EOI_TIMER1EOI_MASK                      0x00000001U
#define TIMER_TIMER1EOI_TIMER1EOI_POS                       0U

/* Bit fields of TIMER1INTSTAT register */
#define TIMER_TIMER1INTSTAT_TIMER1INTSTAT_MASK              0x00000001U
#define TIMER_TIMER1INTSTAT_TIMER1INTSTAT_POS               0U

/* Bit fields of TIMERSINTSTAT register */
#define TIMER_TIMERSINTSTAT_TIMERSINTSTAT_MASK              0x00000001U
#define TIMER_TIMERSINTSTAT_TIMERSINTSTAT_POS               0U

/* Bit fields of TIMERSEOI register */
#define TIMER_TIMERSEOI_TIMERSEOI_MASK                      0x00000001U
#define TIMER_TIMERSEOI_TIMERSEOI_POS                       0U

/* Bit fields of TIMERSRAWINTSTAT register */
#define TIMER_TIMERSRAWINTSTAT_TIMERSRAWINTSTAT_MASK        0x00000001U
#define TIMER_TIMERSRAWINTSTAT_TIMERSRAWINTSTAT_POS         0U

/* Bit fields of TIMERSCOMPVERSION register */
#define TIMER_TIMERSCOMPVERSION_TIMERSCOMPVERSION_MASK      0xFFFFFFFFU
#define TIMER_TIMERSCOMPVERSION_TIMERSCOMPVERSION_POS       0U

#endif
