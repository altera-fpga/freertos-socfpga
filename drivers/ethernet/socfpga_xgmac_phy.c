/*
 * FreeRTOS+TCP V3.1.0
 * Copyright (C) 2022 Amazon.com, Inc. or its affiliates.  All Rights Reserved.
 *
 * SPDX-License-Identifier: MIT
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * http://aws.amazon.com/freertos
 * http://www.FreeRTOS.org
 */

/*
 * SPDX-FileCopyrightText: Copyright (C) 2025 Altera Corporation
 *
 * SPDX-License-Identifier: MIT-0
 *
 * HAL driver implementation for PHY
 */


#include "socfpga_xgmac_phy.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

/* Driver includes */
#include "socfpga_xgmac.h"
#include "socfpga_xgmac_reg.h"
#include "socfpga_xgmac_phy_ll.h"
#include "socfpga_xgmac_ll.h"
#include <osal_log.h>

#define NEGOTIATE_1G_MODE    0

static Basetype_t PHY_SetParameters(uint32_t *pulBaseAddress,
        XGMACPhyConfig_t *pxPhyConfig);
static Basetype_t PHY_LinkStatus(uint32_t *pulBaseAddress,
        XGMACPhyConfig_t *pxPhyConfig);
static Basetype_t PHY_SetAdvCap(uint32_t *pulBaseAddress,
        XGMACPhyConfig_t *pxPhyConfig);
static Basetype_t PHY_Reset(uint32_t *pulBaseAddress,
        XGMACPhyConfig_t *pxPhyConfig);

/* Function to check the validity of the PHY ID */
static bool Is_PHY_ID_Valid(uint32_t ulregVal)
{
    if ((ulregVal != (uint16_t) ~0U) && (ulregVal != ( uint16_t ) 0U))
    {
        return true;
    }
    return false;
}

/* PHY Detect function */
int32_t SocfpgaXGMAC_PhyDiscover(SocfpgaXGMACHandle_t pXGMACHandle,
        XGMACPhyConfig_t *pxPhyConfig)
{
    uint16_t aulPhyReg[2] = {0};
    uint32_t ulPhyAddress = PHY_MIN_ADDRESS;

    XGMAC_BaseAddr_t *pXgmacBaseAddr = pXGMACHandle->pXGMACInstBaseAddr;

    /* Iterate over possible PHY addresses */
    while (ulPhyAddress <= PHY_MAX_ADDRESS)
    {
        /* Read both PHY identifier registers */
        aulPhyReg[0] = read_phy_reg(pXgmacBaseAddr, ulPhyAddress, PHYID1_REG);
        if (Is_PHY_ID_Valid(aulPhyReg[0]))
        {
            aulPhyReg[1] =
                    read_phy_reg(pXgmacBaseAddr, ulPhyAddress, PHYID2_REG);

            /* Update structure members */
            pxPhyConfig->phy_identifier = (aulPhyReg[1] << 16) | aulPhyReg[0];
            pxPhyConfig->phy_address = ulPhyAddress;

            INFO(
                    "PHY at address %d with PHY Identifier2: 0x%04X and PHY identifier3 : 0x%04X.",
                    pxPhyConfig->phy_address, aulPhyReg[0], aulPhyReg[1]);

            INFO("Detected PHY at address %d with ID 0x%08X.",
                    pxPhyConfig->phy_address, pxPhyConfig->phy_identifier);

            return 0;
        }
        ulPhyAddress++;
    }
    ERROR("No PHY detected.");
    return -EIO;
}

int32_t SocfpgaXGMAC_PhyInitialize(
        SocfpgaXGMACHandle_t pXGMACHandle, XGMACPhyConfig_t *pxPhyConfig)
{
    Basetype_t xRetVal;

    /* Retrieve the base address of the XGMAC instance */
    XGMAC_BaseAddr_t *pXgmacBaseAddr = pXGMACHandle->pXGMACInstBaseAddr;

    /* Configure PHY parameters */
    xRetVal = PHY_SetParameters(pXgmacBaseAddr, pxPhyConfig);
    if (xRetVal != true)
    {
        ERROR("XGMAC PHY: Set PHY parameter failed.");
        return -EINVAL;
    }

    /* Check PHY link status */
    xRetVal = PHY_LinkStatus(pXgmacBaseAddr, pxPhyConfig);
    if (xRetVal == true) {
        INFO("PHY link is up.");
    }
    else{
        INFO("PHY link is down.");
        return -EIO;

    }
    return 0;
}


