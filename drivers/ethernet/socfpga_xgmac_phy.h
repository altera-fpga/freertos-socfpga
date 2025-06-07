/*
 * SPDX-FileCopyrightText: Copyright (C) 2025 Altera Corporation
 *
 * SPDX-License-Identifier: MIT-0
 *
 * Header file for PHY HAL driver
 */

#ifndef __SOCFPGA_XGMAC_PHY_H__
#define __SOCFPGA_XGMAC_PHY_H__


#ifdef __cplusplus
extern "C" {
#endif

#include "socfpga_xgmac.h"
#include "socfpga_xgmac_phy_ll.h"


/**
 * @brief  Configuration structure for XGMAC PHY Parameters
 */
typedef struct
{
    uint32_t phy_address;
    uint32_t phy_identifier;
    uint8_t phy_interface;
    bool enable_autonegotiation;
    uint32_t speed_mbps;
    uint8_t duplex;
    uint8_t advertise;
    bool link_status;
    bool async_pause;
    bool pause;

}XGMACPhyConfig_t;

/**
 * @brief Detects presence of phy. This function is called during network interface init.
 *
 * @param[in] pXGMACHandle The handle of initialized xgmac.
 * @param[out] pxPhyConfig The configuration structure which gets populated once the phy is
 *             discovered.
 *
 * @return
 * - SOCFPGA_HAL_XGMAC_SUCCESS, when phy is detected
 * - SOCFPGA_HAL_XGMAC_ERROR, when phy is not detected
 */
int32_t SocfpgaXGMAC_PhyDiscover(SocfpgaXGMACHandle_t pXGMACHandle,
        XGMACPhyConfig_t *pxPhyConfig);

/**
 * @brief Initialize the phy . This function is called after the phy discovery.
 *
 * @param[in] pXGMACHandle The handle of initialized xgmac.
 * @param[in] pxPhyConfig The configuration structure which will be used to configure the phy
 *            and check phy link status.
 *
 * @return
 * - SOCFPGA_HAL_XGMAC_SUCCESS, when phy is initialized
 * - SOCFPGA_HAL_XGMAC_ERROR, when phy is not configured or the link is down.
 */
int32_t SocfpgaXGMAC_PhyInitialize(
    SocfpgaXGMACHandle_t pXGMACHandle, XGMACPhyConfig_t *pxPhyConfig);

/**
 * @brief Configure the speed and duplex on xgmac side. This function takes values from phy
 * registers and update the xgmac configuration according to it.
 *
 * @param[in] pXGMACHandle The handle of initialized xgmac.
 * @param[out] pxPhyConfig The configuration structure which will be populated according to the configured values.
 *
 * @return
 * - SOCFPGA_HAL_XGMAC_SUCCESS, when xgmac is configured succesfully
 * - SOCFPGA_HAL_XGMAC_FAILURE, when xgmac is not configured due to usupported speed or mode.
 */
int32_t SocfpgaXGMAC_CfgSpeedMode(
    SocfpgaXGMACHandle_t pXGMACHandle, XGMACPhyConfig_t *pxPhyConfig);

/**
 * @brief Reconfigures the phy as well as xgmac . This function updates the configuration of phy followed by
 *        checking the link status after phy reconfiguration and update the configuration on xgmac side.
 *
 * @param[in] pXGMACHandle The handle of initialized xgmac.
 * @param[in] pxPhyConfig The configuration structure which will be used to reconfigure the phy as well as
 *            speed and mode on xgmac side.
 *
 * @return
 * - SOCFPGA_HAL_XGMAC_SUCCESS, when phy is initialized
 * - SOCFPGA_HAL_XGMAC_ERROR, when phy or xgmac is failed to reconfigure or the link is down after phy reconfig.
 */
int32_t SocfpgaXGMAC_UpdateXGMACSpeedMode(
    SocfpgaXGMACHandle_t pXGMACHandle, XGMACPhyConfig_t *pxPhyConfig);




#ifdef __cplusplus
}
#endif

#endif /* ifndef __SOCFPGA_XGMAC_PHY_H__ */
