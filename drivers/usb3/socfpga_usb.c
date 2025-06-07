/*
 * SPDX-FileCopyrightText: Copyright (C) 2025 Altera Corporation
 *
 * SPDX-License-Identifier: MIT-0
 *
 * HAL driver implementation for USB
 */
#include <stdlib.h>
#include <strings.h>
#include <errno.h>
#include "FreeRTOS.h"
#include "xhci.h"
#include "socfpga_usb.h"
#include "tusb_types.h"
#include "osal_log.h"

int alloc_xhci_usb_desc(struct xhci_data *xhci_ptr)
{
    xhci_ptr->usb_desc =
        (struct usb_descriptors *)pvPortMalloc(
                sizeof(struct usb_descriptors));
    if (xhci_ptr->usb_desc == NULL)
    {
        ERROR("Cannot allocate memory!!!");
        return ENOMEM;
    }

    xhci_ptr->usb_desc->dev_desc = (usb_device_descriptor_t *)pvPortMalloc(
            sizeof(usb_device_descriptor_t));
    if (xhci_ptr->usb_desc->dev_desc == NULL)
    {
        ERROR("Cannot allocate memory!!!");
        return ENOMEM;
    }

    xhci_ptr->usb_desc->dev_conf_desc = (usb_conf_descriptor_t *)pvPortMalloc(
            sizeof(usb_conf_descriptor_t));
    if (xhci_ptr->usb_desc->dev_conf_desc == NULL)
    {
        ERROR("Cannot allocate memory!!!");
        return ENOMEM;
    }

    xhci_ptr->usb_desc->dev_intf_desc =
        (usb_interface_descriptor_t *)pvPortMalloc(sizeof(
                    usb_interface_descriptor_t));
    if (xhci_ptr->usb_desc->dev_intf_desc == NULL)
    {
        ERROR("Cannot allocate memory!!!");
        return ENOMEM;
    }

    xhci_ptr->usb_desc->root_bos_desc = (struct usb_bos_root_desc  *)pvPortMalloc(sizeof(
                struct usb_bos_root_desc));
    if (xhci_ptr->usb_desc->root_bos_desc == NULL)
    {
        ERROR("Cannot allocate memory!!!");
        return ENOMEM;
    }

    xhci_ptr->usb_desc->ss_ep_comp_desc = (usb3_ss_ep_comp_desc_t *)pvPortMalloc(sizeof(
                usb3_ss_ep_comp_desc_t));
    if (xhci_ptr->usb_desc->ss_ep_comp_desc == NULL)
    {
        ERROR("Cannot allocate memory!!!");
        return ENOMEM;
    }

    bzero(xhci_ptr->usb_desc->dev_desc, sizeof(usb_device_descriptor_t));
    bzero(xhci_ptr->usb_desc->dev_conf_desc, sizeof(usb_conf_descriptor_t));
    bzero(xhci_ptr->usb_desc->dev_intf_desc,
            sizeof(usb_interface_descriptor_t));
    bzero(xhci_ptr->usb_desc->root_bos_desc, sizeof(struct usb_bos_root_desc));
    bzero(xhci_ptr->usb_desc->ss_ep_comp_desc, sizeof(usb3_ss_ep_comp_desc_t));

    return NO_ERROR;
}

void deallocate_usb_desc(struct xhci_data *xhci_ptr)
{
    vPortFree(xhci_ptr->usb_desc->dev_desc);
    vPortFree(xhci_ptr->usb_desc->dev_conf_desc);
    vPortFree(xhci_ptr->usb_desc->dev_intf_desc);
    vPortFree(xhci_ptr->usb_desc->root_bos_desc);
    vPortFree(xhci_ptr->usb_desc->ss_ep_comp_desc);
    vPortFree(xhci_ptr->usb_desc);
}

