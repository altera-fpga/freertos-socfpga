/*
 * SPDX-FileCopyrightText: Copyright (C) 2025 Altera Corporation
 *
 * SPDX-License-Identifier: MIT-0
 *
 * Low level driver implementation for fpga bridges
 */

#include <stdint.h>
#include "socfpga_bridge_ll.h"
#include "socfpga_rst_mngr_reg.h"
#include "socfpga_defines.h"

int poll_idle_status(uint32_t reg_addr, uint32_t bit_msk, uint32_t val)
{
    int timeout = 1000;

    do
    {
        if( (RD_REG32(reg_addr) & bit_msk) == val  )
        {
            return 1;
        }
        osal_delay_ms(10);
        timeout--;

    }while(timeout > 0);

    return 0;
}

int is_enabled(bridge_type_t bridge)
{
    uint32_t bridge_reg;

    switch(bridge)
    {
        case HPS2FPGA : bridge_reg = RESET_BRGMODRST_SOC2FPGA_MASK;
                        break;

        case LWHPS2FPGA : bridge_reg = RESET_BRGMODRST_LWSOC2FPGA_MASK;
                          break;

        case FPGA2HPS : bridge_reg = RESET_BRGMODRST_FPGA2SOC_MASK;
                        break;

        case FPGA2SDRAM : bridge_reg = RESET_BRGMODRST_FPGA2SDRAM_MASK;
                          break;

        default : return RET_FAIL;
    }

    if((RD_REG32(RESET_MANAGER_BASE_ADDR+RESET_BRGMODRST) & bridge_reg) == 0U)
    {
        return RET_SUCCESS;
    }

    return RET_FAIL;
}

int is_disabled(bridge_type_t bridge)
{
    uint32_t bridge_reg;

    switch(bridge)
    {
        case HPS2FPGA : bridge_reg = RESET_BRGMODRST_SOC2FPGA_MASK;
                        break;

        case LWHPS2FPGA : bridge_reg = RESET_BRGMODRST_LWSOC2FPGA_MASK;
                          break;

        case FPGA2HPS : bridge_reg = RESET_BRGMODRST_FPGA2SOC_MASK;
                        break;

        case FPGA2SDRAM : bridge_reg = RESET_BRGMODRST_FPGA2SDRAM_MASK;
                          break;

        default : return RET_FAIL;
    }

    if((RD_REG32(RESET_MANAGER_BASE_ADDR+RESET_BRGMODRST) & bridge_reg) == bridge_reg)
    {
        return RET_SUCCESS;
    }

    return RET_FAIL;
}

int configure_hps2fpga_bridge(void)
{
    uint32_t reg_val;

    /* Enable handshake */
    reg_val = RD_REG32(RESET_MANAGER_BASE_ADDR+RESET_HDSKEN);
    reg_val |= ( RESET_HDSKEN_SOC2FPGA_FLUSH_MASK);
    WR_REG32( (RESET_MANAGER_BASE_ADDR+RESET_HDSKEN), reg_val);

    /* request handshake */
    reg_val = RD_REG32(RESET_MANAGER_BASE_ADDR+RESET_HDSKREQ);
    reg_val |= (RESET_HDSKREQ_SOC2FPGA_FLUSH_REQ_MASK);
    WR_REG32( (RESET_MANAGER_BASE_ADDR+RESET_HDSKREQ), reg_val);

    /* poll for bridge to report idle status */
    if( poll_idle_status( (RESET_MANAGER_BASE_ADDR+RESET_HDSKACK), RESET_HDSKACK_SOC2FPGA_FLUSH_ACK_MASK, RESET_HDSKACK_SOC2FPGA_FLUSH_ACK_MASK) == 0 )
    {
        return RET_FAIL;
    }

    /* clear idle requests */
    reg_val = RD_REG32(RESET_MANAGER_BASE_ADDR+RESET_HDSKREQ);
    reg_val &= ~RESET_HDSKREQ_SOC2FPGA_FLUSH_REQ_MASK;
    WR_REG32( (RESET_MANAGER_BASE_ADDR+RESET_HDSKREQ), reg_val);

    return RET_SUCCESS;
}

