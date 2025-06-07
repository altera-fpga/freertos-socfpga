/*
 * SPDX-FileCopyrightText: Copyright (C) 2025 Altera Corporation
 *
 * SPDX-License-Identifier: MIT-0
 *
 * Low level driver implementation for SoC FPGA SDMMC
 */

#include <stdint.h>
#include "osal_log.h"
#include "socfpga_cache.h"
#include "socfpga_defines.h"
#include "socfpga_combo_phy.h"
#include "socfpga_sdmmc_ll.h"
#include "socfpga_sdmmc_reg.h"
#include "socfpga_clk_mngr_reg.h"


#define SINGLE_BLOCK                 (0U)
#define DATA_WRITE                   (0U)
#define HIGH_SPEED_MODE              (1U)
#define DEF_SPEED_MODE               (0U)
#define CARD_DETECTED                (1U)
#define VAL_DESCRIPTOR               (1U)
#define EN_BUS_PWR                   (1U)
#define DATA_READ                    (1U)
#define MULTI_BLOCK                  (1U)
#define CMD_ID_CHECK_EN              (1U)
#define CMD_SEND_EXT_CSD             (8U)
#define CMD_READ_SINGLE_BLOCK        (17U)
#define CMD_READ_MULT_BLOCK          (18U)
#define CMD_WRITE_SINGLE_BLOCK       (24U)
#define CMD_WRITE_MULT_BLOCK         (25U)
#define DATA_XFER_BITS_512           (0x200U)
#define SHORT_RESP_BUSY              (3U)
#define DATA_PRESENT                 (1U)

#define CLEAR_INT                    (0x0U)
#define EN_CMD_INT                   (0x10001U)
#define EN_XFER_INT                  (0x100002U)

#define SDCLK_FREQ                   (2U)
#define EN_INTERN_CLK                (1U)
#define EN_SD_CLK                    (1U)
#define SEL_ADMA2                    (3U)
#define EN_DMA                       (1U)
#define EN_BCT                       (1U)
#define SET_VOLT_3_3                 (7U)
#define PHY_SW_RST                   (1U)
#define EN_EXT_WR_MODE               (1U)
#define EN_EXT_RDCMD_MODE            (1U)
#define EN_EXT_RDDATA_MODE           (1U)
#define EN_BUS_WIDTH_4               (1U)

#define CMD_INHIBIT_SET              (1U)
#define FREQ_SEL_25MHz               (4U)
#define FREQ_SEL_50MHz               (2U)
#define AUTO_CMD_23_EN               (2U)

#define SDSC_DETECTED                (0x0U)
#define SDHC_DETECTED                (0x1U)
#define DAT_TIMOUT_CTR               (0xeU)

#define SDMMC_DMA_MAX_BUFFER_SIZE    (64U * 1024U)
#define RESET_SOFTPHY                (1U << 6U)
#define RESET_SDMMC                  (1U << 7U)
#define RESET_SDMMC_ECC              ((uint32_t) 1 << 15U)
#define IS_CARD_READY                ((uint32_t) 1 << 31U)
#define END_DESCRIPTOR               (1U << 1U)
#define EN_DMA_INT                   (1U << 2U)
#define XFER_DATA                    (1U << 5U)
#define BIT_MASK_32                  (0xFFFFFFFFU)
#define DESC_MAX_XFER_SIZE           (64U * 1024U)

/*delays approx 12.5 micro-seconds*/
#define SECTOR_SIZE                  512UL
#define RESET_TIMEOUT                10000
#define eMMC_8_BIT_MODE_EN           1

#define SOFTPHY_CLK_200_MHZ          1

static eErrorState reset_dll( void );
static void prgm_host_config( void );
static void program_reg( uint32_t ulVal, uint32_t ulRegAdd );
static void config_phy_xfer_params( void );
static void sdmmc_enable_cmd_int( void );
static void sdmmc_enable_xfer_int(void);

/**
 * @brief Set up the command parameters and send the command to the card.
 */
