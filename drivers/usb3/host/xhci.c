/*
 * SPDX-FileCopyrightText: Copyright (C) 2025 Altera Corporation
 *
 * SPDX-License-Identifier: MIT-0
 *
 * Low level driver implementation for SoC FPGA USB3.1 XHCI controller.
 */

#include <errno.h>
#include "xhci.h"
#include "socfpga_defines.h"
#include "osal_log.h"

struct xhci_data* alloc_xhci_hcd(void)
{
    struct xhci_data* xhci;
    int ret;

    xhci = (struct xhci_data *)pvPortAlignedAlloc(64, sizeof(struct xhci_data));
    if( xhci == NULL )
    {
        ERROR("Cannot allocate memory!!!");
        return NULL;
    }

    ret = is_ptr_mem_aligned((uint64_t)xhci, 64);
    if( ret != NO_ERROR )
    {
        ERROR("XHCI Error -%d", ret);
        if( xhci != NULL )
        {
            vPortFree(xhci);
        }
        return NULL;
    }

    return xhci;
}

int is_ptr_mem_aligned(uint64_t addr, uint32_t byte)
{
    uint64_t fact;

    switch(byte)
    {
        case 8U : fact = 0x7UL;
                  break;

        case 16U: fact = 0xFUL;
                  break;

        case 32U: fact = 0x1FUL;
                  break;

        case 64U: fact = 0x3FUL;
                  break;

        case 128U: fact = 0x7FUL;
                   break;

        default: break;
                 return ENOMEM;
    }

    if( (addr & fact) == 0UL)
    {
        return NO_ERROR;
    }

    return ENOMEM;
}

int configure_xhci_params(struct xhci_data *xhci)
{
    volatile uint32_t config_reg_val;
    uint32_t max_slots;

    max_slots = xhci->xhc_cap_ptr->hcsparams1_params.max_dev_slots;

    XHCI_LOG("\r\n MaxSlots is : %x", max_slots );

    config_reg_val = RD_REG32((USBBASE+USB3_CONFIG));
    max_slots |= ( config_reg_val & (~USB3_CONFIG_MAXSLOTSEN_MASK));
    WR_REG32( (USBBASE+USB3_CONFIG), max_slots);

    config_reg_val = RD_REG32((USBBASE+USB3_CONFIG));
    if( (config_reg_val & USB3_CONFIG_MAXSLOTSEN_MASK) == 0 )
    {
        XHCI_LOG("\r\n Max Slot Enable Bit Not configurred Correctly!!!");
        return EIO;
    }
    return NO_ERROR;
}

void start_xhci_controller(void)
{
    uint32_t reg_val;

    reg_val = RD_REG32((USBBASE+USB3_USBCMD));
    reg_val |= ( START_XHCI << USB3_USBCMD_R_S_POS);
    WR_REG32((USBBASE+USB3_USBCMD), reg_val);
}

int wait_for_controller_ready(void)
{
    uint32_t reg_val;
    volatile uint32_t loop = 1000;
    volatile uint32_t err = 0U;

    do
    {
        reg_val = RD_REG32((USBBASE + USB3_USBSTS));
        if( ( reg_val & USB3_USBSTS_CNR_MASK ) == 0U )    //check for CNR flag
        {
            err = 1U;
            break;
        }
        loop--;

    }while( loop > 0);

    if( err == 0U)
    {
        return ETIMEDOUT;
    }

    return NO_ERROR;
}

int xhci_reset(void)
{
    volatile uint32_t reg_val, err_flag = 0U, loop = 1000U;

    reg_val = RD_REG32((USBBASE + USB3_USBCMD));
    reg_val |= (USB3_USBCMD_HCRST_MASK); //Reset the Host Controller
    WR_REG32((USBBASE + USB3_USBCMD), reg_val );

    do
    {
        reg_val = RD_REG32((USBBASE + USB3_USBCMD));
        if( (reg_val & USB3_USBCMD_HCRST_MASK) == 0U)
        {
            err_flag = 1U;
            break;
        }
       osal_task_delay(10);

    }while( --loop > 0U );

    if( err_flag == 0U)
    {
        return ETIMEDOUT;
    }

    return NO_ERROR;
}