int configure_lwhps2fpga_bridge(void)
{
    uint32_t reg_val;

    /* enable handshake */
    reg_val = RD_REG32(RESET_MANAGER_BASE_ADDR+RESET_HDSKEN);
    reg_val |= (RESET_HDSKEN_LWSOC2FPGA_FLUSH_MASK);
    WR_REG32( (RESET_MANAGER_BASE_ADDR+RESET_HDSKEN), reg_val);

    /* request handshake */
    reg_val = RD_REG32(RESET_MANAGER_BASE_ADDR+RESET_HDSKREQ);
    reg_val |= (RESET_HDSKREQ_LWSOC2FPGA_FLUSH_REQ_MASK);
    WR_REG32( (RESET_MANAGER_BASE_ADDR+RESET_HDSKREQ), reg_val);

    /*poll for idle status */
    if( poll_idle_status( (RESET_MANAGER_BASE_ADDR+RESET_HDSKACK), RESET_HDSKACK_LWSOC2FPGA_FLUSH_ACK_MASK, RESET_HDSKACK_LWSOC2FPGA_FLUSH_ACK_MASK) == 0 )
    {
        return RET_FAIL;
    }

    /* clear idle requests */
    reg_val = RD_REG32(RESET_MANAGER_BASE_ADDR+RESET_HDSKREQ);
    reg_val &= ~(RESET_HDSKREQ_LWSOC2FPGA_FLUSH_REQ_MASK);
    WR_REG32( (RESET_MANAGER_BASE_ADDR+RESET_HDSKREQ), reg_val);

    return RET_SUCCESS;
}

int fpga2hps_bridge_enable_conf(void)
{
    uint32_t reg_val;

    /* clear idle requests */
    /*  hdskreq[fpgahsreq] = 0 */
    reg_val = RD_REG32(RESET_MANAGER_BASE_ADDR+RESET_HDSKREQ);
    reg_val &= ~(RESET_HDSKREQ_FPGAHSREQ_MASK);
    WR_REG32( (RESET_MANAGER_BASE_ADDR+RESET_HDSKREQ), reg_val);

    /* hdskreq[f2s_flush_req] = 0 */
    reg_val = RD_REG32(RESET_MANAGER_BASE_ADDR+RESET_HDSKREQ);
    reg_val &= ~(RESET_HDSKREQ_F2S_FLUSH_REQ_MASK);
    WR_REG32( (RESET_MANAGER_BASE_ADDR+RESET_HDSKREQ), reg_val);

    /* poll for ack */
    /*  hdskack[f2s_flush_ack] = 0 
     *  hdskack[fpgahsack] = 0 
     */

    /*poll for idle status */
    if( poll_idle_status( (RESET_MANAGER_BASE_ADDR+RESET_HDSKACK), RESET_HDSKACK_F2S_FLUSH_ACK_MASK, 0U) == 0 )
    {
        return RET_FAIL;
    }

    /*poll for idle status */
    if( poll_idle_status( (RESET_MANAGER_BASE_ADDR+RESET_HDSKACK), RESET_HDSKACK_FPGAHSACK_MASK, 0U) == 0 )
    {
        return RET_FAIL;
    }

    return RET_SUCCESS;
}