void xhci_parse_interface_descriptor(usb_interface_descriptor_t *xhci_intf_desc,
        usb_interface_descriptor_t *desc)
{

    xhci_intf_desc->bLength = desc->bLength;
    xhci_intf_desc->bDescriptorType = desc->bDescriptorType;
    xhci_intf_desc->bInterfaceNumber = desc->bInterfaceNumber;
    xhci_intf_desc->bAlternateSetting = desc->bAlternateSetting;
    xhci_intf_desc->bNumEndpoints = desc->bNumEndpoints;
    xhci_intf_desc->bInterfaceClass = desc->bInterfaceClass;
    xhci_intf_desc->bInterfaceSubClass = desc->bInterfaceSubClass;
    xhci_intf_desc->bInterfaceProtocol = desc->bInterfaceProtocol;
    xhci_intf_desc->iInterface = desc->iInterface;
}

void xhci_parse_device_descriptor(usb_device_descriptor_t *xhci_dev_desc,
        usb_device_descriptor_t *desc)
{
    xhci_dev_desc->bLength = desc->bLength;
    xhci_dev_desc->bDescriptorType = desc->bDescriptorType;
    xhci_dev_desc->bcdUSB = desc->bcdUSB;
    xhci_dev_desc->bDeviceClass = desc->bDeviceClass;
    xhci_dev_desc->bDeviceSubClass = desc->bDeviceSubClass;
    xhci_dev_desc->bDeviceProtocol = desc->bDeviceProtocol;
    xhci_dev_desc->bMaxPacketSize0 = desc->bMaxPacketSize0;
    xhci_dev_desc->idVendor = desc->idVendor;
    xhci_dev_desc->idProduct = desc->idProduct;
    xhci_dev_desc->bcdDevice = desc->bcdDevice;
    xhci_dev_desc->iManufacturer = desc->iManufacturer;
    xhci_dev_desc->iProduct = desc->iProduct;
    xhci_dev_desc->iSerialNumber = desc->iSerialNumber;
    xhci_dev_desc->bNumConfigurations = desc->bNumConfigurations;
}

void xhci_parse_conf_descriptor(usb_conf_descriptor_t *xhci_conf_desc,
        usb_conf_descriptor_t *desc)
{
    xhci_conf_desc->bLength = desc->bLength;
    xhci_conf_desc->bDescriptorType = desc->bDescriptorType;
    xhci_conf_desc->wTotalLength = desc->wTotalLength;
    xhci_conf_desc->bNumInterfaces = desc->bNumInterfaces;
    xhci_conf_desc->bConfigurationValue = desc->bConfigurationValue;
    xhci_conf_desc->iConfiguration = desc->iConfiguration;
    xhci_conf_desc->bmAttributes = desc->bmAttributes;
    xhci_conf_desc->bMaxPower = desc->bMaxPower;
}

void xhci_parse_ss_endpoint_comp_desc(usb3_ss_ep_comp_desc_t *ss_ep_desc, usb3_ss_ep_comp_desc_t *desc)
{
    ss_ep_desc->bLength = desc->bLength;
    ss_ep_desc->bDescriptorType = desc->bDescriptorType;
    ss_ep_desc->bMaxBurst = desc->bMaxBurst;
    ss_ep_desc->bmAttributes = desc->bmAttributes;
    ss_ep_desc->wBytesPerInterval = desc->wBytesPerInterval;
}

