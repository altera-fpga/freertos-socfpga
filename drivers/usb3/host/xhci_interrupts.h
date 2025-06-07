/*
 * SPDX-FileCopyrightText: Copyright (C) 2025 Altera Corporation
 *
 * SPDX-License-Identifier: MIT-0
 *
 * Header file for Interrupts sub-module of SoC FPGA USB3.1 XHCI low level driver
 */

#ifndef _XHCI_INTERRUPTS_H_
#define _XHCI_INTERRUPTS_H_

#include <stdbool.h>

typedef void (*tusb_int_ptr)(uint8_t, bool);    /* Function pointer for tinyusb interrupt handler */

/*
 * @func  : socfpga_handler
 * @brief : xhci interrupt handler
 * @param[in] handler function pointer to interrupt handler
 */
void socfpga_handler(void *handler);

/*
 * @func  : enable_xhci_interrupts
 * @brief : enable xhci interrupts
 */
void enable_xhci_interrupts(void);

/*
 * @func  : disable_xhc_interrupts
 * @brief : disable the xhci interrupts
 */
void disable_xhci_interrupts(void);

/*
 * @brief : register USB3 ISR 
 * @param[in] int_handler - pointer to tinyusb interrupt handler
 */
bool register_usb3ISR(tusb_int_ptr int_handler);

#endif  /* _XHCI_INTERRUPTS_H_ */
