/*
 * SPDX-FileCopyrightText: Copyright (C) 2025 Altera Corporation
 *
 * SPDX-License-Identifier: MIT-0
 *
 * Header file for FPGA manager driver
 */
#ifndef __SOCFPGA_FPGA_MANAGER_H__
#define __SOCFPGA_FPGA_MANAGER_H__

#include <stdint.h>

#define STATUS_OK            (0x0)
#define RET_OK                 (1)
#define RET_FAIL              (-1)

#define BITSTREAM_CHUNK_SIZE    0xFFFF0

#define FPGA_CONFIG_START           0xC2000001
#define FPGA_CONFIG_WRITE_COMPLETE  0xC2000003
#define FPGA_CONFIG_WRITE           0x42000002
#define FPGA_CONFIG_ISDONE          0xC2000004

/* SMC responses */
#define SMC_CMD_SUCCESS             0x0
#define SMC_STATUS_BUSY             0x1
#define SMC_STATUS_REJECTED         0x2
#define SMC_STATUS_NO_RESPONSE      0x3
#define SMC_STATUS_ERROR            0x4

#define TIME_OUT_ERR                0x110

#define RECONFIG_DELAY_MS           1000
#define RECONFIG_STATUS_POLL_RETRY_MAX          100

/*
 * @func  : load_fpga_bitstream
 * @brief : api to load the bitstream and configure the fpga
 * @param[in] rbf_ptr reference to the bitstream file
 * @param[in] file_size length of bistream file
 * @return
 *  RET_OK, if configuration is success
 *  RET_FAIL, if configuration fails
 */
int load_fpga_bitstream(uint8_t *rbf_ptr, uint32_t file_size);

#endif  /* __SOCFPGA_FPGA_MANAGER_H__ */