int32_t SocfpgaXGMAC_CfgSpeedMode(
        SocfpgaXGMACHandle_t pXGMACHandle, XGMACPhyConfig_t *pxPhyConfig)
{
    uint32_t usVal;
    XGMAC_BaseAddr_t *xgmacBaseAddr = pXGMACHandle->pXGMACInstBaseAddr;


    /* Read 1G status register */
    usVal = read_phy_reg(xgmacBaseAddr,pxPhyConfig->phy_address,
            STATUS_1GBASE_T_REG);
    if (usVal & STATUS_1GBASE_TX_ALLDPLX_MASK)
    {
        pxPhyConfig->speed_mbps = ETH_SPEED_1000_MBPS;

        if (usVal & STATUS_1GBASE_TX_HALFDPLX_MASK)
            pxPhyConfig->duplex = PHY_HALF_DUPLEX;
        else
            pxPhyConfig->duplex = PHY_FULL_DUPLEX;
    }
    else
    {
        /* Read copper status register for speed 100/10 */
        usVal = read_phy_reg(xgmacBaseAddr,pxPhyConfig->phy_address,
                COPPER_STATUS_REG);
        if (usVal & COPPER_STATUS_100BASE_TX_ALLDPLX_MASK)
        {
            pxPhyConfig->speed_mbps = ETH_SPEED_100_MBPS;
            if ((usVal & (1 << FULL_DUPLEX_100M)) == (1 << FULL_DUPLEX_100M))
            {
                pxPhyConfig->duplex = PHY_FULL_DUPLEX;
            }
            else if ((usVal & (1 << HALF_DUPLEX_100M)) ==
                    (1 << HALF_DUPLEX_100M))
            {
                pxPhyConfig->duplex = PHY_HALF_DUPLEX;
            }
            else
            {
                ERROR("Unsupported Mode.");
                return -EIO;
            }
        }
        else if (usVal & COPPER_STATUS_10BASE_TX_ALLDPLX_MASK)
        {
            pxPhyConfig->speed_mbps = ETH_SPEED_10_MBPS;
            if (usVal & COPPER_STATUS_10BASE_TX_HALFDPLC_MASK)
                pxPhyConfig->duplex = PHY_HALF_DUPLEX;
            else
                pxPhyConfig->duplex = PHY_FULL_DUPLEX;
        }
    }

    if ((pxPhyConfig->speed_mbps != ETH_SPEED_10_MBPS) &&
            (pxPhyConfig->speed_mbps != ETH_SPEED_100_MBPS) &&
            (pxPhyConfig->speed_mbps != ETH_SPEED_1000_MBPS)) {

        ERROR("Unsupported speed %u Mbps.", pxPhyConfig->speed_mbps);

        return -EIO;
    }

    /* Configure speed on XGMAC side */
    xgmac_setspeed(xgmacBaseAddr, pxPhyConfig->speed_mbps);

    if ((pxPhyConfig->duplex != PHY_FULL_DUPLEX) &&
            (pxPhyConfig->duplex != PHY_HALF_DUPLEX)) {

        ERROR("Invalid mode specified.");

        return -EINVAL;
    }

    /* Configure mode on XGMAC side */
    xgmac_setduplex(xgmacBaseAddr, pxPhyConfig->duplex);

    /* Successful configuration */
    INFO("Successfully configured XGMAC: Speed = %u Mbps, Mode = %s.",
            pxPhyConfig->speed_mbps,
            (pxPhyConfig->duplex ==
            PHY_FULL_DUPLEX) ? "Full Duplex" : "Half Duplex.");

    return 0;
}
int32_t SocfpgaXGMAC_UpdateXGMACSpeedMode(
        SocfpgaXGMACHandle_t pXGMACHandle, XGMACPhyConfig_t *pxPhyConfig)
{
    XGMAC_BaseAddr_t *xgmacBaseAddr = pXGMACHandle->pXGMACInstBaseAddr;

    /* Set PHY parameters */
    if (PHY_SetParameters(xgmacBaseAddr,pxPhyConfig ) != true)
    {
        ERROR("Failed to do PHY Re-Configuration");
        return -EIO;
    }
    /* Check PHY link status after PHY Reset */
    if (PHY_LinkStatus(xgmacBaseAddr,pxPhyConfig ) != true)
    {
        ERROR("PHY link is down.");
        return -EIO;
    }
    /* Configure updated parameters on XGMAC side */
    if (SocfpgaXGMAC_CfgSpeedMode(pXGMACHandle,
            pxPhyConfig) != 0)
    {
        ERROR("Failed to Re-Configure XGMAC Speed and Mode.");
        return -EIO;
    }
    return 0;
}

