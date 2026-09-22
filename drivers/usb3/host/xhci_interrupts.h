/*
 * SPDX-FileCopyrightText: Copyright (C) 2025 Altera Corporation
 *
 * SPDX-License-Identifier: MIT-0
 *
 * Header file for Interrupts sub-module of SoC FPGA USB3.1 XHCI low level driver
 */

/*
    File: xHCI Interrupts Driver

    Declares the xHCI Interrupts driver interface: 3 functions and 2 types.
*/

#ifndef __XHCI_INTERRUPTS_H__
#define __XHCI_INTERRUPTS_H__

#include <stdbool.h>

/* Function pointer for tinyusb interrupt handler */
typedef void (*tusb_int_ptr)(uint8_t rhport, bool in_isr);

/*
    Struct: xhci_int_desc

    Descriptor to pass to interrupt handler. Callers reach it through xhci_int_ptr_t.
*/
struct xhci_int_desc
{
    tusb_int_ptr int_handler; /* !< pointer to tinyusb interrupt handler */
};

typedef struct xhci_int_desc *xhci_int_ptr_t;

/*
    Function: enable_xhci_interrupts

    enable xhci interrupts
*/
void enable_xhci_interrupts(void);

/*
    Function: disable_xhci_interrupts

    disable the xhci interrupts
*/
void disable_xhci_interrupts(void);

/*
    Function: register_usb3ISR

    register USB3 ISR

    Parameters:

        int_handler - - pointer to tinyusb interrupt handler
*/
bool register_usb3ISR(xhci_int_ptr_t int_handler);

#endif  /* __XHCI_INTERRUPTS_H__ */
