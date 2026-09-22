/*
 * SPDX-FileCopyrightText: Copyright (C) 2025 Altera Corporation
 *
 * SPDX-License-Identifier: MIT-0
 *
 * Header file for commands sub-module in SoC FPGA USB3.1 XHCI low level driver.
 */

#ifndef __XHCI_COMMANDS_H__
#define __XHCI_COMMANDS_H__

#include <stdint.h>
#include "xhci.h"

/* XHCI specific command ring IDs */
#define ENABLE_SLOT_CMD           (9U)       /* !< ID for enable slot command TRB */
#define DISABLE_SLOT_CMD          (10U)      /* !< ID for disable slot command TRB */
#define ADDRESS_DEVICE_CMD        (11U)      /* !< ID for address device command TRB */
#define CONFIGURE_ENDPOINT_CMD    (12U)      /* !< ID for configure endpoint command TRB */
#define EVALUATE_CONTEXT_CMD      (13U)      /* !< ID for evaluate context command TRB */

/*
    File: xHCI Commands Driver

    Declares the xHCI Commands driver interface: 9 functions, 2 types and 11
    constants.
*/

/*
    Struct: xcc_event_t

    : structure to store the command completion Event TRB data
*/
typedef struct  __attribute__ ((packed))
{
    uint64_t cmd_trb_ptr;   /* address of command TRB that generated the event. */
    union
    {
        struct cmd_compl_status_field
        {
            uint32_t compl_parameter : 24;   /* command completion parameter */
            uint32_t compl_code : 8;         /* Completion code */
        } cc_status_params __attribute__ ((packed));
        uint32_t status_field;
    };
    union
    {
        struct cmd_compl_ctrl_field
        {
            uint32_t cbit : 1;          /* cycle bit of the TRB */
            uint32_t rzvd : 9;          /* reserved */
            uint32_t trb_id : 6;         /* TRB id for command completion event */
            uint32_t vf_id : 8;         /* Virtual function id. Valid only if VF are enabled */
            uint32_t slot_id : 8;        /* Slot id of the associated command TRB */
        } cc_ctrl_params __attribute__ ((packed));
        uint32_t control_field;
    };
} xcc_event_t;

#define RESET_ENDPOINT_CMD        14U     /* reset endpoint command trb id */
#define STOP_ENDPOINT_CMD         15U     /* stop endpoint command trb id */
#define SET_TR_DEQUEUE_PTR_CMD    16U     /* set tr dequeue pointer command trb id */

#define ENDPOINT_ID_FIELD         15 /* endpoint field position in the trb structure */
#define SP_BIT_FIELD              23 /* SP bit field position in the trb structure */
#define SLOT_ID_FIELD             24 /* slot id field position in the trb structure */


/*
    Enum: xhci_command_status_t

    Status values used by the xHCI Commands driver.
*/
typedef enum
{
    COMMAND_SUCCESS = 1,
    COMMAND_FAILIURE = 0,

}xhci_command_status_t;

/*
    Function: reset_command_ring

    : reset the command ring

    Parameters:

        xhci - reference to command ring structure
*/
void reset_command_ring(xcr_command_ring_t *xcr_ring);

/*
    Function: set_device_address

    : send the USB set address command to device

    Parameters:

        xhci - reference to xhci structure
*/
void set_device_address(struct xhci_data *xhci_ptr);

/*
    Function: enable_slot_command

    : send the USB enable slot command

    Parameters:

        xhci - reference to xhci command ring structure
*/
void enable_slot_command(xcr_command_ring_t *xcr_ring);

/*
    Function: disable_slot_command

    : send the USB disable slot command

    Parameters:

        xhci - reference to xhci command structure
        xhci - slot id to be disabled
*/
void disable_slot_command(xcr_command_ring_t *xcr_ring, uint32_t slotid);

/*
    Function: configure_endpoint

    : send the USB configure endpoint command to device

    Parameters:

        xhci - reference to xhci structure
*/
void configure_endpoint(struct xhci_data *xhci_ptr);

/*
    Function: evaluate_endpoint

    : send the USB evaluate endpoint command

    Parameters:

        xhci - reference to xhci structure
*/
void evaluate_endpoint(struct xhci_data *xhci_ptr);

/*
    Function: update_ep_transfer_ring

    : Update the endpoint transfer ring

    Parameters:

        xhci - reference to xhci structure
        xtr_ring - updated endpoint transfer ring
*/
void update_ep_transfer_ring(xcr_command_ring_t *xcr_ring,
        xhci_trb_t *xtr_ring);

/*
    Function: get_cmdring_enq_ptr

    : find the enqueue pointer location for the ring

    Parameters:

        cmd_ring - command ring reference pointer

    Returns:

        xhci_trb_t* reference to enqueue pointer
*/
xhci_trb_t *get_cmdring_enq_ptr(xcr_command_ring_t *cmd_ring);

#endif /*__XHCI_COMMANDS_H__*/
