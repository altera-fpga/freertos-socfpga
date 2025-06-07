/*
 * SPDX-FileCopyrightText: Copyright (C) 2025 Altera Corporation
 *
 * SPDX-License-Identifier: MIT-0
 *
 * Low level driver implementation for iossm
 */

#include "socfpga_iossm_ll.h"
#include "socfpga_defines.h"
#include "socfpga_cache.h"
#include "socfpga_sip_handler.h"

#define ERROR_DATA_VALUE 0xFFFFCCCCFFFFCCCC;
#define SYSTEM_BARRIER     __asm__ volatile ( \
        "dmb sy\n" \
        "isb\n" \
        : \
        : \
        : "memory" \
    )

static xReqFormat ReqFormat;

/**
 * @brief  read iossm register
 *
 * @param[in]  base_addr - instance base address
 * @param[in]  reg - register offset
 *
 * @return
 * - Response - register value
 */
uint32_t iossm_read_register( uint32_t base_addr, uint32_t reg)
{
    return RD_REG32(base_addr + reg);
}

/**
 * @brief  send iossm commands
 *
 * @param[in]  iossm_data - command handle holding required data to send the command.
 * @param[in]  xRequest - the type of requests to be send.
 * @param[in]  ulParam - the parameters to support the required functionality.
 *
 * @return
 * - Response - Response Data from the iossm
 */
xResponseData iossm_send_command(iossm_type const*iossm_data,uint32_t xRequest,
        uint32_t ulParam)
{
    uint32_t volatile status = 0U;
    uint32_t volatile ulCmdReq = 0U;
    uint32_t volatile ulRegVal = 0U;

    xResponseData Response;
    xReqFormat *pxReqFormat;

    pxReqFormat = &ReqFormat;

    pxReqFormat->opcode = xRequest;
    /*format the iossm request with command arguments*/
    iossm_format_command();

    /*wait until the response bit is cleared*/
    do{
        status = ulIoRead32( iossm_data->base_addr + CMD_REQ_REG_OFFSET );

    }while((status & (1U)) == 1U);
    /*send parameter to support the command*/
    if (pxReqFormat->ulParamType != NO_PARAM)
    {
        vIoWrite32((iossm_data->base_addr + pxReqFormat->ulParamType),
                ulParam );
    }
    /*prepare the command bitfield as per ip requirements*/
    ulCmdReq =
            ((xRequest << 0U) | (pxReqFormat->ucCmdType <<
            16U) |
            (iossm_data->instance_id << 24U) | (iossm_data->ip_type << 29U));

    vIoWrite32((iossm_data->base_addr  + CMD_REQ_REG_OFFSET), ulCmdReq );
    /*wait until the response is ready*/
    do{
        status = ulIoRead32((iossm_data->base_addr  + RESP_STAT_REG_OFF));

    }while(!((status & (1U)) == 1U));

    ulRegVal = ulIoRead32((iossm_data->base_addr  + RESP_STAT_REG_OFF));
    ulRegVal &= ~(1U << 0U);
    vIoWrite32((iossm_data->base_addr  + RESP_STAT_REG_OFF), ulRegVal );
    /*capture responses */
    Response.status = status;
    Response.ulResp0 = ulIoRead32((iossm_data->base_addr  + CMD_RESP0_REG_OFF));
    Response.ulResp1 = ulIoRead32((iossm_data->base_addr  + CMD_RESP1_REG_OFF));
    Response.ulResp2 = ulIoRead32((iossm_data->base_addr  + CMD_RESP2_REG_OFF));

    return Response;
}

/* Logic to create a delay of 40us without using DDR
 * This means using no stack/heap/data sections and only CPU registers
 * This logic is not portable as this might cause register clobering
 * If some one wants to reuse this logic, please inspect the assembly
 * and make sure no clobering is happening
 * */
__attribute__((always_inline)) static inline void wait_40us(void) {
    __asm__ volatile (
        "mrs    x1, CNTFRQ_EL0                 \n"
        "mov    x2, #40                        \n" // 40 (microseconds)
        "mul    x2, x1, x2                     \n" // freq * 40

        "movz   x3, #0x4240                    \n"
        "movk   x3, #0xF, lsl #16              \n" // construct 1000000 in register

        "udiv   x2, x2, x3                     \n" // calculate period

        "mrs    x4, CNTVCT_EL0                 \n" // start count

    "1:                                        \n"
        "mrs    x5, CNTVCT_EL0                 \n"
        "sub    x6, x5, x4                     \n"
        "cmp    x6, x2                         \n"
        "blo    1b                             \n" // loop till time
        :
        :
        : "x1", "x2", "x3", "x4", "x5", "x6", "cc" // clobbered list
    );
}

/**
 * @brief  inject iossm error
 *
 * @param[in]  iossm_data - command handle holding required data to send the command.
 * @param[in]  addresst - the address to inject the error into.
 * @param[in]  ulParam - injection type single/double bit error.
 *
 * @return
 * - Response - Response Data from the iossm
 */
