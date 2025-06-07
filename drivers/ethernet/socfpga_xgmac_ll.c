/*
 * SPDX-FileCopyrightText: Copyright (C) 2025 Altera Corporation
 *
 * SPDX-License-Identifier: MIT-0
 *
 * Low level driver implementation for SoC FPGA XGMAC
 */

#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include "socfpga_xgmac_ll.h"
#include "socfpga_xgmac_reg.h"
#include "socfpga_xgmac_phy_ll.h"

void xgmac_set_macaddress(uint32_t *baseAddress, void *addressPtr,
        uint8_t indexVal)
{
    uint32_t macAddr;
    uint8_t *aptr = (uint8_t *)(void *)addressPtr;
    uint32_t addrOffset;

    /* Hardware MAc Address index ranges from 1 to 32,and index will start from 0 to 31 */
    indexVal--;
    addrOffset = indexVal * 8U;

    /* Set the MAC bits [31:0] in MAC_Addressx_Low register*/
    macAddr = *(aptr);
    macAddr |= ((uint32_t)(*(aptr + 1)) << 8U);
    macAddr |= ((uint32_t)(*(aptr + 2)) << 16U);
    macAddr |= ((uint32_t)(*(aptr + 3)) << 24U);

    WR_REG32(baseAddress +
            ((uint32_t)XGMAC_MAC_ADDRESS0_LOW + (uint32_t) addrOffset),
            macAddr);

    /* Read the MAC_Addressx_High register and clear address bits */
    macAddr =
            RD_REG32(baseAddress +
            ((uint32_t) XGMAC_MAC_ADDRESS0_HIGH + (uint32_t) addrOffset));

    /* Clear the lower 16 bits of the MAC_Addressx_High register */
    macAddr &= (uint32_t)(~XGMAC_MAC_ADDRESS0_HIGH_ADDRHI_MASK);


    /* Set MAC bits [47:32] from MAC_Addressx_High[15:0]  */
    macAddr |= (uint32_t)(*(aptr + 4));
    macAddr |= (uint32_t)(*(aptr + 5)) << 8U;

    WR_REG32(baseAddress +
            ((uint32_t)XGMAC_MAC_ADDRESS0_HIGH + (uint32_t) addrOffset),
            macAddr);
}

void  xgmac_get_macaddress(uint32_t *baseAddress, uint8_t indexVal,
        uint8_t*aptr)
{
    uint32_t macAddr;
    uint32_t addrOffset = 0;

    /* Hardware MAc Address index ranges from 1 to 32,and index will start from 0 to 31 */
    indexVal--;
    addrOffset = indexVal * 8U;

    macAddr =
            RD_REG32(baseAddress +
            ((uint32_t)XGMAC_MAC_ADDRESS0_LOW + (uint32_t) addrOffset));

    *aptr = (uint8_t) macAddr;
    *(aptr + 1) = (uint8_t) (macAddr >> 8U);
    *(aptr + 2) = (uint8_t) (macAddr >> 16U);
    *(aptr + 3) = (uint8_t) (macAddr >> 24U);

    /* Get MAC bits [47:32] from MAC_Addressx_High[15:0]  */
    macAddr =
            RD_REG32(baseAddress +
            ((uint32_t)XGMAC_MAC_ADDRESS0_HIGH + (uint32_t) addrOffset));
    *(aptr + 4) = (uint8_t) macAddr;
    *(aptr + 5) = (uint8_t) (macAddr >> 8U);
}

void  xgmac_setduplex(uint32_t *baseAddress, uint8_t duplex)
{
    /* Full Duplex */
    if (duplex == PHY_FULL_DUPLEX)
        DISABLE_BIT(baseAddress + XGMAC_MAC_EXTENDED_CONFIGURATION,
                XGMAC_MAC_EXT_CONF_HD);

    else     /* Half Duplex */
        ENABLE_BIT(baseAddress + XGMAC_MAC_EXTENDED_CONFIGURATION,
                XGMAC_MAC_EXT_CONF_HD);
}
void xgmac_setspeed(uint32_t *baseAddress, uint32_t speed)
{
    uint32_t regData = 0;

    regData =  RD_REG32(baseAddress + XGMAC_MAC_TX_CONFIGURATION);
    /* 1Gbps */
    if (speed == ETH_SPEED_1000_MBPS)
        regData |=
                (XGMAC_MAC_CONF_SS_1G_GMII <<
                XGMAC_MAC_TX_CONFIGURATION_SS_POS);

    /* 100Mbps */
    else if (speed == ETH_SPEED_100_MBPS)
        regData |=
                (XGMAC_MAC_CONF_SS_100M_MII <<
                XGMAC_MAC_TX_CONFIGURATION_SS_POS);

    /* 10Mbps */
    else
        regData |=
                (XGMAC_MAC_CONF_SS_10M_MII <<
                XGMAC_MAC_TX_CONFIGURATION_SS_POS);

    WR_REG32(baseAddress + XGMAC_MAC_TX_CONFIGURATION, regData);
}

void xgmac_assert_reset(int32_t emacindex)
{
    uint32_t val;

    val = RD_REG32(RESET_MNGR_BASE_ADDRESS + PER0MODRST_REG);
    val |= (1 << (RST_SHIFT_POS + emacindex));
    WR_REG32(RESET_MNGR_BASE_ADDRESS + PER0MODRST_REG, val);
}

void xgmac_deassert_reset(int32_t emacindex)
{
    uint32_t val;

    val = RD_REG32(RESET_MNGR_BASE_ADDRESS + PER0MODRST_REG);
    val &= ~(1 << (RST_SHIFT_POS + emacindex));
    WR_REG32(RESET_MNGR_BASE_ADDRESS + PER0MODRST_REG, val);
}

void xgmac_sysmgr_physelect(int32_t emacindex, uint8_t physelect)
{
    uint32_t regOffset, reg;
    uint8_t val;

    switch (emacindex) {
    case 0:
        regOffset = TSN0_REG;
        break;
    case 1:
        regOffset = TSN1_REG;
        break;
    case 2:
        regOffset = TSN2_REG;
        break;
    }

    /* Value to write based on physelect */
    val =
            (physelect ==
            XGMAC_PHY_TYPE_RGMII) ? (1 << PHY_INTF_SEL_FIELD_POS) : (0 <<
            PHY_INTF_SEL_FIELD_POS);

    /* Read Sys Manager Register and Clear 0:1 bits */
    reg = RD_REG32(SYSTEM_MNGR_BASE_ADDRESS + regOffset);
    reg &= ~(PHY_INTF_SEL_FIELD_MASK);

    /* Write the value to the to register */
    val |= reg;
    WR_REG32(SYSTEM_MNGR_BASE_ADDRESS + regOffset, val);
}

