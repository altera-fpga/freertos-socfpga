/*
 * SPDX-FileCopyrightText: Copyright (C) 2025 Altera Corporation
 *
 * SPDX-License-Identifier: MIT-0
 *
 * Header file for SoC FPGA XGMAC low level driver
 */

#ifndef __SOCFPGA_XGMAC_LL_H__
#define __SOCFPGA_XGMAC_LL_H__


#ifdef __cplusplus
extern "C" {
#endif
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "socfpga_xgmac_reg.h"
#include "socfpga_cache.h"

#define BIT(nr)    (1UL << (nr))
#define TRUE                                     (1)
#define FALSE                                    (-1)
#define XGMAC_MAC_CONF_SS_1G_GMII                3
#define XGMAC_MAC_CONF_SS_100M_MII               4
#define XGMAC_MAC_CONF_SS_10M_MII                7
#define XGMAC_MAC_EXT_CONF_HD                    BIT(24)

#define EQOS_MAC_RXQ_CTRL0_RXQ0EN_ENABLED_DCB    2
#define EQOS_MAC_RXQ_CTRL0_RXQ0EN_ENABLED_AV     1

#define XGMAC_DMA_CH0_TX_CONTROL_OSP             BIT(4)
#define XGMAC_MAC_CONF_SS_1G_GMII                3
#define XGMAC_MAC_CONF_SS_100M_MII               4
#define XGMAC_MAC_CONF_SS_10M_MII                7
#define XGMAC_MAC_EXT_CONF_HD                    BIT(24)

#define MAC_ADRRESS_INDEX1                       1

#define XGMAC_RESET_ASSERT                       (1)
#define XGMAC_RESET_DEASSERT                     (0)
#define XGMAC_LL_RETVAL_SUCCESS                  (0x1)
#define XGMAC_LL_RETVAL_FAIL                     (0xFFFFFFFF)

#define XGMAC_PHY_TYPE_GMII                      0
#define XGMAC_PHY_TYPE_RGMII                     1
#define XGMAC_PHY_TYPE_RMII                      2
#define XGMAC_PHY_TYPE_SGMII                     3
#define XGMAC_PHY_TYPE_SGMII_PLUS                4

#define XGMAC_DMA_TRANSMIT_START                 1
#define XGMAC_DMA_RECEIVE_START                  2
#define XGMAC_DMA_TRANSMIT_STOP                  3
#define XGMAC_DMA_RECEIVE_STOP                   4

#define XGMAC_GET_BASE_ADDRESS(instance)         ((uint32_t )( \
        XGMAC_EMAC_BASEADDR + (instance * 0x10000)))
#define XGMAC_GET_CORE_BASE_ADDRESS(instance)    ((uint32_t )( \
        XGMAC_EMAC_CORE_BASEADDR + (instance * 0x10000)))
#define XGMAC_GET_DMA_BASE_ADDRESS(instance)     ((uint32_t )( \
        XGMAC_EMAC_DMA_BASEADDR + (instance * 0x10000)))
#define XGMAC_GET_MTL_BASE_ADDRESS(instance)     ((uint32_t )( \
        XGMAC_EMAC_MTL_BASEADDR + (instance * 0x10000)))

#define XGMAC_DMA_INTR_POS_TI             0
#define XGMAC_DMA_INTR_MASK_TI            0x00000001
#define XGMAC_DMA_INTR_POS_TXS            1
#define XGMAC_DMA_INTR_MASK_TXS           0x00000002
#define XGMAC_DMA_INTR_POS_TBU            2
#define XGMAC_DMA_INTR_MASK_TBU           0x00000004

#define XGMAC_DMA_INTR_POS_RI             6
#define XGMAC_DMA_INTR_MASK_RI            0x00000040
#define XGMAC_DMA_INTR_POS_RBU            7
#define XGMAC_DMA_INTR_MASK_RBU           0x00000080
#define XGMAC_DMA_INTR_POS_RS             8
#define XGMAC_DMA_INTR_MASK_RS            0x00000100
#define XGMAC_DMA_INTR_POS_DDE            9
#define XGMAC_DMA_INTR_MASK_DDE           0x00000200

