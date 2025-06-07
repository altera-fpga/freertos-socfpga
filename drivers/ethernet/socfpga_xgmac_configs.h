/*
 * SPDX-FileCopyrightText: Copyright (C) 2025 Altera Corporation
 *
 * SPDX-License-Identifier: MIT-0
 *
 * This contains the configuration initializations for SoC FPGA XGMAC DMA, MTL
 * and MAC registers
 */

#ifndef __SOCFGPA_XGMAC_CONFIGS_H__
#define __SOCFGPA_XGMAC_CONFIGS_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include "socfpga_xgmac_ll.h"

/**
 * @brief  Configuration parameters for XGMAC DMA Parameters
 */
static const XGMACDMAConfig_t xDmaConfig = {

    .ubl = 0,
    .blen4 = 1,
    .blen8 = 1,
    .blen16 = 1,
    .blen32 = 1,
    .blen64 = 0,
    .blen128 = 0,
    .blen256 = 0,
    .aal = 0,
    .eame = 1,
    .rd_osr_lmt = 31,
    .wr_osr_lmt = 31,
};

/**
 * @brief  Configuration parameters for XGMAC DMA Channel Parameters
 */

static const XGMACDMAChanlConfig_t xDmaChnlConfig = {
    /* DMA Channel Control register */
    .pblx8 = 1,
    .sph = 0,
    .dsl = 0,

    /* DMA Channel Tx Control register */
    .tse = 0,
    .txpbl = 32,

    /* DMA Channel Rx Control register */
    .rxpbl = 8,
    .rbsz = XGMAC_MAX_PACKET_SIZE,
};


/**
 * @brief  Configuration parameters for XGMAC MAC Rx parameters
 */
static const XGMACMACRxQCtrlConfig_t xMacRxqCtrlConfig = {
    /* MAC RxQ control0 register fields */
    .rxq0en = 2,

    /* rxq1end - rxq7en not supported in this version */

    /* MAC RxQ control1 register fields */
    .mcbcqen = 1,
};

/**
 * @brief   Configuration parameters for XGMAC MTL Tx Queue parameters
 */
static const XGMACMTLTxQueueConfig_t xMtlTxqConfig = {
    .tsf =  1,
    .txqen = 2,
};

/**
 * @brief   Configuration parameters for XGMAC MTL Rx Queue parameters
 */
static const XGMACMTLRxQueueConfig_t xMtlRxqConfig = {
    .rsf = 1,
    .ehfc = 0,
};


/**
 * @brief  Configuration parameters for XGMAC MAC Tx Flow Control parameters
 */
static const XGMACMACTxFlowCtrlConfig_t xMacTxFlowCtrlConfig = {
    .fcb = 0,
    .tfe = 0,
    .plt = 0,
    .dzpq = 0,
    .pt = 0xFFFF,
};

/**
 * @brief  Configuration parameters for XGMAC MAC Rx Flow Control parameters
 */
static const XGMACMACRxFlowCtrlConfig_t xMacRxFlowCtrlConfig = {
    .rfe = 0,
    .up = 0,
    .pfce = 0,
};
/**
 * @brief  Configuration parameters for XGMAC MAC Tx parameters
 */
static const XGMACMACTxConfig_t xMacTxConfig = {
    .jd = 0,
};

/**
 * @brief  Configuration parameters for XGMAC MAC Rx parameters
 */
static const XGMACMACRxConfig_t xMacRxConfig = {
    .acs = 0,
    .cst = 0,
    .dcrcc =  0,
    .spen  =  0,
    .usp  =  0,
    .gpslce = 0,
    .wd = 0,
    .je = 0,
    .ipc = 1,
    .gpsl = 0,
    .arpen = 0,
};

/**
 * @brief  Configuration parameters for XGMAC MAC Packet Filter parameters
 */
static const XGMACMACPktFilterConfig_t xMacPktFilterConfig = {
    .pr = 0,
    .huc = 0,
    .hmc = 0,
    .daif = 0,
    .pm = 0,
    .dbf = 0,
    .pcf = 0,
    .saif = 0,
    .saf = 0,
    .hpf = 0,
    .dhlfrs = 0,
    .vtfe = 0,
    .ipfe = 0,
    .dntu = 0,
    .ra = 1,
};

/**
 * @brief  Configuration parameters for XGMAC Device Parameters
 */
static const XGMACDevConfig_t xMacDevConfig = {

    .nofdmachannels = 1,
    .noftxqueues = 1,
    .nofrxqueues = 1,
};

/* Main XGMAC Configuration instance */
XGMACDevConfigStr_t xXGMACDevConfigStr =
{
    .macDevConfig = &xMacDevConfig,
    .dmaConfig = &xDmaConfig,
    .dmaChannelConfig = &xDmaChnlConfig,
    .mtlTxQConfig = &xMtlTxqConfig,
    .mtlRxQConfig = &xMtlRxqConfig,
    .macRxQCtrlConfig = &xMacRxqCtrlConfig,
    .macTxFlowCtrlConfig = &xMacTxFlowCtrlConfig,
    .macRxFlowCtrlConfig = &xMacRxFlowCtrlConfig,
    .macTxConfig = &xMacTxConfig,
    .macRxConfig = &xMacRxConfig,
    .macPktFilterConfig = &xMacPktFilterConfig,
};

#endif /* __SOCFGPA_XGMAC_CONFIGS_H__ */