int32_t xgmac_verify_sys_settings(int32_t emacindex, uint8_t physelect)
{
    uint32_t readReg1, readReg2;
    uint32_t tsnfieldmask;
    uint8_t fpgaIntf, tsnEnd, tsnfieldpos, phyinfsel;

    readReg1 = RD_REG32(SYSTEM_MNGR_BASE_ADDRESS + FPGA_INTF_EN3_REG);

    switch (emacindex) {
    case 0:
        readReg2 = RD_REG32(SYSTEM_MNGR_BASE_ADDRESS + TSN0_REG);
        tsnfieldmask = TSN0_FIELD_MASK;
        tsnfieldpos = TSN0_FIELD_POS;
        break;
    case 1:
        readReg2 = RD_REG32(SYSTEM_MNGR_BASE_ADDRESS + TSN1_REG);
        tsnfieldmask = TSN1_FIELD_MASK;
        tsnfieldpos = TSN1_FIELD_POS;
        break;
    case 2:
        readReg2 = RD_REG32(SYSTEM_MNGR_BASE_ADDRESS + TSN2_REG);
        tsnfieldmask = TSN2_FIELD_MASK;
        tsnfieldpos = TSN2_FIELD_POS;
        break;
    default:
        return XGMAC_LL_RETVAL_FAIL;
    }

    /* Verify the signals from the FPGA fabric to the TSN modules is enable or disabled */
    fpgaIntf = (readReg1 & tsnfieldmask) >> tsnfieldpos;

    /* Verify the endianness -> little endianness */
    tsnEnd = (readReg2 & SDB_DATA_ENDIAN_MASK) >> SDB_DATA_ENDIAN_POS;

    /* Verify phy_intf_sel field -> RGMII */
    phyinfsel = (readReg2 & PHY_INTF_SEL_FIELD_MASK);

    /* FPGA fabric -> Disabled && endianness -> little endianness && Phy_inf_sel-> RGMII interface */
    return (fpgaIntf == 0 && tsnEnd == 0 &&
           phyinfsel ==
           physelect) ? XGMAC_LL_RETVAL_SUCCESS : XGMAC_LL_RETVAL_FAIL;
}

int32_t xgmac_dma_software_reset(uint32_t *baseAddress)
{
    ENABLE_BIT(baseAddress + XGMAC_DMA_MODE, XGMAC_DMA_MODE_SWR_MASK);

    return XGMAC_LL_RETVAL_SUCCESS;
}

bool xgmac_is_dma_reset_done(uint32_t *baseAddress)
{
    int32_t regData;

    /* Check if the DMA Software Reset bit is cleared */
    regData = RD_REG32(baseAddress + XGMAC_DMA_MODE);
    return !(regData & XGMAC_DMA_MODE_SWR_MASK);
}

void xgmac_dma_init(uint32_t *baseAddress,
        const XGMACDevConfigStr_t *xgmacdevconfig)
{
    xgmac_config_dma_sysbus_mode(baseAddress, (const XGMACDMAConfig_t *)
            xgmacdevconfig->dmaConfig);
}


void xgmac_stop_dma(uint32_t *baseAddress, uint8_t dmachindx, uint8_t txrxflag)
{
    uint32_t val;

    if (txrxflag == XGMAC_DMA_TRANSMIT_STOP)
    {
        val = RD_DMA_CHNL_REG32(baseAddress,dmachindx, XGMAC_DMA_CH_TX_CONTROL);
        val &= ~(1 << XGMAC_DMA_CH_TX_CONTROL_ST_POS);
        WR_DMA_CHNL_REG32(baseAddress,dmachindx, XGMAC_DMA_CH_TX_CONTROL, val);
    }

    if (txrxflag == XGMAC_DMA_RECEIVE_STOP)
    {
        val = RD_DMA_CHNL_REG32(baseAddress,dmachindx, XGMAC_DMA_CH_RX_CONTROL);
        val &= ~(1 << XGMAC_DMA_CH_RX_CONTROL_SR_POS);
        WR_DMA_CHNL_REG32(baseAddress,dmachindx, XGMAC_DMA_CH_RX_CONTROL, val);
    }
}

void xgmac_start_dma(uint32_t *baseAddress, uint8_t dmachindx, uint8_t txrxflag)
{
    uint32_t val;

    if (txrxflag == XGMAC_DMA_TRANSMIT_START)
    {
        val = RD_DMA_CHNL_REG32(baseAddress,dmachindx, XGMAC_DMA_CH_TX_CONTROL);
        val |= (1 << XGMAC_DMA_CH_TX_CONTROL_ST_POS);
        WR_DMA_CHNL_REG32(baseAddress,dmachindx, XGMAC_DMA_CH_TX_CONTROL, val);
    }

    if (txrxflag == XGMAC_DMA_RECEIVE_START)
    {
        val = RD_DMA_CHNL_REG32(baseAddress,dmachindx, XGMAC_DMA_CH_RX_CONTROL);
        val |= (1 << XGMAC_DMA_CH_RX_CONTROL_SR_POS);
        WR_DMA_CHNL_REG32(baseAddress,dmachindx, XGMAC_DMA_CH_RX_CONTROL, val);
    }
}

void xgmac_config_dma_sysbus_mode(uint32_t *baseAddress,
        const XGMACDMAConfig_t *dmaconfig)
{
    uint32_t setmask = 0, clrmask = 0;

    /* Set/Clear mask bits for Undefined Burst Length */
    dmaconfig->ubl == 1? (setmask |= XGMAC_DMA_SYSBUS_MODE_UBL_MASK) : \
                    (clrmask |= XGMAC_DMA_SYSBUS_MODE_UBL_MASK);

    /* Set/Clear mask bits for AXI Burst Length 4 */
    dmaconfig->blen4 == 1? (setmask |= XGMAC_DMA_SYSBUS_MODE_BLEN4_MASK) : \
                      (clrmask |= XGMAC_DMA_SYSBUS_MODE_BLEN4_MASK);

    /* Set/Clear mask bits for AXI Burst Length 8 */
    dmaconfig->blen8 == 1? (setmask |= XGMAC_DMA_SYSBUS_MODE_BLEN8_MASK) : \
                      (clrmask |= XGMAC_DMA_SYSBUS_MODE_BLEN8_MASK);

    /* Set/Clear mask bits for AXI Burst Length 16 */
    dmaconfig->blen16 == 1? (setmask |= XGMAC_DMA_SYSBUS_MODE_BLEN16_MASK) : \
                       (clrmask |= XGMAC_DMA_SYSBUS_MODE_BLEN16_MASK);

    /* Set/Clear mask bits for AXI Burst Length 32 */
    dmaconfig->blen32 == 1? (setmask |= XGMAC_DMA_SYSBUS_MODE_BLEN32_MASK) : \
                       (clrmask |= XGMAC_DMA_SYSBUS_MODE_BLEN32_MASK);

    /* Set/Clear mask bits for AXI Burst Length 64 */
    dmaconfig->blen64 == 1? (setmask |= XGMAC_DMA_SYSBUS_MODE_BLEN64_MASK) : \
                       (clrmask |= XGMAC_DMA_SYSBUS_MODE_BLEN64_MASK);

    /* Set/Clear mask bits for AXI Burst Length 128 */
    dmaconfig->blen128 == 1? (setmask |= XGMAC_DMA_SYSBUS_MODE_BLEN128_MASK) : \
                        (clrmask |= XGMAC_DMA_SYSBUS_MODE_BLEN128_MASK);

    /* Set/Clear mask bits for AXI Burst Length 256 */
    dmaconfig->blen256 == 1? (setmask |= XGMAC_DMA_SYSBUS_MODE_BLEN256_MASK) : \
                        (clrmask |= XGMAC_DMA_SYSBUS_MODE_BLEN256_MASK);

    /* Set/Clear mask bits for Enhanced Address Mode Enable */
    dmaconfig->eame == 1? (setmask |= XGMAC_DMA_SYSBUS_MODE_EAME_MASK) : \
                     (clrmask |= XGMAC_DMA_SYSBUS_MODE_EAME_MASK);

    /* Set/Clear mask bits for Address-Aligned Beats */
    dmaconfig->aal == 1? (setmask |= XGMAC_DMA_SYSBUS_MODE_AAL_MASK) : \
                    (clrmask |= XGMAC_DMA_SYSBUS_MODE_AAL_MASK);

    /* Configure Maximum Read Outstanding Request Limit */
    setmask |= XGMAC_DMA_SYSBUS_MODE_RD_OSR_LMT_MASK;

    /* Configure Maximum Write Outstanding Request Limit */
    setmask |= XGMAC_DMA_SYSBUS_MODE_WR_OSR_LMT_MASK;

    /* Set DMA Sysbus Mode setmask bits */
    ENABLE_BIT(baseAddress + XGMAC_DMA_SYSBUS_MODE, setmask);

    /* Clear DMA Sysbus Mode clrmask bits */
    DISABLE_BIT(baseAddress + XGMAC_DMA_SYSBUS_MODE, clrmask);
}

