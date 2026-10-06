/*
    SPDX-FileCopyrightText: Copyright (C) 2025-2026 Altera Corporation

    SPDX-License-Identifier: MIT-0

    Header file for I3C HAL driver
*/

#ifndef __SOCFPGA_I3C_H__
#define __SOCFPGA_I3C_H__

/*
    File: I3C Driver

    I3C HAL driver header file.

    APIs for Soc FPGA I3C driver.

    This driver provides methods to perform operations with an I3C or an I2C
    device connected to the SoC FPGA I3C bus.

    The driver supports DAA and DASA and can communicate with both I2C and I3C
    devices connected to the bus in interrupt mode. For I3C devices the I3C
    controller will assign an address if needed whereas for an I2C device its
    static address is used to communicate with the controller.

    A typical usage flow is as follows:

    1. Initialize the driver using the `i3c_open` API.
    2. Prepare an array of type `struct i3c_device`. Each entry in the array should be
    filled with the properties of the I3C or I2C device present on the bus.
    3. Use the `I3C_IOCTL_TARGET_ATTACH` IOCTL to attach the devices. Use the above
    array as the parameter to this IOCTL.
    4. Use the `I3C_IOCTL_BUS_INIT` IOCTL to trigger dynamic address assignment.
    5. For I3C devices, use the `I3C_IOCTL_GET_DYNADDRESS` IOCTL to get the dynamic address.
    6. For I2C devices, use the `I2C_IOCTL_ADDRESS_VALID` IOCTL to verify that the slave address is valid.
    7. Perform read/write operations as desired using the `i3c_transfer_sync` API.

    To see example usage, see I3C sample application
*/

#include <errno.h>
#include <stdbool.h>
#include <stdint.h>

/*
    Group: Macros

    Enumerated values and constants for the I3C driver.
*/
/*
    Constant: I3C_INSTANCE1

    I3C controller instance specification.
*/
#define I3C_INSTANCE1        0x0U
#define I3C_INSTANCE2        0x1U
#define I3C_NUM_INSTANCES    0x2U

/*
    Enum: i3c_ioctl_request

    Specifies the different IOCTL requests allowed by the I3C controller
    driver.
*/
enum i3c_ioctl_request
{
    I3C_IOCTL_TARGET_ATTACH,
    I3C_IOCTL_BUS_INIT,
    I3C_IOCTL_DO_DAA,
    I3C_IOCTL_GET_DYNADDRESS,
    I2C_IOCTL_ADDRESS_VALID,
    I3C_IOCTL_CONFIG_IBI
};

/*
    Group: Structures

    Callback signatures, structures and variables for the I3C driver.
*/

/*
    Struct: i3c_device

    Structure used to describe an I3C device that is connected to the I3C
    controller instance.
*/
struct i3c_device
{
    /* 48-bit provisioned device ID in lower bits. Set to zero for I2C devices. */
    uint64_t device_id;

    /*
        Variable: static_address

        Static target address.

        Set to 0 to use dynamic address assignment through DAA.
    */
    uint8_t static_address;

    /*
        Variable: preferred_dynamic_address

        Preferred dynamic address for the target.

        The final assigned value is reflected in dynamic_address.
    */
    uint8_t preferred_dynamic_address;

    /* Final target address assigned under the controller. */
    uint8_t dynamic_address;

    /* Bus characteristics register value. */
    uint8_t bcr;
};


/*
    Struct: i3c_dev_list

    Structure used to pass the information of all the I3C devices that are
    connected to the I3C controller instance.
*/
struct i3c_dev_list
{
    /* Number of device objects in list. */
    uint16_t num_devices;

    /* List of targets connected to this controller. */
    struct i3c_device *list;
};

/*
    Struct: i3c_ibi_config

    Structure used to configure IBI enable/disable for a device.
*/
struct i3c_ibi_config
{
    /* Target device object. */
    struct i3c_device *dev;

    /* Set true to enable IBI, false to disable IBI. */
    bool enable;
};

struct i3c_descriptor;
typedef struct i3c_descriptor *i3c_handle_t;

/*
    Callback: i3c_callback_t

    Callback function for i3c operations

    Parameters:

        stat - I3C instance
        param - function param
*/
typedef void (*i3c_callback_t)(int stat,
        void *param);

/*
    Callback: i3c_ibi_callback_t

    Callback function for IBI events.

    Parameters:

        ibi_id - IBI ID received in IBI queue status.
        status - IBI status field.
        payload - Pointer to payload bytes if present.
        payload_len - Number of payload bytes.
        param - user context.
*/
typedef void (*i3c_ibi_callback_t)(uint8_t ibi_id, uint8_t status,
        const uint8_t *payload, uint8_t payload_len, void *param);

/*
    Struct: i3c_xfer_request

    Structure used by the user layer to make read/write from/to a connected
    device (after I3C_IOCTL_TARGET_ATTACH and I3C_IOCTL_BUS_INIT).
*/
struct i3c_xfer_request
{
    /*
        Variable: buffer

        Data buffer.

        For write, points to source bytes. For read, points to destination
        bytes.
    */
    uint8_t *buffer;

    /* Transfer size in bytes. */
    uint16_t length;

    /* Set true for read transfer, false for write transfer. */
    bool read;
};

