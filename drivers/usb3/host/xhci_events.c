/*
 * SPDX-FileCopyrightText: Copyright (C) 2025 Altera Corporation
 *
 * SPDX-License-Identifier: MIT-0
 *
 * Low level driver implementation for SoC FPGA USB3.1 XHCI controller.
 * Events sub-module
 */

#include <stdio.h>
#include "xhci_events.h"
#include "socfpga_defines.h"
#include "socfpga_cache.h"
#include "osal_log.h"

void display_event_trbs(struct xhci_data *xhci)
{
    cache_force_invalidate(xhci->xer_ring.xer_enqueue_ptr, 256*sizeof(xhci_trb_t));
    for(int i = 0; i < 10; i++)
    {
        printf("\r\n ID --> %d", i);
        printf("\r\n Address : %p", &xhci->xer_ring.xer_enqueue_ptr[i]);
        printf("\r\n Buffer : %lx", xhci->xer_ring.xer_enqueue_ptr[i].buffer);
        printf("\r\n Status : %x", xhci->xer_ring.xer_enqueue_ptr[i].status);
        printf("\r\n Control : %x", xhci->xer_ring.xer_enqueue_ptr[i].control);
        printf("\r\n TRB Value is : %d", ( (xhci->xer_ring.xer_enqueue_ptr[i].control) & 0xfc00) >> 10 );
        printf("\r\n");
    }
}

uint8_t get_xhc_port_speed(uint8_t rhport)
{
    uint32_t portsc_reg_val, portsc_reg;
    uint8_t port_speed;

    xhci_oper_reg_params_t op_reg = get_xhci_op_registers();
    portsc_reg = (op_reg.xhci_op_base + PORTSC_REG_OFFSET + (0x10*(rhport-1))) & 0x00000000ffffffff;

    portsc_reg_val = RD_REG32(portsc_reg);
    port_speed = ((portsc_reg_val & USB3_PORTSC_20_PORTSPEED_MASK) >> USB3_PORTSC_20_PORTSPEED_POS);

    return port_speed;
}

/*
 * @note : Bit 0 of PORTSC register returns the device connect/disconnect
 * status
 */
bool xhci_port_status(uint8_t rhport)
{
    uint32_t reg_val;
    uint32_t portsc_reg;

    xhci_oper_reg_params_t op_reg = get_xhci_op_registers();
    portsc_reg = (op_reg.xhci_op_base + PORTSC_REG_OFFSET + (0x10*(rhport-1))) & 0x00000000ffffffff;

    reg_val = RD_REG32(portsc_reg);
    if( reg_val & 1)    /*Bit 0 gives attach/detach status */
    {
        return true;
    }
    return false;
}

/* reset the RH port */
void reset_xhci_port(uint8_t rhport)
{
    uint32_t reg_val;
    xhci_oper_reg_params_t op_reg = get_xhci_op_registers();
    uint32_t portsc_reg = (op_reg.xhci_op_base + PORTSC_REG_OFFSET + (0x10*(rhport-1))) & 0x00000000ffffffff;

    reg_val = RD_REG32(portsc_reg);
    reg_val |= ( USB3_PORTSC_20_PR_MASK);
    WR_REG32(portsc_reg, reg_val);
}

bool is_xhci_port_reset_end(uint8_t rhport)
{
    volatile uint32_t reg_val, portsc_reg;
    uint32_t loop = 100;

    xhci_oper_reg_params_t op_reg = get_xhci_op_registers();
    portsc_reg = (op_reg.xhci_op_base + PORTSC_REG_OFFSET + (0x10*(rhport-1))) & 0x00000000ffffffff;
    do
    {
        reg_val = RD_REG32(portsc_reg);
        if( (reg_val & (1U << 4 )) == 0 )
        {
            break;
        }

        loop--;
        osal_task_delay(10);
    }while(loop > 0);

    if( loop <= 0 )
    {
        ERROR("usb3 port reset failure");
        return false;
    }

    return true;
}