#define XGMAC_DMA_INTR_POS_FBE            12
#define XGMAC_DMA_INTR_MASK_FBE           0x00001000
#define XGMAC_DMA_INTR_POS_CDE            13
#define XGMAC_DMA_INTR_MASK_CDE           0x00002000
#define XGMAC_DMA_INTR_POS_AIS            14
#define XGMAC_DMA_INTR_MASK_AIS           0x00004000
#define XGMAC_DMA_INTR_POS_NIS            15
#define XGMAC_DMA_INTR_MASK_NIS           0x00008000

#define XGMAC_MMC_IPC_RX_INTR_MASK_ALL    0xFFFFFFFF
/* Get the fifo size in bytes from Feature1 register
 * */
#define XGMAC_MTL_RX_FIFOSZ_BYTES(feature1_val)    (128 << \
            ((feature1_val & XGMAC_MAC_HW_FEATURE1_RXFIFOSIZE_MASK) >> \
    XGMAC_MAC_HW_FEATURE1_RXFIFOSIZE_POS))

#define XGMAC_MTL_TX_FIFOSZ_BYTES(feature1_val)    (128 << \
            ((feature1_val & XGMAC_MAC_HW_FEATURE1_TXFIFOSIZE_MASK) >> \
    XGMAC_MAC_HW_FEATURE1_TXFIFOSIZE_POS))

/* calculate block count from fifo size in bytes (256 bytes per block, value 0 means 256 bytes)
 * */
#define XGMAC_MTL_RX_FIFO_BLK_CNT(feature1_val)    ((XGMAC_MTL_RX_FIFOSZ_BYTES( \
        feature1_val) >> 8) - 1)
#define XGMAC_MTL_TX_FIFO_BLK_CNT(feature1_val)    ((XGMAC_MTL_TX_FIFOSZ_BYTES( \
        feature1_val) >> 8) - 1)


/* Common register read/write macros */
#define WR_REG32(ulAddress, \
            ulVal)             *(uint32_t volatile*)(( \
        uintptr_t)ulAddress) = ulVal
#define RD_REG32(ulAddress)    *(uint32_t \
    volatile*)((uintptr_t)ulAddress)

/* Common register bit set/clear macros */
#define ENABLE_BIT(ulAddress, \
            bit)               *(uint32_t volatile*)((uintptr_t) \
    ulAddress) |= bit
#define DISABLE_BIT(ulAddress, \
            bit)               *(uint32_t volatile*)((uintptr_t) \
    ulAddress) &= ~bit

/* MAC register bit enable/disable macros */
#define ENABLE_MAC_FLOW_CNTL_REGBIT(addr, queueindx, \
            bit)               *(uint32_t volatile*)((uintptr_t)addr + \
    (queueindx * XGMAC_TX_FLOW_CONTROL_INC)) |= bit
#define DISABLE_MAC_FLOW_CNTL_REGBIT(addr, queueindx, \
            bit)               *(uint32_t volatile*)((uintptr_t)addr + \
    (queueindx * XGMAC_TX_FLOW_CONTROL_INC)) &= ~bit

/* MTL register bit enable/disable macros */
#define ENABLE_MTL_QX_REGBIT(addr, queueindx, \
            bit)               *(uint32_t volatile*)((uintptr_t)addr + \
    XGMAC_MTL_TC_BASE + (queueindx * XGMAC_MTL_TC_INC)) |= bit
#define DISABLE_MTL_QX_REGBIT(addr, queueindx, \
            bit)               *(uint32_t volatile*)((uintptr_t)addr + \
    XGMAC_MTL_TC_BASE + (queueindx * XGMAC_MTL_TC_INC)) &= ~bit

/* MTL register read/write macros */
#define RD_MTL_QX_REG32(BaseAddress,QueueIndx, \
            RegOffset)         *(uint32_t volatile*)((uintptr_t)BaseAddress + \
    XGMAC_MTL_TC_BASE + (QueueIndx * XGMAC_MTL_TC_INC) + RegOffset)
