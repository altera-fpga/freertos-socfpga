/*
 * SPDX-FileCopyrightText: Copyright (C) 2025 Altera Corporation
 *
 * SPDX-License-Identifier: MIT-0
 *
 * Low level driver implementation for SoC FPGA USB3.1 XHCI controller.
 * Endpoints sub-module.
 */

#include "socfpga_cache.h"
#include "xhci_doorbell.h"
#include "xhci_endpoints.h"
#include "xhci_rings.h"

void configure_setup_stage(struct xhci_data *xhci, void *buffer, usb_control_request_t *setup_req)
{
    uint32_t ctrl_flags = 0U;
    xhci_trb_t *ptr = xhci->ep0.ep_tr_enq_ptr;
    do
    {
        if( is_link_trb(ptr) == 1)
        {
            ptr = get_next_trb_segment(ptr, &xhci->ep0.pcs_flag);
        }
        if( (ptr->control & 0x1) == 1U) //checking for Enqueue pointer location
        {
            ptr = ptr + 1;
        }
        else
        {
            break;
        }
    }while(1);

    ptr->buffer = (setup_req->bmRequestType & 0xff); //Type
    ptr->buffer |= ( setup_req->bRequest << 8); //Request
    ptr->buffer |= ( setup_req->wValue << 16); //Wvalue
    ptr->buffer |= ( (uint64_t)setup_req->wIndex  << 32); //WIndex
    ptr->buffer |= ( (uint64_t)setup_req->wLength << 48); //wLength

    ptr->status = SETUP_STAGE_TRB_LEN; // transfer length = 8

    ctrl_flags |=  (SETUP_STAGE_TRB << 10);
    ctrl_flags |= (SETUP_STAGE_DIR_IN << 16);
    ctrl_flags |=  (1 << IDT_FLAG);
    ctrl_flags |= xhci->ep0.pcs_flag;

    ptr->control = ctrl_flags;  //PCS FLAG

    ptr++;

    if( is_link_trb(ptr) == 1)
    {
        ptr = get_next_trb_segment(ptr, &xhci->ep0.pcs_flag);
    }

    if( setup_req->wLength > 0 )
    {
        //Data Stage TRB
        ptr->status = (uint32_t)setup_req->wLength;
        ptr->buffer = (uint64_t)buffer;

        ctrl_flags = 0U;
        ctrl_flags |= ( DATA_STAGE_TRB << 10 );   //TRB TYPE = 3
        ctrl_flags |= ( DATA_STAGE_DIR_IN << 16); //DIR =1
        ctrl_flags |= xhci->ep0.pcs_flag;

        ptr->control = ctrl_flags;

        ptr++;

        if( is_link_trb(ptr) == 1)
        {
            ptr = get_next_trb_segment(ptr, &xhci->ep0.pcs_flag);
        }
    }

    //Status Stage TRB

    ctrl_flags = 0U;
    ctrl_flags |= ( STATUS_STAGE_TRB << 10);
    ctrl_flags |= ( 1 << IOC_FLAG ); //IOC =1
    ctrl_flags |= ( STATUS_STAGE_DIR_OUT << 16 ); //IOC =1
    ctrl_flags |= xhci->ep0.pcs_flag; //PCS

    ptr->control = ctrl_flags;

    cache_force_write_back(xhci->ep0.ep_tr_enq_ptr, EP_TRB_SEG_LENGTH*sizeof(xhci_trb_t));
}

uint8_t get_ep_dci(uint8_t ep_addr)
{
    uint8_t ep_dir;
    uint8_t ep_num;
    uint8_t ep_dci;

    ep_dir = get_endpoint_dir(ep_addr);
    ep_num = get_endpoint_num(ep_addr);

    if( ep_num == 0 )
    {
        ep_dci = 1U;
    }
    else
    {
        ep_dci = (ep_num*2) + ep_dir; //EP DCI  is ICI -1
    }

    return ep_dci;
}

uint32_t get_td_size(uint16_t bytes_trasnferred, uint16_t trb_transfer_len, uint32_t total_len, uint16_t mps, bool is_last_trb)
{
    uint32_t total_pkt_count;
    uint32_t total_pkt_xferred;
    uint32_t td_size;

    if( is_last_trb == true )
    {
        return 0U;
    }

    total_pkt_count = ROUNDUP(total_len, mps);
    total_pkt_xferred = ROUNDDOWN((bytes_trasnferred+trb_transfer_len), mps);

    td_size = total_pkt_count-total_pkt_xferred;

    if( td_size > MAX_TD_SIZE )
    {
        td_size = MAX_TD_SIZE;
    }

    return td_size;
}