void xgmac_init_dma_channel_desc_reg(uint32_t *baseAddress, uint8_t dmachindx,
        XgmacDmaDescAddr_t *dmadescparams)
{
    uint32_t val;

    /* Program the transmit ring length registers */
    val = dmadescparams->ulTxRingLen & XGMAC_DMA_CH0_TX_CONTROL2_TDRL_MASK;
    WR_DMA_CHNL_REG32(baseAddress,dmachindx, XGMAC_DMA_CH_TX_CONTROL2, val);

    /* Program the receive ring length registers */
    val = dmadescparams->ulRxRingLen & XGMAC_DMA_CH0_RX_CONTROL2_RDRL_MASK;
    WR_DMA_CHNL_REG32(baseAddress,dmachindx, XGMAC_DMA_CH_RX_CONTROL2, val);

    /* Program Tx List Address Registers with Base Address of Ring Descriptor */
    val = dmadescparams->ulTxDescHighAddr;
    WR_DMA_CHNL_REG32(baseAddress,dmachindx, XGMAC_DMA_CH_TXDESC_LIST_HADDRESS,
            val);
    val = dmadescparams->ulTxDescLowAddr;
    WR_DMA_CHNL_REG32(baseAddress,dmachindx, XGMAC_DMA_CH_TXDESC_LIST_LADDRESS,
            val);

    /* Program Rx List Address Registers with Base Address of Ring Descriptor */
    val = dmadescparams->ulRxDescHighAddr;
    WR_DMA_CHNL_REG32(baseAddress,dmachindx, XGMAC_DMA_CH_RXDESC_LIST_HADDRESS,
            val);
    val = dmadescparams->ulRxDescLowAddr;
    WR_DMA_CHNL_REG32(baseAddress,dmachindx, XGMAC_DMA_CH_RXDESC_LIST_LADDRESS,
            val);

    /* Program the  Tx Tail Pointer Register */
    val = dmadescparams->ulTxLastDescAddr;
    WR_DMA_CHNL_REG32(baseAddress,dmachindx, XGMAC_DMA_CH_TXDESC_TAIL_LPOINTER,
            val);

    /* Program the  Rx Tail Pointer Register */
    val = dmadescparams->ulRxLastDescAddr;
    WR_DMA_CHNL_REG32(baseAddress,dmachindx, XGMAC_DMA_CH_RXDESC_TAIL_LPOINTER,
            val);
}

void xgmac_config_dma_channel_control(uint32_t *baseAddress, uint8_t dmachindx,
        const XGMACDMAChanlConfig_t *dmachnlconfig )
{
    uint32_t val;

    /* Program  DMA Control Settings - PBLx8 Enable*/
    if (dmachnlconfig->pblx8 == TRUE)
        ENABLE_DMA_CHNL_REGBIT(baseAddress + XGMAC_DMA_CH_CONTROL, dmachindx,
                XGMAC_DMA_CH0_CONTROL_PBLX8_MASK);
    else
        DISABLE_DMA_CHNL_REGBIT(baseAddress + XGMAC_DMA_CH_CONTROL, dmachindx,
                XGMAC_DMA_CH0_CONTROL_PBLX8_MASK);

    /* Program  DMA Control Settings - SPH Enable*/
    if (dmachnlconfig->sph == TRUE)
        ENABLE_DMA_CHNL_REGBIT(baseAddress + XGMAC_DMA_CH_CONTROL, dmachindx,
                XGMAC_DMA_CH0_CONTROL_SPH_MASK);
    else
        DISABLE_DMA_CHNL_REGBIT(baseAddress + XGMAC_DMA_CH_CONTROL, dmachindx,
                XGMAC_DMA_CH0_CONTROL_SPH_MASK);

    /* Program  DMA Control Settings - DSL Descriptor Skip Length */
    val = RD_DMA_CHNL_REG32(baseAddress,dmachindx, XGMAC_DMA_CH_CONTROL);
    val |= dmachnlconfig->dsl << XGMAC_DMA_CH_CONTROL_DSL_POS;
    WR_DMA_CHNL_REG32(baseAddress,dmachindx, XGMAC_DMA_CH_CONTROL,val);

    /* Program  DMA Tx Control Settings - TSE Enable*/
    if (dmachnlconfig->tse == TRUE)
        ENABLE_DMA_CHNL_REGBIT(baseAddress + XGMAC_DMA_CH_TX_CONTROL, dmachindx,
                XGMAC_DMA_CH0_TX_CONTROL_TSE_MASK);
    else
        DISABLE_DMA_CHNL_REGBIT(baseAddress + XGMAC_DMA_CH_TX_CONTROL,
                dmachindx,
                XGMAC_DMA_CH0_TX_CONTROL_TSE_MASK);

    /* Program  DMA Tx Control Settings - Write Txpbl */
    val = RD_DMA_CHNL_REG32(baseAddress,dmachindx, XGMAC_DMA_CH_TX_CONTROL);
    val |= dmachnlconfig->txpbl << XGMAC_DMA_CH_TX_CONTROL_TXPBL_POS;
    WR_DMA_CHNL_REG32(baseAddress,dmachindx, XGMAC_DMA_CH_TX_CONTROL, val);

    /* Program  DMA Rx Control Settings - Write Rxpbl */
    val = RD_DMA_CHNL_REG32(baseAddress,dmachindx, XGMAC_DMA_CH_RX_CONTROL);
    val |= dmachnlconfig->rxpbl << XGMAC_DMA_CH_RX_CONTROL_RXPBL_POS;
    WR_DMA_CHNL_REG32(baseAddress,dmachindx, XGMAC_DMA_CH_RX_CONTROL,val);

    /* Program  DMA Rx Control Settings - RBSZ Receive Buffer Size*/
    val = RD_DMA_CHNL_REG32(baseAddress,dmachindx, XGMAC_DMA_CH_RX_CONTROL);
    val |= dmachnlconfig->rbsz << XGMAC_DMA_CH_RX_CONTROL_RBSZ_POS;
    WR_DMA_CHNL_REG32(baseAddress,dmachindx, XGMAC_DMA_CH_RX_CONTROL, val);

}
int32_t xgmac_enable_dma_interrupt( uint32_t *baseAddress, uint8_t chindx,
        XgmacDmaInterruptID_t id)
{
    uint32_t val;
    uint32_t intrmask;

    /* Clear the DMA channel status register bits if set. Its a sticky bit hence write back to clear */
    val = RD_DMA_CHNL_REG32(baseAddress,chindx, XGMAC_DMA_CH_STATUS);
    WR_DMA_CHNL_REG32(baseAddress,chindx, XGMAC_DMA_CH_STATUS, val);

    switch (id)
    {
    case eInterruptNIS:
        intrmask = XGMAC_DMA_INTR_MASK_TI | XGMAC_DMA_INTR_MASK_RI |
                XGMAC_DMA_INTR_MASK_NIS;
        break;

    case eInterruptAIS:
        intrmask = XGMAC_DMA_INTR_MASK_FBE | XGMAC_DMA_INTR_MASK_TXS |
                XGMAC_DMA_INTR_MASK_RBU | XGMAC_DMA_INTR_MASK_RS |
                XGMAC_DMA_INTR_MASK_DDE | XGMAC_DMA_INTR_MASK_AIS;
        break;

    default:
        return XGMAC_LL_RETVAL_FAIL;
    }

    val = RD_DMA_CHNL_REG32(baseAddress,chindx, XGMAC_DMA_CH_INTERRUPT_ENABLE);
    val |= intrmask;
    WR_DMA_CHNL_REG32(baseAddress,chindx, XGMAC_DMA_CH_INTERRUPT_ENABLE, val);

    return XGMAC_LL_RETVAL_SUCCESS;
}