eErrorState sdmmc_send_command( const CmdParameters_t *params )
{
    uint32_t count = 0;
    uint32_t ulSrs03RegValue = 0;
    uint32_t ulSrs02RegValue;
    uint32_t ulSrs11RegValue = 0;
    /*should retain the loaded configs*/
    ulSrs03RegValue = RD_REG32(SRS_BASE_ADDR + SDMMC_SRS03);
    ulSrs11RegValue = RD_REG32(SRS_BASE_ADDR + SDMMC_SRS11);

    ulSrs03RegValue &=
            ~(SDMMC_SRS03_CIDX_MASK | SDMMC_SRS03_DPS_MASK |
            SDMMC_SRS03_CRCCE_MASK | SDMMC_SRS03_CICE_MASK |
            SDMMC_SRS03_RTS_MASK);
    /*reset cmd line*/
    ulSrs11RegValue |= SDMMC_SRS11_SRCMD_MASK;
    /*reset data line */
    ulSrs11RegValue |= SDMMC_SRS11_SRDAT_MASK;
    WR_REG32(SRS_BASE_ADDR + SDMMC_SRS11, ulSrs11RegValue);
    /*host requires some delay to reset cmd line*/
    count = RESET_TIMEOUT;
    while ((RD_REG32(SRS_BASE_ADDR + SDMMC_SRS11) & SDMMC_SRS11_SRCMD_MASK)
            == SDMMC_SRS11_SRCMD_MASK)
    {
        if (count == 0U)
        {
            return CTRL_CONFIG_FAIL;
        }
        else
        {
            count--;
        }
    }
    /*Host requires some delay to reset data line*/
    count = RESET_TIMEOUT;
    while ((RD_REG32(SRS_BASE_ADDR + SDMMC_SRS11) & SDMMC_SRS11_SRDAT_MASK)
            == SDMMC_SRS11_SRDAT_MASK)
    {
        if (count == 0U)
        {
            return CTRL_CONFIG_FAIL;
        }
        else
        {
            count--;
        }
    }
    /*argument and dependent param configuration*/
    ulSrs03RegValue |=
            (uint32_t) params->ucCommandIndex << SDMMC_SRS03_CIDX_POS;
    ulSrs03RegValue |=
            (uint32_t) params->ucDataXferPresent << SDMMC_SRS03_DPS_POS;
    ulSrs03RegValue |= (uint32_t) params->ucIdCheckEnable
            << SDMMC_SRS03_CICE_POS;
    ulSrs03RegValue |= (uint32_t) params->ucCrcCheckEnable
            << SDMMC_SRS03_CRCCE_POS;
    ulSrs03RegValue |= (uint32_t) params->ucResponseType << SDMMC_SRS03_RTS_POS;

    ulSrs02RegValue = (uint32_t) params->ulArgument;
    /*send stop request once block count decrements to 0 in case of multi block read/write*/
    if ((params->ucCommandIndex == CMD_WRITE_MULT_BLOCK)
            || (params->ucCommandIndex == CMD_READ_MULT_BLOCK))
    {
        ulSrs03RegValue |= (AUTO_CMD_23_EN << SDMMC_SRS03_ACE_POS);
    }
    /*check if the cmd line is inhibited to send command*/
    if ((RD_REG32(SRS_BASE_ADDR + SDMMC_SRS09) & CMD_INHIBIT_SET) == 0U)
    {
        if ((params->ucDataXferPresent == DATA_PRESENT) ||
                (params->ucResponseType == SHORT_RESP_BUSY))
        {
            sdmmc_enable_xfer_int();
        }
        else
        {
            sdmmc_enable_cmd_int();
        }

        WR_REG32(SRS_BASE_ADDR + SDMMC_SRS02, ulSrs02RegValue);
        WR_REG32(SRS_BASE_ADDR + SDMMC_SRS03, ulSrs03RegValue);
    }
    else
    {
        return CMD_ERR;
    }

    return CMD_SND_OK;
}

/**
 * @brief Configure the host for data transmission and reception.
 */
