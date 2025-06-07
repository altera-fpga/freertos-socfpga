/*
 * SPDX-FileCopyrightText: Copyright (C) 2025 Altera Corporation
 *
 * SPDX-License-Identifier: MIT-0
 *
 * Header file for Endpoints sub-module of SoC FPGA USB3.1 XHCI low level driver
 */

#ifndef _SOCFPGA_XHC_ENDPOINT_CONTEXT_H_
#define _SOCFPGA_XHC_ENDPOINT_CONTEXT_H_

#include "xhci.h"

#define USB_EP_DIR_IN_MSK (0x80)    /* !< Endpoint direction mask for xhci */
#define USB_EP_DIR_OUT      (0U)    /* !< OUT endpoint */
#define USB_EP_DIR_IN       (1U)    /* !< IN endpoint */

#define SETUP_STAGE_TRB     (2U)    /* !< trb id of setup stage */
#define DATA_STAGE_TRB      (3U)    /* !< trb id of data stage  */
#define STATUS_STAGE_TRB    (4U)    /* !< trb id of status stage*/
#define NORMAL_TRB          (1U)    /* !< trb id of normal trb*/
#define EVENT_DATA_TRB      (7U)    /* !< trb id of event data trb*/

#define ENT_FLAG             (1)    /* !< Bit postiion for Evaluate Next TRB field */
#define CH_FLAG              (4)    /* !< Bit postiion for CHAIN bit field */
#define IOC_FLAG             (5)    /* !< Bit postiion for IOC field */
#define IDT_FLAG             (6)    /* !< Bit postiion for IDT field */

#define SETUP_STAGE_TRB_LEN (8U)    /* !< xhci setup stage TRB len */

#define STATUS_STAGE_DIR_OUT (0U)   /* !< xhci status stage direction is OUT */
#define STATUS_STAGE_DIR_IN  (1U)   /* !< xhci status stage direction is IN  */

#define DATA_STAGE_DIR_IN   (1U)    /* !< xhci data stage direction is IN */
#define SETUP_STAGE_DIR_IN  (3U)    /* !< xhci setup stage direction is IN*/

#define ROUNDUP(x,y) (((x) + (y) - 1) / (y))
#define ROUNDDOWN(x,y) (((x) + (y)) / (y))
#define MIN(x,y) ((x<y)?x:y)
#define MAX_TD_SIZE  (31U)

#define MAX_TRB_LEN (65536U)

/*
 * @func   : get_endpoint_dir
 * @brief  : get the endpoint direction
 * @param[in] ep_addr : endpoint address
 * @return : endpoint direction
 */
static inline uint8_t get_endpoint_dir(uint8_t ep_addr)
{
    return (uint8_t)((ep_addr & USB_EP_DIR_IN_MSK) ? USB_EP_DIR_IN : USB_EP_DIR_OUT);
}

/*
 * @func   : get_endpoint_num
 * @brief  : get the endpoint number
 * @param[in] ep_addr endpoint address
 * @return : endpoint number
 */
static inline uint8_t get_endpoint_num(uint8_t ep_addr)
{
    return (uint8_t)(ep_addr & 0x3 );
}

/*
 * @func   : get_ep_dci
 * @brief  : find the xhci device context index from the endpoint number
 * @param[in]  xhci_ptr -  endpoint address
 * @return : device context index
 */
uint8_t get_ep_dci(uint8_t ep_addr);

/*
 * @func  : configure_setup_stage
 * @brief : initialize the endpoint0 transfer ring for control transfers
 * @param[in]  xhci reference to xhci hcd structure
 * @param[in]  buffer reference to buffer to hold the control transfer data
 * @param[in]  setup_req reference to the USB control request
 */
void configure_setup_stage(struct xhci_data *xhci, void *buffer, usb_control_request_t *setup_req);


/*
 * @func  : fill_endpoint_transfer_ring
 * @brief : initialize the bulk endpoint transfer rings
 * @param[in]  xhci reference to xhci hcd structure
 * @param[in]  buffer reference to buffer to hold the control transfer data
 * @param[in]  len  number of bytes of data to transfer out/in
 */
void fill_endpoint_transfer_ring(struct ep_priv *ep_ctx, void *buffer, uint32_t len);

/*
 * @func  : endpoint_transfer
 * @brief :  handle the bulk endpoint transfer
 * @param[in]  xhci reference to xhci hcd structure
 * @param[in]  ep_num endpoint number to initiate the transfer
 * @param[in]  dir endpoint direction
 * @param[in]  buffer reference to buffer to hold the control transfer data
 * @param[in]  buflen  number of bytes of data to transfer out/in
 * @return
 *  true  , on successful transfer
 *  false , on failure
 */
bool endpoint_transfer(struct xhci_data *xhci, int ep_num, uint8_t dir, void *buffer, uint32_t buflen);

/*
 * @func  : get_td_size
 * @brief : find the td size field of the transfer TRB for the current transfer
 * @param[in]  bytes_trasnferred : total bytes transferred until currrent trb
 * @param[in]  trb_transfer_len  : length to be transferred in this trb
 * @param[in]  total_len  : total length to be transferred in the current TD
 * @param[in]  mps  : max packet size for the endpoint
 * @param[in]  last_trb  : flag to check if it is last trb of the td or not
 * @return td_size : td size value
 */
uint32_t get_td_size(uint16_t bytes_trasnferred, uint16_t trb_transfer_len, uint32_t total_len, uint16_t mps, bool is_last_trb);

#endif  /* _SOCFPGA_XHC_ENDPOINT_CONTEXT_H_ */