int32_t xgmac_disable_dma_interrupt( uint32_t *baseAddress, uint8_t chindx,
        XgmacDmaInterruptID_t id)
{
    switch (id)
    {
    /* Received data available interrupt */
    case eInterruptTI:
        DISABLE_DMA_CHNL_REGBIT(baseAddress + XGMAC_DMA_CH_INTERRUPT_ENABLE,
                chindx, XGMAC_DMA_CH0_INTERRUPT_ENABLE_TIE_MASK);
        break;

    case eInterruptTXS:
        DISABLE_DMA_CHNL_REGBIT(baseAddress + XGMAC_DMA_CH_INTERRUPT_ENABLE,
                chindx, XGMAC_DMA_CH0_INTERRUPT_ENABLE_TXSE_MASK);
        break;

    case eInterruptTBU:
        DISABLE_DMA_CHNL_REGBIT(baseAddress + XGMAC_DMA_CH_INTERRUPT_ENABLE,
                chindx, XGMAC_DMA_CH0_INTERRUPT_ENABLE_TBUE_MASK);
        break;

    case eInterruptRI:
        DISABLE_DMA_CHNL_REGBIT(baseAddress + XGMAC_DMA_CH_INTERRUPT_ENABLE,
                chindx, XGMAC_DMA_CH0_INTERRUPT_ENABLE_RIE_MASK);
        break;

    case eInterruptRBU:
        DISABLE_DMA_CHNL_REGBIT(baseAddress + XGMAC_DMA_CH_INTERRUPT_ENABLE,
                chindx, XGMAC_DMA_CH0_INTERRUPT_ENABLE_RBUE_MASK);
        break;

    case eInterruptRS:
        DISABLE_DMA_CHNL_REGBIT(baseAddress + XGMAC_DMA_CH_INTERRUPT_ENABLE,
                chindx, XGMAC_DMA_CH0_INTERRUPT_ENABLE_RSE_MASK);
        break;

    case eInterruptDDE:
        DISABLE_DMA_CHNL_REGBIT(baseAddress + XGMAC_DMA_CH_INTERRUPT_ENABLE,
                chindx, XGMAC_DMA_CH0_INTERRUPT_ENABLE_DDEE_MASK);
        break;
    case eInterruptFBE:
        DISABLE_DMA_CHNL_REGBIT(baseAddress + XGMAC_DMA_CH_INTERRUPT_ENABLE,
                chindx, XGMAC_DMA_CH0_INTERRUPT_ENABLE_FBEE_MASK);
        break;

    case eInterruptCDE:
        DISABLE_DMA_CHNL_REGBIT(baseAddress + XGMAC_DMA_CH_INTERRUPT_ENABLE,
                chindx, XGMAC_DMA_CH0_INTERRUPT_ENABLE_CDEE_MASK);
        break;

    case eInterruptAIS:
        DISABLE_DMA_CHNL_REGBIT(baseAddress + XGMAC_DMA_CH_INTERRUPT_ENABLE,
                chindx, XGMAC_DMA_CH0_INTERRUPT_ENABLE_AIE_MASK);
        break;

    case eInterruptNIS:
        DISABLE_DMA_CHNL_REGBIT(baseAddress + XGMAC_DMA_CH_INTERRUPT_ENABLE,
                chindx, XGMAC_DMA_CH0_INTERRUPT_ENABLE_NIE_MASK);
        break;

    default:
        return -1;

    }
    return 0;
}

void xgmac_mac_init(uint32_t *baseAddress,
        const XGMACDevConfigStr_t *xgmacdevconfig )
{
    uint8_t numqueues;
    const XGMACDevConfig_t *macdevconfig =
            (const XGMACDevConfig_t *)(xgmacdevconfig->macDevConfig);

    /* Configure  MAC Rx Queue Control Register */
    xgmac_config_macrxqctrl_regs(baseAddress, (const XGMACMACRxQCtrlConfig_t *)
            xgmacdevconfig->macRxQCtrlConfig);

    /* Program MAC Frame Filter Register  for Promiscuous mode and Receive all */
    xgmac_config_mac_frame_filter(baseAddress,(const XGMACMACPktFilterConfig_t *)
            xgmacdevconfig->macPktFilterConfig);

    /* Program MAC Transmit Flow Control Register */
    numqueues = macdevconfig->noftxqueues;
    for (uint8_t qindex=0; qindex < numqueues; qindex++)
    {
        xgmac_enable_tx_flow_control(baseAddress, qindex,
                (const XGMACMACTxFlowCtrlConfig_t *)
                xgmacdevconfig->macTxFlowCtrlConfig);
    }

    /* Program MAC Receive Flow Control Register */
    xgmac_enable_rx_flow_control(baseAddress,
            (const XGMACMACRxFlowCtrlConfig_t *)
            xgmacdevconfig->macRxFlowCtrlConfig);

    /* Program MAC Tx Configuration Registers */
    xgmac_config_mac_tx(baseAddress, (const XGMACMACTxConfig_t *)
            xgmacdevconfig->macTxConfig);


    /* Set Checksum Offload to COE for IPv4 Header & TCP, UDP, ICMP Payload  Rx packets */
    xgmac_config_mac_rx(baseAddress, (const XGMACMACRxConfig_t *)
            xgmacdevconfig->macRxConfig);

    /* MAC Management counter config */
    xgmac_mmc_setup(baseAddress);
}

