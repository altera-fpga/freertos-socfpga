/*
 * SPDX-FileCopyrightText: Copyright (C) 2025 Altera Corporation
 *
 * SPDX-License-Identifier: MIT-0
 *
 * Header file for fpga bridge low level driver
 */

#ifndef __SOCFPGA_BRIDGE_LL_H__
#define __SOCFPGA_BRIDGE_LL_H__

#define RET_SUCCESS ( 1)
#define RET_FAIL    (-1)

typedef enum
{
    HPS2FPGA,
    LWHPS2FPGA,
    FPGA2HPS,
    FPGA2SDRAM,
}bridge_type_t;

int poll_idle_status(uint32_t reg_addr, uint32_t bit_msk, uint32_t val);

int is_enabled(bridge_type_t bridge);
int is_disabled(bridge_type_t bridge);

int configure_hps2fpga_bridge(void);
int configure_lwhps2fpga_bridge(void);
int fpga2hps_bridge_enable_conf(void);
int fpga2hps_bridge_disable_conf(void);
int fpga2sdram_bridge_enable_conf(void);
int fpga2sdram_bridge_disable_conf(void);

#endif  /*__SOCFPGA_BRIDGE_LL_H__ */