void sdmmc_set_xfer_config( CmdParameters_t const *params )
{
    uint32_t ulSrs11RegValue = 0;
    uint32_t ulSrs03RegValue = 0;
    /*should retain the loaded configs*/
    ulSrs11RegValue = RD_REG32(SRS_BASE_ADDR + SDMMC_SRS11);
    ulSrs03RegValue = RD_REG32(SRS_BASE_ADDR + SDMMC_SRS03);

    ulSrs11RegValue |= SDMMC_SRS11_SRDAT_MASK;
    WR_REG32(SRS_BASE_ADDR + SDMMC_SRS11, ulSrs11RegValue);

    ulSrs03RegValue &= ~(SDMMC_SRS03_DTDS_MASK | SDMMC_SRS03_MSBS_MASK);
    /*configure no of blocks for the transaction*/
    switch (params->ucCommandIndex)
    {
    case CMD_READ_MULT_BLOCK:
    case CMD_READ_SINGLE_BLOCK:
    case CMD_SEND_EXT_CSD:
        ulSrs03RegValue |= DATA_READ << SDMMC_SRS03_DTDS_POS;
        break;

    case CMD_WRITE_MULT_BLOCK:
    case CMD_WRITE_SINGLE_BLOCK:
        ulSrs03RegValue |= DATA_WRITE << SDMMC_SRS03_DTDS_POS;
        break;
    default:
        /*Do Nothing*/
        break;
    }
    /*configure for write and read*/
    switch (params->ucCommandIndex)
    {
    case CMD_READ_MULT_BLOCK:
    case CMD_WRITE_MULT_BLOCK:
        ulSrs03RegValue |= (uint32_t) MULTI_BLOCK << SDMMC_SRS03_MSBS_POS;
        break;

    case CMD_READ_SINGLE_BLOCK:
    case CMD_WRITE_SINGLE_BLOCK:
    case CMD_SEND_EXT_CSD:
        ulSrs03RegValue |= (uint32_t) SINGLE_BLOCK << SDMMC_SRS03_MSBS_POS;
        break;
    default:
        /*Do Nothing*/
        break;
    }

    WR_REG32(SRS_BASE_ADDR + SDMMC_SRS03, ulSrs03RegValue);
    /*wait for the configurations to reflect*/
    for (int i =0; i < 10000; i++)
    {

    }
}

/**
 * @brief Set up DMA attributes.
 */
void sdmmc_set_up_xfer( DmaDescriptor_t *pxDesc, uint64_t *buff,
        uint32_t ulBlockSize, uint32_t ulBlockCt )
{
    uint64_t ullDesc = 0;
    uint32_t volatile val = 0;
    uint32_t volatile i = 0;
    uint32_t size = ulBlockSize * ulBlockCt;
    uint32_t DescriptorCount = 0;

    DescriptorCount = (size + (DESC_MAX_XFER_SIZE) -1U) / DESC_MAX_XFER_SIZE;
    cache_force_invalidate((uint64_t*) buff, size);
    ullDesc = (uint64_t) pxDesc;
    /*count number of descriptors required*/
    while ((i + 1U) < DescriptorCount)
    {
        pxDesc->attribute = XFER_DATA | VAL_DESCRIPTOR | EN_DMA_INT;
        pxDesc->reserved = 0U;
        pxDesc->len = 0U;
        pxDesc->addr_lo = (uint32_t) ((uint64_t) buff & BIT_MASK_32) +
                (DESC_MAX_XFER_SIZE * i);
        pxDesc->addr_hi = (uint32_t) (((uint64_t) buff >> 32U) & BIT_MASK_32);
        size -= SDMMC_DMA_MAX_BUFFER_SIZE;
        pxDesc++;
        i++;
    }
    pxDesc->attribute = XFER_DATA | VAL_DESCRIPTOR | EN_DMA_INT |
            END_DESCRIPTOR;
    pxDesc->reserved = 0U;
    pxDesc->len = (uint16_t) size;
    pxDesc->addr_lo =
            (uint32_t) (((uint64_t) buff & BIT_MASK_32) +
            (DESC_MAX_XFER_SIZE * i));
    pxDesc->addr_hi = (uint32_t) ((((uint64_t) buff >> 32U) & BIT_MASK_32));

    cache_force_write_back((uint64_t*) ullDesc,
            DescriptorCount * sizeof(DmaDescriptor_t));

    WR_REG32(SRS_BASE_ADDR + SDMMC_SRS22, (uint32_t )ullDesc);
    WR_REG32(SRS_BASE_ADDR + SDMMC_SRS23,
            (uint32_t )((uint64_t )ullDesc >> 32U));
    WR_REG32(SRS_BASE_ADDR + SDMMC_SRS00, (uint32_t )ulBlockCt);

    val = (uint32_t) (ulBlockCt << SDMMC_SRS01_BCCT_POS);
    val |= (uint32_t) ulBlockSize << SDMMC_SRS01_TBS_POS;

    WR_REG32(SRS_BASE_ADDR + SDMMC_SRS01, val);
}

/**
 * @brief Initialize sdmmc host configuration
 */
