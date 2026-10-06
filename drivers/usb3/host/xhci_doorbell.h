/*
 * SPDX-FileCopyrightText: Copyright (C) 2025 Altera Corporation
 *
 * SPDX-License-Identifier: MIT-0
 *
 * Header file for Doorbell sub-module of SoC FPGA USB3.1 XHCI low level driver
 */

#ifndef __XHCI_DOORBELL_H__
#define __XHCI_DOORBELL_H__

#include "xhci.h"

#define XHCI_DB_REG_SIZE         (4U)                           /* !< Doorbell register size */
#define XHCI_DB_EP0_TARGET       (0x1U)                         /* !< doorbell target for endpoint 0 transfers */

#define XHCI_COMMAND_DOORBELL    (0U << USB3_DB_DB_TARGET_POS)  /* !< Doorbell target for command ring */

/*
    File: xHCI Doorbell Driver

    Declares the xHCI Doorbell driver interface: 3 functions and 3
    constants.
*/

/*
    Function: ring_xhci_host_db

    ring the XHCI host controller command doorbell
*/
void ring_xhci_host_db(void);

/*
    Function: ring_xhci_ep0_db

    ring the XHCI host controller Endpoint 0 doorbell

    Parameters:

        op_regs - reference to operation register structure
*/
void ring_xhci_ep0_db(xhci_oper_reg_params_t *op_regs);

/*
    Function: ring_xhci_ep_doorbell

    ring the XHCI host controller doorbell

    Parameters:

        op_regs - reference to operation register structure
        db_target - doorbell target to ring
*/
void ring_xhci_ep_doorbell(xhci_oper_reg_params_t *op_regs, uint8_t db_target);

#endif  /* __XHCI_DOORBELL_H__ */