#define WR_MTL_QX_REG32(BaseAddress,QueueIndx,RegOffset, \
            Data)              *(uint32_t volatile*)((uintptr_t)BaseAddress + \
    XGMAC_MTL_TC_BASE + (QueueIndx * XGMAC_MTL_TC_INC) + \
    RegOffset) = Data

/* DMA register bit enable/disable macros */
#define ENABLE_DMA_CHNL_REGBIT(addr, channel, \
            bit)               *(uint32_t volatile*)((uintptr_t)addr + \
    XGMAC_DMA_CHANNEL_BASE + (channel * XGMAC_DMA_CHANNEL_INC)) |= bit
#define DISABLE_DMA_CHNL_REGBIT(addr, channel, \
            bit)               *(uint32_t volatile*)((uintptr_t)addr + \
    XGMAC_DMA_CHANNEL_BASE + (channel * XGMAC_DMA_CHANNEL_INC)) &= ~bit

/* DMA register read/write macros */
#define RD_DMA_CHNL_REG32(BaseAddress,Channel, \
            RegOffset)         *(uint32_t volatile*)((uintptr_t)BaseAddress + \
    XGMAC_DMA_CHANNEL_BASE + (Channel * XGMAC_DMA_CHANNEL_INC) + \
    RegOffset)
#define WR_DMA_CHNL_REG32(BaseAddress,Channel,RegOffset, \
            Data)              *(uint32_t volatile*)((uintptr_t)BaseAddress + \
    XGMAC_DMA_CHANNEL_BASE + (Channel * XGMAC_DMA_CHANNEL_INC) + \
    RegOffset) = Data

typedef enum
{
    eInterruptTI,                /* Transmit Interrupt .*/
    eInterruptTXS,              /* Transmit Stopped. */
    eInterruptTBU,             /*Transmit Buffer Unavailable.*/
    eInterruptRI,             /* Receive Interrupt */
    eInterruptRBU,           /* Receive Buffer Unavailable */
    eInterruptRS,           /* Receive Stopped */
    eInterruptDDE,         /* Descriptor Definition Error  */
    eInterruptFBE,        /* Fatal Bus Error  */
    eInterruptCDE,       /* Context Descriptor Error  */
    eInterruptAIS,      /* Abnormal Interrupt Summary */
    eInterruptNIS,      /* Normal Interrupt Summary */
    eInterruptUnhandled      /* Unhandled Interrupt  */
} XgmacDmaInterruptID_t;

typedef bool Basetype_t;

typedef struct
{
    uint32_t ulTxRingLen;
    uint32_t ulRxRingLen;
    uint32_t ulTxDescHighAddr;
    uint32_t ulTxDescLowAddr;
    uint32_t ulRxDescHighAddr;
    uint32_t ulRxDescLowAddr;
    uint32_t ulTxLastDescAddr;
    uint32_t ulRxLastDescAddr;
}XgmacDmaDescAddr_t;

/**
 * @brief  Configuration structure for XGMAC DMA Parameters
 */
typedef struct
{
    bool ubl;
    bool blen4;
    bool blen8;
    bool blen16;
    bool blen32;
    bool blen64;
    bool blen128;
    bool blen256;
    bool aal;
    bool eame;
    uint8_t rd_osr_lmt;
    uint8_t wr_osr_lmt;
}XGMACDMAConfig_t;


/**
 * @brief  Configuration structure for XGMAC DMA Channel Parameters
 */
typedef struct
{
    /* DMA Channel Control register fields */
    bool pblx8;
    bool sph;
    uint8_t dsl;

    /* DMA Channel Tx Control register fields */
    bool tse;
    uint8_t txpbl;
    uint8_t tqos;

    /* DMA Channel Rx Control register fields */
    uint8_t rxpbl;
    uint8_t rqos;
    uint16_t rbsz;
}XGMACDMAChanlConfig_t;


/**
 * @brief  Configuration structure for XGMAC MAC Rx parameters
 */
typedef struct
{
    /* MAC RxQ control0 register fields */
    uint8_t rxq0en;
    uint8_t rxq1en;
    uint8_t rxq2en;
    uint8_t rxq3en;
    uint8_t rxq4en;
    uint8_t rxq5en;
    uint8_t rxq6en;
    uint8_t rxq7en;

    /* MAC RxQ control1 register fields */
    bool mcbcqen;
}XGMACMACRxQCtrlConfig_t;


