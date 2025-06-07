/*
 * SPDX-FileCopyrightText: Copyright (C) 2025 Altera Corporation
 *
 * SPDX-License-Identifier: MIT-0
 *
 * Low level driver implementation for SoC FPGA USB3.1 XHCI controller.
 * Capabilities sub-module
 */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include "xhci.h"
#include "osal_log.h"

/*
 * @brief : read the host controller capability registers
 */
int get_xhc_cap_params(struct xhci_data *xhci)
{
    xhci->xhc_cap_ptr = (xhci_cap_reg_t *)USBBASE;

    XHCI_LOG("\r\n  CAPLENGTH : %x",  xhci->xhc_cap_ptr->caplength);
    XHCI_LOG("\r\n  HCSPARAMS1 : %x", xhci->xhc_cap_ptr->hcsparams1);
    XHCI_LOG("\r\n  HCSPARAMS2 : %x", xhci->xhc_cap_ptr->hcsparams2);
    XHCI_LOG("\r\n  HCSPARAMS3 : %x", xhci->xhc_cap_ptr->hcsparams3);
    XHCI_LOG("\r\n  HCCPARAMS1 : %x", xhci->xhc_cap_ptr->hccparams1);
    XHCI_LOG("\r\n  DBOFF : %x",  xhci->xhc_cap_ptr->dboff_params.db_array_offset);
    XHCI_LOG("\r\n  RTOFF : %x",  xhci->xhc_cap_ptr->rtsoff_params.rts_offset);
    XHCI_LOG("\r\n  HCCPARAMS2 : %x", xhci->xhc_cap_ptr->hccparams2);

    xhci->op_regs = (xhci_oper_reg_params_t *)malloc(sizeof(xhci_oper_reg_params_t));
    if( xhci->op_regs == NULL )
    {
        ERROR("Cannot allocate memory");
        return ENOMEM;
    }

    xhci->op_regs->xhci_op_base = ( USBBASE + xhci->xhc_cap_ptr->caplength);
    xhci->op_regs->xhci_runtime_base = ( USBBASE + xhci->xhc_cap_ptr->rtsoff_params.rts_offset);
    xhci->op_regs->xhci_db_base = ( USBBASE + xhci->xhc_cap_ptr->dboff_params.db_array_offset);

    XHCI_LOG(" Operational base : %lx \r\n", xhci->op_regs->xhci_op_base);
    XHCI_LOG(" Runtime  base : %lx\r\n", xhci->op_regs->xhci_runtime_base);
    XHCI_LOG(" Doorbell base : %lx\r\n", xhci->op_regs->xhci_db_base);

    return NO_ERROR;
}

xhci_oper_reg_params_t get_xhci_op_registers(void)
{
    const xhci_cap_reg_t *xhc_cap_ptr;
    xhci_oper_reg_params_t op_reg = { 0 };

    xhc_cap_ptr = (xhci_cap_reg_t *)USBBASE;

    op_reg.xhci_op_base = ( USBBASE + xhc_cap_ptr->caplength);
    op_reg.xhci_runtime_base = ( USBBASE + xhc_cap_ptr->rtsoff_params.rts_offset);
    op_reg.xhci_db_base = ( USBBASE + xhc_cap_ptr->dboff_params.db_array_offset);

    return op_reg;
}