/*
    Group: Functions

    Entry points for the I3C driver.
*/
/*
    Function: i3c_open

    Initialize and open an I3C controller instance.

    Parameters:

        instance - Instance of the I3C controller.

    Returns:

        - valid `i3c_handle_t` on success.
        - `NULL` on failure.
*/
extern i3c_handle_t i3c_open(uint8_t instance);

/*
    Function: i3c_close

    Close an open I3C controller handle.

    Parameters:

        hi3c - Handle returned by i3c_open().

    Returns:

        - 0 on success.
        - -EINVAL if handle is NULL or not open.
*/
extern int32_t i3c_close(i3c_handle_t hi3c);

/*
    Function: i3c_send_ccc

    Send a CCC command to a target.

    Parameters:

        hi3c - I3C controller handle.
        cmd_id - CCC command ID.
        target_address - Target address (0 for broadcast).
        data - Pointer to payload data buffer.
        data_length - Payload length.
        read - Read command if true.

    Returns:

        0 on success, negative error code on failure.
*/
extern int32_t i3c_send_ccc(i3c_handle_t hi3c, uint8_t cmd_id,
        uint8_t target_address, uint8_t *data, uint16_t data_length, bool read);

/*
    Function: i3c_enable_ibi

    Enable IBI for a target device.

    Parameters:

        hi3c - I3C controller handle.
        dev - Target device descriptor.

    Returns:

        0 on success, negative error code on failure.
*/
extern int32_t i3c_enable_ibi(i3c_handle_t hi3c, struct i3c_device *dev);

/*
    Function: i3c_disable_ibi

    Disable IBI for a target device.

    Parameters:

        hi3c - I3C controller handle.
        dev - Target device descriptor.

    Returns:

        0 on success, negative error code on failure.
*/
extern int32_t i3c_disable_ibi(i3c_handle_t hi3c, struct i3c_device *dev);

/*
    Function: i3c_set_ibi_callback

    Register IBI callback for an I3C handle.

    Parameters:

        hi3c - I3C controller handle.
        callback - Callback function.
        param - user context.

    Notes:

        Callback runs in ISR context and must be non-blocking.
*/
extern void i3c_set_ibi_callback(i3c_handle_t hi3c, i3c_ibi_callback_t callback,
        void *param);

/*
    Function: i3c_ioctl

    Configure I3C controller instance with user configuration.

    Parameters:

        hi3c - I3C controller handle.
        ioctl - IOCTL request.
        pargs - Pointer to the arguments for the IOCTL request.

    Returns:

        - 0 on success.
        - -EINVAL for invalid args/handle.
        - negative errno on failure.

    Notes:

        I3C_IOCTL_TARGET_ATTACH: Request for adding the devices connected to
        the controller.
        Uses struct i3c_dev_list to pass the number of devices and their
        properties to the controller.
        I3C_IOCTL_BUS_INIT: Used for enumerating the I3C bus. The
        I3C_IOCTL_TARGET_ATTACH should
        be called before calling this request. This ioctl will try to assign
        dynamic addresses to all the
        connected devices. Once this is completed, read/write transactions can
        be performed with the devices.
        I3C_IOCTL_DO_DAA: Perform the dynamic address assignment to the
        connected devices
        which do not have static addresses.
        I3C_IOCTL_CONFIG_IBI: Enable/disable the slave in-bound interrupt.
*/
extern int32_t i3c_ioctl(i3c_handle_t hi3c, enum i3c_ioctl_request ioctl,
        void *pargs);

/*
    Function: i3c_set_callback

    set I3C callback

    Parameters:

        hi3c - I3C controller handle.
        callback - The callback to be registered.
        param - Parameters that are to be passed to the callback.

    Returns:

        NIL
*/
extern void i3c_set_callback(i3c_handle_t hi3c,
        i3c_callback_t callback, void *param);

/*
    Function: i3c_transfer_sync

    Perform a data transfer between the controller and the target device.

    Parameters:

        hi3c - I3C controller handle.
        address - Dynamic address of the target to which the transfer is
            requested.
        pxfer_request - Pointer to the transfer request list.
        num_xfers - Number of transfers requested.
        is_i2c - Indicates if the transfer is for an I2C device.

    Returns:

        - 0 on success.
        - -EINVAL if arguments are invalid.
        - -EBUSY if transfer is already in progress.
        - negative errno on failure.
*/
extern int32_t i3c_transfer_sync(i3c_handle_t hi3c, uint8_t address, struct
        i3c_xfer_request *pxfer_request, uint8_t num_xfers, bool is_i2c);

/*
    Function: i3c_transfer_async

    Perform a data transfer between the controller and the target device.

    Parameters:

        hi3c - I3C controller handle.
        address - Dynamic address of the target to which the transfer is
            requested.
        pxfer_request - Pointer to the transfer request list.
        num_xfers - Number of transfers requested.
        is_i2c - Indicates if the transfer is for an I2C device.

    Returns:

        - 0 on success.
        - -EINVAL if arguments are invalid.
        - -EBUSY if transfer is already in progress.
        - negative errno on failure.
*/
extern int32_t i3c_transfer_async(i3c_handle_t hi3c, uint8_t address, struct
        i3c_xfer_request *pxfer_request, uint8_t num_xfers, bool is_i2c);

#endif
