/*
 * SPDX-FileCopyrightText: Copyright (C) 2025 Altera Corporation
 *
 * SPDX-License-Identifier: MIT-0
 *
 * Header file for UART low level driver
 */

/*
    File: UART Low-Level Driver

    Header file for UART low level driver.
*/

/*
    Constants: UART constants

    UART_MAX_INSTANCE    - Value: 2U.
    MODEM_STATUS         - Value: 0x0U.
    NO_INTERRUPT         - Value: 0x1U.
    THR_EMPTY            - Value: 0x2U.
    RX_DATA_RDY          - Value: 0x4U.
    RX_LINE_STATUS       - Value: 0x6U.
    BUSY_DETECT          - Value: 0x7U.
    CHAR_TIMEOUT         - Value: 0xCU.
    UART_NO_INT          - Value: 0U.
    UART_TXBUF_EMPTY_INT - Value: 0x1U.
    UART_RXBUF_RDY_INT   - Value: 0x2U.
    UART_HW_ERR_INT      - Value: 0x3U.
*/

#ifndef __SOCFPGA_UART_LL_H__
#define __SOCFPGA_UART_LL_H__

#include <stdint.h>

#include "socfpga_uart.h"

#define UART_MAX_INSTANCE    2U

#define MODEM_STATUS      0x0U
#define NO_INTERRUPT      0x1U
#define THR_EMPTY         0x2U
#define RX_DATA_RDY       0x4U
#define RX_LINE_STATUS    0x6U
#define BUSY_DETECT       0x7U
#define CHAR_TIMEOUT      0xCU

#define UART_NO_INT             0U
#define UART_TXBUF_EMPTY_INT    0x1U
#define UART_RXBUF_RDY_INT      0x2U
#define UART_HW_ERR_INT         0x3U

/*
    Enum: uart_interrupt_id_t

    Values used by the UART low-level driver: INTERRUPT_RX and INTERRUPT_TX. Passed to uart_enable_interrupt and uart_disable_interrupt.
*/
typedef enum
{
    INTERRUPT_RX, INTERRUPT_TX, INTERRUPT_HW
} uart_interrupt_id_t;

void uart_init(uint32_t instance);
void uart_deinit(uint32_t instance);

uint32_t uart_tx_polling(uint32_t base_address, uint8_t *const buffer,
        uint32_t nbytes);
uint16_t uart_write_fifo(uint32_t base_address, uint8_t *const buffer,
        uint32_t bytes);
uint16_t uart_read_fifo(uint32_t base_address, uint8_t *const buffer,
        uint32_t bytes);


void uart_enable_interrupt(uint32_t base_address, uart_interrupt_id_t id);
void uart_disable_interrupt(uint32_t base_address, uart_interrupt_id_t id);

uint32_t get_int_status(uint32_t base_address);

void uart_config_fifo(uint32_t base_address);

void uart_set_config(uint32_t base_address, uart_partity_t parity,
        uart_stop_bits_t stopbit, uint32_t wordlen);

uint32_t uart_set_baud(uint32_t base_address, uint32_t baud);


void uart_get_config(uint32_t base_address, uint32_t *baud,
        uart_partity_t *parity, uart_stop_bits_t *stopbit,
        uint32_t *wordlen);
uint32_t uart_get_baud(uint32_t base_address);

#endif /* __SOCFPGA_UART_LL_H__ */