/**
 * @brief   Configuration structure for XGMAC MTL Tx Queue parameters
 */
typedef struct
{
    bool tsf;
    uint8_t txqen;
    uint8_t tqs;
}XGMACMTLTxQueueConfig_t;


/**
 * @brief   Configuration structure for XGMAC MTL Rx Queue parameters
 */
typedef struct
{
    bool rsf;
    bool ehfc;
    uint8_t rqs;
}XGMACMTLRxQueueConfig_t;


/**
 * @brief  Configuration structure for XGMAC MAC Tx Flow Control parameters
 */
typedef struct
{
    bool fcb;
    bool tfe;
    uint8_t plt;
    bool dzpq;
    uint32_t pt;
}XGMACMACTxFlowCtrlConfig_t;

/**
 * @brief  Configuration structure for XGMAC MAC Rx Flow Control parameters
 */
typedef struct
{
    bool rfe;
    bool up;
    bool pfce;
}XGMACMACRxFlowCtrlConfig_t;


/**
 * @brief  Configuration structure for XGMAC MAC Tx parameters
 */
typedef struct
{
    bool jd;
}XGMACMACTxConfig_t;


/**
 * @brief  Configuration structure for XGMAC MAC Rx parameters
 */
typedef struct
{
    bool acs;
    bool cst;
    bool dcrcc;
    bool spen;
    bool usp;
    bool gpslce;
    bool wd;
    bool je;
    bool ipc;
    uint16_t gpsl;
    bool arpen;
}XGMACMACRxConfig_t;

/**
 * @brief  Configuration structure for XGMAC MAC Packet Filter parameters
 */
typedef struct
{
    bool pr;
    bool huc;
    bool hmc;
    bool daif;
    bool pm;
    bool dbf;
    uint8_t pcf;
    bool saif;
    bool saf;
    bool hpf;
    uint8_t dhlfrs;
    bool vtfe;
    bool ipfe;
    bool dntu;
    bool ra;
}XGMACMACPktFilterConfig_t;

/**
 * @brief  Configuration structure for XGMAC Device Parameters
 */
typedef struct
{
    uint8_t nofdmachannels;
    uint8_t noftxqueues;
    uint8_t nofrxqueues;
}XGMACDevConfig_t;

/**
 * @brief  Configuration structure for XGMAC DMA, MTL and MAC Parameters
 */
typedef struct
{
    const XGMACDevConfig_t *macDevConfig;
    const XGMACDMAConfig_t *dmaConfig;
    const XGMACDMAChanlConfig_t *dmaChannelConfig;
    const XGMACMTLTxQueueConfig_t *mtlTxQConfig;
    const XGMACMTLRxQueueConfig_t *mtlRxQConfig;
    const XGMACMACRxQCtrlConfig_t *macRxQCtrlConfig;
    const XGMACMACTxFlowCtrlConfig_t *macTxFlowCtrlConfig;
    const XGMACMACRxFlowCtrlConfig_t *macRxFlowCtrlConfig;
    const XGMACMACTxConfig_t *macTxConfig;
    const XGMACMACRxConfig_t *macRxConfig;
    const XGMACMACPktFilterConfig_t *macPktFilterConfig;

}XGMACDevConfigStr_t;

/* MAC Function prototypes */
void xgmac_set_macaddress(uint32_t *baseAddress, void *addressPtr,
        uint8_t indexVal);
void xgmac_setduplex(uint32_t *baseAddress, uint8_t duplexCond);
void xgmac_setspeed(uint32_t *baseAddress, uint32_t speedCond);

void xgmac_assert_reset(int32_t emacindex);
void xgmac_deassert_reset(int32_t emacindex);
void xgmac_sysmgr_physelect(int32_t emacindex, uint8_t physelect);

void xgmac_start_stop_mac_rx(uint32_t *baseAddress, bool stflag);
void xgmac_start_stop_mac_tx(uint32_t *baseAddress, bool stflag);

