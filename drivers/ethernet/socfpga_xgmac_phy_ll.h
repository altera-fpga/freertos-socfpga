/*
 * SPDX-FileCopyrightText: Copyright (C) 2025 Altera Corporation
 *
 * SPDX-License-Identifier: MIT-0
 *
 * Header file for SoC FPGA PHY low-level driver
 */

/*
    File: Ethernet XGMAC PHY Low-Level Driver

    Header file for SoC FPGA PHY low-level driver.
*/

/*
    Constants: Ethernet XGMAC PHY constants

    BIT                                        - Expands to (1UL << (nr)).
    PHY_MIN_ADDRESS                            - Value: 0U.
    PHY_MAX_ADDRESS                            - Value: 31U.
    COPPER_CONTROL_REG                         - Value: 0.
    COPPER_STATUS_REG                          - Value: 1.
    PHYID1_REG                                 - Value: 2.
    PHYID2_REG                                 - Value: 3.
    COPPER_AUTONEG_ADV_REG                     - Value: 4.
    COPPER_LINK_ABILITY_REG                    - Value: 5.
    PHY_1GBASE_T_CONTROL_REG                   - Value: 9.
    STATUS_1GBASE_T_REG                        - Value: 10.
    COPPER_SPECIFIC_STATUS_REG1                - Value: 17.
    GENERAL_CONTROL_REG_1                      - Value: 20.
    MAC_SPECIFIC_CONTROL_REG_2                 - Value: 21.
    PAGE_ADDRESS_SELECT_REG                    - Value: 22.
    PHY_ID_MARVELL                             - Value: 0x0DD10141.
    MMD_ACCESS_CONTROL_REG                     - Value: 13.
    MMD_ACCESS_DATA_REG                        - Value: 14.
    PHY_CONTROL_REG                            - Value: 31.
    PHY_ID_MICROCHIP                           - Value: 0x16220022.
    MICROCHIP_GTX_CLK_SKEW                     - Value: 18U.
    MICROCHIP_RX_CLK_SKEW                      - Value: 13U.
    SELECT_PAGE_ZERO                           - Value: 0.
    SELECT_PAGE_EIGHTEEN                       - Value: 0x12.
    SELECT_PAGE_TWO                            - Value: 2.
    MIN_COUNT                                  - Value: 0.
    MAX_GEN_TIMER_COUNT                        - Value: 100U.
    MAX_COUNT_FOR_AUTONEG_COMPLETE             - Value: 1000.
    FULL_DUPLEX_100M                           - Value: 14.
    HALF_DUPLEX_100M                           - Value: 13.
    COPPER_CONTROL_PHY_RESET_MASK              - Value: 0x8000U.
    COPPER_REAL_TIME_LINK_STATUS_MASK          - Value: 0x0400U.
    COPPER_LINK_STATUS_MASK                    - Value: 0x0004U.
    COPPER_SPEED_SELECT_1000MBPS_MASK          - Value: 0x0040U.
    COPPER_SPEED_SELECT_100MBPS_MASK           - Value: 0x2000U.
    COPPER_SPEED_SELECT_10MBPS_MASK            - Value: 0x0000U.
    COPPER_CONTROL_FULLDPLX_MASK               - Value: 0x0100U.
    COPPER_CONTROL_ISOLATE_MASK                - Value: 0x0400U.
    COPPER_CONTROL_AUTONEG_ENABLE_MASK         - Value: 0x1000U.
    COPPER_CONTROL_AUTONEG_RESET_MASK          - Value: 0x0200U.
    COPPER_STATUS_AUTONEG_COMPLETE_MASK        - Value: 0x0020U.
    MAC_RGMII_RXTX_DELAY_MASK                  - Value: 0x0030U.
    CSCR1_MDI_CROSSOVER_ENABLE_ALL_MASK        - Value: 0x0060U.
    COPPER_STATUS_100BASE_TX_ALLDPLX_MASK      - Value: 0x6000U.
    COPPER_STATUS_100BASE_TX_FULLDPLC_MASK     - Value: 0x4000U.
    COPPER_STATUS_100BASE_TX_HALFDPLC_MASK     - Value: 0x2000U.
    COPPER_STATUS_10BASE_TX_ALLDPLX_MASK       - Value: 0x1800U.
    COPPER_STATUS_10BASE_TX_FULLDPLC_MASK      - Value: 0x1000U.
    COPPER_STATUS_10BASE_TX_HALFDPLC_MASK      - Value: 0x0800U.
    STATUS_1GBASE_TX_ALLDPLX_MASK              - Value: 0x0C00U.
    STATUS_1GBASE_TX_FULLDPLX_MASK             - Value: 0x0800U.
    STATUS_1GBASE_TX_HALFDPLX_MASK             - Value: 0x0400U.
    COPPER_AUTONEG_ASYNC_PAUSE_MASK            - Value: 0x0800U.
    COPPER_AUTONEG_PAUSE_MASK                  - Value: 0x0400U.
    GENERAL_CONTROL_RGMII_COPPER_SELECT_MASK   - Value: 0x0007U.
    GENERAL_CONTROL_RESET_MASK                 - Value: 0x8000U.
    AUTONEG_ADVERTISE_1GBASE_TX_FULLDPLX_MASK  - Value: 0x0200U.
    AUTONEG_ADVERTISE_1GBASE_TX_HALFDPLX_MASK  - Value: 0x0100U.
    AUTONEG_ADVERTISE_100BASE_TX_FULLDPLX_MASK - Value: 0x0100U.
    AUTONEG_ADVERTISE_100BASE_TX_HALFDPLX_MASK - Value: 0x0080U.
    AUTONEG_ADVERTISE_10BASE_TX_FULLDPLX_MASK  - Value: 0x0040U.
    AUTONEG_ADVERTISE_10BASE_TX_HALFDPLX_MASK  - Value: 0x0020U.
    AUTONEG_ADV_1GBASE_TX_ALLDPLX_MASK         - Value: 0x0300U.
    AUTONEG_ADV_100_10_TX_ALLDPLX_MASK         - Value: 0x01E0U.
    AUTO_ADV_100_10_TX_FULLDPX_MASK            - Value: 0x0140U.
    AUTONEG_ADV_100_10_TX_HALFDPLX_MASK        - Value: 0x00A0U.
    REAL_TIME_LINK_STATUS                      - Value: 0U.
    ADVERTISE_1GBASE_TX_ALLDPLX                - Value: 22U.
    ADVERTISE_1GBASE_TX_FULLDPLX               - Value: 20U.
    ADVERTISE_1GBASE_TX_HALFDPLX               - Value: 18U.
    ADVERTISE_100_10_BASE_TX_ALLDPLX           - Value: 16U.
    ADVERTISE_100_10_BASE_TX_FULLDPLX          - Value: 14U.
    ADVERTISE_100_10_BASE_TX_HALFDPLX          - Value: 12U.
    ADVERTISE_100BASE_TX_ALLDPLX               - Value: 10U.
    ADVERTISE_100BASE_TX_FULLDPLX              - Value: 8U.
    ADVERTISE_100BASE_TX_HALFDPLX              - Value: 6U.
    ADVERTISE_10BASE_TX_ALLDPLX                - Value: 4U.
    ADVERTISE_10BASE_TX_FULLDPLX               - Value: 2U.
    ADVERTISE_10BASE_TX_HALFDPLX               - Value: 1U.
    ENABLE_ASYNC_PAUSE                         - Value: 1U.
    ENABLE_MAC_PAUSE                           - Value: 1U.
    ETH_PHY                                    - Expands to MARVEL.
*/

