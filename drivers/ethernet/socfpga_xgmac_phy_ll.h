/*
 * SPDX-FileCopyrightText: Copyright (C) 2025 Altera Corporation
 *
 * SPDX-License-Identifier: MIT-0
 *
 * Header file for SoC FPGA PHY low level driver
 */

#ifndef __SOCFPGA_XGMAC_PHY_LL_H__
#define __SOCFPGA_XGMAC_PHY_LL_H__

#include <stdint.h>
#include <stdbool.h>
#define BIT(nr)    (1UL << (nr))


#define PHY_MIN_ADDRESS                  0
#define PHY_MAX_ADDRESS                  31

/* modes and speed */
#define PHY_FULL_DUPLEX                  1
#define PHY_HALF_DUPLEX                  2

/* Define PHY Registers */
#define COPPER_CONTROL_REG               0
#define COPPER_STATUS_REG                1
#define PHYID1_REG                       2
#define PHYID2_REG                       3
#define COPPER_AUTONEG_ADV_REG           4
#define COPPER_LINK_ABILITY_REG          5
#define PHY_1GBASE_T_CONTROL_REG         9
#define STATUS_1GBASE_T_REG              10
#define PAGE_ADDRESS_SELECT_REG          22
#define MAC_SPECIFIC_CONTROL_REG_2       21
#define COPPER_SPECIFIC_CONTROL_REG_1    16
#define GENERAL_CONTROL_REG_1            20


/* Pages numbers */
#define SELECT_PAGE_ZERO                          0
#define SELECT_PAGE_EIGHTEEN                      0x12
#define SELECT_PAGE_TWO                           2
#define MIN_COUNT                                 0
#define MAX_GEN_TIMER_COUNT                       100
#define MAX_COUNT_FOR_AUTONEG_COMPLETE            1000
#define FULL_DUPLEX_100M                          14
#define HALF_DUPLEX_100M                          13

/* Define Mask fields */
#define COPPER_CONTROL_PHY_RESET_MASK             0x8000
#define COPPER_REAL_TIME_LINK_STATUS_MASK         0x0400
#define COPPER_LINK_STATUS_MASK                   0x0004
#define COPPER_SPEED_SELECT_1000MBPS_MASK         0x0040
#define COPPER_SPEED_SELECT_100MBPS_MASK          0x2000
#define COPPER_SPEED_SELECT_10MBPS_MASK           0x0000
#define COPPER_CONTROL_FULLDPLX_MASK              0x0100
#define COPPER_CONTROL_ISOLATE_MASK               0x0400
#define COPPER_CONTROL_AUTONEG_ENABLE_MASK        0x1000
#define COPPER_CONTROL_AUTONEG_RESET_MASK         0x0200
#define COPPER_STATUS_AUTONEG_COMPLETE_MASK       0x0020
#define MAC_RGMII_RXTX_DELAY_MASK                 0x0030
#define CSCR1_MDI_CROSSOVER_ENABLE_ALL_MASK       0x0060

#define COPPER_STATUS_100BASE_TX_ALLDPLX_MASK     0x6000
#define COPPER_STATUS_100BASE_TX_FULLDPLC_MASK    0x4000
#define COPPER_STATUS_100BASE_TX_HALFDPLC_MASK    0x2000

#define COPPER_STATUS_10BASE_TX_ALLDPLX_MASK      0x1800
#define COPPER_STATUS_10BASE_TX_FULLDPLC_MASK     0x1000
#define COPPER_STATUS_10BASE_TX_HALFDPLC_MASK     0x0800

#define STATUS_1GBASE_TX_ALLDPLX_MASK             0x0C00
#define STATUS_1GBASE_TX_FULLDPLX_MASK            0x0800
#define STATUS_1GBASE_TX_HALFDPLX_MASK            0x0400




#define COPPER_AUTONEG_ASYNC_PAUSE_MASK               0x0800
#define COPPER_AUTONEG_PAUSE_MASK                     0x0400

#define GENERAL_CONTROL_RGMII_COPPER_SELECT_MASK      0x0007
#define GENERAL_CONTROL_RESET_MASK                    0x8000

#define AUTONEG_ADVERTISE_1GBASE_TX_FULLDPLX_MASK     0x0200
#define AUTONEG_ADVERTISE_1GBASE_TX_HALFDPLX_MASK     0x0100

#define AUTONEG_ADVERTISE_100BASE_TX_FULLDPLX_MASK    0x0100
#define AUTONEG_ADVERTISE_100BASE_TX_HALFDPLX_MASK    0x0080
#define AUTONEG_ADVERTISE_10BASE_TX_FULLDPLX_MASK     0x0040
#define AUTONEG_ADVERTISE_10BASE_TX_HALFDPLX_MASK     0x0020

#define AUTONEG_ADV_1GBASE_TX_ALLDPLX_MASK            0x0300
#define AUTONEG_ADV_100_10_TX_ALLDPLX_MASK            0x01E0

#define AUTO_ADV_100_10_TX_FULLDPX_MASK               0x0140
#define AUTONEG_ADV_100_10_TX_HALFDPLX_MASK           0x00A0


#define ETH_SPEED_1000_MBPS                           1000
#define ETH_SPEED_100_MBPS                            100
#define ETH_SPEED_10_MBPS                             10
#define PHY_IF_SELECT_RGMII                           1
#define PHY_IF_SELECT_SGMII                           2
#define ENABLE_AUTONEG                                1
#define DISABLE_AUTONEG                               0
#define LINK_UP                                       1
#define LINK_DOWN                                     0
#define REAL_TIME_LINK_STATUS                         0


#define ADVERTISE_1GBASE_TX_ALLDPLX                   22
#define ADVERTISE_1GBASE_TX_FULLDPLX                  20
#define ADVERTISE_1GBASE_TX_HALFDPLX                  18


#define ADVERTISE_100_10_BASE_TX_ALLDPLX              16
#define ADVERTISE_100_10_BASE_TX_FULLDPLX             14
#define ADVERTISE_100_10_BASE_TX_HALFDPLX             12

#define ADVERTISE_100BASE_TX_ALLDPLX                  10
#define ADVERTISE_100BASE_TX_FULLDPLX                 8
#define ADVERTISE_100BASE_TX_HALFDPLX                 6

#define ADVERTISE_10BASE_TX_ALLDPLX                   4
#define ADVERTISE_10BASE_TX_FULLDPLX                  2
#define ADVERTISE_10BASE_TX_HALFDPLX                  1

#define ADVERTISE_ALL                                 1
#define ADVERTISE_ALL_TX_FULLDPLX                     2
#define ADVERTISE_ALL_TX_HALFDPLX                     4

#define ENABLE_ASYNC_PAUSE                            1
#define ENABLE_MAC_PAUSE                              1

/* Function prototypes */
int16_t read_phy_reg(uint32_t *baseAddress, uint32_t phyAddress,
        uint8_t phyReg);
int8_t write_phy_reg(uint32_t *baseAddress, uint32_t phyAddress, uint8_t phyReg,
        uint16_t regVal);

#endif
