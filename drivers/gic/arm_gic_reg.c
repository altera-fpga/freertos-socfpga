/*
 * SPDX-FileCopyrightText: Copyright (C) 2025 Altera Corporation
 *
 * SPDX-License-Identifier: MIT-0
 *
 * Implementation of low level functions for GIC driver
 */

/*
    File: GIC Registers Implementation

    Implementation of low level functions for GIC driver.
*/

/*
    Macros: GIC macros

    GIC_REG_WRITE - Expands to a toolchain-specific definition.
*/

#include <stdint.h>
#include "arm_gic_reg.h"

#define GIC_REG_WRITE(reg, \
            val)    __asm__ __volatile__ ("MSR " #reg ", %0\n\tDSB SY\n\tISB" : \
    : "r" (val))


/*
    Function: gic_reg_enable_group0_interrupts

    Wrapper around GIC_REG_WRITE.
*/
void gic_reg_enable_group0_interrupts(void)
{
    GIC_REG_WRITE(ICC_IGRPEN0_EL1, (uint64_t)1);
}

/*
    Function: gic_reg_enable_group1_interrupts

    Wrapper around GIC_REG_WRITE.
*/
void gic_reg_enable_group1_interrupts(void)
{
    GIC_REG_WRITE(ICC_IGRPEN1_EL1, (uint64_t)1);
}

/*
    Function: gic_reg_set_priority_mask

    Wrapper around GIC_REG_WRITE.
*/
void gic_reg_set_priority_mask(uint32_t interrupt_id)
{
    GIC_REG_WRITE(ICC_PMR_EL1, (uint64_t)interrupt_id);
}

/*
    Function: gic_reg_get_cpu_affinity

    Calls volatile.
*/
uint64_t gic_reg_get_cpu_affinity(void)
{
    uint64_t affinity = 0U;
    __asm__ volatile (
        "MRS x0, MPIDR_EL1 \n\t"
        "UBFX x1, x0, #32, #8 \n\t"
        "BFI w0, w1, #24, #8 \r\n"
        "MOV %0, x0" : "=r" (affinity));
    return affinity;
}