#ifndef __SOCFPGA_XGMAC_PHY_LL_H__
#define __SOCFPGA_XGMAC_PHY_LL_H__

#include <stdint.h>
#include <stdbool.h>

#include "socfpga_xgmac_phy.h"

#define BIT(nr) (1UL << (nr))

#define PHY_MIN_ADDRESS 0U
#define PHY_MAX_ADDRESS 31U

/* Define PHY Registers */
#define COPPER_CONTROL_REG 0
#define COPPER_STATUS_REG 1
#define PHYID1_REG 2
#define PHYID2_REG 3
#define COPPER_AUTONEG_ADV_REG 4
#define COPPER_LINK_ABILITY_REG 5
#define PHY_1GBASE_T_CONTROL_REG 9
#define STATUS_1GBASE_T_REG 10

/*Marvell Vendor Specific*/
#define COPPER_SPECIFIC_STATUS_REG1 17
#define GENERAL_CONTROL_REG_1 20
#define MAC_SPECIFIC_CONTROL_REG_2 21
#define PAGE_ADDRESS_SELECT_REG 22
#define PHY_ID_MARVELL 0x0DD10141

/* Microchip vendor specific*/
#define MMD_ACCESS_CONTROL_REG 13
#define MMD_ACCESS_DATA_REG 14
#define PHY_CONTROL_REG 31
#define PHY_ID_MICROCHIP 0x16220022
/* Values tuned for Microchip KSZ9031RNX PHY */
#define MICROCHIP_GTX_CLK_SKEW 18U
#define MICROCHIP_RX_CLK_SKEW 13U

/* Page numbers */
#define SELECT_PAGE_ZERO 0
#define SELECT_PAGE_EIGHTEEN 0x12
#define SELECT_PAGE_TWO 2
#define MIN_COUNT 0
#define MAX_GEN_TIMER_COUNT 100U
#define MAX_COUNT_FOR_AUTONEG_COMPLETE 1000
#define FULL_DUPLEX_100M 14
#define HALF_DUPLEX_100M 13