xhci_event_trb_t get_xhc_event(struct xhci_data *xhci_ptr)
{
    uint64_t erdp_addr;
    xhci_event_trb_type_t trb_id;
    xhci_event_trb_t event;

    //decrement the event ring trb count
    xhci_ptr->xer_ring.trb_count -= 1;

    erdp_addr = RD_REG32(ERDP);
    erdp_addr &= 0xfffffff0UL;

    cache_force_invalidate((void *)erdp_addr, sizeof(xhci_trb_t));

    xhci_trb_t *event_trb = (xhci_trb_t *)(uint64_t )erdp_addr;

    trb_id = (xhci_event_trb_type_t)( (event_trb->control & 0xfc00) >> 10);

    if( trb_id == TRANSFER_EVENT )
    {
        event.tr_event.tr_trb_ptr = event_trb->buffer;
        event.tr_event.status_field = event_trb->status;
        event.tr_event.control_field = event_trb->control;
    }
    else if( trb_id == COMMAND_COMPLETION_EVENT )
    {
        event.cc_event.cmd_trb_ptr = event_trb->buffer;
        event.cc_event.status_field = event_trb->status;
        event.cc_event.control_field = event_trb->control;
    }
    else if ( trb_id == PORT_STATUS_CHANGE_EVENT )
    {
        event.psc_event.buf = event_trb->buffer;
        event.psc_event.status_field = event_trb->status;
        event.psc_event.control_field = event_trb->control;
    }
    else
    {
        //error TRB type
        configASSERT(trb_id >= 0); 
    }
    erdp_register_update(&xhci_ptr->xer_ring);
    return event;
}

void erdp_register_update(xer_event_ring_t *xer_ring)
{
    uint32_t trb_count;
    uint64_t erdp_addr;

    trb_count = xer_ring->trb_count;

    if( trb_count == 0)
    {
        erdp_addr = RD_REG32(ERDP);
        erdp_addr = (uint64_t)xer_ring->xer_enqueue_ptr;
        erdp_addr |= 8; //EHB flag
        xer_ring->trb_count = XHCI_EVENT_RING_SEG_LENTH;     //reload the event ring trb count to initial value of 512
    }
    else
    {
        erdp_addr = RD_REG32(ERDP);
        erdp_addr += 0x10;
    }
    WR_REG32(ERDP, erdp_addr);
}

int check_device_detach(void)
{
    xhci_oper_reg_params_t op_reg = get_xhci_op_registers();
    const uint8_t rhport1 = 1;
    const uint8_t rhport2 = 1;
    uint32_t portsc_register;
    uint8_t psc1_status;
    uint8_t psc2_status;

    portsc_register = (op_reg.xhci_op_base + PORTSC_REG_OFFSET + (0x10*(rhport1-1))) & 0x00000000ffffffff;
    psc1_status = RD_REG32(portsc_register) & 0x1;

    portsc_register = (op_reg.xhci_op_base + PORTSC_REG_OFFSET + (0x10*(rhport2-1))) & 0x00000000ffffffff;
    psc2_status = RD_REG32(portsc_register) & 0x1;

    if( psc1_status == 0 && psc2_status == 0 )
    {
        return 1;   /* Actual device disconnect occured */
    }

    return 0;       /* RH transition occured, hence device is still connected */
}

xhci_psceg_params_t handle_psceg_event(xpsc_event_t psc_event)
{
    uint32_t portsc_reg, portsc_register;
    xhci_psceg_params_t port_params = { 0 };

    uint8_t rhport = (psc_event.buf & 0xff000000) >> 24;

    xhci_oper_reg_params_t op_reg = get_xhci_op_registers();
    portsc_register = (op_reg.xhci_op_base + PORTSC_REG_OFFSET + (0x10*(rhport-1))) & 0x00000000ffffffff;
    portsc_reg = RD_REG32(portsc_register);

    if( (portsc_reg & USB3_PORTSC_20_CSC_MASK) > 0U)
    {
        //device connected to RH PORT
        if( (portsc_reg & USB3_PORTSC_20_CCS_MASK) == 1U)
        {
            //acknowledge the status bit change
            portsc_reg &= ~PORTSC_CLEAR_FLAGS;
            portsc_reg |= (USB3_PORTSC_20_CSC_MASK);
            WR_REG32(portsc_register, portsc_reg);
            port_params.rhport = rhport;
            port_params.dev_attach_flag = 1;
        }
        //device detach event from RH PORT
        else
        {
            //acknowledge the status bit change
            portsc_reg &= ~PORTSC_CLEAR_FLAGS;
            portsc_reg |= (USB3_PORTSC_20_CSC_MASK);
            WR_REG32(portsc_register, portsc_reg);
            port_params.rhport = rhport;
            port_params.dev_attach_flag = -1;
        }

    }
    else if( (portsc_reg & USB3_PORTSC_20_PRC_MASK) > 0U)
    {
        //Port Reset Event
        portsc_reg &= ~PORTSC_CLEAR_FLAGS;
        portsc_reg |= USB3_PORTSC_20_PRC_MASK;
        WR_REG32(portsc_register, portsc_reg);
    }
    else
    {
        //do nothing
    }

    return port_params;
}