void sdmmc_init_configs( uint32_t emmc_bus_width,uint32_t def_speed )
{
    uint32_t ulSrs10RegValue = 0;
    uint32_t ulSrs11RegValue = 0;
    uint32_t ulSrs03RegValue = 0;

    ulSrs03RegValue = RD_REG32(SRS_BASE_ADDR + SDMMC_SRS03);
    ulSrs10RegValue = RD_REG32(SRS_BASE_ADDR + SDMMC_SRS10);
    ulSrs10RegValue &=
            ~((SDMMC_SRS10_DMASEL_MASK) | (SDMMC_SRS10_DTW_MASK) |
            (SDMMC_SRS10_EDTW_MASK) | (SDMMC_SRS10_SBGR_MASK) |
            (SDMMC_SRS10_BVS_MASK) | (SDMMC_SRS10_BVS_MASK));

    if (def_speed == 1U)
    {
        ulSrs11RegValue = (uint32_t) FREQ_SEL_25MHz << SDMMC_SRS11_SDCFSL_POS;
        ulSrs10RegValue |= DEF_SPEED_MODE << SDMMC_SRS10_HSE_POS;
    }
    else
    {
        ulSrs11RegValue = (uint32_t) FREQ_SEL_50MHz << SDMMC_SRS11_SDCFSL_POS;
        ulSrs10RegValue |= HIGH_SPEED_MODE << SDMMC_SRS10_HSE_POS;
    }
    ulSrs11RegValue |= ((uint32_t) DAT_TIMOUT_CTR << SDMMC_SRS11_DTCV_POS);
    WR_REG32(SRS_BASE_ADDR + SDMMC_SRS11, ulSrs11RegValue);

    ulSrs11RegValue |= (EN_SD_CLK << SDMMC_SRS11_SDCE_POS);
    ulSrs11RegValue |= (EN_INTERN_CLK << SDMMC_SRS11_ICE_POS);
    WR_REG32(SRS_BASE_ADDR + SDMMC_SRS11, ulSrs11RegValue);

    ulSrs10RegValue |= ((uint32_t) SET_VOLT_3_3 << SDMMC_SRS10_BVS_POS);
    ulSrs10RegValue |= (SEL_ADMA2 << SDMMC_SRS10_DMASEL_POS);
    ulSrs10RegValue |= (uint32_t) EN_BUS_PWR << SDMMC_SRS10_BP_POS;
    ulSrs10RegValue |= emmc_bus_width << SDMMC_SRS10_EDTW_POS;
    ulSrs10RegValue |= EN_BUS_WIDTH_4 << SDMMC_SRS10_DTW_POS;

    ulSrs03RegValue |= (EN_BCT << SDMMC_SRS03_BCE_POS);
    ulSrs03RegValue |= (EN_DMA << SDMMC_SRS03_DMAE_POS);

    WR_REG32(SRS_BASE_ADDR + SDMMC_SRS03, ulSrs03RegValue);
    WR_REG32(SRS_BASE_ADDR + SDMMC_SRS10, ulSrs10RegValue);

    config_phy_xfer_params();
}

/**
 * @brief Parse and read relative card address from the response.
 */
void sd_read_response_rel_addr( CardData_t *pxcard )
{
    uint32_t ulSrs04RegValue = 0;
    ulSrs04RegValue = RD_REG32(SRS_BASE_ADDR + SDMMC_SRS04);
    pxcard->usRelativeAddress = ((uint64_t) ulSrs04RegValue >> 16U) & 0xFFFFUL;
}

/**
 * @brief Reset all sdmmc host configurations.
 */
eErrorState sdmmc_reset_configs( void )
{
    uint32_t count = 0;
    uint32_t ulHrs00RegValue = 0;
    uint32_t ulhrs09RegValue = 0;

    ulHrs00RegValue = RD_REG32(HRS_BASE_ADDR + SDMMC_HRS00);
    ulhrs09RegValue = RD_REG32(HRS_BASE_ADDR + SDMMC_HRS09);

    ulHrs00RegValue |= 1U << SDMMC_HRS00_SWR_POS;
    WR_REG32(HRS_BASE_ADDR + SDMMC_HRS00, ulHrs00RegValue);
    /*Host requires some delay to reset*/
    count = RESET_TIMEOUT;
    while ((RD_REG32(HRS_BASE_ADDR + SDMMC_HRS00) & 1U) == 1U)
    {
        if (count == 0U)
        {
            return CTRL_CONFIG_FAIL;
        }
        else
        {
            count--;
        }
    }
    ulhrs09RegValue = 0U;
    WR_REG32(HRS_BASE_ADDR + SDMMC_HRS09, ulhrs09RegValue);
    return CTRL_CONFIG_PASS;
}

