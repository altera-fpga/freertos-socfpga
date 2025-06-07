/*
 * SPDX-FileCopyrightText: Copyright (C) 2025 Altera Corporation
 *
 * SPDX-License-Identifier: MIT-0
 *
 * Header file for reset manager driver
 */

#ifndef __SOCFPGA_RST_MNGR_H__
#define __SOCFPGA_RST_MNGR_H__

/**
 * @file    socfpga_rst_mngr.h
 * @brief   This file contains all the Reset Manager HAL API definitions
 */


#include <stdint.h>

/**
 * @defgroup rst Reset Manager
 * @ingroup drivers
 * @{
 */

/**
 * @defgroup rst_fns Functions
 * @ingroup rst
 * Reset Manager HAL APIs
 */

/**
 * @defgroup rst_enums Enumerations
 * @ingroup rst
 * Reset Manager Specific Enumerations
 */

#include <stdint.h>

/**
 * Return values used by reset driver
 */
#define RSTMGR_SUCCESS          (0)                       /**< Reset operation completed successfully. */
#define RSTMGR_INVALID_VALUE    (1)                       /**< At least one parameter is invalid. */

/**
 * @brief enum for the peripheral modules
 */
typedef enum
{
    RST_PERIPHERAL_START, /**<  STARTING OF THE HARDWARE PERIPHERAL IDS*/
    RST_TSN0 = RST_PERIPHERAL_START,
    RST_TSN1,
    RST_TSN2,
    RST_USB0,
    RST_USB1,
    RST_NAND,
    RST_SOFTPHY,
    RST_SDMMC,
    RST_TSN0ECC,
    RST_TSN1ECC,
    RST_TSN2ECC,
    RST_USB0ECC,
    RST_USB1ECC,
    RST_NANDECC,
    RST_RSRVD1,
    RST_SDMMCECC,
    RST_DMA,
    RST_SPIM0,
    RST_SPIM1,
    RST_SPIS0,
    RST_SPIS1,
    RST_DMAECC,
    RST_EMACPTP,
    RST_RSRVD2,
    RST_DMAIF0,
    RST_DMAIF1,
    RST_DMAIF2,
    RST_DMAIF3,
    RST_DMAIF4,
    RST_DMAIF5,
    RST_DMAIF6,
    RST_DMAIF7,
    RST_WATCHDOG0,
    RST_WATCHDOG1,
    RST_WATCHDOG2,
    RST_WATCHDOG3,
    RST_L4SYSTIMER0,
    RST_L4SYSTIMER1,
    RST_SPTIMER0,
    RST_SPTIMER1,
    RST_I2C0,
    RST_I2C1,
    RST_I2C2,
    RST_I2C3,
    RST_I2C4,
    RST_I3C0,
    RST_I3C1,
    RST_RSRVD3,
    RST_UART0,
    RST_UART1,
    RST_RSRVD4,
    RST_RSRVD5,
    RST_RSRVD6,
    RST_RSRVD7,
    RST_RSRVD8,
    RST_RSRVD9,
    RST_GPIO0,
    RST_GPIO1,
    RST_WATCHDOG4,
    RST_RSRVD10,
    RST_RSRVD11,
    RST_RSRVD12,
    RST_RSRVD13,
    RST_RSRVD14,
    RST_SOC2FPGA_BRIDGE,
    RST_LWSOC2FPGA_BRIDGE,
    RST_FPGA2SOC_BRIDGE,
    RST_FPGA2SDRAM_BRIDGE,
    RST_PERIPHERAL_END = RST_FPGA2SDRAM_BRIDGE/**<  Ending of the hardware peripheral ids*/
} ResetPeriphrl_t;

/**
 * @defgroup Reset Manager HAL APIs
 * @{
 */

/**
 * @brief   rstmgr_assert_reset is used to assert reset to a hardware peripheral.
 *          The application can call this function to keep the peripheral mentioned in inactive state.
 *
 * @param[in]   per The peripheral to assert the reset.
 *
 * @return
 *   - RSTMGR_SUCCESS on success
 *   - RSTMGR_INVALID_VALUE if per is invalid
 */
int32_t rstmgr_assert_reset( ResetPeriphrl_t per );

/**
 * @brief   rstmgr_deassert_reset is used to activate a hardware peripheral.
 *          The application must call this function to activate the mentioned peripheral to use.
 *
 * @param[in]   per The peripheral to deassert the reset.
 *
 * @return
 *   - RSTMGR_SUCCESS on success
 *   - RSTMGR_INVALID_VALUE if per is invalid
 */
int32_t rstmgr_deassert_reset( ResetPeriphrl_t per );

/**
 * @brief   rstmgr_toggle_reset is used to toggle the activation state of a hardware peripheral.
 *          The application can call this function to restart the hardware peripheral.
 *
 * @param[in]   per The peripheral to restart.
 *
 * @return
 *   - RSTMGR_SUCCESS on success
 *   - RSTMGR_INVALID_VALUE if per is invalid
 */
int32_t rstmgr_toggle_reset( ResetPeriphrl_t per );

/**
 * @brief   rstmgr_get_reset_status is used to get the state of a hardware peripheral.
 *          The application can call this function to get the status of the hardware peripheral.
 *
 * @param[in]   per The peripheral to restart.
 * @param[out]  status The status of the peripheral.
 *
 * @return
 *   - RSTMGR_SUCCESS on success
 *   - RSTMGR_INVALID_VALUE if per is invalid
 */
int32_t rstmgr_get_reset_status( ResetPeriphrl_t per, uint8_t *status );

#endif /* __SOCFPGA_RST_MNGR_H__ */
