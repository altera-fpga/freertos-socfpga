/*
 * SPDX-FileCopyrightText: Copyright (C) 2025 Altera Corporation
 *
 * SPDX-License-Identifier: MIT-0
 *
 * Header file for i2c ll driver
 */

#ifndef __SOCFPGA_I2C_LL_H__
#define __SOCFPGA_I2C_LL_H__

#include <stdint.h>

/*Max number of i2c instances*/
#define MAX_I2C_INSTANCES    5U

#define I2C_NO_INT                    0U /*!< No interrupt*/
#define I2C_TX_EMPTY_INT              1U /*!< Tx FIFO Empty interrupt*/
#define I2C_RX_FULL_INT               2U /*!< Rx FIFO Full interrupt*/
#define I2C_TX_ABORT_INT              4U /*!< Tx Abort interrupt*/

void i2c_enable_interrupt( uint32_t baseAddr, uint32_t interruptReq );

void i2c_disable_interrupt( uint32_t baseAddr, uint32_t interruptReq );

uint32_t i2c_get_interrupt_status( uint32_t baseAddr );

void i2c_clear_interrupt( uint32_t baseAddr );

void i2c_set_target_addr( uint32_t baseAddr, uint32_t slaveAddr );

uint16_t i2c_write_fifo( uint32_t baseAddr, uint8_t *const pucBuffer,
        uint32_t xBytes, BaseType_t noStopFlag );

uint16_t i2c_enq_read_cmd( uint32_t baseAddr, uint32_t xBytes,
        BaseType_t noStopFlag );

uint16_t i2c_read_fifo( uint32_t baseAddr, uint8_t *const pucBuffer,
        uint32_t xBytes );

uint32_t i2c_config_master( uint32_t baseAddr, uint32_t speed );

void i2c_init( uint32_t baseAddr );

void i2c_ll_cancel( uint32_t baseAddr );

uint32_t i2c_get_config( uint32_t baseAddr );

#endif   /* ifndef __SOCFPGA_I2C_LL_H__ */