int fpga2hps_bridge_disable_conf(void)
{
    uint32_t reg_val;

    /* enable handshake */
    reg_val = RD_REG32(RESET_MANAGER_BASE_ADDR+RESET_HDSKEN);
    reg_val |= (1U << RESET_HDSKEN_FPGAHSEN_POS);
    WR_REG32( (RESET_MANAGER_BASE_ADDR+RESET_HDSKEN), reg_val);

    /* clear handshake request */
    reg_val = RD_REG32(RESET_MANAGER_BASE_ADDR+RESET_HDSKREQ);
    reg_val &= ~(RESET_HDSKREQ_FPGAHSREQ_MASK);
    WR_REG32( (RESET_MANAGER_BASE_ADDR+RESET_HDSKREQ), reg_val);

    /* clear handshake request */
    reg_val = RD_REG32(RESET_MANAGER_BASE_ADDR+RESET_HDSKREQ);
    reg_val &= ~(RESET_HDSKREQ_F2S_FLUSH_REQ_MASK);
    WR_REG32( (RESET_MANAGER_BASE_ADDR+RESET_HDSKREQ), reg_val);

    /*poll for idle status */
    if( poll_idle_status( (RESET_MANAGER_BASE_ADDR+RESET_HDSKACK), RESET_HDSKACK_F2S_FLUSH_ACK_MASK, 0U) == 0 )
    {
        return RET_FAIL;
    }

    /*poll for idle status */
    if( poll_idle_status( (RESET_MANAGER_BASE_ADDR+RESET_HDSKACK), RESET_HDSKACK_FPGAHSACK_MASK, 0U) == 0 )
    {
        return RET_FAIL;
    }

    return RET_SUCCESS;
}

int fpga2sdram_bridge_enable_conf(void)
{
    uint32_t reg_val;

    /* clear idle requests */
    /*  hdskreq[fpgahsreq] = 0 */
    reg_val = RD_REG32(RESET_MANAGER_BASE_ADDR+RESET_HDSKREQ);
    reg_val &= ~(RESET_HDSKREQ_FPGAHSREQ_MASK);
    WR_REG32( (RESET_MANAGER_BASE_ADDR+RESET_HDSKREQ), reg_val);

    /* hdskreq[f2sdram_flush_req] = 0 */
    reg_val = RD_REG32(RESET_MANAGER_BASE_ADDR+RESET_HDSKREQ);
    reg_val &= ~(RESET_HDSKREQ_F2SDRAM_FLUSH_REQ_MASK);
    WR_REG32( (RESET_MANAGER_BASE_ADDR+RESET_HDSKREQ), reg_val);

    /*poll for idle status */
    if( poll_idle_status( (RESET_MANAGER_BASE_ADDR+RESET_HDSKACK), RESET_HDSKACK_F2SDRAM_FLUSH_ACK_MASK, 0U) == 0 )
    {
        return RET_FAIL;
    }

    if( poll_idle_status( (RESET_MANAGER_BASE_ADDR+RESET_HDSKACK), RESET_HDSKACK_FPGAHSACK_MASK, 0U) == 0 )
    {
        return RET_FAIL;
    }
    return RET_SUCCESS;
}

int fpga2sdram_bridge_disable_conf(void)
{
    uint32_t reg_val;

    /* enable handshake */
    reg_val = RD_REG32(RESET_MANAGER_BASE_ADDR+RESET_HDSKEN);
    reg_val |= (1U << RESET_HDSKEN_FPGAHSEN_POS);
    WR_REG32( (RESET_MANAGER_BASE_ADDR+RESET_HDSKEN), reg_val);

    /* clear handshake request */
    reg_val = RD_REG32(RESET_MANAGER_BASE_ADDR+RESET_HDSKREQ);
    reg_val &= ~(RESET_HDSKREQ_FPGAHSREQ_MASK);
    WR_REG32( (RESET_MANAGER_BASE_ADDR+RESET_HDSKREQ), reg_val);

    /* clear handshake request */
    reg_val = RD_REG32(RESET_MANAGER_BASE_ADDR+RESET_HDSKREQ);
    reg_val &= ~(RESET_HDSKREQ_F2SDRAM_FLUSH_REQ_MASK);
    WR_REG32( (RESET_MANAGER_BASE_ADDR+RESET_HDSKREQ), reg_val);

    /*poll for idle status */
    if( poll_idle_status( (RESET_MANAGER_BASE_ADDR+RESET_HDSKACK), RESET_HDSKACK_F2SDRAM_FLUSH_ACK_MASK, 0U) == 0 )
    {
        return RET_FAIL;
    }

    /*poll for idle status */
    if( poll_idle_status( (RESET_MANAGER_BASE_ADDR+RESET_HDSKACK), RESET_HDSKACK_FPGAHSACK_MASK, 0U) == 0 )
    {
        return RET_FAIL;
    }

    return RET_SUCCESS;
}