void fill_endpoint_transfer_ring(struct ep_priv *ep_ctx, void *buffer, uint32_t len)
{
    uint32_t ep_pcs_flag;
    uint16_t e_mps = ep_ctx->ep_desc->wMaxPacketSize;

    volatile uint32_t trb_transfer_len;
    volatile uint32_t bytes_xferred;
    volatile uint32_t total_bytes;

    volatile uint32_t status_field;
    volatile uint32_t ctrl_flags;
    volatile uint32_t td_size;

    uint32_t bytes_pending;
    uint64_t addr;
    bool last_trb = false;
    volatile uint32_t c_bit = 0;

    xhci_trb_t *ptr = ep_ctx->ep_tr_enq_ptr;

    while(1)
    {
        if( is_link_trb(ptr) == 1)
        {
            ptr = get_next_trb_segment(ptr, &ep_ctx->pcs_flag);
        }

        c_bit = (ptr->control & 0x00000001U);
        if( c_bit == ep_ctx->pcs_flag )
        {
            ptr = ptr + 1;
        }
        else
        {
            break;
        }
    };

    trb_transfer_len = 0U;
    bytes_xferred = 0U;
    total_bytes = len;
    bytes_pending = len;
    addr = (uint64_t)buffer;

    for( bytes_xferred = 0U; bytes_xferred < total_bytes; bytes_xferred += trb_transfer_len)
    {
        ep_pcs_flag = ep_ctx->pcs_flag;
        status_field = 0U;
        ctrl_flags = 0U;

        if( bytes_pending > MAX_TRB_LEN)
        {
            trb_transfer_len = 65536;
        }
        else
        {
            trb_transfer_len = bytes_pending;
        }

        if( bytes_xferred + trb_transfer_len > total_bytes)
        {
            trb_transfer_len = total_bytes - bytes_xferred;
        }

        if( bytes_xferred + trb_transfer_len < total_bytes )
        {
            ctrl_flags |= ( 1 << CH_FLAG );
        }
        //Last TRB of the TD, Next will be Event TRB
        if( bytes_xferred + trb_transfer_len >= total_bytes )
        {
            ctrl_flags |= ( 1U << ENT_FLAG );
            ctrl_flags |= ( 1U << CH_FLAG );
            last_trb = true;
        }

        td_size = get_td_size(bytes_xferred, trb_transfer_len, total_bytes, e_mps, last_trb);

        status_field |= (td_size << 16);
        status_field |= trb_transfer_len;

        ctrl_flags |= ( NORMAL_TRB << TRB_FIELD );
        ctrl_flags |= ( ep_pcs_flag );

        ptr->buffer = addr;
        ptr->status = status_field;
        ptr->control = ctrl_flags;

        addr += (uint64_t)trb_transfer_len;
        bytes_pending -= trb_transfer_len;

        ptr++;

        if( is_link_trb(ptr) == 1)
        {
            ptr = get_next_trb_segment(ptr, &ep_ctx->pcs_flag);
        }
    }

    ptr->buffer = (uint64_t)ep_ctx->ep_tr_enq_ptr;
    ptr->status = 0U;

    ctrl_flags = 0U;
    ctrl_flags |= ( 1 << IOC_FLAG );
    ctrl_flags |= ( EVENT_DATA_TRB << TRB_FIELD );
    ctrl_flags |= ( ep_ctx->pcs_flag);

    ptr->control = ctrl_flags;
    cache_force_write_back(ep_ctx->ep_tr_enq_ptr, EP_TRB_SEG_LENGTH*sizeof(xhci_trb_t));
}

bool endpoint_transfer(struct xhci_data *xhci, int ep_num, uint8_t dir, void *buffer, uint32_t buflen)
{
    uint8_t ICI;
    ICI = (ep_num*2) + dir + 1U;
    uint8_t db_target = (uint8_t)(ICI-1);

    if( dir == 0U )  //out transfer
    {
        cache_force_write_back(buffer, buflen);
        fill_endpoint_transfer_ring(xhci->msc_eps.ep_out, buffer, buflen);
        ring_xhci_ep_doorbell(xhci->op_regs, db_target);
    }
    else if( dir == 1U )
    {
        cache_force_write_back(buffer, buflen);
        fill_endpoint_transfer_ring(xhci->msc_eps.ep_in, buffer, buflen);
        ring_xhci_ep_doorbell(xhci->op_regs, db_target);
        cache_force_invalidate(buffer, buflen);
    }
    else
    {
        return false;
    }
    return true;
}

