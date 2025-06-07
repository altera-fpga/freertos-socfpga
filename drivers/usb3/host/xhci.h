/*
 * SPDX-FileCopyrightText: Copyright (C) 2025 Altera Corporation
 *
 * SPDX-License-Identifier: MIT-0
 *
 * Header file for xHCI implementation
 */

#ifndef _SOCFPGA_XHCI_H_
#define _SOCFPGA_XHCI_H_

#include <stdio.h>
#include "FreeRTOS.h"
#include "xhci_context.h"
#include "xhci_rings.h"
#include "socfpga_usb.h"
#include "socfpga_usb3_reg.h"

#define NO_ERROR  ( 0)

#define XHCI_LOG_LEVEL  (0U)
#define XHCI_LOG(...) XHCI_DEBUG_LOG(XHCI_LOG_LEVEL, __VA_ARGS__)
#define XHCI_DEBUG_LOG(n, ...) \
{\
    if( n > 1U) \
    printf(__VA_ARGS__); \
}

#define LINK_TRB                     (6U)               /* !< Link TRB id */
#define TRB_FIELD                   (10U)             /* !< TRB id field position */
#define SLOTID_FIELD                (24U)          /* !< Slot id field position */
#define XHC_DEV_ADDR_MSK (          0xff)     /* !< device address field mask */
#define XHCI_CR_TRB_LEN              (16)        /* !< xHCI command ring length   */
#define XHCI_CR_CCS_FLAG             (1U)       /* !< xHCI command ring CCS flag */
#define XHCI_EP_TR_RING_ALIGN        (16)  /* !< EP transfer ring alignment requirement */
#define EP_TRB_SEG_LENGTH            (512)      /* !< EP TR segment length */
#define XHCI_EVENT_RING_SEG_LENTH  (512U)    /* !< Event Ring segment length */
#define START_XHCI                   (1U)              /* !< Start the xHCI controller */

typedef enum
{
    ISO_OUT = 1,        /* !< Isochronous OUT EP */
    BULK_OUT,           /* !< Bulk OUT EP */
    INTERRUPT_OUT,      /* !< Interrupt OUT EP */
    CONTROL,            /* !< Control  EP */
    ISO_IN,             /* !< Isochronous IN EP */
    BULK_IN,            /* !< Bulk IN EP */
    INTERRUPT_IN,       /* !< Interrupt IN EP */
}xhci_ep_type_t;

/*
 * @struct : xhci_cap_reg
 * @brief  : structure to store xhci capability register parameters
 */
typedef struct __attribute__ ((packed))
{
    uint8_t caplength;                  /* !< caplength register */
    uint8_t rsvd;                       /* !< reserved */
    uint16_t hciversion;                /* !< hci version */
    union
    {
        struct hcsparams1_reg_
        {
            uint32_t max_dev_slots : 8;     /* !< max device slots supported by xHCI */
            uint32_t max_interrupters : 11; /* !< max interrupters */
            uint32_t rsvd : 5;              /* !< reserved */
            uint32_t max_ports : 8;         /* !< Max ports supported by the xHCI */

        }hcsparams1_params __attribute__ ((packed));
        uint32_t hcsparams1;
    };
    uint32_t hcsparams2;        /* !< xHCI hcsparams2 register  */
    uint32_t hcsparams3;        /* !< xHCI hcsparams3 register  */
    uint32_t hccparams1;        /* !< xHCI hccparams1 register  */
    union
    {
        struct dboff_reg
        {
            uint32_t db_array_offset : 30;  /* !< Doorbell register offset */
            uint32_t rsvd : 2;              /* !< reserved */
        }dboff_params __attribute__ ((packed));
        uint32_t dboff;
    };
    union
    {
        struct rtsoff_reg
        {
            uint32_t rts_offset : 27;       /* !< Runtime register space offset */
            uint32_t rsvd : 5;              /* !< reserved */

        }rtsoff_params __attribute__ ((packed));
        uint32_t rtsoff;
    };
    uint32_t hccparams2;
}xhci_cap_reg_t;

/*
 * @struct : xhci_oper_reg_params
 * @brief  : stores xhci operational base, runtime tbase and doorbell base
 *           address
 */
typedef struct xhci_oper_reg_params
{
    uint64_t xhci_op_base;      /* !< operation base offset */
    uint64_t xhci_runtime_base; /* !< Runtime base offset */
    uint64_t xhci_db_base;      /* !< Doorbell register offset */
}xhci_oper_reg_params_t;

typedef enum
{
    DEV_ADDRESS_FIELD = 32,
    SLOT_STATE_FIELD = 59

}xhci_op_context_params_t;

/*
 * @struct : ep_priv
 * @brief  : stores endpoint specific parameters
 */
