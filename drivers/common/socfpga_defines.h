/*
 * SPDX-FileCopyrightText: Copyright (C) 2025 Altera Corporation
 *
 * SPDX-License-Identifier: MIT-0
 *
 * HAL driver implementation for i2c
 */

#ifndef __SOCFPGA_DEFINES_H__
#define __SOCFPGA_DEFINES_H__

#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include "osal.h"

#define WR_REG32(ulAddress, ulVal)    vIoWrite32(ulAddress, ulVal)
#define RD_REG32(ulAddress)           ulIoRead32(ulAddress)

#define WR_REG64(ulAddress, ulVal)    vIoWrite64(ulAddress, ulVal)
#define RD_REG64(ulAddress)           ullIoRead64(ulAddress)

static inline void vIoWrite32(uint32_t ulAddress, uint32_t ulValue) {
    volatile uint32_t*pulAddress = (volatile uint32_t *)((uint64_t)ulAddress);
    *pulAddress = ulValue;
}

static inline uint32_t ulIoRead32(uint32_t ulAddress) {
    volatile uint32_t*pulAddress = (volatile uint32_t *)((uint64_t)ulAddress);
    return *pulAddress;
}

static inline void vIoWrite64(uint32_t ullAddress, uint64_t ullValue) {
    volatile uint64_t*pullAddress = (volatile uint64_t *)((uint64_t)ullAddress);
    *pullAddress = ullValue;
}

static inline uint64_t ullIoRead64(uint32_t ullAddress) {
    volatile uint64_t*pullAddress = (volatile uint64_t *)((uint64_t)ullAddress);
    return *pullAddress;
}

#endif //__SOCFPGA_DEFINES_H__