int xhci_parse_endpoint_descriptor(struct xhci_data *xhci_ptr,
        usb_endpoint_descriptor_t *desc)
{
    const int ep_dir = tu_edpt_dir(desc->bEndpointAddress);
    uint8_t max_streams = 0;
    if (ep_dir == 1)
    {
        xhci_ptr->msc_eps.ep_in =
            (struct ep_priv *)pvPortMalloc(sizeof(struct ep_priv));
        if (xhci_ptr->msc_eps.ep_in == NULL)
        {
            ERROR("Cannot allocate memory!!!");
            return ENOMEM;
        }
        xhci_ptr->msc_eps.ep_in->ep_desc =
            (usb_endpoint_descriptor_t *)pvPortMalloc(sizeof(
                        usb_endpoint_descriptor_t));
        if (xhci_ptr->msc_eps.ep_in->ep_desc == NULL)
        {
            ERROR("Cannot allocate memory!!!");
            return ENOMEM;
        }

        xhci_ptr->msc_eps.ep_in->ep_desc->bLength = desc->bLength;
        xhci_ptr->msc_eps.ep_in->ep_desc->bDescriptorType =
            desc->bDescriptorType;
        xhci_ptr->msc_eps.ep_in->ep_desc->bEndpointAddress =
            desc->bEndpointAddress;
        xhci_ptr->msc_eps.ep_in->ep_desc->bmAttributes = desc->bmAttributes;
        xhci_ptr->msc_eps.ep_in->ep_desc->wMaxPacketSize = desc->wMaxPacketSize;
        xhci_ptr->msc_eps.ep_in->ep_desc->bInterval = desc->bInterval;

        xhci_ptr->msc_eps.ep_in->ep_type = BULK_IN;
        xhci_ptr->msc_eps.ep_in->pcs_flag = 1U;
        xhci_ptr->msc_eps.ep_in->ep_index = 4U;
        xhci_ptr->msc_eps.ep_in->ep_addr = 0x82U;

        //max_streams = xhci_ptr->msc_eps.ep_out->ep_desc->companion.max_streams;
        if( max_streams > 0 )
        {
            /* Stream support not enabled */
        }
        else
        {
            xhci_trb_t *enq_ptr;
            enq_ptr =
                allocate_ring_segment(XHCI_EP_TR_RING_ALIGN, EP_TRB_SEG_LENGTH);
            if (enq_ptr == NULL)
            {
                ERROR("Canot allocate memory!!!");
                return ENOMEM;
            }

            xhci_ptr->msc_eps.ep_in->tr_ptr = enq_ptr;
            xhci_ptr->msc_eps.ep_in->ep_tr_enq_ptr = enq_ptr;
            xhci_ptr->msc_eps.ep_in->ep_tr_deq_ptr = enq_ptr;
        }
    }

    if (ep_dir == 0)
    {
        xhci_trb_t *enq_ptr;
        uint8_t max_streams = 0; 

        xhci_ptr->msc_eps.ep_out =
            (struct ep_priv *)pvPortMalloc(sizeof(struct ep_priv));
        if (xhci_ptr->msc_eps.ep_out == NULL)
        {
            ERROR("Cannot allocate memory!!!");
            return ENOMEM;
        }
        xhci_ptr->msc_eps.ep_out->ep_desc =
            (usb_endpoint_descriptor_t *)pvPortMalloc(sizeof(
                        usb_endpoint_descriptor_t));
        if (xhci_ptr->msc_eps.ep_out->ep_desc == NULL)
        {
            ERROR("Cannot allocate memory!!!");
            return ENOMEM;
        }

        xhci_ptr->msc_eps.ep_out->ep_desc->bLength = desc->bLength;
        xhci_ptr->msc_eps.ep_out->ep_desc->bDescriptorType =
            desc->bDescriptorType;
        xhci_ptr->msc_eps.ep_out->ep_desc->bEndpointAddress =
            desc->bEndpointAddress;
        xhci_ptr->msc_eps.ep_out->ep_desc->bmAttributes = desc->bmAttributes;
        xhci_ptr->msc_eps.ep_out->ep_desc->wMaxPacketSize =
            desc->wMaxPacketSize;
        xhci_ptr->msc_eps.ep_out->ep_desc->bInterval = desc->bInterval;

        xhci_ptr->msc_eps.ep_out->ep_type = BULK_OUT;
        xhci_ptr->msc_eps.ep_out->pcs_flag = 1U;
        xhci_ptr->msc_eps.ep_out->ep_index = 1U;
        xhci_ptr->msc_eps.ep_out->ep_addr = 0x01U;

        //max_streams = xhci_ptr->msc_eps.ep_out->ep_desc->companion.max_streams;

        if( max_streams > 0 )
        {
            /* Stream support not enabled */
        }
        else
        {
            enq_ptr =
                allocate_ring_segment(XHCI_EP_TR_RING_ALIGN, EP_TRB_SEG_LENGTH);
            if (enq_ptr == NULL)
            {
                ERROR("Cannot allocate memory!!!");
                return ENOMEM;
            }
            xhci_ptr->msc_eps.ep_out->tr_ptr = enq_ptr;
            xhci_ptr->msc_eps.ep_out->ep_tr_enq_ptr = enq_ptr;
            xhci_ptr->msc_eps.ep_out->ep_tr_deq_ptr = enq_ptr;
        }
    }
    return NO_ERROR;
}