struct ep_priv
{
    xhci_trb_t *tr_ptr;                 /* !< Transfer Ring pointer */
    xhci_trb_t *ep_tr_enq_ptr;          /* !< TR enque pointer */
    xhci_trb_t *ep_tr_deq_ptr;          /* !< TR dequeue pointer */
    usb_endpoint_descriptor_t *ep_desc; /* !< endpoint descriptor */
    uint8_t pcs_flag;                   /* !< pcs flag for EP TR */  
    uint8_t ep_addr;                    /* !< endpoint address */
    uint8_t ep_index;                   /* !< endpoint index */
    xhci_ep_type_t ep_type;             /* !< endpoint type */
}__attribute__ ((packed));

/*
 * struct to store MSC specific endpoint data
 */
typedef struct
{
    struct ep_priv *ep_in;      /* !< Endpoint IN specific parameters */
    struct ep_priv *ep_out;     /* !< Endpoint OUT specific parameters */

}msc_eps_params_t;

struct xhci_device_data
{
    uint8_t slot_id;        /* !< slot id of the device */
    uint8_t dev_addr;       /* !< address of the device */
    uint8_t dev_speed;      /* !< speed of the device */
    uint8_t rh_port;        /* !< RH port of the device */
};

/*
 * @brief xhci controller structure
 */
struct xhci_data
{
    struct xhci_device_data *dev_data;  /* !< device specific data */
    struct ep_priv ep0;                 /* !< EP 0 params */
    msc_eps_params_t msc_eps;           /* !< MSC specific endpoint params */

    xcr_command_ring_t *xcr_ring;       /* !< xHCI command ring */
    xer_event_ring_t xer_ring;          /* !< xHCI event ring */

    xhci_cap_reg_t *xhc_cap_ptr;        /* !< xHCI capability register pointer */
    xhci_oper_reg_params_t *op_regs;    /* !< xHCI operational register */

    xhci_ip_device_context_t *ip_ctx;   /* !< xHCI input context */
    xhci_op_device_context_t *op_ctx;   /* !< xHCI output context */

    struct xhci_device_context_array *dcbaa; /* !< dcbaa pointer array */
    struct usb_descriptors *usb_desc;   /* !< usb descriptor reference */
};

/*
 * @func  : alloc_xhci_hcd
 * @brief : allocate memory for xhci hcd structure members
 * @return
 *  struct xhci_data* reference to hcd structure
 */
struct xhci_data* alloc_xhci_hcd(void);

/*
 * @func  : xhci_reset
 * @brief : reset the xhci host controller
 * @return 
 *  NO_ERROR, on successful reset operation
 *  errno, incase of failure
 */
int xhci_reset(void);

/*
 * @func  : start_xhci_controller
 * @brief : start the xhci controller
 */
void start_xhci_controller(void);

/*
 * @func  : alloc_xhci_contexts
 * @brief : allocate xhci context memories
 * @param[in]  xhci_ptr -  reference of xhci hcd controller
 * @return
 *  NO_ERROR, on successful allocation
 *  errno, incase of failure
 */
int alloc_xhci_contexts(struct xhci_data *xhci_ptr);

/*
 * @func  : update_dcbaa_entry
 * @brief : update the ouput device context entry for the corresponding device
 * slot id
 * @param[in] xhci_ptr : reference to xhci hcd structure
 */
void update_dcbaa_entry(struct xhci_data *xhci_ptr);

/*
 * @func  : deallocate_xhci_context
 * @brief : deallocate xhci context memories
 * @param[in]  xhci_ptr -  reference of xhci hcd controller
 */
void deallocate_xhci_context(struct xhci_data *xhci_ptr);

/*
 * @func  : alloc_input_device_context
 * @brief : allocate xhci input context
 * @param[in]  xhci_ptr -  reference of xhci hcd controller
 * @return
 *  NO_ERROR, on successful allocation
 *  errno, incase of failure
 */
int alloc_input_device_context(struct xhci_data *xhci_ptr);

/*
 * @func  : alloc_output_device_context
 * @brief : allocate xhci output device context
 * @param[in]  xhci_ptr -  reference of xhci hcd controller
 * @return 
 *  NO_ERROR, on successful allocation
 *  errno, incase of failure
 */
int alloc_output_device_context(struct xhci_data *xhci);

/*
 * @func  : init_input_device_context
 * @brief : initialize input device context data structure
 * @param[in]  xhci -  reference of xhci hcd controller
 *  NO_ERROR, on successful allocation
 *  errno, incase of failure
 */
int init_input_device_context(struct xhci_data *xhci_ptr);

