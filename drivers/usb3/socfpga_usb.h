/*
 * SPDX-FileCopyrightText: Copyright (C) 2025 Altera Corporation
 *
 * SPDX-License-Identifier: MIT-0
 *
 * Header file for USB HAL driver
 */

#ifndef __SOCFPGA_USB_H__
#define __SOCFPGA_USB_H__

/**
 * @file socfpga_usb.h
 * @brief USB HAL driver header file
 */

#include <stdio.h>

/**
 * @defgroup usb USB
 * @ingroup drivers
 * @{
 */

/**
 * @defgroup usb_fns Functions
 * @ingroup usb
 * USB HAL APIs
 */

/**
 * @defgroup usb_structs Structures
 * @ingroup usb
 * USB Specific Structures
 */

/**
 * @addtogroup usb_structs
 * @{
 */
/**
 * @brief  Structure to hold usb3 device descriptor
 * @struct usb_device_descriptor
 */
typedef struct __attribute__ ((packed))
{
    uint8_t bLength;
    uint8_t bDescriptorType;
    uint16_t bcdUSB;
    uint8_t bDeviceClass;
    uint8_t bDeviceSubClass;
    uint8_t bDeviceProtocol;
    uint8_t bMaxPacketSize0;
    uint16_t idVendor;
    uint16_t idProduct;
    uint16_t bcdDevice;
    uint8_t iManufacturer;
    uint8_t iProduct;
    uint8_t iSerialNumber;
    uint8_t bNumConfigurations;
} usb_device_descriptor_t;

/*
 * @struct: usb_configuration_descriptor
 * @brief : structure to hold usb3 configuration descriptor
 */
typedef struct __attribute__ ((packed))
{
    uint8_t bLength;
    uint8_t bDescriptorType;
    uint16_t wTotalLength;
    uint8_t bNumInterfaces;
    uint8_t bConfigurationValue;
    uint8_t iConfiguration;
    uint8_t bmAttributes;
    uint8_t bMaxPower;
} usb_conf_descriptor_t;

/*
 * @struct: usb_string_descriptor
 * @brief : structure to hold usb3 string descriptor
 */
typedef struct __attribute__ ((packed))
{
  uint8_t  bLength         ; 
  uint8_t  bDescriptorType ;
  uint8_t unicode_string[];
} usb_string_descriptor_t;

/*
 * @struct: usb_interface_descriptor
 * @brief : structure to hold usb3 interface descriptor
 */
typedef struct __attribute__ ((packed))
{
    uint8_t bLength;
    uint8_t bDescriptorType;
    uint8_t bInterfaceNumber;
    uint8_t bAlternateSetting;
    uint8_t bNumEndpoints;
    uint8_t bInterfaceClass;
    uint8_t bInterfaceSubClass;
    uint8_t bInterfaceProtocol;
    uint8_t iInterface;
} usb_interface_descriptor_t;

/*
 * @struct: usb_endpoint_descriptor
 * @brief : structure to hold endpoint descriptor
 */
typedef struct __attribute__ ((packed))
{
    uint8_t bLength;
    uint8_t bDescriptorType;
    uint8_t bEndpointAddress;
    uint8_t bmAttributes;
    uint16_t wMaxPacketSize;
    uint8_t bInterval;

} usb_endpoint_descriptor_t;

typedef struct __attribute__ ((packed))
{
    uint8_t bLength; 
    uint8_t bDescriptorType;
    uint16_t wTotalLength;
    uint8_t bNumDeviceCaps;
} usb_bos_descriptor_t;


/*
 * @struct : usb_20_ext_desc_t
 * @brief  : contains the USB 2.0 Extension Descriptor
 */
typedef struct __attribute__ ((packed))
{
    uint8_t bLength; 
    uint8_t bDescriptorType;
    uint8_t bDevCapabilityType;
    union
    {
        uint32_t bmAttributes;
        struct bos_usb20_attr
        {
            uint32_t rsvd0 : 1;
            uint32_t lpm   : 1;
            uint32_t rsvd1 : 30;
        }bos_battr;
    };
}usb_20_ext_desc_t;

/*
 * @struct : usb_ss_dev_cap_desc_t
 * @brief  : contains the USB SS Device Capability Descriptor
 */
