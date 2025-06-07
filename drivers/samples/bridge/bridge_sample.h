/*
 * SPDX-FileCopyrightText: Copyright (C) 2025 Altera Corporation
 *
 * SPDX-License-Identifier: MIT-0
 *
 * Definitions used in SoC FPGA bridge sample app
 */


#ifndef _BRIDGE_SAMPLE_H_
#define _BRIDGE_SAMPLE_H_

/* f2h bridge example memory mapped base addresses */
#define F2H_SIG         (0x20483246)
#define F2H_SYSID_ID_BASE   (0x20000000)
#define F2H_SYSID_TS_BASE   (0x20000004)
#define F2H_MSGDMA_CSR_BASE (0x20001000)
#define F2H_MSGDMA_DES_BASE (0x20002000)
#define F2H_OCRAM_32K_BASE  (0x20008000)
#define F2H_OCRAM_32K_SPAN  (0x00008000)
#define F2H_L3_CACHE_SPAN   (0x00200000)
#define F2H_BUF_0_BASE      (0xA0000000)
#define F2H_BUF_1_BASE      (F2H_BUF_0_BASE + F2H_L3_CACHE_SPAN)

/* f2h and f2sdram MSGDMA memory mapped base addresses */
#define MSGDMA_OCRAM_32K_BASE   (0x100000000ULL)

/* f2sdram bridge example memory mapped base addresses */
#define F2SD_SIG        (0x44533246)
#define F2SDRAM_SYSID_ID_BASE   (0x20000000)
#define F2SDRAM_SYSID_TS_BASE   (0x20000004)
#define F2SDRAM_MSGDMA_CSR_BASE (0x20001000)
#define F2SDRAM_MSGDMA_DES_BASE (0x20002000)
#define F2SDRAM_OCRAM_32K_BASE  (0x20008000)
#define F2SDRAM_OCRAM_32K_SPAN  (0x00008000)
#define F2SDRAM_L3_CACHE_SPAN   (0x00200000)
#define F2SDRAM_BUF_0_BASE  (0xA0000000)
#define F2SDRAM_BUF_1_BASE  (F2SDRAM_BUF_0_BASE + F2SDRAM_L3_CACHE_SPAN)

 /* lwh2f bridge example memory mapped base addresses */
#define LWH2_SIG        (0x3248574C)
#define LWH2F_512M_BASE     (0x20000000)
#define LWH2F_512M_SPAN     (0x20000000)
#define LWH2F_SYSID_ID_BASE (0x20000800)
#define LWH2F_SYSID_TS_BASE (0x20000804)

/* h2f bridge example memory mapped base addresses */
#define H2F_SIG         (0x20463248)
#define H2F_1G_BASE     (0x0040000000ULL)
#define H2F_1G_SPAN     (0x0040000000ULL)
#define H2F_SYSID_ID_BASE   (0x0040000800ULL)
#define H2F_SYSID_TS_BASE   (0x0040000804ULL)
#define H2F_15G_BASE        (0x0440000000ULL)
#define H2F_15G_SPAN        (0x03C0000000ULL)
#define H2F_240G_BASE       (0x4400000000ULL)
#define H2F_240G_SPAN       (0x3C00000000ULL)

int fpga2hps_bridge_sample(void);
int f2sdram_bridge_sample(void);
int hps2fpga_bridge_sample(void);
int lwhps2fpga_bridge_sample(void);

#endif  /*_BRIDGE_SAMPLE_H_*/