/*
 * @func  : alloc_dcbaa
 * @brief : allocate xhci device context base address array and store the base
 *         address array into DCBAA register
 * @param[in]  xhci_ptr -  reference of xhci hcd controller
 *  NO_ERROR, on successful allocation
 *  errno, incase of failure
 */
int alloc_dcbaa(struct xhci_data *xhci_ptr);

/*
 * @func  : update_xhc_slot_context
 * @brief : update xhci slot context data structure
 * @param[in]  xhci_ptr -  reference of xhci hcd controller
 */
void update_xhc_slot_context(struct xhci_data *xhci_ptr);

/*
 * @func  : init_xhc_endpoint_context
 * @brief : initialize endpoint context data structure
 * @param[in]  xhci -  reference of xhci hcd controller
 * @param[in]  ep_type -  endpoint type  (bulk, control etc)
 * @return
 *  RET_SUCCESS, on successful allocation
 *  RET_FAIL, incase of failure
 */
void init_xhc_endpoint_context(struct xhci_data *xhci, xhci_ep_type_t ep_type);

/*
 * @func  : get_xhc_device_address
 * @brief : return the device address from the output context data structure
 * @param[in]  xhci -  reference of xhci hcd controller
 * @return device address
 */
uint8_t get_xhc_device_address(struct xhci_data *xhci_ptr);

/*XHCI capability register APIs*/

/*
 * @func  : get_xhci_op_registers
 * @brief : function to return the operational register parameters of xhci
 *        controller
 * @return operational register parameters
 */
xhci_oper_reg_params_t get_xhci_op_registers(void);

/*
 * @func  : get_xhc_cap_params
 * @brief : funtion to get the xhci capability paramaeters controller
 * @param[in]  xhci -  reference of xhci hcd controller
 * @return 
 *  NO_ERROR, if operaiton is successful
 *  errno, on failure
 */
int get_xhc_cap_params(struct xhci_data *xhci);

/*
 * @func  : configure_xhci_params
 * @brief : configure the CONFIG register with device slot
 * @param[in]  ptr -  reference to xhci strucute
 *  0 fail, eveng ring init fails
 *  1 success
 */
int configure_xhci_params(struct xhci_data *xhci);

/*
 * @func  : init_xhc_event_ring
 * @brief : function to check if the pointer address passed is byte aligned
 * @param[in]  ptr -  reference to xhci strucute
 * @return
 *  NO_ERROR, on successful allocation
 *  errno, incase of failure
 */
int init_xhc_event_ring(struct xhci_data *xhci_ptr);

/*
 * @func : init_xhci_context_params
 * @param[in] xhci_ptr reference xhci data structure
 *  NO_ERROR, on successful allocation
 *  errno, incase of failure
 */
int init_xhci_context_params(struct xhci_data *xhci_ptr);

/*
 * @func  : wait_for_controller_ready
 * @brief : function to wait until the controller is ready to accept register write/read
 * @return
 *  NO_ERROR, if controller becomes ready before timeout occurs
 *  errno, incase the controller is not ready
 */
int wait_for_controller_ready(void);

/* Generic xhci APIS */

/*
 * @func : alloc_xhci_usb_desc
 * @brief : alloc memory for storing usb descriptors
 * @param[in] xhci_ptr reference to xhci hcd structure
 * @return 
 *  NO_ERROR, on successful allocation
 *  errno, incase of failure
 */
int alloc_xhci_usb_desc(struct xhci_data *xhci_ptr);

/*
 * @func : deallocate_usb_desc
 * @brief : deallocate memory for storing usb descriptors
 */
void deallocate_usb_desc(struct xhci_data *xhci_ptr);

/*
 * @func  : is_ptr_mem_aligned
 * @brief : function to check if the pointer address passed is byte aligned
 * @param[in]  ptr -  pointer to be checked byte aligned or not
 * @param[in]  byte - pointer byte alignement size
 * @return
 *  NO_ERROR, on successful allocation
 *  errno, incase of failure
 */
int is_ptr_mem_aligned(uint64_t ptr, uint32_t byte);

/*
 * @func  : xhci_parse_endpoint_descriptor
 * @brief : parse the endpoint descriptors
 * @param[in] xhci_ptr reference to xhci hcd structure
 * @param[in] desc endpoint descriptor
 * @return
 *  NO_ERROR, on successful allocation
 *  errno, incase of failure
 */

int xhci_parse_endpoint_descriptor(struct xhci_data *xhci_ptr, usb_endpoint_descriptor_t *desc);
/*
 * @func  : display_usb_descriptor
 * @brief : display complete usb descriptors
 * @param[in] xhci_ptr reference xhci data structure
 */
void display_usb_descriptor(struct xhci_data *xhci_ptr);

#endif  /* _SOCFPGA_XHCI_H_ */
