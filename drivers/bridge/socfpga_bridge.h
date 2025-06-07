/*
 * SPDX-FileCopyrightText: Copyright (C) 2025 Altera Corporation
 *
 * SPDX-License-Identifier: MIT-0
 *
 * Header file for fpga bridge HAL driver
 */

#ifndef __SOCFPGA_BRIDGE_H__
#define __SOCFPGA_BRIDGE_H__

#define RET_SUCCESS ( 1)    /*!< Bridge operation completed successfully.   */
#define RET_FAIL    (-1)    /*!< Error occured during the bridge operation. */

/*
 * @func    enable_hps2fpga_bridge
 * @brief   enables the hps2fpga bridge
 *          The application should call this api to enable the HPS2FPGA bridge
 *          before any bridge opreation is being performed.
 *
 * @return 
 * - RET_SUCCESS, if bridge enable operation is successful
 * - RET_FAIL,    if bridge enable operation fails or any timeout occurs
 */
int enable_hps2fpga_bridge(void);

/*
 * @func   disable_hps2fpga_bridge
 * @brief  disable the hps2fpga bridge
 *         The api disables the HPS2FPGA bridge. 
 *
 * @return 
 * - RET_SUCCESS, if bridge is disabled successfully
 * - RET_FAIL,    if bridge disable operation is unsuccessful
 */
int disable_hps2fpga_bridge(void);

/*
 * @func   enable_lwhps2fpga_bridge
 * @brief  The api enabled the l2hps2fpga bridge.
 * @return 
 * - RET_SUCCESS, if l2hps2fpga bridge is enabled successfully
 * - RET_FAIL,    if bridge enable operation is  unsuccessful
 */
int enable_lwhps2fpga_bridge(void);

/*
 * @func   disable_lwhps2fpga_bridge
 * @brief  The api disables the lwhps2fpga bridge
 * @return 
 * - RET_SUCCESS, if bridge is disabled successfully
 * - RET_FAIL,    if bridge disable operation fails
 */
int disable_lwhps2fpga_bridge(void);

/*
 * @func   enable_fpga2hps_bridge
 * @brief  This api enables the fpga2hps bridge
 * @return 
 * - RET_SUCCESS, if bridge is enabled successfully
 * - RET_FAIL,    if bridge enable operation is  unsuccessful
 */
int enable_fpga2hps_bridge(void);

/*
 * @func   disable_fpga2hps_bridge
 * @brief  The api disbles the fpga2hps bridge
 * @return 
 * - RET_SUCCESS, if bridge disable operation is successful
 * - RET_FAIL,    if bridge disable operation is unsuccessful
 */
int disable_fpga2hps_bridge(void);

/*
 * @func   enable_fpga2sdram_bridge
 * @brief  The api enables the fpga2sdram bridge
 * @return 
 *  RET_SUCCESS, if bridge is enabled successful
 *  RET_FAIL,    if bridge enable operation is unsuccessful
 */
int enable_fpga2sdram_bridge(void);

/*
 * @func   disable_fpga2sdram_bridge
 * @brief  The api disbles the fpga2sdram bridge
 * @return 
 * - RET_SUCCESS, if bridge disable operation is successful
 * - RET_FAIL,    if bridge disable operation is unsuccessful
 */
int disable_fpga2sdram_bridge(void);

#endif  /*__SOCFPGA_BRIDGE_H__ */