typedef struct __attribute__ ((packed))
{
    uint8_t bLength; 
    uint8_t bDescriptorType;
    uint8_t bDevCapabilityType;
    union
    {
        uint8_t bmAttributes;
        struct bos_ss_attr
        {
            uint8_t rsvd0 : 1;
            uint8_t lpm   : 1;
            uint8_t rsvd1 : 6;
        }bos_battr;
    };
    uint16_t wSpeedsSupported;
    uint8_t bFunctionalitySupport;
    uint8_t bU1DevExitLat;
    uint8_t wU2DevExitLat;
}usb_ss_dev_cap_desc_t;

struct usb_bos_root_desc
{
    usb_bos_descriptor_t bos_desc;
    usb_20_ext_desc_t usb20_ext_desc;
    usb_ss_dev_cap_desc_t ss_dev_desc;
};

/*
 * @struct: usb_control_request
 * @brief : structure to store usb device requests
 */
typedef struct __attribute__ ((packed))
{
    uint8_t bmRequestType;
    uint8_t bRequest;
    uint16_t wValue;
    uint16_t wIndex;
    uint16_t wLength;
} usb_control_request_t;

/*
 * @struct : usb3_ss_ep_comp_desc_t
 * @brief  : structure to hold SS endpoint companion descriptor
 */
typedef struct __attribute__ ((packed))
{
    uint8_t bLength;
    uint8_t bDescriptorType;
    uint8_t bMaxBurst;
    union
    {
        struct
        {
            uint8_t max_streams : 5;
            uint8_t rsvd : 3;
        }bulk_ep_attributes;
        uint8_t bmAttributes;
    };
    uint8_t wBytesPerInterval;
}usb3_ss_ep_comp_desc_t;

struct usb_descriptors
{
    usb_device_descriptor_t *dev_desc;
    usb_conf_descriptor_t *dev_conf_desc;
    usb_interface_descriptor_t *dev_intf_desc;
    usb_endpoint_descriptor_t *dev_ep_desc;
    struct usb_bos_root_desc  *root_bos_desc;
    usb3_ss_ep_comp_desc_t *ss_ep_comp_desc;
    char m_string[128];
    char p_string[128];
    char s_string[128];

} __attribute__ ((packed));

/*
 * @brief     : parse usb device descriptor
 * @param[in] : xhci_dev_desc xhci device descriptor pointer
 * @param[in] : desc tinyusb stack device descriptor pointer
 */
void xhci_parse_device_descriptor(usb_device_descriptor_t *xhci_dev_desc,
        usb_device_descriptor_t *desc);

/*
 * @brief     : parse usb configuration descriptor
 * @param[in] : xhci_dev_desc xhci configuration  descriptor pointer
 * @param[in] : desc tinyusb stack configuration descriptor pointer
 */
void xhci_parse_conf_descriptor(usb_conf_descriptor_t *xhci_conf_desc,
        usb_conf_descriptor_t *desc);

/*
 * @brief     : parse usb3 SS EP companion descriptor
 * @param[in] : ss_ep_desc reference to ss ep comp. descriptor
 * @param[in] : desc     - reference to the descriptor to be parsed
 */
void xhci_parse_ss_endpoint_comp_desc(usb3_ss_ep_comp_desc_t *ss_ep_desc, usb3_ss_ep_comp_desc_t *desc);

/*
 * @brief     : parse usb interface descriptor
 * @param[in] : xhci_dev_desc xhci interface descriptor pointer
 * @param[in] : desc tinyusb stack interface descriptor pointer
 */
void xhci_parse_interface_descriptor(usb_interface_descriptor_t *xhci_intf_desc,
        usb_interface_descriptor_t *desc);

/*
 * @func  : xhci_parse_bos_descriptor
 * @brief : parse usb root bos descriptor
 * @param[in] root_bos_desc referance to xhci bos descriptor structure
 * @param[in] desc    referance to bos descriptor pointer from the stack
 */
void xhci_parse_bos_descriptor(struct usb_bos_root_desc  *root_bos_desc, uint8_t *desc);

#endif /* __SOCFPGA_USB_H__ */
