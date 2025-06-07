/*
 * SPDX-FileCopyrightText: Copyright (C) 2025 Altera Corporation
 *
 * SPDX-License-Identifier: MIT-0
 *
 * Header file for I3C HAL driver
 */

#ifndef __SOCFPGA_I3C_H__
#define __SOCFPGA_I3C_H__

/**
 * @file socfpga_i3c.h
 * @brief I3C HAL driver header file.
 *
 * This driver provides methods to perform  operations with an I3C
 * or an I2C device connected to the SoC FPGA I3C bus.
 *
 * The driver supports DAA and DASA and can communicate with both I2C and I3C
 * devices connected to the bus in interrupt mode. For I3C devices the
 * I3C controller will provide an address if needed whereas for an I2C device its 
 * static address is used to communicate with the controller.
 *
 */

/**
 * @defgroup i3c I3C
 * @ingroup drivers
 * @{
 */
/**
 * @defgroup i3c_fns Functions
 * @ingroup i3c
 * I3C HAL APIs
 */

/**
 * @defgroup i3c_structs Structures
 * @ingroup i3c
 * I3C Specific Structures
 */

/**
 * @defgroup i3c_enums Enumerations
 * @ingroup i3c
 * I3C Specific Enumerations
 */

/**
 * @defgroup i3c_macros Macros
 * @ingroup i3c
 * I3C Specific Macros
 */

/**
 * @addtogroup i3c_macros
 * @{
 */
/**
 * @brief I3C controller instance specification.
 */
#define I3C_INSTANCE1                        0x0U   /*!< I3C  instance number 1. */
#define I3C_INSTANCE2                        0x1U   /*!< I3C  instance number 2. */
#define I3C_NUM_INSTANCES                    0x2U   /*!< I3C  maximum number of instances */
/**
 * @}
 */

/**
 * @brief Specifies the different IOCTL requests allowed by the I3C controller driver.
 * @ingroup i3c_enums
 */
enum i3c_IoctlRequest
{
    I3C_IOCTL_TARGET_ATTACH,          /*!< Command to attach the I3C device. */
    I3C_IOCTL_BUS_INIT,               /*!< Command for I3C bus initialization. */
    I3C_IOCTL_DO_DAA,                 /*!< command to start DAA. */
    I3C_IOCTL_GET_DYNADDRESS,         /*!< Command to get I3C device dynamic address. */
    I2C_IOCTL_ADDRESS_VALID           /*!< Command to check if I2C device is valid. */
};

/**
 * @addtogroup i3c_structs
 * @{
 */

/**
 * @brief Structure used to describe an I3C device that is connected to the I3C controller instance.
 */
struct i3c_I3cDevice
{

    uint64_t deviceId : 48;            /*!< 48 bit device provisioned ID.For i2c device mark it as zero*/

    uint8_t staticAddress;             /*!< staticAddress to be used for the target/device. If
					* it is set to 0, controller should accessing a dynamic
					* address through the DAA scheme
					*/

    uint8_t preferredDynamicAddress;   /*!< preferred dynamic address for the device,
					* The final dynamic address assigned will be set in
					* the dynamicAddress field.
					*/

    uint8_t dynamicAddress;            /*!< Final address of the device under the controller */
};


/**
 * @brief Structure used to pass the information of all the I3C devices that are connected to the I3C controller instance.
 */
struct i3c_DevList
{
    uint16_t numDevices;               /*!< number of device objects in hte pDeviceList */

    struct i3c_I3cDevice *list;        /*!< List of i3c device connected to the controller*/
};

/**
 * @brief Structure used by the user layer to make read/write from/to a connected device (after I3C_IOCTL_TARGET_ATTACH and I3C_IOCTL_BUS_INIT).
 */
struct i3c_XferRequest
{
    uint8_t     *buffer;    /*!< Pointer to the location where data is present for write
                               or to where it should be copied for read */
    uint16_t length;        /*!< length in bytes of the data request */

    bool read;              /*!< read or write xfer*/
};
/**
 * @}
 */

/**
 * @addtogroup i3c_fns
 * @{
*/
/**
 * @brief Initialize the I3C controller instance.
 *
 * @param[in] instance  Instance of the I3C controller.
 * @return int          I3C_OK if successful, -EINVAL if incorrect instance, -EBUSY if bus is busy.
 */
extern int32_t i3c_open(uint8_t instance);

/**
 * @brief Configure I3C controller instance with user configuration.
 *
 * @note I3C_IOCTL_TARGET_ATTACH: Request for adding the devices connected to the controller.
 * Uses i3c_I3cDevList structure to pass the number of devices and their properties to the controller.
 *
 * @note I3C_IOCTL_BUS_INIT: Used for enumerating the I3C bus. The I3C_IOCTL_TARGET_ATTACH should
 * be called before calling this request. This ioctl will try to assign dynamic addresses to all the
 * connected devices. Once this is completed, read/write transactions can be performed with the devices.
 *
 * @note I3C_IOCTL_DO_DAA: Perform the dynamic address assignment to the connected devices
 * which do not have static addresses.
 *
 * @note I3C_IOCTL_CONFIG_IBI: Enable/disable the slave in-bound interrupt.
 *
 * @param[in] instance  Instance of the I3C controller.
 * @param[in] ioctl     IOCTL request.
 * @param[in] pArgs     Pointer to the arguments for the IOCTL request.
 *
 * @return I3C_OK if successful, I3C_INVALID if incorrect instance, ERROR otherwise.
 * @return
 * - I3C_OK, if successful
 * - -EINVAL, if incorrect instance
 * - -EBUSY, if busy
 */
extern int32_t i3c_ioctl(uint8_t instance, enum i3c_IoctlRequest ioctl,
        void *pArgs );

/**
 * @brief Perform a data transfer between the controller and the target device.
 *
 * @param[in] instance        Instance of the I3C controller.
 * @param[in] address         Dynamic address of the target to which the transfer is requested.
 * @param[in] pXferRequest    Pointer to the transfer request list.
 * @param[in] numXfers        Number of transfers requested.
 * @param[in] isI2c           Indicates if the transfer is for an I2C device.
 * @return int                -EINVAL if invalid arguments were passed, -EINVAL/-EBUSY if transfer was not successful, I3C_OK if transfer was successful.
 */
extern int32_t i3c_TransferSync(uint8_t instance, uint8_t address,
        struct i3c_XferRequest *pXferRequest,
        uint8_t numXfers, bool isI2c );

/**
 * @}
 */
/**
 * @}
 */

#endif /*__SOCFPGA_I3C_H__*/