void xhci_parse_bos_descriptor(struct usb_bos_root_desc  *root_bos_desc, uint8_t *desc)
{
    usb_bos_descriptor_t *bos_desc = (usb_bos_descriptor_t *)desc;

    const uint16_t total_len = bos_desc->wTotalLength;
    uint8_t const* desc_end = ((uint8_t const*) desc) + total_len;
    uint8_t *desc_next  = desc + desc[0];

    root_bos_desc->bos_desc.bLength = bos_desc->bLength;
    root_bos_desc->bos_desc.bDescriptorType = bos_desc->bDescriptorType;
    root_bos_desc->bos_desc.wTotalLength = bos_desc->wTotalLength;
    root_bos_desc->bos_desc.bNumDeviceCaps = bos_desc->bNumDeviceCaps;

    while(desc_next < desc_end)
    {
        uint8_t desc_len;
        uint8_t bdesc_type;
        uint8_t *next_dev_cap = desc_next;

        desc_len = desc_next[DESC_OFFSET_LEN];

        bdesc_type = desc_next[2];

        switch( bdesc_type )
        {
            //USB2.0 extension capability descriptor
            case 2 :
                {
                    usb_20_ext_desc_t *cap_desc;
                    cap_desc = (usb_20_ext_desc_t *)next_dev_cap;

                    root_bos_desc->usb20_ext_desc.bLength = cap_desc->bLength;
                    root_bos_desc->usb20_ext_desc.bDescriptorType = cap_desc->bDescriptorType;
                    root_bos_desc->usb20_ext_desc.bDevCapabilityType = cap_desc->bDevCapabilityType;
                    root_bos_desc->usb20_ext_desc.bmAttributes = cap_desc->bmAttributes;

                    break;
                }
                //USB SS Device Capability descriptor
            case 3 :
                {
                    usb_ss_dev_cap_desc_t *ss_desc;
                    ss_desc = (usb_ss_dev_cap_desc_t *)next_dev_cap;

                    root_bos_desc->ss_dev_desc.bLength = ss_desc->bLength;
                    root_bos_desc->ss_dev_desc.bDescriptorType = ss_desc->bDescriptorType;
                    root_bos_desc->ss_dev_desc.bDevCapabilityType = ss_desc->bDevCapabilityType;
                    root_bos_desc->ss_dev_desc.wSpeedsSupported = ss_desc->wSpeedsSupported;
                    root_bos_desc->ss_dev_desc.bFunctionalitySupport = ss_desc->bFunctionalitySupport;
                    root_bos_desc->ss_dev_desc.bU1DevExitLat = ss_desc->bU1DevExitLat;
                    root_bos_desc->ss_dev_desc.wU2DevExitLat = ss_desc->wU2DevExitLat;

                    break;
                }
            default: break;
        }
        desc_next += desc_len;
    }
}