/**
 * @brief Configure the sdmmc combo-phy
 */
eErrorState sdmmc_init_phy( void )
{
    uint32_t ulRegValue;
    uint16_t val;
    eErrorState ret = CTRL_CONFIG_PASS;

    ulRegValue = USE_EXT_LPBK_DQS | USE_LPBK_DQS | USE_PHONY_DQS |
            USE_PHONY_DQS_CMD | DQS_SEL_OE_END;
    program_reg(ulRegValue, PHY_DQS_TIM_REG_ADD);

    ulRegValue = SYNC_METHOD_EN | RD_DEL_SEL | SW_HALF_CYCLE_SHIFT |
            SET_UNDERRUN_SUPPRESS | GATE_CFG_ALWAYS_ON;
    program_reg(ulRegValue, PHY_GATE_LPBK_CTL_ADD);

    ulRegValue = SEL_DLL_BYPASS_MODE | PARAM_DLL_START_POINT;
    program_reg(ulRegValue, PHY_DLL_MASTER_CTL_ADD);

    ulRegValue = READ_DQS_CMD_DELAY | CLK_WRDQS_DELAY | CLK_WR_DELAY |
            READ_DQS_DELAY;

    program_reg(ulRegValue, PHY_DLL_SLAVE_CTL_ADD);

    val = PHY_CTL_REG_ADD & 0xFFFFU;
    WR_REG32(HRS_BASE_ADDR + SDMMC_HRS04, (uint16_t )val);
    ulRegValue = RD_REG32(HRS_BASE_ADDR + SDMMC_HRS05);
    ulRegValue &= ~PHONY_DQS_DELAY;
    WR_REG32(HRS_BASE_ADDR + SDMMC_HRS05, ulRegValue);

    ret = reset_dll();

    ulRegValue = IO_MASK_DISABLE | IO_MASK_END | IO_MASK_START |
            DATA_SEL_OE_END;
    program_reg(ulRegValue, (uint32_t) PHY_DQ_TIM_REG_ADD);

    prgm_host_config();
    return ret;

}

/**
 * @brief Program phy registers.
 */
static void program_reg( uint32_t ulVal, uint32_t ulRegAdd )
{
    ulRegAdd &= REG_ADD_LSB_MASK;
    WR_REG32(HRS_BASE_ADDR + SDMMC_HRS04, ulRegAdd);
    WR_REG32(HRS_BASE_ADDR + SDMMC_HRS05, ulVal);
}

/**
 * @brief Reset all dll registers.
 */
static eErrorState reset_dll( void )
{
    uint32_t ulRegVal;
    uint32_t count = RESET_TIMEOUT;

    do
    {
        ulRegVal = RD_REG32(HRS_BASE_ADDR + SDMMC_HRS09);
        ulRegVal |= 1U;
        WR_REG32(HRS_BASE_ADDR + SDMMC_HRS09, ulRegVal);
        if (count == 0U)
        {
            return CTRL_CONFIG_FAIL;
        }
        else
        {
            count--;
        }
    } while ((RD_REG32(HRS_BASE_ADDR + SDMMC_HRS09) &
    (1U << SDMMC_HRS09_PHY_INIT_COMPLETE_POS))
    == (0U << SDMMC_HRS09_PHY_INIT_COMPLETE_POS));
    return CTRL_CONFIG_PASS;
}

/**
 * @brief Program host configuration for cmd/xfer.
 */
