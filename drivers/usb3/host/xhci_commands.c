/*
 * SPDX-FileCopyrightText: Copyright (C) 2025 Altera Corporation
 *
 * SPDX-License-Identifier: MIT-0
 *
 * Low level driver implementation for SoC FPGA USB3.1 XHCI controller.
 * Commands sub-module.
 */

#include "xhci_commands.h"
#include "xhci_doorbell.h"
#include "socfpga_cache.h"
#include "osal_log.h"

xhci_trb_t* get_cmdring_enq_ptr(xcr_command_ring_t *cmd_ring)
{
    xhci_trb_t *enq_ptr = cmd_ring->xcr_enqueue_ptr;
    volatile uint32_t c_bit;

    while(1)
    {
        if( is_link_trb(enq_ptr) == 1 )
        {
            enq_ptr = get_next_trb_segment(enq_ptr, &cmd_ring->xcr_ccs_flag);
        }
        c_bit = (enq_ptr->control & 0x00000001U);
        if( c_bit == cmd_ring->xcr_ccs_flag )
        {
            enq_ptr = enq_ptr+1;
        }
        else
        {
            break;
        }
    }

    return enq_ptr;
}

void set_device_address( struct xhci_data *xhci_ptr)
{
    uint32_t ctrl_flags = 0U;
    xhci_trb_t *enq_ptr;

    enq_ptr = get_cmdring_enq_ptr(xhci_ptr->xcr_ring);

    enq_ptr->buffer = (uint64_t)xhci_ptr->ip_ctx;

    ctrl_flags = ( 1 << SLOT_ID_FIELD );
    ctrl_flags |= (ADDRESS_DEVICE_CMD << TRB_FIELD );
    ctrl_flags |= xhci_ptr->xcr_ring->xcr_ccs_flag;
    enq_ptr->control = ctrl_flags;

    cache_force_write_back((void*) xhci_ptr->xcr_ring->xcr_enqueue_ptr, XHCI_CR_TRB_LEN * sizeof(xhci_trb_t));

    ring_xhci_host_db();
}

void reset_command_ring(xcr_command_ring_t *xcr_ring)
{
    uint32_t crcr_reg;
    uint32_t crcr_addr;

    /* first stop the command ring operation */
    crcr_reg = RD_REG32((USBBASE+USB3_CRCR_LO));
    crcr_reg |= (0x2);
    WR_REG32( (USBBASE+USB3_CRCR_LO), crcr_reg);

    crcr_addr = ((uint64_t)&xcr_ring->xcr_dequeue_ptr & 0xffffffff);
    WR_REG32((USBBASE+USB3_CRCR_LO), crcr_addr);
}

void enable_slot_command(xcr_command_ring_t *xcr_ring)
{
    xhci_trb_t *enq_ptr;
    uint32_t ctrl_flags = 0U;

    enq_ptr = get_cmdring_enq_ptr(xcr_ring);

    ctrl_flags = (ENABLE_SLOT_CMD << TRB_FIELD);
    ctrl_flags |= xcr_ring->xcr_ccs_flag;

    enq_ptr->control = ctrl_flags;

    cache_force_write_back((void*) xcr_ring->xcr_enqueue_ptr, XHCI_CR_TRB_LEN * sizeof(xhci_trb_t));

    //call doorbell register
    ring_xhci_host_db();

}
void disable_slot_command(xcr_command_ring_t *xcr_ring, uint32_t slotid)
{
    xhci_trb_t *enq_ptr;
    uint32_t ctrl_flags = 0U;

    enq_ptr = get_cmdring_enq_ptr(xcr_ring);

    ctrl_flags = (DISABLE_SLOT_CMD << TRB_FIELD);
    ctrl_flags |= ( slotid << SLOTID_FIELD);
    ctrl_flags |= xcr_ring->xcr_ccs_flag;

    enq_ptr->control = ctrl_flags;

    cache_force_write_back((void*) xcr_ring->xcr_enqueue_ptr, XHCI_CR_TRB_LEN * sizeof(xhci_trb_t));

    //call doorbell register
    ring_xhci_host_db();
}