void display_usb_descriptor(struct xhci_data *xhci_ptr)
{
    printf(
            "\r\n ******************* USB DEVICE DESCRIPTOR *************************** \r\n ");

    printf("\r\n DEVICE DESCRIPTORS \r\n");

    printf(" bLength : %x \r\n", xhci_ptr->usb_desc->dev_desc->bLength);
    printf(" bDescriptorType : %x \r\n",
            xhci_ptr->usb_desc->dev_desc->bDescriptorType);
    printf(" bcdUSB : %x \r\n", xhci_ptr->usb_desc->dev_desc->bcdUSB);
    printf(" bDeviceClass : %x \r\n",
            xhci_ptr->usb_desc->dev_desc->bDeviceClass);
    printf(" bDeviceSubClass : %x \r\n",
            xhci_ptr->usb_desc->dev_desc->bDeviceSubClass);
    printf(" bDeviceProtocol : %x \r\n",
            xhci_ptr->usb_desc->dev_desc->bDeviceProtocol);
    printf(" bMaxPacketSize0 : %x \r\n",
            xhci_ptr->usb_desc->dev_desc->bMaxPacketSize0);
    printf(" idVendor : %x \r\n", xhci_ptr->usb_desc->dev_desc->idVendor);
    printf(" idProduct : %x \r\n", xhci_ptr->usb_desc->dev_desc->idProduct);
    printf(" bcdDevice: %x \r\n", xhci_ptr->usb_desc->dev_desc->bcdDevice);
    printf(" iManufacturer : %x  -- %s\r\n",
            xhci_ptr->usb_desc->dev_desc->iManufacturer, xhci_ptr->usb_desc->m_string);
    printf(" iProduct : %x -- %s \r\n", xhci_ptr->usb_desc->dev_desc->iProduct, xhci_ptr->usb_desc->p_string);
    printf(" iSerialNumber : %x -- %s \r\n",
            xhci_ptr->usb_desc->dev_desc->iSerialNumber, xhci_ptr->usb_desc->s_string);
    printf(" bNumConfigurations : %x \r\n",
            xhci_ptr->usb_desc->dev_desc->bNumConfigurations);

    printf("\r\n CONFIGURATION DESCRIPTOR \r\n");

    printf(" bLength : %x \r\n", xhci_ptr->usb_desc->dev_conf_desc->bLength);
    printf(" bDescriptorType : %x \r\n",
            xhci_ptr->usb_desc->dev_conf_desc->bDescriptorType);
    printf(" wTotalLength : %x \r\n",
            xhci_ptr->usb_desc->dev_conf_desc->wTotalLength);
    printf(" bNumInterfaces : %x \r\n",
            xhci_ptr->usb_desc->dev_conf_desc->bNumInterfaces);
    printf(" bConfigurationValue : %x \r\n",
            xhci_ptr->usb_desc->dev_conf_desc->bConfigurationValue);
    printf(" iConfiguration : %x \r\n",
            xhci_ptr->usb_desc->dev_conf_desc->iConfiguration);
    printf(" bmAttributes: %x \r\n",
            xhci_ptr->usb_desc->dev_conf_desc->bmAttributes);
    printf(" bMaxPower : %x \r\n",
            xhci_ptr->usb_desc->dev_conf_desc->bMaxPower);

    usb_endpoint_descriptor_t *ep_desc = xhci_ptr->msc_eps.ep_out->ep_desc;

    printf("\r\n \tINTERFACE DESCRIPTORS \r\n");
    printf(" \tbLength : %x \r\n",
            xhci_ptr->usb_desc->dev_intf_desc->bLength);
    printf(" \tbDescriptorType : %x \r\n",
            xhci_ptr->usb_desc->dev_intf_desc->bDescriptorType);
    printf(" \tbInterfaceNumber : %x \r\n",
            xhci_ptr->usb_desc->dev_intf_desc->bInterfaceNumber);
    printf(" \tbAlternateSetting : %x \r\n",
            xhci_ptr->usb_desc->dev_intf_desc->bAlternateSetting);
    printf(" \tbNumEndpoints : %x \r\n",
            xhci_ptr->usb_desc->dev_intf_desc->bNumEndpoints);
    printf(" \tbInterfaceClass : %x \r\n",
            xhci_ptr->usb_desc->dev_intf_desc->bInterfaceClass);
    printf(" \tbInterfaceSubClass : %x \r\n",
            xhci_ptr->usb_desc->dev_intf_desc->bInterfaceSubClass);
    printf(" \tbInterfaceProtocol : %x \r\n",
            xhci_ptr->usb_desc->dev_intf_desc->bInterfaceProtocol);
    printf(" \tiInterface : %x \r\n",
            xhci_ptr->usb_desc->dev_intf_desc->iInterface);

    printf("\r\n \t\tENDPOINT DESCRIPTORS \r\n");
    printf(" \t\tLength : %x \r\n", ep_desc->bLength);
    printf(" \t\tbDescriptorType: %x \r\n", ep_desc->bDescriptorType);
    printf(" \t\tbEndpointAddress : %x \r\n", ep_desc->bEndpointAddress);
    printf(" \t\tbmAttributes: %x \r\n", ep_desc->bmAttributes);
    printf(" \t\twMaxPacketSize: %x \r\n", ep_desc->wMaxPacketSize);
    printf(" \t\tbInterval: %x \r\n", ep_desc->bInterval);

    ep_desc = xhci_ptr->msc_eps.ep_in->ep_desc;

    printf(" \r\n\t\tLength : %x \r\n", ep_desc->bLength);
    printf(" \t\tbDescriptorType: %x \r\n", ep_desc->bDescriptorType);
    printf(" \t\tbEndpointAddress : %x \r\n", ep_desc->bEndpointAddress);
    printf(" \t\tbmAttributes: %x \r\n", ep_desc->bmAttributes);
    printf(" \t\twMaxPacketSize: %x \r\n", ep_desc->wMaxPacketSize);
    printf(" \t\tbInterval: %x \r\n", ep_desc->bInterval);

    struct usb_bos_root_desc *bos_desc = xhci_ptr->usb_desc->root_bos_desc;

    printf("\r\nBOS DESCRIPTORS \r\n");
    printf("\tLength : %x \r\n", bos_desc->bos_desc.bLength);
    printf("\tbDescriptorType: %x \r\n", bos_desc->bos_desc.bDescriptorType);
    printf("\twTotalLength : %x \r\n", bos_desc->bos_desc.wTotalLength);
    printf("\tbNumDeviceCaps : %x \r\n", bos_desc->bos_desc.bNumDeviceCaps);

    printf("\r\n\tUSB2.0 EXTENSION DESCRIPTOR \r\n");
    printf("\t\tLength : %x \r\n", bos_desc->usb20_ext_desc.bLength);
    printf("\t\tbDescriptorType: %x \r\n", bos_desc->usb20_ext_desc.bDescriptorType);
    printf("\t\tbDevCapabilityType : %x \r\n", bos_desc->usb20_ext_desc.bDevCapabilityType);
    printf("\t\tbmAttributes : %x \r\n", bos_desc->usb20_ext_desc.bmAttributes);

    printf("\r\n\tSS DEVICE CAPABILITY DESCRIPTOR \r\n");
    printf("\t\tLength : %x \r\n", bos_desc->ss_dev_desc.bLength);
    printf("\t\tbDescriptorType: %x \r\n", bos_desc->ss_dev_desc.bDescriptorType);
    printf("\t\tbDevCapabilityType : %x \r\n", bos_desc->ss_dev_desc.bDevCapabilityType);
    printf("\t\tbmAttributes : %x \r\n", bos_desc->ss_dev_desc.bmAttributes);
    printf("\t\twSpeedsSupported : %x \r\n", bos_desc->ss_dev_desc.wSpeedsSupported);
    printf("\t\tbFunctionalitySupport : %x \r\n", bos_desc->ss_dev_desc.bFunctionalitySupport);
    printf("\t\tbU1DevExitLat : %x \r\n", bos_desc->ss_dev_desc.bU1DevExitLat);
    printf("\t\twU2DevExitLat : %x \r\n", bos_desc->ss_dev_desc.wU2DevExitLat);
}
