/*
 * SPDX-FileCopyrightText: Copyright (C) 2025 Altera Corporation
 *
 * SPDX-License-Identifier: MIT-0
 *
 * Low level driver implementation for SoC FPGA USB3.1 XHCI controller.
 * Interrupt sub-module.
 */

#include "xhci.h"
#include "xhci_interrupts.h"
#include "socfpga_interrupt.h"
#include "socfpga_usb3_reg.h"
#include "socfpga_defines.h"
#include "socfpga_interrupt_priority.h"

void socfpga_usb3_handler(void *handler)
{
    tusb_int_ptr usb3_int_handler = (tusb_int_ptr)handler;
    uint32_t reg_val;
    struct xhci_oper_reg_params op_regs;

    op_regs = get_xhci_op_registers();

    reg_val = RD_REG32((USBBASE+USB3_USBSTS));

    if( (reg_val & USB3_USBSTS_HSE_MASK) | (reg_val & USB3_USBSTS_HCE_MASK))
    {
        xhci_reset();
        start_xhci_controller();
        return;
    }

    reg_val |= (USB3_USBSTS_EINT_MASK);
    WR_REG32((USBBASE+USB3_USBSTS), reg_val);

    reg_val = RD_REG32((op_regs.xhci_runtime_base+USB3_IMAN));
    reg_val |= USB3_IMAN_IP_MASK;
    WR_REG32((op_regs.xhci_runtime_base+USB3_IMAN), reg_val);

    /* rh_port = 1 -> RH port number as per tinyusb stack */
    /* in_isr = true, ISR flag for tinyusb stack */
    usb3_int_handler(1, true);
}

void enable_xhci_interrupts(void)
{
    volatile uint32_t reg_val;
    struct xhci_oper_reg_params op_regs;

    op_regs = get_xhci_op_registers();

    reg_val = RD_REG32((op_regs.xhci_runtime_base+USB3_IMAN));
    reg_val |= USB3_IMAN_IE_MASK;
    WR_REG32((op_regs.xhci_runtime_base+USB3_IMAN), reg_val);

    reg_val = RD_REG32((USBBASE+USB3_USBCMD));
    reg_val |= USB3_USBCMD_INTE_MASK;
    WR_REG32((USBBASE+USB3_USBCMD), reg_val);
}

void disable_xhc_interrupts(void)
{

    volatile uint32_t reg_val;
    struct xhci_oper_reg_params op_regs;

    op_regs = get_xhci_op_registers();

    reg_val = RD_REG32((op_regs.xhci_runtime_base+USB3_IMAN));
    reg_val &= ~(USB3_IMAN_IE_MASK);
    WR_REG32((op_regs.xhci_runtime_base+USB3_IMAN), reg_val);

    reg_val = RD_REG32((op_regs.xhci_runtime_base+USB3_IMAN));
    reg_val &= ~(USB3_IMAN_IP_MASK);
    WR_REG32((op_regs.xhci_runtime_base+USB3_IMAN), reg_val);

    reg_val = RD_REG32((USBBASE+USB3_USBCMD));
    reg_val &= ~(USB3_USBCMD_INTE_MASK);
    WR_REG32((USBBASE+USB3_USBCMD), reg_val);
}

bool register_usb3ISR(tusb_int_ptr int_handler)
{
    SocfpgaInterruptErr_t intr_ret;

    intr_ret = interrupt_register_isr(eUsb1Irq, socfpga_usb3_handler, (void *)int_handler);
    if( intr_ret != eErrOk )
    {
        return false;
    }

    intr_ret = interrupt_spi_enable(eUsb1Irq, eSpiInterruptTypeLevel ,eSpiInterruptModeAny, GIC_INTERRUPT_PRIORITY_USB3); //PDK
    if( intr_ret != eErrOk )
    {
        return false;
    }

    return true;
}