void configure_endpoint(struct xhci_data *xhci_ptr)
{
    uint32_t ctrl_flags = 0U;
    xhci_trb_t *enq_ptr;

    enq_ptr = get_cmdring_enq_ptr(xhci_ptr->xcr_ring);

    enq_ptr->buffer = (uint64_t)xhci_ptr->ip_ctx;
    enq_ptr->status = 0U;

    ctrl_flags = 0;
    ctrl_flags |= ( 1 << SLOT_ID_FIELD );
    ctrl_flags |= ( CONFIGURE_ENDPOINT_CMD << TRB_FIELD );
    ctrl_flags |= xhci_ptr->xcr_ring->xcr_ccs_flag;

    enq_ptr->control = ctrl_flags;

    cache_force_write_back((void*) xhci_ptr->xcr_ring->xcr_enqueue_ptr, XHCI_CR_TRB_LEN * sizeof(xhci_trb_t));

    ring_xhci_host_db();
}

void stop_endpoint_command(xcr_command_ring_t *xcr_ring)
{
    uint32_t ctrl_flags = 0U;
    xhci_trb_t *enq_ptr;

    enq_ptr = get_cmdring_enq_ptr(xcr_ring);

    enq_ptr->buffer = 0U;
    enq_ptr->status = 0U;

    ctrl_flags = 0;
    ctrl_flags |= ( 1 << SLOT_ID_FIELD );
    ctrl_flags |= ( 1 << SP_BIT_FIELD );
    ctrl_flags |= ( 1 << ENDPOINT_ID_FIELD );
    ctrl_flags |= ( STOP_ENDPOINT_CMD << TRB_FIELD );
    ctrl_flags |= xcr_ring->xcr_ccs_flag;

    enq_ptr->control = ctrl_flags;

    cache_force_write_back((void*) xcr_ring->xcr_enqueue_ptr, XHCI_CR_TRB_LEN * sizeof(xhci_trb_t));

    ring_xhci_host_db();
}

void update_ep_transfer_ring(xcr_command_ring_t *xcr_ring, xhci_trb_t *xtr_ring)
{
    uint32_t ctrl_flags = 0U;
    xhci_trb_t *enq_ptr;
    /*
     * first stop the endpoint using stop endpoint command xhci_trb_t
     * Then update the TR pointer
     */
    stop_endpoint_command(xcr_ring);

    enq_ptr = get_cmdring_enq_ptr(xcr_ring);

    enq_ptr->buffer = (uint64_t)xtr_ring;
    enq_ptr->status = 0U;

    ctrl_flags = 0;
    ctrl_flags |= ( 1 << SLOT_ID_FIELD );
    ctrl_flags |= ( 1 << ENDPOINT_ID_FIELD );
    ctrl_flags |= ( SET_TR_DEQUEUE_PTR_CMD << TRB_FIELD );
    ctrl_flags |= xcr_ring->xcr_ccs_flag;

    enq_ptr->control = ctrl_flags;

    cache_force_write_back((void*) xcr_ring->xcr_enqueue_ptr, XHCI_CR_TRB_LEN * sizeof(xhci_trb_t));

    ring_xhci_host_db();
}

void evaluate_endpoint(struct xhci_data *xhci_ptr)
{
    uint32_t ctrl_flags = 0U;
    const uint8_t slotid = xhci_ptr->dev_data->slot_id;
    xhci_trb_t *enq_ptr;

    enq_ptr = get_cmdring_enq_ptr(xhci_ptr->xcr_ring);

    enq_ptr->buffer = (uint64_t)xhci_ptr->ip_ctx;
    enq_ptr->status = 0U;

    ctrl_flags |= ( slotid << SLOT_ID_FIELD );
    ctrl_flags |= ( EVALUATE_CONTEXT_CMD << TRB_FIELD );
    ctrl_flags |= xhci_ptr->xcr_ring->xcr_ccs_flag;
    enq_ptr->control = ctrl_flags;

    cache_force_write_back((void *) xhci_ptr->xcr_ring->xcr_enqueue_ptr, XHCI_CR_TRB_LEN * sizeof(xhci_trb_t));

    ring_xhci_host_db();
}