void xgmac_start_stop_mac_tx(uint32_t *baseAddress, bool stflag)
{
    if (stflag == TRUE)
    {
        ENABLE_BIT(baseAddress + XGMAC_MAC_TX_CONFIGURATION,
                XGMAC_MAC_TX_CONFIGURATION_TE_MASK);
    }
    else
    {
        DISABLE_BIT(baseAddress + XGMAC_MAC_TX_CONFIGURATION,
                XGMAC_MAC_TX_CONFIGURATION_TE_MASK);
    }
}

void xgmac_start_stop_mac_rx(uint32_t *baseAddress, bool stflag)
{
    if (stflag == TRUE)
    {
        ENABLE_BIT(baseAddress + XGMAC_MAC_RX_CONFIGURATION,
                XGMAC_MAC_RX_CONFIGURATION_RE_MASK);
    }
    else
    {
        DISABLE_BIT(baseAddress + XGMAC_MAC_RX_CONFIGURATION,
                XGMAC_MAC_RX_CONFIGURATION_RE_MASK);
    }
}

void xgmac_config_macrxqctrl_regs(uint32_t *baseAddress,
        const XGMACMACRxQCtrlConfig_t *macrxqctrlconfig)
{
    /* Clear and set the Receive Queue 0 */
    DISABLE_BIT(baseAddress + XGMAC_MAC_RXQ_CTRL0,
            XGMAC_MAC_RXQ_CTRL0_RXQ0EN_MASK);

    /* Enable for Data Center Bridging/Generic */
    ENABLE_BIT(baseAddress + XGMAC_MAC_RXQ_CTRL0,
            macrxqctrlconfig->rxq0en <<
                XGMAC_MAC_RXQ_CTRL0_RXQ0EN_POS);

    /* Enable/Disable Multicast and Broadcast Queue Enable */
    if (macrxqctrlconfig->mcbcqen == 1)
        ENABLE_BIT(baseAddress + XGMAC_MAC_RXQ_CTRL1,
                XGMAC_MAC_RXQ_CTRL1_MCBCQEN_MASK);
    else
        DISABLE_BIT(baseAddress + XGMAC_MAC_RXQ_CTRL1,
                XGMAC_MAC_RXQ_CTRL1_MCBCQEN_MASK);
}

void xgmac_config_mac_frame_filter(uint32_t *baseAddress,
        const XGMACMACPktFilterConfig_t *macpktfilterconfig)
{
    uint8_t val;

    /* Program Promiscuous Mode */
    if (macpktfilterconfig->pr == 1)
        ENABLE_BIT(baseAddress + XGMAC_MAC_PACKET_FILTER,
                XGMAC_MAC_PACKET_FILTER_PR_MASK);
    else
        DISABLE_BIT(baseAddress + XGMAC_MAC_PACKET_FILTER,
                XGMAC_MAC_PACKET_FILTER_PR_MASK);

    /* Program Hash Unicast */
    if (macpktfilterconfig->huc == 1)
        ENABLE_BIT(baseAddress + XGMAC_MAC_PACKET_FILTER,
                XGMAC_MAC_PACKET_FILTER_HUC_MASK);
    else
        DISABLE_BIT(baseAddress + XGMAC_MAC_PACKET_FILTER,
                XGMAC_MAC_PACKET_FILTER_HUC_MASK);

    /* Program Hash Multicast */
    if (macpktfilterconfig->hmc == 1)
        ENABLE_BIT(baseAddress + XGMAC_MAC_PACKET_FILTER,
                XGMAC_MAC_PACKET_FILTER_HMC_MASK);
    else
        DISABLE_BIT(baseAddress + XGMAC_MAC_PACKET_FILTER,
                XGMAC_MAC_PACKET_FILTER_HMC_MASK);

    /* Program DA Inverse Filtering */
    if (macpktfilterconfig->daif == 1)
        ENABLE_BIT(baseAddress + XGMAC_MAC_PACKET_FILTER,
                XGMAC_MAC_PACKET_FILTER_DAIF_MASK);
    else
        DISABLE_BIT(baseAddress + XGMAC_MAC_PACKET_FILTER,
                XGMAC_MAC_PACKET_FILTER_DAIF_MASK);

    /* Program Pass All Multicast */
    if (macpktfilterconfig->pm == 1)
        ENABLE_BIT(baseAddress + XGMAC_MAC_PACKET_FILTER,
                XGMAC_MAC_PACKET_FILTER_PM_MASK);
    else
        DISABLE_BIT(baseAddress + XGMAC_MAC_PACKET_FILTER,
                XGMAC_MAC_PACKET_FILTER_PM_MASK);

    /* Program Disable Broadcast Packets  */
    if (macpktfilterconfig->dbf == 1)
        ENABLE_BIT(baseAddress + XGMAC_MAC_PACKET_FILTER,
                XGMAC_MAC_PACKET_FILTER_DBF_MASK);
    else
        DISABLE_BIT(baseAddress + XGMAC_MAC_PACKET_FILTER,
                XGMAC_MAC_PACKET_FILTER_DBF_MASK);

    /* Program Pass Control Packets  */
    val = RD_REG32(baseAddress + XGMAC_MAC_PACKET_FILTER);
    val |= macpktfilterconfig->pcf << XGMAC_MAC_PACKET_FILTER_PCF_POS;
    WR_REG32(baseAddress + XGMAC_MAC_PACKET_FILTER, val);

    /* Program SA Inverse Filtering */
    if (macpktfilterconfig->saif == 1)
        ENABLE_BIT(baseAddress + XGMAC_MAC_PACKET_FILTER,
                XGMAC_MAC_PACKET_FILTER_SAIF_MASK);
    else
        DISABLE_BIT(baseAddress + XGMAC_MAC_PACKET_FILTER,
                XGMAC_MAC_PACKET_FILTER_SAIF_MASK);

    /* Program Source Address Filter Enable */
    if (macpktfilterconfig->saf == 1)
        ENABLE_BIT(baseAddress + XGMAC_MAC_PACKET_FILTER,
                XGMAC_MAC_PACKET_FILTER_SAF_MASK);
    else
        DISABLE_BIT(baseAddress + XGMAC_MAC_PACKET_FILTER,
                XGMAC_MAC_PACKET_FILTER_SAF_MASK);

    /* Program Hash or Perfect Filter */
    if (macpktfilterconfig->hpf == 1)
        ENABLE_BIT(baseAddress + XGMAC_MAC_PACKET_FILTER,
                XGMAC_MAC_PACKET_FILTER_HPF_MASK);
    else
        DISABLE_BIT(baseAddress + XGMAC_MAC_PACKET_FILTER,
                XGMAC_MAC_PACKET_FILTER_HPF_MASK);

    /* Program DA Hash Index or L3/L4 Filter Number in Receive  Filter */
    val = RD_REG32(baseAddress + XGMAC_MAC_PACKET_FILTER);
    val |= macpktfilterconfig->dhlfrs << XGMAC_MAC_PACKET_FILTER_DHLFRS_POS;
    WR_REG32(baseAddress + XGMAC_MAC_PACKET_FILTER, val);

    /* Program VLAN Tag Filter Enable */
    if (macpktfilterconfig->vtfe == 1)
        ENABLE_BIT(baseAddress + XGMAC_MAC_PACKET_FILTER,
                XGMAC_MAC_PACKET_FILTER_VTFE_MASK);
    else
        DISABLE_BIT(baseAddress + XGMAC_MAC_PACKET_FILTER,
                XGMAC_MAC_PACKET_FILTER_VTFE_MASK);

    /* Program Layer 3 and Layer 4 Filter Enable */
    if (macpktfilterconfig->ipfe == 1)
        ENABLE_BIT(baseAddress + XGMAC_MAC_PACKET_FILTER,
                XGMAC_MAC_PACKET_FILTER_IPFE_MASK);
    else
        DISABLE_BIT(baseAddress + XGMAC_MAC_PACKET_FILTER,
                XGMAC_MAC_PACKET_FILTER_IPFE_MASK);

    /* Program Drop Non-TCP/UDP over IP Packets */
    if (macpktfilterconfig->dntu == 1)
        ENABLE_BIT(baseAddress + XGMAC_MAC_PACKET_FILTER,
                XGMAC_MAC_PACKET_FILTER_DNTU_MASK);
    else
        DISABLE_BIT(baseAddress + XGMAC_MAC_PACKET_FILTER,
                XGMAC_MAC_PACKET_FILTER_DNTU_MASK);

    /* Program Receive All */
    if (macpktfilterconfig->ra == 1)
        ENABLE_BIT(baseAddress + XGMAC_MAC_PACKET_FILTER,
                XGMAC_MAC_PACKET_FILTER_RA_MASK);
    else
        DISABLE_BIT(baseAddress + XGMAC_MAC_PACKET_FILTER,
                XGMAC_MAC_PACKET_FILTER_RA_MASK);
}

