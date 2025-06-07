/*
 * SPDX-FileCopyrightText: Copyright (C) 2025 Altera Corporation
 *
 * SPDX-License-Identifier: MIT-0
 *
 * Header file for Doorbell sub-module of SoC FPGA USB3.1 XHCI low level driver
 */

#ifndef __XHCI_DOORBELL_H__
#define __XHCI_DOORBELL_H__

#include <stdint.h>
#include "xhci.h"
#include "socfpga_usb3_reg.h"

#define XHCI_DB_REG_SIZE    (4U)                                /* !< Doorbell register size */
#define XHCI_DB_EP0_TARGET (0x1U)                               /* !< doorbell target for endpoint 0 transfers */

#define XHCI_COMMAND_DOORBELL (0U << USB3_DB_DB_TARGET_POS)     /* !< Doorbell target for command ring */

/*
 * @func  : ring_xhci_host_db
 * @brief : ring the XHCI host controller command doorbell
 */
void ring_xhci_host_db(void);

/*
 * @func  : ring_xhci_ep0_db
 * @brief : ring the XHCI host controller Endpoint 0 doorbell
 * @param[in] op_regs reference to operation register structure
 */
void ring_xhci_ep0_db(xhci_oper_reg_params_t *op_regs);

/*
 * @func  : ring_xhci_ep_doorbell
 * @brief : ring the XHCI host controller doorbell
 * @param[in] op_regs reference to operation register structure
 * @param[in] db_target  doorbell target to ring
 */
void ring_xhci_ep_doorbell(xhci_oper_reg_params_t *op_regs, uint8_t db_target);

#endif  /* _XHCI_DOORBELL_H_ */
