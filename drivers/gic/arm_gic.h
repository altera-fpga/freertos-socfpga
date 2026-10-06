/*
 * SPDX-FileCopyrightText: Copyright (C) 2025 Altera Corporation
 *
 * SPDX-License-Identifier: MIT-0
 *
 * Header file for GIC driver
 */

/*
    File: GIC Driver

    Header file for GIC driver.
*/

/*
    Constants: REGISTER definitions

    REGISTER_SOCFPGA_DIST_BASE_ADDR - Value: 0x1D000000U.
    REGISTER_SOCFPGA_RD_BASE_ADDR   - Value: 0x1D060000U.
*/

/*
    Constants: GICV3 definitions

    GICV3_GROUP0                - Value: 0U.
    GICV3_GROUP1_SECURE         - Value: 1U.
    GICV3_GROUP1_NON_SECURE     - Value: 2U.
    GICV3_CONFIG_LEVEL          - Value: 0U.
    GICV3_CONFIG_EDGE           - Value: 2U.
    GICV3_ROUTE_MODE_ANY        - Value: 0x80000000U.
    GICV3_ROUTE_MODE_COORDINATE - Value: 0U.
*/

/*
    Constants: INTERRUPT definitions

    INTERRUPT_RETURN_SUCCESS          - Value: 0.
    INTERRUPT_RETURN_ERROR            - Value: -1.
    INTERRUPT_RETURN_INVALID_PRIORITY - Value: -2.
    INTERRUPT_RETURN_INVALID_RDIST    - Value: -3.
    INTERRUPT_RETURN_INVALID_ID       - Value: -4.
    INTERRUPT_RETURN_INVALID_GROUP    - Value: -5.
    INTERRUPT_RETURN_INVALID_SPI      - Value: -6.
*/

#ifndef __ARM_GIC_H__
#define __ARM_GIC_H__

#include <stdint.h>

#define REGISTER_SOCFPGA_DIST_BASE_ADDR      0x1D000000U
#define REGISTER_SOCFPGA_RD_BASE_ADDR        0x1D060000U

#define GICV3_GROUP0                        0U
#define GICV3_GROUP1_SECURE                 1U
#define GICV3_GROUP1_NON_SECURE             2U

#define GICV3_CONFIG_LEVEL                  0U
#define GICV3_CONFIG_EDGE                   2U
#define GICV3_ROUTE_MODE_ANY                0x80000000U
#define GICV3_ROUTE_MODE_COORDINATE         0U

/* Return macros */
#define INTERRUPT_RETURN_SUCCESS             0
#define INTERRUPT_RETURN_ERROR               -1
#define INTERRUPT_RETURN_INVALID_PRIORITY    -2
#define INTERRUPT_RETURN_INVALID_RDIST       -3
#define INTERRUPT_RETURN_INVALID_ID          -4
#define INTERRUPT_RETURN_INVALID_GROUP       -5
#define INTERRUPT_RETURN_INVALID_SPI         -6

int32_t gic_enable_gic(void);
int32_t gic_get_redist_id(uint32_t affinity);
int32_t gic_wakeup_redist(uint32_t rd);

int32_t gic_set_int_priority(uint32_t id, uint32_t rd, uint8_t priority);
int32_t gic_set_int_group(uint32_t id, uint32_t rd, uint32_t security);
int32_t gic_set_int_route( uint32_t id, uint32_t mode, uint32_t affinity );

int32_t gic_enable_int(uint32_t id, uint32_t rd);
int32_t gic_disable_int( uint32_t id, uint32_t rd );
int32_t gic_set_int_type(uint32_t id, uint32_t rd, uint32_t type);
int32_t gic_clear_int_pending(uint32_t id, uint32_t rd);
int32_t gic_set_int_pending( uint32_t id, uint32_t rd );

void gic_enable_interrupts(void);
#endif /* __ARM_GIC_H__ */