void xgmac_config_mac_tx(uint32_t *baseAddress,
        const XGMACMACTxConfig_t *mactxconfig)
{
    /* Program JD MAC configuration to disable Jumbo frames in Tx */
    if (mactxconfig->jd == 1)
        ENABLE_BIT(baseAddress + XGMAC_MAC_TX_CONFIGURATION,
                XGMAC_MAC_TX_CONFIGURATION_JD_MASK);
    else
        DISABLE_BIT(baseAddress + XGMAC_MAC_TX_CONFIGURATION,
                XGMAC_MAC_TX_CONFIGURATION_JD_MASK);
}

void xgmac_config_mac_rx(uint32_t *baseAddress,
        const XGMACMACRxConfig_t *macrxconfig)
{
    uint32_t setmask = 0, clrmask = 0;
    uint16_t val;

    /* Set/Clear mask bits for Automatic Pad or CRC Stripping */
    macrxconfig->acs == 1? (setmask |= XGMAC_MAC_RX_CONFIGURATION_ACS_MASK) : \
    (clrmask |= XGMAC_MAC_RX_CONFIGURATION_ACS_MASK);

    /* Set/Clear mask bits for CRC stripping for Type packets */
    macrxconfig->cst == 1? (setmask |= XGMAC_MAC_RX_CONFIGURATION_CST_MASK) : \
    (clrmask |= XGMAC_MAC_RX_CONFIGURATION_CST_MASK);

    /* Set/Clear mask bits for Disable CRC Checking for Received Packets */
    macrxconfig->dcrcc ==
    1? (setmask |= XGMAC_MAC_RX_CONFIGURATION_DCRCC_MASK) : \
    (clrmask |= XGMAC_MAC_RX_CONFIGURATION_DCRCC_MASK);

    /* Set/Clear mask bits for Slow Protocol Detection Enable */
    macrxconfig->spen == 1? (setmask |= XGMAC_MAC_RX_CONFIGURATION_SPEN_MASK) : \
    (clrmask |= XGMAC_MAC_RX_CONFIGURATION_SPEN_MASK);

    /* Set/Clear mask bits for Unicast Slow Protocol Packet Detect */
    macrxconfig->usp == 1? (setmask |= XGMAC_MAC_RX_CONFIGURATION_USP_MASK) : \
    (clrmask |= XGMAC_MAC_RX_CONFIGURATION_USP_MASK);

    /* Set/Clear mask bits for Giant Packet Size Limit Control Enable */
    macrxconfig->gpslce ==
    1? (setmask |= XGMAC_MAC_RX_CONFIGURATION_GPSLCE_MASK) : \
    (clrmask |= XGMAC_MAC_RX_CONFIGURATION_GPSLCE_MASK);

    /* Set/Clear mask bits for Watchdog Disable Enable */
    macrxconfig->wd == 1? (setmask |= XGMAC_MAC_RX_CONFIGURATION_WD_MASK) : \
    (clrmask |= XGMAC_MAC_RX_CONFIGURATION_WD_MASK);

    /* Set/Clear mask bits for Jumbo Packet Enable */
    macrxconfig->je == 1? (setmask |= XGMAC_MAC_RX_CONFIGURATION_JE_MASK) : \
    (clrmask |= XGMAC_MAC_RX_CONFIGURATION_JE_MASK);

    /* Set/Clear mask bits for Checksum Offload Enable */
    macrxconfig->ipc == 1? (setmask |= XGMAC_MAC_RX_CONFIGURATION_IPC_MASK) : \
    (clrmask |= XGMAC_MAC_RX_CONFIGURATION_IPC_MASK);

    /* Set/Clear mask bits for ARP enable */
    macrxconfig->arpen ==
    1? (setmask |= XGMAC_MAC_RX_CONFIGURATION_ARPEN_MASK) : \
    (clrmask |= XGMAC_MAC_RX_CONFIGURATION_ARPEN_MASK);

    ENABLE_BIT(baseAddress + XGMAC_MAC_RX_CONFIGURATION, setmask);

    DISABLE_BIT(baseAddress + XGMAC_MAC_RX_CONFIGURATION, clrmask);

    /* Program Giant Packet Size Limit */
    val = RD_REG32(baseAddress + XGMAC_MAC_RX_CONFIGURATION);
    val |= macrxconfig->gpsl << XGMAC_MAC_RX_CONFIGURATION_GPSL_POS;
    WR_REG32(baseAddress + XGMAC_MAC_RX_CONFIGURATION, val);
}