xResponseData iossm_err_inject_command(iossm_type const*iossm_data, void *address,
        uint32_t ulParam)
{
    uint32_t volatile status = 0U;
    uint32_t volatile ulCmdReq = 0U;
    uint32_t volatile ulRegVal = 0U;

    volatile uint64_t *error_addr = (uint64_t *)address;
    volatile uint64_t error_data = ERROR_DATA_VALUE;
    xResponseData Response;
    xReqFormat *pxReqFormat;

    pxReqFormat = &ReqFormat;

    /*Setup data*/
    *error_addr = 0;

    pxReqFormat->opcode = ECC_ERR_INJ;
    /*format the iossm request with command arguments*/
    iossm_format_command();

    /*wait until the response bit is cleared*/
    do{
        status = ulIoRead32( iossm_data->base_addr + CMD_REQ_REG_OFFSET );

    }while((status & (1U)) == 1U);

    /*send parameter to support the command*/
    if (pxReqFormat->ulParamType != NO_PARAM)
    {
        vIoWrite32((iossm_data->base_addr + pxReqFormat->ulParamType),
                ulParam );
    }
    /*prepare the command bitfield as per ip requirements*/
    ulCmdReq = ((ECC_ERR_INJ<< 0U) | (pxReqFormat->ucCmdType << 16U) |
            (iossm_data->instance_id << 24U) | (iossm_data->ip_type << 29U));

    SYSTEM_BARRIER;
    vIoWrite32((iossm_data->base_addr  + CMD_REQ_REG_OFFSET), ulCmdReq );
    SYSTEM_BARRIER;

    wait_40us();

    *error_addr = ERROR_DATA_VALUE;
    error_data = *error_addr;
    SYSTEM_BARRIER;
    (void)error_data;

    /*wait until the response is ready*/
    do{
        status = ulIoRead32((iossm_data->base_addr  + RESP_STAT_REG_OFF));
    }while(!((status & (1U)) == 1U));

    ulRegVal = ulIoRead32((iossm_data->base_addr  + RESP_STAT_REG_OFF));
    ulRegVal &= ~(1U << 0U);
    vIoWrite32((iossm_data->base_addr  + RESP_STAT_REG_OFF), ulRegVal );
    /*capture responses */
    Response.status = status;
    Response.ulResp0 = ulIoRead32((iossm_data->base_addr  + CMD_RESP0_REG_OFF));
    Response.ulResp1 = ulIoRead32((iossm_data->base_addr  + CMD_RESP1_REG_OFF));
    Response.ulResp2 = ulIoRead32((iossm_data->base_addr  + CMD_RESP2_REG_OFF));

    return Response;
}


/**
 * @brief  format iossm command request parameters
 *
 * @param[in]  formatter - handle containing the data to be formatted.
 *
 */
void iossm_format_command(void)
{
    xReqFormat *formatter;
    formatter = &ReqFormat;
    /*capturing appropriate parameter type and command type
       for each commands
     */
    switch (formatter->opcode)
    {
    case GET_SYS_INFO:
        formatter->ulParamType = NO_PARAM;
        formatter->ucCmdType = SYSTEM_INFO;
        break;

    case ECC_EN:
        formatter->ulParamType = CMD_PARAM0_REG_OFFSET;
        formatter->ucCmdType = TRIG_CNTRL_OP;
        break;

    case ECC_EN_STAT:
        formatter->ulParamType = NO_PARAM;
        formatter->ucCmdType = TRIG_CNTRL_OP;
        break;

    case ECC_INT_STAT:
        formatter->ulParamType = NO_PARAM;
        formatter->ucCmdType = TRIG_CNTRL_OP;
        break;

    case ECC_INT_ACK:
        formatter->ulParamType = CMD_PARAM0_REG_OFFSET;
        formatter->ucCmdType = TRIG_CNTRL_OP;
        break;

    case ECC_INT_MASK:
        formatter->ulParamType = CMD_PARAM0_REG_OFFSET;
        formatter->ucCmdType = TRIG_CNTRL_OP;
        break;

    case ECC_GET_SBE_INFO:
        formatter->ulParamType = NO_PARAM;
        formatter->ucCmdType = TRIG_CNTRL_OP;
        break;

    case ECC_GET_DBE_INFO:
        formatter->ulParamType = NO_PARAM;
        formatter->ucCmdType = TRIG_CNTRL_OP;
        break;

    case ECC_ERR_INJ:
        formatter->ulParamType = CMD_PARAM0_REG_OFFSET;
        formatter->ucCmdType = TRIG_CNTRL_OP;
        break;

    default:
        formatter->ulParamType = NO_PARAM;
        formatter->ucCmdType = TRIG_CNTRL_OP;
        /* do nothing */
        break;
    }
}