void xgmac_config_macrxqctrl_regs(uint32_t *baseAddress,
        const XGMACMACRxQCtrlConfig_t *macrxqctrlconfig);
void xgmac_config_mac_frame_filter(uint32_t *baseAddress,
        const XGMACMACPktFilterConfig_t *macpktfilterconfig);
void xgmac_config_mac_tx(uint32_t *baseAddress,
        const XGMACMACTxConfig_t *mactxconfig);
void xgmac_config_mac_rx(uint32_t *baseAddress,
        const XGMACMACRxConfig_t *macrxconfig);
void xgmac_enable_tx_flow_control(uint32_t *baseAddress, uint8_t qindx,
        const XGMACMACTxFlowCtrlConfig_t *mactxflowctrlconfig);
void xgmac_enable_rx_flow_control(uint32_t *baseAddress,
        const XGMACMACRxFlowCtrlConfig_t *macrxflowctrlconfig);

void xgmac_set_mtl_tx_regs(uint32_t *baseAddress, uint8_t qindx, \
        const XGMACMTLTxQueueConfig_t *mtltxqcfgparams);
void xgmac_set_mtl_rx_regs(uint32_t *baseAddress, uint8_t qindx, \
        const XGMACMTLRxQueueConfig_t *mtlrxqcfgparams);

void xgmac_get_macaddress(uint32_t *baseAddress, uint8_t indexVal,
        uint8_t *aptr);
int32_t xgmac_verify_sys_settings(int32_t emacindex, uint8_t physelect);

/* DMA Function prototypes */
void xgmac_dma_init(uint32_t *baseAddress,
        const XGMACDevConfigStr_t *xgmacdevconfig);
void xgmac_start_dma(uint32_t *baseAddress, uint8_t dmachindx,
        uint8_t txrxflag);
void xgmac_stop_dma(uint32_t *baseAddress, uint8_t dmachindx, uint8_t txrxflag);
void xgmac_config_dma_sysbus_mode(uint32_t *baseAddress,
        const XGMACDMAConfig_t *dmaconfig);
void xgmac_init_dma_channel_desc_reg(uint32_t *baseAddress, uint8_t dmachindx,
        XgmacDmaDescAddr_t *dmadescparams);
void xgmac_config_dma_channel_control(uint32_t *baseAddress, uint8_t dmachindx,
        const XGMACDMAChanlConfig_t *dmachnlconfig);

int32_t xgmac_dma_software_reset(uint32_t *baseAddress);
bool xgmac_is_dma_reset_done(uint32_t *baseAddress);
int32_t xgmac_enable_dma_interrupt(uint32_t *baseAddress, uint8_t chindx,
        XgmacDmaInterruptID_t id);
int32_t xgmac_disable_dma_interrupt( uint32_t *baseAddress, uint8_t chindx,
        XgmacDmaInterruptID_t id);
void xgmac_start_dma_dev(uint32_t *baseAddress,
        const XGMACDevConfigStr_t *xgmacdevconfig);
void xgmac_stop_dma_dev(uint32_t *baseAddress,
        const XGMACDevConfigStr_t *xgmacdevconfig);

void xgmac_mtl_init (uint32_t *baseAddress,
        const XGMACDevConfigStr_t *xgmacdevconfig);
void xgmac_mac_init(uint32_t *baseAddress,
        const XGMACDevConfigStr_t *xgmacdevconfig );
void xgmac_dev_start(uint32_t *baseAddress);
void xgmac_dev_stop(uint32_t *baseAddress);

void xgmac_disable_interrupt(uint32_t *baseAddress);
XgmacDmaInterruptID_t check_and_clear_xgmac_interrupt_status(
    uint32_t baseAddress );
void check_and_clear_link_interrupt_status( uint32_t baseAddress );

void xgmac_flush_descriptor(void *buf, size_t size);
void xgmac_invalidate_descriptor(void *buf, size_t size);
void xgmac_invalidate_buffer(void *buf, size_t size);
void xgmac_flush_buffer(void *buf, size_t size);

void xgmac_mmc_setup(uint32_t *baseAddress);

#endif