/* Define Mask fields */
#define COPPER_CONTROL_PHY_RESET_MASK 0x8000U
#define COPPER_REAL_TIME_LINK_STATUS_MASK 0x0400U
#define COPPER_LINK_STATUS_MASK 0x0004U
#define COPPER_SPEED_SELECT_1000MBPS_MASK 0x0040U
#define COPPER_SPEED_SELECT_100MBPS_MASK 0x2000U
#define COPPER_SPEED_SELECT_10MBPS_MASK 0x0000U
#define COPPER_CONTROL_FULLDPLX_MASK 0x0100U
#define COPPER_CONTROL_ISOLATE_MASK 0x0400U
#define COPPER_CONTROL_AUTONEG_ENABLE_MASK 0x1000U
#define COPPER_CONTROL_AUTONEG_RESET_MASK 0x0200U
#define COPPER_STATUS_AUTONEG_COMPLETE_MASK 0x0020U
#define MAC_RGMII_RXTX_DELAY_MASK 0x0030U
#define CSCR1_MDI_CROSSOVER_ENABLE_ALL_MASK 0x0060U

#define COPPER_STATUS_100BASE_TX_ALLDPLX_MASK 0x6000U
#define COPPER_STATUS_100BASE_TX_FULLDPLC_MASK 0x4000U
#define COPPER_STATUS_100BASE_TX_HALFDPLC_MASK 0x2000U

#define COPPER_STATUS_10BASE_TX_ALLDPLX_MASK 0x1800U
#define COPPER_STATUS_10BASE_TX_FULLDPLC_MASK 0x1000U
#define COPPER_STATUS_10BASE_TX_HALFDPLC_MASK 0x0800U

#define STATUS_1GBASE_TX_ALLDPLX_MASK 0x0C00U
#define STATUS_1GBASE_TX_FULLDPLX_MASK 0x0800U
#define STATUS_1GBASE_TX_HALFDPLX_MASK 0x0400U

#define COPPER_AUTONEG_ASYNC_PAUSE_MASK 0x0800U
#define COPPER_AUTONEG_PAUSE_MASK 0x0400U

#define GENERAL_CONTROL_RGMII_COPPER_SELECT_MASK 0x0007U
#define GENERAL_CONTROL_RESET_MASK 0x8000U

#define AUTONEG_ADVERTISE_1GBASE_TX_FULLDPLX_MASK 0x0200U
#define AUTONEG_ADVERTISE_1GBASE_TX_HALFDPLX_MASK 0x0100U

#define AUTONEG_ADVERTISE_100BASE_TX_FULLDPLX_MASK 0x0100U
#define AUTONEG_ADVERTISE_100BASE_TX_HALFDPLX_MASK 0x0080U
#define AUTONEG_ADVERTISE_10BASE_TX_FULLDPLX_MASK 0x0040U
#define AUTONEG_ADVERTISE_10BASE_TX_HALFDPLX_MASK 0x0020U

#define AUTONEG_ADV_1GBASE_TX_ALLDPLX_MASK 0x0300U
#define AUTONEG_ADV_100_10_TX_ALLDPLX_MASK 0x01E0U

#define AUTO_ADV_100_10_TX_FULLDPX_MASK 0x0140U
#define AUTONEG_ADV_100_10_TX_HALFDPLX_MASK 0x00A0U

#define REAL_TIME_LINK_STATUS 0U

#define ADVERTISE_1GBASE_TX_ALLDPLX 22U
#define ADVERTISE_1GBASE_TX_FULLDPLX 20U
#define ADVERTISE_1GBASE_TX_HALFDPLX 18U

#define ADVERTISE_100_10_BASE_TX_ALLDPLX 16U
#define ADVERTISE_100_10_BASE_TX_FULLDPLX 14U
#define ADVERTISE_100_10_BASE_TX_HALFDPLX 12U

#define ADVERTISE_100BASE_TX_ALLDPLX 10U
#define ADVERTISE_100BASE_TX_FULLDPLX 8U
#define ADVERTISE_100BASE_TX_HALFDPLX 6U

#define ADVERTISE_10BASE_TX_ALLDPLX 4U
#define ADVERTISE_10BASE_TX_FULLDPLX 2U
#define ADVERTISE_10BASE_TX_HALFDPLX 1U

#define ENABLE_ASYNC_PAUSE 1U
#define ENABLE_MAC_PAUSE 1U
#define ETH_PHY MARVEL

/* Function prototypes */
uint16_t read_phy_reg(uint32_t base_address, uint32_t phy_address, uint8_t phy_reg);

int8_t write_phy_reg(uint32_t base_address, uint32_t phy_address, uint8_t phy_reg, uint16_t reg_val);

BaseType_t phy_setup_microchip(uint32_t base_address, xgmac_phy_config_t *pphy_config);

BaseType_t phy_setup_marvell(uint32_t base_address, xgmac_phy_config_t *pphy_config);

void phy_cfg_link_microchip(uint32_t xgmac_base_address, xgmac_phy_config_t *pphy_config);

void phy_cfg_link_marvell(uint32_t xgmac_base_address, xgmac_phy_config_t *pphy_config);
#endif /* __SOCFPGA_XGMAC_PHY_LL_H__ */