void xgmac_enable_tx_flow_control(uint32_t *baseAddress, uint8_t qindx,
        const XGMACMACTxFlowCtrlConfig_t *mactxflowctrlconfig)
{
    /* Set Pause frame */
    ENABLE_MAC_FLOW_CNTL_REGBIT(baseAddress + XGMAC_MAC_Q0_TX_FLOW_CTRL, qindx,
            mactxflowctrlconfig->pt << XGMAC_MAC_Q6_TX_FLOW_CTRL_PT_POS);

    /* Set Pause Low Threshold */
    ENABLE_MAC_FLOW_CNTL_REGBIT(baseAddress + XGMAC_MAC_Q0_TX_FLOW_CTRL, qindx,
            mactxflowctrlconfig->plt << XGMAC_MAC_Q0_TX_FLOW_CTRL_PLT_POS);

    /* Enable Transmit Flow control */
    if (mactxflowctrlconfig->tfe == 1)
        ENABLE_MAC_FLOW_CNTL_REGBIT(baseAddress + XGMAC_MAC_Q0_TX_FLOW_CTRL,
                qindx,
                XGMAC_MAC_Q0_TX_FLOW_CTRL_TFE_MASK);
    else
        DISABLE_MAC_FLOW_CNTL_REGBIT(baseAddress + XGMAC_MAC_Q0_TX_FLOW_CTRL,
                qindx,
                XGMAC_MAC_Q0_TX_FLOW_CTRL_TFE_MASK);

    /* Enable Flow Control Busy */
    if (mactxflowctrlconfig->fcb == 1)
        ENABLE_MAC_FLOW_CNTL_REGBIT(baseAddress + XGMAC_MAC_Q0_TX_FLOW_CTRL,
                qindx,
                XGMAC_MAC_Q0_TX_FLOW_CTRL_FCB_MASK);
    else
        DISABLE_MAC_FLOW_CNTL_REGBIT(baseAddress + XGMAC_MAC_Q0_TX_FLOW_CTRL,
                qindx,
                XGMAC_MAC_Q0_TX_FLOW_CTRL_FCB_MASK);

    /* Disable Zero-Quanta Pause */
    if (mactxflowctrlconfig->dzpq == 1)
        ENABLE_MAC_FLOW_CNTL_REGBIT(baseAddress + XGMAC_MAC_Q0_TX_FLOW_CTRL,
                qindx,
                XGMAC_MAC_Q0_TX_FLOW_CTRL_DZPQ_MASK);
    else
        DISABLE_MAC_FLOW_CNTL_REGBIT(baseAddress + XGMAC_MAC_Q0_TX_FLOW_CTRL,
                qindx,
                XGMAC_MAC_Q0_TX_FLOW_CTRL_DZPQ_MASK);
}

void xgmac_enable_rx_flow_control(uint32_t *baseAddress,
        const XGMACMACRxFlowCtrlConfig_t *macrxflowctrlconfig)
{
    /* Program Rx Flow Control */
    if (macrxflowctrlconfig->rfe == 1)
        ENABLE_BIT(baseAddress + XGMAC_MAC_RX_FLOW_CTRL,
                XGMAC_MAC_RX_FLOW_CTRL_RFE_MASK);
    else
        DISABLE_BIT(baseAddress + XGMAC_MAC_RX_FLOW_CTRL,
                XGMAC_MAC_RX_FLOW_CTRL_RFE_MASK);

    /* Program Unicast Pause Packet Detect */
    if (macrxflowctrlconfig->up == 1)
        ENABLE_BIT(baseAddress + XGMAC_MAC_RX_FLOW_CTRL,
                XGMAC_MAC_RX_FLOW_CTRL_UP_MASK);
    else
        DISABLE_BIT(baseAddress + XGMAC_MAC_RX_FLOW_CTRL,
                XGMAC_MAC_RX_FLOW_CTRL_UP_MASK);

    /* Program Priority Based Flow Control Enable */
    if (macrxflowctrlconfig->pfce == 1)
        ENABLE_BIT(baseAddress + XGMAC_MAC_RX_FLOW_CTRL,
                XGMAC_MAC_RX_FLOW_CTRL_PFCE_MASK);
    else
        DISABLE_BIT(baseAddress + XGMAC_MAC_RX_FLOW_CTRL,
                XGMAC_MAC_RX_FLOW_CTRL_PFCE_MASK);
}

void xgmac_mtl_init (uint32_t *baseAddress,
        const XGMACDevConfigStr_t *xgmacdevconfig)
{
    uint8_t qindex;
    uint8_t numqueues;
    const XGMACDevConfig_t *macdevconfig =
            (const XGMACDevConfig_t *)(xgmacdevconfig->macDevConfig);

    /* Program MTL configuration registers for Tx */
    numqueues = macdevconfig->noftxqueues;
    for (qindex=0; qindex < numqueues; qindex++)
    {
        xgmac_set_mtl_tx_regs(baseAddress, qindex,
                (const XGMACMTLTxQueueConfig_t *)
                xgmacdevconfig->mtlTxQConfig);
    }

    /* Program MTL configuration registers for Rx */
    numqueues = macdevconfig->nofrxqueues;
    for (qindex= 0; qindex < numqueues; qindex++)
    {
        xgmac_set_mtl_rx_regs(baseAddress, qindex,
                (const XGMACMTLRxQueueConfig_t *)
                xgmacdevconfig->mtlRxQConfig);
    }
}

/* XGMAC Device start */
void xgmac_dev_start(uint32_t *baseAddress)
{
    /* Start the MAC Transmitter */
    xgmac_start_stop_mac_tx(baseAddress, TRUE);

    /* Start the MAC Receiver  */
    xgmac_start_stop_mac_rx(baseAddress, TRUE);
}

/* XGMAC Device stop */
void xgmac_dev_stop(uint32_t *baseAddress)
{
    /* Stop the MAC Transmitter */
    xgmac_start_stop_mac_tx(baseAddress, FALSE);

    /* Start the MAC Receiver  */
    xgmac_start_stop_mac_rx(baseAddress, FALSE);
}

void xgmac_set_mtl_tx_regs(uint32_t *baseAddress, uint8_t qindx, \
        const XGMACMTLTxQueueConfig_t *mtltxqcfgparams)
{
    uint32_t val;
    uint32_t reg_val;
    uint8_t tqs;

    /* Compute Tqs */
    reg_val = RD_REG32(baseAddress + XGMAC_MAC_HW_FEATURE1);
    tqs = XGMAC_MTL_TX_FIFO_BLK_CNT(reg_val);

    /* Enable Transmit Queue Store and Forward */
    if (mtltxqcfgparams->tsf == 1)
        ENABLE_MTL_QX_REGBIT(baseAddress + XGMAC_MTL_TXQ_OPERATION_MODE, qindx,
                XGMAC_MTL_TXQ0_OPERATION_MODE_TSF_MASK);

    /* Disable Transmit Queue Store and Forward */
    else
        DISABLE_MTL_QX_REGBIT(baseAddress + XGMAC_MTL_TXQ_OPERATION_MODE, qindx,
                XGMAC_MTL_TXQ0_OPERATION_MODE_TSF_MASK);

    /* Enable Tx Queue */
    ENABLE_MTL_QX_REGBIT(baseAddress + XGMAC_MTL_TXQ_OPERATION_MODE, qindx,
            mtltxqcfgparams->txqen << XGMAC_MTL_TXQ0_OPERATION_MODE_TXQEN_POS);

    /* Program Transmit Queue Size */
    val = RD_MTL_QX_REG32(baseAddress, qindx, XGMAC_MTL_TXQ_OPERATION_MODE);
    val |= tqs << XGMAC_MTL_TXQ0_OPERATION_MODE_TQS_POS;
    WR_MTL_QX_REG32(baseAddress, qindx, XGMAC_MTL_TXQ_OPERATION_MODE, val);
}

