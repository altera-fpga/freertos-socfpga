/*
 * SPDX-FileCopyrightText: Copyright (C) 2025 Altera Corporation
 *
 * SPDX-License-Identifier: MIT-0
 *
 * Header file for iossm ll driver
 */

/*
    File: IOSSM Low-Level Driver

    Header file for iossm ll driver.
*/

/*
    Constants: IOSSM constants

    IOSSM0_BASE_ADDR            - Value: 0x18400000U.
    IOSSM1_BASE_ADDR            - Value: 0x18800000U.
    IOSSM_ECC_ERROR_STATUS      - Value: 0x300U.
    IOSSM_ECC_COUNT_MASK        - Value: 0xFFU.
    IOSSM_ECC_ERR_DATA_START    - Value: 0x310U.
    IOSSM_ECC_GET_ERR_TYPE      - Expands to (IOSSM_ECC_ERR_DATA_START + ((idx) * 8U)).
    IOSSM_ECC_ERR_TYP_FIELD     - Expands to (((data) >> 6) & 0xFU).
    IOSSM_ECC_GET_ERR_OFFSET    - Expands to (IOSSM_ECC_ERR_DATA_START + ((idx) * 8U) + 4U).
    CALIBRATION_STAT_REG_OFFSET - Value: 0x400U.
    CMD_PARAM0_REG_OFFSET       - Value: 0x438U.
    CMD_PARAM1_REG_OFFSET       - Value: 0x434U.
    CMD_PARAM2_REG_OFFSET       - Value: 0x430U.
    CMD_PARAM3_REG_OFFSET       - Value: 0x42CU.
    CMD_PARAM4_REG_OFFSET       - Value: 0x428U.
    CMD_PARAM5_REG_OFFSET       - Value: 0x424U.
    CMD_PARAM6_REG_OFFSET       - Value: 0x420U.
    CMD_RESP0_REG_OFF           - Value: 0x458U.
    CMD_RESP1_REG_OFF           - Value: 0x454U.
    CMD_RESP2_REG_OFF           - Value: 0x450U.
    RESP_STAT_REG_OFF           - Value: 0x45CU.
    CMD_REQ_REG_OFFSET          - Value: 0x43CU.
    NO_PARAM                    - Value: 0U.
    CMD_TYPE_NO_OP              - Value: 0U.
    TRIG_CNTRL_OP               - Value: 4U.
    SYSTEM_INFO                 - Value: 1U.
    NO_TYPE                     - Value: 0U.
    NO_ID                       - Value: 0U.
    SING_BIT_ERR_PARAM          - Value: 0xF4U.
    DBL_BIT_ERR_PARAM           - Value: 0x03U.
    GET_SYS_INFO                - Value: 0x01U.
    ECC_EN                      - Value: 0x101U.
    ECC_EN_STAT                 - Value: 0x102U.
    ECC_INT_STAT                - Value: 0x103U.
    ECC_INT_ACK                 - Value: 0x104U.
    ECC_INT_MASK                - Value: 0x105U.
    ECC_WRBK_EN                 - Value: 0x106U.
    ECC_GET_SBE_INFO            - Value: 0x107U.
    ECC_GET_DBE_INFO            - Value: 0x108U.
    ECC_ERR_INJ                 - Value: 0x109U.
    ECC_CLEAR_ERR               - Value: 0x110U.
*/

#ifndef __SOCFPGA_IOSSM_LL_H__
#define __SOCFPGA_IOSSM_LL_H__

#include <stdint.h>

#define IOSSM0_BASE_ADDR    0x18400000U
#define IOSSM1_BASE_ADDR    0x18800000U

#define IOSSM_ECC_ERROR_STATUS    0x300U
#define IOSSM_ECC_COUNT_MASK      0xFFU

#define IOSSM_ECC_ERR_DATA_START    0x310U
#define IOSSM_ECC_GET_ERR_TYPE(idx)      (IOSSM_ECC_ERR_DATA_START + ((idx) * \
    8U))
#define IOSSM_ECC_ERR_TYP_FIELD(data)    (((data) >> 6) & 0xFU)
#define IOSSM_ECC_GET_ERR_OFFSET(idx)    (IOSSM_ECC_ERR_DATA_START + ((idx) * \
    8U) + 4U)

#define CALIBRATION_STAT_REG_OFFSET    0x400U

#define CMD_PARAM0_REG_OFFSET    0x438U
#define CMD_PARAM1_REG_OFFSET    0x434U
#define CMD_PARAM2_REG_OFFSET    0x430U
#define CMD_PARAM3_REG_OFFSET    0x42CU
#define CMD_PARAM4_REG_OFFSET    0x428U
#define CMD_PARAM5_REG_OFFSET    0x424U
#define CMD_PARAM6_REG_OFFSET    0x420U

#define CMD_RESP0_REG_OFF    0x458U
#define CMD_RESP1_REG_OFF    0x454U
#define CMD_RESP2_REG_OFF    0x450U

#define RESP_STAT_REG_OFF    0x45CU

#define CMD_REQ_REG_OFFSET    0x43CU

#define NO_PARAM          0U
#define CMD_TYPE_NO_OP    0U
#define TRIG_CNTRL_OP     4U
#define SYSTEM_INFO       1U

#define NO_TYPE    0U
#define NO_ID      0U

#define SING_BIT_ERR_PARAM    0xF4U
#define DBL_BIT_ERR_PARAM     0x03U

/*
 * command requirement modes
 */
#define GET_SYS_INFO        0x01U
#define ECC_EN              0x101U
#define ECC_EN_STAT         0x102U
#define ECC_INT_STAT        0x103U
#define ECC_INT_ACK         0x104U
#define ECC_INT_MASK        0x105U
#define ECC_WRBK_EN         0x106U
#define ECC_GET_SBE_INFO    0x107U
#define ECC_GET_DBE_INFO    0x108U
#define ECC_ERR_INJ         0x109U
#define ECC_CLEAR_ERR       0x110U

/*
    Struct: iossm_req_payload_t

    IOSSM command request data. Declared as req_format in socfpga_iossm_ll.c.
*/
typedef struct
{
    uint32_t param_type;
    uint32_t cmd_type;
    uint32_t opcode;
    uint32_t reg_val;
} iossm_req_payload_t;

/*
    Struct: iossm_response_data_t

    IOSSM command response data. Passed to iossm_send_command and iossm_err_inject_command.
*/
typedef struct
{
    uint32_t resp0;
    uint32_t resp1;
    uint32_t resp2;
    uint32_t status;
} iossm_response_data_t;

/*
    Struct: iossm_type_t

    IOSSM interface data. Passed to iossm_send_command and iossm_err_inject_command.
*/
typedef struct
{
    uint32_t base_addr;
    uint32_t ip_type;
    uint32_t instance_id;
} iossm_type_t;

iossm_response_data_t iossm_send_command(iossm_type_t const *iossm_data, uint32_t request,
        uint32_t param);

iossm_response_data_t iossm_err_inject_command(iossm_type_t const *iossm_data, void *address,
        uint32_t param);

uint32_t iossm_read_register(uint32_t base_addr, uint32_t reg);

void iossm_format_command(void);

#endif /* __SOCFPGA_IOSSM_LL_H__ */