static Basetype_t PHY_SetParameters(uint32_t *pulBaseAddress,
        XGMACPhyConfig_t *pxPhyConfig)
{
    uint32_t ulData;
    Basetype_t xRetVal;
    uint8_t ucMaxCount;
    uint16_t cont_reg2;

    /* Check the selected PHY interface */
    if ((pxPhyConfig->phy_interface & PHY_IF_SELECT_RGMII)) {

        /* Select page from page address register */
        ulData = write_phy_reg(pulBaseAddress, pxPhyConfig->phy_address,
                PAGE_ADDRESS_SELECT_REG,SELECT_PAGE_EIGHTEEN);

        /*Set Interface mode in General control register and changes
         * to this mode bit should be needed to reset the General PHY
         */
        ulData = read_phy_reg(pulBaseAddress, pxPhyConfig->phy_address,
                GENERAL_CONTROL_REG_1);
        ulData &= ~GENERAL_CONTROL_RGMII_COPPER_SELECT_MASK;
        write_phy_reg(pulBaseAddress, pxPhyConfig->phy_address,
                GENERAL_CONTROL_REG_1, ulData);

        ulData &= ~GENERAL_CONTROL_RESET_MASK;
        write_phy_reg(pulBaseAddress, pxPhyConfig->phy_address,
                GENERAL_CONTROL_REG_1, ulData);

        write_phy_reg(pulBaseAddress, pxPhyConfig->phy_address,
                PAGE_ADDRESS_SELECT_REG, SELECT_PAGE_TWO);

        cont_reg2 = read_phy_reg(pulBaseAddress, pxPhyConfig->phy_address,
                MAC_SPECIFIC_CONTROL_REG_2);
        cont_reg2 &= ~(3 << 4);
        write_phy_reg(pulBaseAddress, pxPhyConfig->phy_address,
                MAC_SPECIFIC_CONTROL_REG_2, cont_reg2);

        /* Poll the reset bit to complete. */
        while (((ulData & GENERAL_CONTROL_RESET_MASK) != 0) &&
                (ucMaxCount++ < MAX_GEN_TIMER_COUNT)) {
            ulData = read_phy_reg(pulBaseAddress, pxPhyConfig->phy_address,
                    GENERAL_CONTROL_REG_1);
        }
        if (ucMaxCount == MAX_GEN_TIMER_COUNT) {
            ERROR("General PHY reset address timed out:%0d.", ucMaxCount);
            return -EIO;
        }
    }
    else{
        ERROR("Unsupported interface type selected.");
        return -EINVAL;
    }

    /* Select the speed -> If auto-negotiation not enable set the speed
     * manually in copper control register
     * bits [6][13] -> 00 -> 10MBPS
     *		[6][13] -> 01 -> 100MBPS
     *		[6][13] -> 10 -> 1000MBPS
     *		[6][13] -> 11 -> Reserved
     */
    write_phy_reg(pulBaseAddress, pxPhyConfig->phy_address,
            PAGE_ADDRESS_SELECT_REG, SELECT_PAGE_ZERO);

    /* Auto-negotiation disabled */
    if (pxPhyConfig->enable_autonegotiation != ENABLE_AUTONEG)
    {
        /* Make sure auto-negotiation disabled for setting speed and mode manually
         */
        ulData = read_phy_reg(pulBaseAddress, pxPhyConfig->phy_address,
                COPPER_CONTROL_REG);

        ulData &= ~COPPER_CONTROL_AUTONEG_ENABLE_MASK;

        write_phy_reg(pulBaseAddress, pxPhyConfig->phy_address,
                COPPER_CONTROL_REG, ulData);

        ulData = read_phy_reg(pulBaseAddress, pxPhyConfig->phy_address,
                COPPER_CONTROL_REG);
        switch (pxPhyConfig->speed_mbps)
        {
        /* Select 10Mbps */
        case ETH_SPEED_1000_MBPS:
            ulData |= COPPER_SPEED_SELECT_1000MBPS_MASK;
            break;
        /* Select 100Mbps */
        case ETH_SPEED_100_MBPS:
            ulData |= COPPER_SPEED_SELECT_100MBPS_MASK;
            break;
        /* Select 10Mbps */
        case ETH_SPEED_10_MBPS:
            ulData |= COPPER_SPEED_SELECT_10MBPS_MASK;
            break;
        default:
            WARN("Unsupported speed configuration selected.");
            return -EIO;
        }
        if (pxPhyConfig->duplex == PHY_HALF_DUPLEX)
            ulData |= COPPER_CONTROL_FULLDPLX_MASK;
        else
            ulData &= ~COPPER_CONTROL_FULLDPLX_MASK;

        /* Write Mode and Speed to Copper Control Register */
        write_phy_reg(pulBaseAddress, pxPhyConfig->phy_address,
                COPPER_CONTROL_REG, ulData);

        /* Resetting the PHY to effect the changes made for speed and mode */
        xRetVal = PHY_Reset(pulBaseAddress, pxPhyConfig);
        if (xRetVal != true) {
            ERROR("Failed to reset the PHY to set speed and mode.");
            return false;
        }
    }
    /* Auto-negotiation is enabled, configure advertised features */
    else
    {
        if (PHY_SetAdvCap(pulBaseAddress,pxPhyConfig) != true)
        {
            ERROR("Speed Advertisement failed.");
            return false;
        }
        ulData = read_phy_reg(pulBaseAddress, pxPhyConfig->phy_address,
                COPPER_CONTROL_REG);

        ulData |=
                (COPPER_CONTROL_AUTONEG_ENABLE_MASK |
                COPPER_CONTROL_AUTONEG_RESET_MASK);
        ulData &= ~COPPER_CONTROL_ISOLATE_MASK;

        write_phy_reg(pulBaseAddress, pxPhyConfig->phy_address,
                COPPER_CONTROL_REG, ulData);

        /* Resetting the PHY to effect the changes made for speed and mode */
        xRetVal = PHY_Reset(pulBaseAddress, pxPhyConfig);
        if (xRetVal != true) {
            ERROR(
                    "Failed to reset the PHY to reset the auto-negotiation in CCR.");
        }
        ulData = read_phy_reg(pulBaseAddress, pxPhyConfig->phy_address,
                COPPER_STATUS_REG);
        while (!(ulData & COPPER_STATUS_AUTONEG_COMPLETE_MASK))
        {
            ulData = read_phy_reg(pulBaseAddress, pxPhyConfig->phy_address,
                    COPPER_STATUS_REG);
            for (int i =MIN_COUNT; i < MAX_COUNT_FOR_AUTONEG_COMPLETE; i++)
                ;
        }
        INFO("Auto negotiation process completed.");
    }
    return true;
}
static Basetype_t PHY_SetAdvCap(uint32_t *pulBaseAddress,
        XGMACPhyConfig_t *pxPhyConfig)
{
    uint32_t ulAdv = 0;

    /* Advertise all speed and mode */
    if (pxPhyConfig->advertise == ADVERTISE_ALL)
    {
        /*Enable 100/10 Full/Half duplex Advertisement*/
        ulAdv = AUTONEG_ADV_100_10_TX_ALLDPLX_MASK;
        write_phy_reg(pulBaseAddress, pxPhyConfig->phy_address,
                COPPER_AUTONEG_ADV_REG,ulAdv );

        /* Enable 1G Full/Half duplex Advertisement */
#if NEGOTIATE_1G_MODE
        ulAdv = AUTONEG_ADV_1GBASE_TX_ALLDPLX_MASK;
#else
        ulAdv = 0;
#endif
        write_phy_reg(pulBaseAddress, pxPhyConfig->phy_address,
                PHY_1GBASE_T_CONTROL_REG, ulAdv );
    }
    else if ((pxPhyConfig->advertise == ADVERTISE_ALL_TX_FULLDPLX) ||
            (pxPhyConfig->advertise == ADVERTISE_ALL_TX_HALFDPLX))
    {
        if (pxPhyConfig->advertise == ADVERTISE_ALL_TX_HALFDPLX)
        {

            /*Enable 100/10 Half duplex Advertisement*/
            ulAdv = AUTONEG_ADV_100_10_TX_HALFDPLX_MASK;
            write_phy_reg(pulBaseAddress, pxPhyConfig->phy_address,
                    COPPER_AUTONEG_ADV_REG,ulAdv );

#if NEGOTIATE_1G_MODE
            /* Enable 1G Half duplex Advertisement */
            ulAdv = AUTONEG_ADVERTISE_1GBASE_TX_HALFDPLX_MASK;
#else
            ulAdv = 0;
#endif
            write_phy_reg(pulBaseAddress, pxPhyConfig->phy_address,
                    PHY_1GBASE_T_CONTROL_REG,ulAdv );
        }
        else
        {
            /*Enable 100/10 Full duplex Advertisement*/
            ulAdv = AUTO_ADV_100_10_TX_FULLDPX_MASK;
            write_phy_reg(pulBaseAddress, pxPhyConfig->phy_address,
                    COPPER_AUTONEG_ADV_REG,ulAdv );

#if NEGOTIATE_1G_MODE
            /* Enable 1G Full duplex Advertisement */
            ulAdv = AUTONEG_ADVERTISE_100BASE_TX_FULLDPLX_MASK;
#else
            ulAdv = 0;
#endif
            write_phy_reg(pulBaseAddress, pxPhyConfig->phy_address,
                    PHY_1GBASE_T_CONTROL_REG,ulAdv );
        }
    }
    else if ((pxPhyConfig->advertise == ADVERTISE_1GBASE_TX_ALLDPLX) ||
            (pxPhyConfig->advertise == ADVERTISE_1GBASE_TX_FULLDPLX) ||
            (pxPhyConfig->advertise == ADVERTISE_1GBASE_TX_HALFDPLX))
    {
        if (pxPhyConfig->advertise == ADVERTISE_1GBASE_TX_ALLDPLX)
        {
            /* Enable 1G Full/Half duplex Advertisement */
            ulAdv = AUTONEG_ADV_1GBASE_TX_ALLDPLX_MASK;
            write_phy_reg(pulBaseAddress, pxPhyConfig->phy_address,
                    PHY_1GBASE_T_CONTROL_REG,ulAdv );
        }
        else if (pxPhyConfig->advertise == ADVERTISE_1GBASE_TX_FULLDPLX)
        {
            /* Enable 1G Full duplex Advertisement */
            ulAdv = AUTONEG_ADVERTISE_100BASE_TX_FULLDPLX_MASK;
            write_phy_reg(pulBaseAddress, pxPhyConfig->phy_address,
                    PHY_1GBASE_T_CONTROL_REG,ulAdv );
        }
        else
        {
            /* Enable 1G Half duplex Advertisement */
            ulAdv = AUTONEG_ADVERTISE_100BASE_TX_HALFDPLX_MASK;
            write_phy_reg(pulBaseAddress, pxPhyConfig->phy_address,
                    PHY_1GBASE_T_CONTROL_REG,ulAdv );
        }
    }
    else if ((pxPhyConfig->advertise == ADVERTISE_100_10_BASE_TX_ALLDPLX) ||
            (pxPhyConfig->advertise == ADVERTISE_100_10_BASE_TX_FULLDPLX) ||
            (pxPhyConfig->advertise == ADVERTISE_100_10_BASE_TX_HALFDPLX))
    {
        if (pxPhyConfig->advertise == ADVERTISE_100_10_BASE_TX_ALLDPLX)
        {
            /*Enable 100/10 Full/Half duplex Advertisement*/
            ulAdv = AUTONEG_ADV_100_10_TX_ALLDPLX_MASK;
            write_phy_reg(pulBaseAddress, pxPhyConfig->phy_address,
                    COPPER_AUTONEG_ADV_REG,ulAdv );
        }
        else if (pxPhyConfig->advertise == ADVERTISE_100_10_BASE_TX_FULLDPLX)
        {
            /*Enable 100/10 Full duplex Advertisement*/
            ulAdv = AUTO_ADV_100_10_TX_FULLDPX_MASK;
            write_phy_reg(pulBaseAddress, pxPhyConfig->phy_address,
                    COPPER_AUTONEG_ADV_REG,ulAdv );
        }
        else
        {
            /*Enable 100/10 Half duplex Advertisement*/
            ulAdv = AUTONEG_ADV_100_10_TX_HALFDPLX_MASK;
            write_phy_reg(pulBaseAddress, pxPhyConfig->phy_address,
                    COPPER_AUTONEG_ADV_REG,ulAdv );
        }
    }
    else
    {
        INFO("Unsupported advertisement.");
        return false;
    }
    if ((pxPhyConfig->pause == ENABLE_ASYNC_PAUSE) ||
            (pxPhyConfig->pause == ENABLE_MAC_PAUSE))
    {
        if (pxPhyConfig->async_pause == ENABLE_ASYNC_PAUSE)
        {
            /*Enable Asymmetric Pause frame */
            ulAdv = COPPER_AUTONEG_ASYNC_PAUSE_MASK;
            write_phy_reg(pulBaseAddress, pxPhyConfig->phy_address,
                    COPPER_AUTONEG_ADV_REG, ulAdv);
        }
        if (pxPhyConfig->pause == ENABLE_MAC_PAUSE) {

            /*Enable MAC Pause frame */
            ulAdv = COPPER_AUTONEG_PAUSE_MASK;
            write_phy_reg(pulBaseAddress, pxPhyConfig->phy_address,
                    COPPER_AUTONEG_ADV_REG, ulAdv);
        }
    }
    return true;
}
static Basetype_t PHY_Reset(uint32_t *pulBaseAddress,
        XGMACPhyConfig_t *pxPhyConfig)
{
    uint32_t ulData;
    uint8_t ucCnt = 0;

    /* Read the copper control register */
    write_phy_reg(pulBaseAddress, pxPhyConfig->phy_address,
            PAGE_ADDRESS_SELECT_REG, SELECT_PAGE_ZERO);
    ulData = read_phy_reg(pulBaseAddress, pxPhyConfig->phy_address,
            COPPER_CONTROL_REG);

    ulData |= COPPER_CONTROL_PHY_RESET_MASK;
    write_phy_reg(pulBaseAddress, pxPhyConfig->phy_address, COPPER_CONTROL_REG,
            ulData);

    /* Wait for PHY reset to complete */
    do{
        ulData = read_phy_reg(pulBaseAddress, pxPhyConfig->phy_address,
                COPPER_CONTROL_REG);

        ucCnt++;
        if (ucCnt >= MAX_GEN_TIMER_COUNT)
        {
            ERROR("PHY reset timeout reached.");
            return false;
        }

    }while(ulData & COPPER_CONTROL_PHY_RESET_MASK);     /* Check if the reset bit is still set */

    return true;
}

static Basetype_t PHY_LinkStatus(uint32_t *pulBaseAddress,
        XGMACPhyConfig_t *pxPhyConfig)
{
    uint8_t cRegister;
    uint16_t usMask;
    uint32_t ulData;
#if (REAL_TIME_LINK_STATUS == 1)
    cRegister = COPPER_SPECIFIC_STATUS_REG_1;
    usMask = COPPER_REAL_TIME_LINK_STATUS_MASK;
#else
    cRegister = COPPER_STATUS_REG;
    usMask = COPPER_LINK_STATUS_MASK;
#endif

    /* Read the copper status register */
    ulData = read_phy_reg(pulBaseAddress, pxPhyConfig->phy_address, cRegister);

    if (ulData & usMask)
    {
        pxPhyConfig->link_status = LINK_UP;
        return true;
    }
    else
    {
        pxPhyConfig->link_status = LINK_DOWN;
        return false;
    }
}