void xgmac_set_mtl_rx_regs(uint32_t *baseAddress, uint8_t qindx, \
        const XGMACMTLRxQueueConfig_t *mtlrxqcfgparams)
{
    uint32_t val;
    uint32_t reg_val;
    uint8_t rqs;

    /* Compute Rqs */
    reg_val = RD_REG32(baseAddress + XGMAC_MAC_HW_FEATURE1);
    rqs = XGMAC_MTL_RX_FIFO_BLK_CNT(reg_val);

    /* Enable Receive Queue Store and Forward */
    if (mtlrxqcfgparams->rsf == 1)
        ENABLE_MTL_QX_REGBIT(baseAddress + XGMAC_MTL_RXQ_OPERATION_MODE, qindx,
                XGMAC_MTL_RXQ_OPERATION_MODE_RSF_MASK);

    /* Disable Receive Queue Store and Forward */
    else
        DISABLE_MTL_QX_REGBIT(baseAddress + XGMAC_MTL_RXQ_OPERATION_MODE, qindx,
                XGMAC_MTL_RXQ_OPERATION_MODE_RSF_MASK);

    /* Enable Hardware Flow Control */
    if (mtlrxqcfgparams->ehfc == 1)
        ENABLE_MTL_QX_REGBIT(baseAddress + XGMAC_MTL_RXQ_OPERATION_MODE, qindx,
                XGMAC_MTL_RXQ_OPERATION_MODE_EHFC_MASK);

    /* Disable Hardware Flow Control */
    else
        DISABLE_MTL_QX_REGBIT(baseAddress + XGMAC_MTL_RXQ_OPERATION_MODE, qindx,
                XGMAC_MTL_RXQ_OPERATION_MODE_EHFC_MASK);

    /* Program Receive Queue Size */
    val = RD_MTL_QX_REG32(baseAddress, qindx, XGMAC_MTL_RXQ_OPERATION_MODE);
    val |= rqs << XGMAC_MTL_RXQ_OPERATION_MODE_RQS_POS;
    WR_MTL_QX_REG32(baseAddress, qindx, XGMAC_MTL_RXQ_OPERATION_MODE,val);
}

void xgmac_disable_interrupt(uint32_t *baseAddress)
{
    /* Read the XGMAC interrupt status register */
    uint32_t ulMACIntrStatus =  RD_REG32(
            baseAddress + XGMAC_MAC_INTERRUPT_STATUS);

    /* Check for any active interrupt */
    ulMACIntrStatus &= ~XGMAC_MAC_INTERRUPT_STATUS_LSI_MASK;

    /* Clear the interrupt */
    WR_REG32(baseAddress + XGMAC_MAC_INTERRUPT_STATUS,ulMACIntrStatus);
}

void xgmac_start_dma_dev(uint32_t *baseAddress,
        const XGMACDevConfigStr_t *xgmacdevconfig)
{
    uint8_t dmachnlindex;
    uint8_t dnanumchnls;

    dnanumchnls = xgmacdevconfig->macDevConfig->nofdmachannels;

    for (dmachnlindex= 0; dmachnlindex < dnanumchnls; dmachnlindex++)
    {
        /* Start Receive and Transmit DMA */
        xgmac_start_dma(baseAddress,dmachnlindex, XGMAC_DMA_TRANSMIT_START);

        xgmac_start_dma(baseAddress,dmachnlindex, XGMAC_DMA_RECEIVE_START);
    }
}
void xgmac_stop_dma_dev(uint32_t *baseAddress,
        const XGMACDevConfigStr_t *xgmacdevconfig)
{
    uint8_t dmachnlindex;
    uint8_t dmanumchnls;

    dmanumchnls = xgmacdevconfig->macDevConfig->nofdmachannels;

    for (dmachnlindex= 0; dmachnlindex < dmanumchnls; dmachnlindex++)
    {
        /* Stop Receive and Transmit DMA */
        xgmac_stop_dma(baseAddress,dmachnlindex, XGMAC_DMA_TRANSMIT_STOP);

        xgmac_stop_dma(baseAddress,dmachnlindex, XGMAC_DMA_RECEIVE_STOP);
    }
}


XgmacDmaInterruptID_t check_and_clear_xgmac_interrupt_status(
        uint32_t baseAddress )
{
    uint32_t val;
    XgmacDmaInterruptID_t res;

    /* Read the status */
    val = RD_REG32(baseAddress + XGMAC_DMA_CH_STATUS);

    /* Clear the status */
    WR_REG32(baseAddress + XGMAC_DMA_CH_STATUS, val);

    if ((val & XGMAC_DMA_INTR_MASK_TI) != 0)
    {
        res = eInterruptTI;
    }
    else if ((val & XGMAC_DMA_INTR_MASK_RI) != 0)
    {
        res = eInterruptRI;
    }
    else if ((val & XGMAC_DMA_INTR_MASK_FBE) != 0)
    {
        res = eInterruptFBE;
    }
    else if ((val & XGMAC_DMA_INTR_MASK_TXS) != 0)
    {
        res = eInterruptTXS;
    }
    else if ((val & XGMAC_DMA_INTR_MASK_RBU) != 0)
    {
        res = eInterruptRBU;
    }
    else if ((val & XGMAC_DMA_INTR_MASK_RS) != 0)
    {
        res = eInterruptRS;
    }
    else
    {
        res = eInterruptUnhandled;
    }

    return res;
}
void check_and_clear_link_interrupt_status( uint32_t baseAddress )
{
    uint32_t val;
    /*register reads ,clears the interrupts*/
    val = RD_REG32( baseAddress + XGMAC_MAC_RX_TX_STATUS);
    val = RD_REG32( baseAddress + XGMAC_MAC_INTERRUPT_STATUS);
    (void)val;
}

void xgmac_flush_descriptor(void *buf, size_t size)
{
    cache_force_write_back(buf, size);

}

void xgmac_invalidate_descriptor(void *buf, size_t size)
{
    cache_force_invalidate(buf,  size);
}

void xgmac_flush_buffer(void *buf, size_t size)
{

    cache_force_write_back(buf, size);

}

void xgmac_invalidate_buffer(void *buf, size_t size)
{
    cache_force_invalidate(buf,  size);
}

void xgmac_mmc_setup(uint32_t *baseAddress)
{
    /*
     * Disable all the Receive IPC statistics counter,
     * in the management counter
     *
     * MMC block is unused, the interrupt masking is done to avoid some
     * unwanted interrupts
     * */
    DISABLE_BIT(baseAddress + XGMAC_MMC_IPC_RX_INTERRUPT_MASK,
            XGMAC_MMC_IPC_RX_INTR_MASK_ALL);
}