static void prgm_host_config( void )
{
    uint32_t ulRegVal;

    ulRegVal =
            ((uint32_t) 1 <<
            SDMMC_HRS09_RDDATA_EN_POS) |
            ((uint32_t) 1 << SDMMC_HRS09_RDCMD_EN_POS) | (1U <<
            SDMMC_HRS09_EXTENDED_RD_MODE_POS) | (1U <<
            SDMMC_HRS09_EXTENDED_WR_MODE_POS);
    WR_REG32(HRS_BASE_ADDR + SDMMC_HRS09, ulRegVal);

    ulRegVal = (uint32_t) 6 << SDMMC_HRS10_HCSDCLKADJ_POS;
    WR_REG32(HRS_BASE_ADDR + SDMMC_HRS10, ulRegVal);

    ulRegVal = (WRCMD0_SDCLK_DLY) |
            (1U <<
            SDMMC_HRS16_WRDATA0_DLY_POS) |
            ((uint32_t) 1 << SDMMC_HRS16_WRCMD0_DLY_POS);
    WR_REG32(HRS_BASE_ADDR + SDMMC_HRS16, ulRegVal);

    ulRegVal = (uint32_t) 0xA << SDMMC_HRS07_RW_COMPENSATE_POS;
    WR_REG32(HRS_BASE_ADDR + SDMMC_HRS07, ulRegVal);

    ulRegVal = (uint32_t) 0x3 << 20U;
    WR_REG32(HRS_BASE_ADDR + SDMMC_HRS10, ulRegVal);
}

/**
 * @brief Calculate the sector count of the sd card.
 */
uint64_t sdmmc_read_sector_count( void )
{
    /*C_SIZE is parsed from bit 40 to 61*/
    uint32_t reg[ 4 ];
    uint64_t card_size;
    uint64_t sector_count;
    uint64_t temp_ops;

    reg[0] = RD_REG32(SRS_BASE_ADDR + SDMMC_SRS04);
    reg[1] = RD_REG32(SRS_BASE_ADDR + SDMMC_SRS05);

    /*extract C_SIZE*/
    temp_ops = (((uint64_t) reg[1] >> 8) & (0x3FFFFFUL));
    /*card size in bytes*/
    card_size = (temp_ops + 1UL) * SECTOR_SIZE * 1024UL;
    sector_count = card_size / SECTOR_SIZE;

    return sector_count;
}

/**
 * @brief Reset the sdmmc peripherel.
 */
eErrorState sdmmc_reset_per0( void )
{
    uint32_t ulRegAddVal;
    uint32_t count = 0;

#if SOFTPHY_CLK_200_MHZ
    /*The clock gated to combo-phy is 200MHz instead of 800MHz.
     * Dividing the combo-phy clock by 1 instead of 4
     * to make the combo-phy to its optimal
     * clock frequency.
     */
    ulRegAddVal = RD_REG32(PER0MODRST_ADDR);
    /*Assert the sdmmc module reset signal*/
    ulRegAddVal |= RESET_SOFTPHY;
    ulRegAddVal |= RESET_SDMMC;
    WR_REG32(PER0MODRST_ADDR, ulRegAddVal);
    uint32_t readVal = RD_REG32(CLK_MNGR_BASE_ADDR + MAINPLL_CLK_MNGR_NOCDIV);

    /*divide the clock by 1*/
    readVal &= ~(0x30000U);
    WR_REG32((CLK_MNGR_BASE_ADDR + MAINPLL_CLK_MNGR_NOCDIV),readVal);
#endif

    ulRegAddVal = RD_REG32(PER0MODRST_ADDR);
    ulRegAddVal |= RESET_SOFTPHY;
    WR_REG32(PER0MODRST_ADDR, ulRegAddVal);

    ulRegAddVal &= ~RESET_SOFTPHY;
    WR_REG32(PER0MODRST_ADDR, ulRegAddVal);
    /*Host requires some delay to reset*/
    count = RESET_TIMEOUT;

    while ((RD_REG32(PER0MODRST_ADDR) & RESET_SOFTPHY) == (1U << 6U))
    {
        if (count == 0U)
        {
            ERROR("RESET FAILED");
            return CTRL_CONFIG_FAIL;
        }
        else
        {
            count--;
        }
    }

    ulRegAddVal |= RESET_SDMMC;
    WR_REG32(PER0MODRST_ADDR, ulRegAddVal);

    ulRegAddVal &= ~RESET_SDMMC;
    WR_REG32(PER0MODRST_ADDR, ulRegAddVal);
    /*Host requires some delay to reset*/
    count = RESET_TIMEOUT;

    while ((RD_REG32(PER0MODRST_ADDR) & RESET_SDMMC) == (1U << 7U))
    {
        if (count == 0U)
        {
            ERROR("RESET FAILED");
            return CTRL_CONFIG_FAIL;
        }
        else
        {
            count--;
        }
    }

    ulRegAddVal |= RESET_SDMMC_ECC;
    WR_REG32(PER0MODRST_ADDR, ulRegAddVal);

    ulRegAddVal &= ~RESET_SDMMC_ECC;
    WR_REG32(PER0MODRST_ADDR, ulRegAddVal);
    while ((RD_REG32(PER0MODRST_ADDR) & RESET_SDMMC_ECC) ==
            ((uint32_t) 1U << 15U))
    {
        if (count == 0U)
        {
            ERROR("RESET FAILED");
            return CTRL_CONFIG_FAIL;
        }
        else
        {
            count--;
        }
    }
    return CTRL_CONFIG_PASS;
}

/**
 * @brief Enable the extended read,write and command mode.
 */
static void config_phy_xfer_params( void )
{
    uint32_t ulHrs09RegVal;

    ulHrs09RegVal = RD_REG32(HRS_BASE_ADDR + SDMMC_HRS09);
    ulHrs09RegVal &= ~(PHY_SW_RST);
    WR_REG32(HRS_BASE_ADDR + SDMMC_HRS09, ulHrs09RegVal);

    ulHrs09RegVal |=
            ((EN_EXT_WR_MODE <<
            SDMMC_HRS09_EXTENDED_WR_MODE_POS) | ((uint32_t) EN_EXT_RDCMD_MODE <<
            SDMMC_HRS09_RDCMD_EN_POS) |
            ((uint32_t) EN_EXT_RDDATA_MODE << SDMMC_HRS09_RDDATA_EN_POS));

    WR_REG32(HRS_BASE_ADDR + SDMMC_HRS09, ulHrs09RegVal);
    ulHrs09RegVal |= PHY_SW_RST;
    WR_REG32(HRS_BASE_ADDR + SDMMC_HRS09, ulHrs09RegVal);
}

/**
 * @brief Parse the response and check if the card is ready.
 */
uint32_t sdmmc_is_card_ready( void )
{
    uint32_t card_read_status =
            (RD_REG32(SRS_BASE_ADDR + SDMMC_SRS04) & IS_CARD_READY);
    card_read_status =  card_read_status >> 31;
    return card_read_status;
}

/**
 * @brief Parse the response and check the card type.
 */
void sd_get_card_type( CardData_t *pxcard )
{
    pxcard->usCardType =
            ((RD_REG32(SRS_BASE_ADDR + SDMMC_SRS04) & ((uint32_t) 1 << 31U)) ==
            ((uint32_t) 1 << 31U)) ?
            SDHC_DETECTED : SDSC_DETECTED;
}

/**
 * @brief Enable command interrupts.
 */
static void sdmmc_enable_cmd_int( void )
{
    WR_REG32(SRS_BASE_ADDR + SDMMC_SRS14, EN_CMD_INT);
    WR_REG32(SRS_BASE_ADDR + SDMMC_SRS13, EN_CMD_INT);
}

/**
 * @brief Enable transaction interrupts.
 */
static void sdmmc_enable_xfer_int( void )
{
    WR_REG32(SRS_BASE_ADDR + SDMMC_SRS14, EN_XFER_INT);
    WR_REG32(SRS_BASE_ADDR + SDMMC_SRS13, EN_XFER_INT);
}

/**
 * @brief Clear sdmmc interrupt flags.
 */
void sdmmc_clear_int( void )
{
    WR_REG32(SRS_BASE_ADDR + SDMMC_SRS14, CLEAR_INT);
    WR_REG32(SRS_BASE_ADDR + SDMMC_SRS13, CLEAR_INT);
}

/**
 * @brief Read interrupt status register.
 */
uint32_t sdmmc_get_int_status( void )
{
    return RD_REG32(SRS_BASE_ADDR + SDMMC_SRS12);
}

/**
 * @brief Disable interrupts for data and response triggers.
 */
void sdmmc_disable_int( void )
{
    WR_REG32(SRS_BASE_ADDR + SDMMC_SRS12, CLEAR_INT);
}


/**
 * @brief Check if the SD/eMMC card detected.
 */
uint32_t sdmmc_is_card_detected( void )
{
    /*INFO:sdmmc_is_card_detected is used to check if the card is detected or not.
       The detection bitfield is always set regardless of the presence or absence of the card.*/
    uint32_t ulSrs09RegValue;
    uint32_t card_check;

    ulSrs09RegValue = RD_REG32(SRS_BASE_ADDR + SDMMC_SRS09);
    card_check = (ulSrs09RegValue >> SDMMC_SRS09_CI_POS) & CARD_DETECTED;
    return card_check;
}
