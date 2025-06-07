/*
 * SPDX-FileCopyrightText: Copyright (C) 2025 Altera Corporation
 *
 * SPDX-License-Identifier: MIT-0
 *
 * HAL driver implementation for I3C
 */

#include <errno.h>

#include "osal_log.h"
#include "socfpga_defines.h"
#include "socfpga_i3c.h"
#include "socfpga_i3c_ll.h"

#define GET_ADDRESS_ALLOTMENT_ENTRY_BIT_INDEX(addr)      (((addr) << 1U) % \
        NUM_BITS_PER_TABLE_ENTRY)
#define GET_ADDRESS_ALLOTMENT_TABLE_INDEX(addr)          (((addr) << 1U) / \
        NUM_BITS_PER_TABLE_ENTRY)

#define SET_ADDRESS_ALLOTMENT_ENTRY(idx, pos, status)    do {                                                                                                     \
    i3c_Obj[(instance)].addrAllotmentTable[(idx)] &= \
    ~((uint32_t)ADDRESS_ENTRY_STATUS_MAX << (pos)); \
    i3c_Obj[(instance)].addrAllotmentTable[(idx)] |= ((uint32_t) (status) << (pos));                             \
}while(false)


#define GET_ADDRESS_ALLOTMENT_ENTRY(idx, pos) \
    ((i3c_Obj[(instance)].addrAllotmentTable[(idx)] >> (pos)) & \
     ADDRESS_ENTRY_STATUS_MAX)

/**
 * @brief I3C_CCC Common Command Codes (CCC)
 */
#define I3C_CCC_RSTDAA     (0x06U)
#define I3C_CCC_RSTACT(broadcast)    ((broadcast) ? 0x2AU : 0x9AU)
#define I3C_CCC_SETDASA    (0x87U)
#define I3C_CCC_ENTDAA     (0x07U)

#define I3C_OK                               0

/**
 * @brief I3C_CCC_Events Enable Events
 */
#define I3C_CCC_EVT_INTR    ((uint32_t) 1)
#define I3C_CCC_EVT_CR      ((uint32_t) 1 << 1U)
#define I3C_CCC_EVT_HJ      ((uint32_t) 1 << 3U)

#define I3C_CCC_EVT_ALL     (I3C_CCC_EVT_INTR | I3C_CCC_EVT_CR | I3C_CCC_EVT_HJ)

#define I3C_CCC_DISEC(broadcast)    ((broadcast) ? 0x01U : 0x81U)

#define I3C_CCC_RSTACT_PERIPHERAL_ONLY       (0x01U)
#define I3C_CCC_RSTACT_RESET_WHOLE_TARGET    (0x02U)


/* I3C Controller Instance object  */
extern struct i3c_DriverObj i3c_Obj[I3C_NUM_INSTANCES];


/**
 * @brief Get the status of the entry in the Address allotment table corresponding
 *        to the I3C address specified.
 *
 * @param[in] instance  Instance of the I3C controller.
 * @param[in] address   Address for which the status is inquired.
 * @return uint8_t      Entry in the location corresponding to the address.
 */
static uint32_t GetAddressAllotmentTableEntry(uint8_t instance, uint8_t address)
{
    uint8_t idx, bitIndex;
    uint32_t status;

    idx      = GET_ADDRESS_ALLOTMENT_TABLE_INDEX(address);
    bitIndex = GET_ADDRESS_ALLOTMENT_ENTRY_BIT_INDEX(address);

    status = GET_ADDRESS_ALLOTMENT_ENTRY(idx, bitIndex);

    return status;

}


/**
 * @brief Get the status of the entry in the Address allotment table corresponding
 *        to the I3C address specified.
 *
 * @param[in] instance  Instance of the I3C controller.
 * @param[in] address   Address for which the status is inquired.
 * @return uint8_t      Entry in the location corresponding to the address.
 */
static uint8_t GetNextFreeAddressAllotmentTableEntry(uint8_t instance)
{
    uint8_t addr;

    for (addr = 8U; addr < I3C_MAX_ADDR; addr++)
    {
        if (GetAddressAllotmentTableEntry (instance,
                addr) == ADDRESS_ENTRY_STATUS_FREE)
        {
            break;
        }
    }

    return addr;
}

/**
 * @brief Initialize address allotment table, reserving invalid and broadcast addresses, setting others
 *
 * @param[in] instance  Instance of the I3C controller.
 * @return None.
 */
static void i3c_InitAddressTable(uint8_t instance)
{
    uint8_t idx, bitIndex;
    uint8_t addr, tempAddr;

    /* target address from 0 to 7 are reserved*/
    for (addr = 0U; addr <= 7U; addr++)
    {

        idx      = GET_ADDRESS_ALLOTMENT_TABLE_INDEX(addr);
        bitIndex = GET_ADDRESS_ALLOTMENT_ENTRY_BIT_INDEX(addr);

        /* set the entry corresponding to the address 'addr'  as reserved */
        SET_ADDRESS_ALLOTMENT_ENTRY(idx, bitIndex, ADDRESS_ENTRY_STATUS_RSVD);

        /* set the Addresses that are one bit different from the broadcast address
         * as reserved too.
         */
        tempAddr = I3C_BROADCAST_ADDR ^ (1U << addr);

        idx      = GET_ADDRESS_ALLOTMENT_TABLE_INDEX(tempAddr);
        bitIndex = GET_ADDRESS_ALLOTMENT_ENTRY_BIT_INDEX(tempAddr);

        SET_ADDRESS_ALLOTMENT_ENTRY(idx, bitIndex, ADDRESS_ENTRY_STATUS_RSVD);

    }

    /* Reserve the broadcast address as well */
    idx      = GET_ADDRESS_ALLOTMENT_TABLE_INDEX(I3C_BROADCAST_ADDR);
    bitIndex = GET_ADDRESS_ALLOTMENT_ENTRY_BIT_INDEX(I3C_BROADCAST_ADDR);

    SET_ADDRESS_ALLOTMENT_ENTRY(idx, bitIndex, ADDRESS_ENTRY_STATUS_RSVD);
    return;
}

/**
 * @brief Get initial address for the device, preferring static or preferred dynamic address, or find a free address.
 *
 * @param[in] instance  Instance of the I3C controller.
 * @param[in] pDevice   Pointer to the device descriptor.
 * @return uint8_t      Initial (potential dynamic) address for the device.
 */
static uint8_t i3c_PrepareAddress(uint8_t instance,
        struct i3c_I3cDevice *pDevice)
{
    uint8_t initialAddress = 0U;

    if ((instance > I3C_NUM_INSTANCES) || (pDevice == NULL))
    {
        initialAddress = -EINVAL;
    }

    if (pDevice->staticAddress != 0U)
    {
        if (GetAddressAllotmentTableEntry(instance,
                pDevice->staticAddress) == ADDRESS_ENTRY_STATUS_FREE)
        {
            initialAddress = pDevice->staticAddress;
        }
        else
        {
            initialAddress = 0U;
        }
    }
    else
    {
        if ((pDevice->preferredDynamicAddress != 0U) &&
                (GetAddressAllotmentTableEntry(instance,
                        pDevice->preferredDynamicAddress) ==
                ADDRESS_ENTRY_STATUS_FREE))
        {
            initialAddress = pDevice->preferredDynamicAddress;
        }
        else
        {
            initialAddress = GetNextFreeAddressAllotmentTableEntry(instance);
        }
    }

    return initialAddress;

}

/**
 * @brief Assign a free dynamic address to the controller.
 *
 * @param[in] instance  Instance of the I3C controller.
 * @return 0          If the operation was successful.
 */
static int32_t i3c_AssignOwnDA( uint8_t instance )
{
    uint8_t address, idx, bitIndex;

    int32_t status = -EBUSY;

    address = GetNextFreeAddressAllotmentTableEntry(instance);
    if (address <= I3C_MAX_ADDR)
    {
        idx      = GET_ADDRESS_ALLOTMENT_TABLE_INDEX(address);
        bitIndex = GET_ADDRESS_ALLOTMENT_ENTRY_BIT_INDEX(address);

        /* set the entry corresponding to the address 'addr'  as reserved */
        SET_ADDRESS_ALLOTMENT_ENTRY(idx, bitIndex, ADDRESS_ENTRY_STATUS_I3C);

        i3c_Obj[instance].ownDA = address;

        status = 0;
    }

    return status;
}


/**
 * @brief Remove device from controller and update address allotment table.
 *
 * @param[in] instance     Instance of the I3C controller.
 * @param[in] pDeviceDesc  Pointer to the device descriptor.
 * @param[in] address      Address of the device.
 * @return 0             If the operation was successful.
 */
static int32_t i3c_DetachDevice(uint8_t instance,
        struct i3c_DeviceDesc *pDeviceDesc,
        uint8_t address)
{
    uint8_t idx, bitIndex;
    int32_t ret = 0;
    uint32_t status;

    /* use the address to check if the addressallotmenTable is set ot I3C device*/
    idx      = GET_ADDRESS_ALLOTMENT_TABLE_INDEX(address);
    bitIndex = GET_ADDRESS_ALLOTMENT_ENTRY_BIT_INDEX(address);

    /* set the entry corresponding to the address 'addr'  as reserved */
    status = GET_ADDRESS_ALLOTMENT_ENTRY(idx, bitIndex);
    if (status == ADDRESS_ENTRY_STATUS_I3C)
    {
        /* set the entry to free*/
        SET_ADDRESS_ALLOTMENT_ENTRY(idx, bitIndex, ADDRESS_ENTRY_STATUS_FREE);
    }
    /* Now remove the address from the device address table too*/
    ret = i3c_ll_DetachDevice(instance, pDeviceDesc, address);

    return ret;
}

/**
 * @brief Add device to controller and update address allotment table.
 *
 * @param[in] instance     Instance of the I3C controller.
 * @param[in] pDeviceDesc  Pointer to the device descriptor.
 * @param[in] address      Address of the device.
 * @return 0               If the operation was successful.
 */
static int32_t i3c_AttachI2cDevice(uint8_t instance,
        struct i3c_DeviceDesc *pDeviceDesc,
        uint8_t address)
{

    uint8_t idx, bitIndex;
    int32_t ret = 0;

    /* set the to entry corresponding to address as I2C device*/
    idx      = GET_ADDRESS_ALLOTMENT_TABLE_INDEX(address);
    bitIndex = GET_ADDRESS_ALLOTMENT_ENTRY_BIT_INDEX(address);

    /* set the entry corresponding to the address 'addr'  as reserved */
    SET_ADDRESS_ALLOTMENT_ENTRY(idx, bitIndex, ADDRESS_ENTRY_STATUS_I2C);

    ret = i3c_ll_AttachI2cDevice(instance, pDeviceDesc, address);

    return ret;
}

/**
 * @brief Add device to controller and update address allotment table.
 *
 * @param[in] instance     Instance of the I3C controller.
 * @param[in] pDeviceDesc  Pointer to the device descriptor.
 * @param[in] address      Address of the device.
 * @return 0               If the operation was successful.
 */
static int32_t i3c_AttachDevice(uint8_t instance,
        struct i3c_DeviceDesc *pDeviceDesc,
        uint8_t address)
{
    uint8_t idx, bitIndex;
    int32_t ret = 0;

    /* set the to entry corresponding to address as I3C device*/
    idx      = GET_ADDRESS_ALLOTMENT_TABLE_INDEX(address);
    bitIndex = GET_ADDRESS_ALLOTMENT_ENTRY_BIT_INDEX(address);

    /* set the entry corresponding to the address 'addr'  as reserved */
    SET_ADDRESS_ALLOTMENT_ENTRY(idx, bitIndex, ADDRESS_ENTRY_STATUS_I3C);

    ret = i3c_ll_AttachDevice(instance, pDeviceDesc, address);

    return ret;
}

/**
 * @brief Add devices to the driver object for DASA and DAA checks.
 *
 * @param[in] instance  Instance of the I3C controller.
 * @param[in] pDevList  Pointer to the device list.
 * @return 0            If the operation was successful.
 * @return -EINVAL      If arguments were incorrect.
 */
static int32_t i3c_AddDevices(uint8_t instance, struct i3c_DevList *pDevList)
{
    int32_t ret = 0;
    uint8_t devIdx, i;

    if ((instance > I3C_NUM_INSTANCES) || (pDevList == NULL))
    {
	ERROR("Invalid arguments");
        ret =  -EINVAL;
    }

    for (devIdx = 0; (ret == 0) && (devIdx < pDevList->numDevices);
            devIdx++)
    {
        struct i3c_I3cDevice *pDevice = &pDevList->list[devIdx];

        /* verify if the device is already attached to the controller*/
        for (i = 0; i < i3c_Obj[instance].numDev; i++)
        {
            /* Device ID is zero  means that it is a legacy I2C device*/
            if (i3c_Obj[instance].i3cDevDescList[i].device.deviceId == 0U)
            {
                if (i3c_Obj[instance].i3cDevDescList[i].device.staticAddress ==
                        pDevice->staticAddress)
                {
                    break;
                }
            }
            /*Non zero device IDs indicate that the device is an I3C device*/
            else
            {
                if (i3c_Obj[instance].i3cDevDescList[i].device.deviceId ==
                        pDevice->deviceId)
                {
                    break;
                }
            }
        }
        /* add the device to the attached list if it is a new device*/
        if (i >= i3c_Obj[instance].numDev)
        {

            /*Add the device to the attached device list in the controller object*/
            (void) memcpy((void *)&i3c_Obj[instance].i3cDevDescList[i3c_Obj[instance].
                    numDev].device,
                    pDevice,
                    sizeof(struct i3c_I3cDevice));
            i3c_Obj[instance].numDev++;

            /*Mark the address as taken in the address allotment table for I2C devices (Done inside the attach function)*/
            if (pDevice->deviceId == 0U)
            {
                struct i3c_DeviceDesc *pDeviceDesc =
                        &i3c_Obj[instance].i3cDevDescList[i3c_Obj[instance].
                                numDev];
                ret = i3c_AttachI2cDevice(instance, pDeviceDesc,
                        pDevice->staticAddress);
            }
        }

    }

    return ret;
}


/**
 * @brief Perform DASA to assign dynamic addresses to devices with static addresses.
 *
 * @param[in] instance  Instance of the I3C controller.
 * @return None.
 */
static void i3c_PerformDASA(uint8_t instance, bool *requireDAA)
{
    uint8_t address, pos;
    struct i3c_DeviceDesc *pDeviceDesc;
    struct i3c_CmdPayload cmdPayload;
    int32_t ret;

    for (pos = 0U; pos < i3c_Obj[instance].numDev; pos++)
    {
        pDeviceDesc = &i3c_Obj[instance].i3cDevDescList[pos];

        if ((pDeviceDesc->device.staticAddress == 0U) ||
                (pDeviceDesc->device.deviceId == 0U))
        {
            *requireDAA = true;
            continue;
        }
        /* prepare the address for the i3c device*/
        address = i3c_PrepareAddress(instance, &pDeviceDesc->device);

        /* Prepare the DAT for the DASA command. For this we will attach the
         *  device to the controller with the initial address.
         * 1. add entry in the address allotment table (device address table [dev] = i3c Device;) /common part
         * 2. add the address in the free position in the device address table register (device address table [devIdx] = address;) //LL part
         */
        ret = i3c_AttachDevice(instance, pDeviceDesc, address);

        /* do dasa*/
        /* if both static address and preferred dynamicaddress is given,
         * DASA command is send to the target with preferred address
         * as dynamic address. If no preferred dynamic address
         * is given, dynamic address become same as static address.*/
        address = (pDeviceDesc->device.preferredDynamicAddress != 0U) ? (pDeviceDesc->
                device.
                preferredDynamicAddress) :
                (pDeviceDesc->device.staticAddress);
        address = (address << 1U);    /* 7 bit address needs to left justified.*/

        /* set dynamic address for all this particular devices*/
        cmdPayload.cmdId         = I3C_CCC_SETDASA;      /* set DA using SA command*/
        cmdPayload.read          = false;               /* send(write) action*/
        cmdPayload.data          = &address;
        cmdPayload.targetAddress = pDeviceDesc->device.staticAddress;
        cmdPayload.dataLength    = 1U;

        /* Target address is used to get the index in the device address table, so
           prior to this the device address table should be populated with this static
           address */
        ret = i3c_ll_SendAddrAssignmentCommand(instance, &cmdPayload,
                pDeviceDesc->datIndex);
        if (ret == 0)
        {
            /* if successfully assigned the address, update it in the controller
               device list
             */
            pDeviceDesc->device.dynamicAddress =  (address >> 1U);
        }
        if (pDeviceDesc->device.dynamicAddress !=
                pDeviceDesc->device.staticAddress)
        {
            /* re-attach this device from static address position to the final
               dynamic Address position*/
            if (i3c_DetachDevice(instance, pDeviceDesc, (address >> 1U)) != 0)
            {
                return;
            }

            /* attach back the device uisng the assigned dynamic address*/
            if (i3c_AttachDevice(instance, pDeviceDesc,
                    pDeviceDesc->device.dynamicAddress) != 0)
            {
                return;
            }
        }
    }
    return;
}


/**
 * @brief Perform DAA to assign dynamic addresses to remaining devices after DASA.
 *
 * @param[in] instance  Instance of the I3C controller.
 * @return None.
 */
static void i3c_PerformDAA(uint8_t instance)
{
    uint8_t address, pos;
    struct i3c_DeviceDesc *pDeviceDesc;
    struct i3c_CmdPayload cmdPayload;
    uint32_t startIdx = 0U;
    uint8_t numDevices = 0U;
    int32_t ret = 0;

    for (pos = 0U; pos < i3c_Obj[instance].numDev; pos++)
    {
        pDeviceDesc = &i3c_Obj[instance].i3cDevDescList[pos];

        if (pDeviceDesc->device.staticAddress != 0U)
        {
            /* devices with static address would have been already
               assigned during DASA
             */
            continue;
        }
        /* prepare the address for the i3c device*/
        address = i3c_PrepareAddress(instance, &pDeviceDesc->device);

        /* Prepare the DAT for the DAA command. For this we will attach the
         *  device to the controller with the initial address.
         * 1. add entry in the address allotment table (device address table [dev] = i3c Device;) /common part
         * 2. add the address in the free position in the device address table register (device address table [devIdx] = address;) //LL part
         */
        ret = i3c_AttachDevice(instance, pDeviceDesc, address);

        /* save the start index of the first device in the list for whch DAA is required.
           This is needed for sending the DAA command.
         */
        if (startIdx == 0U)
        {
            startIdx = pDeviceDesc->datIndex;
        }

        /*storing the dynamic address, but should the actual command fails this needs to be cleared!!*/
        pDeviceDesc->device.dynamicAddress = address;

        numDevices++; /* number of devices that needs DAA*/
    }

    /* Prepare the device*/
    pDeviceDesc = &i3c_Obj[instance].i3cDevDescList[pos];

    /* set dynamic address for all this particular devices*/
    cmdPayload.cmdId         = I3C_CCC_ENTDAA;      /* enter DAA command*/
    cmdPayload.read          = false;               /* send(write) action*/
    cmdPayload.dataLength    = numDevices;

    ret = i3c_ll_SendAddrAssignmentCommand(instance, &cmdPayload, startIdx );
    if (ret != 0)
    {
        for (pos = 0U; pos < i3c_Obj[instance].numDev; pos++)
        {
            pDeviceDesc = &i3c_Obj[instance].i3cDevDescList[pos];

            if (pDeviceDesc->device.staticAddress == 0U)
            {
                /*remove the devices from the controller*/
                if (i3c_DetachDevice(instance, pDeviceDesc,
                        pDeviceDesc->device.dynamicAddress) != 0)
                {
                    return;
                }
            }
        }
    }

    return;
}


/**
 * @brief Initialize I3C bus and assign dynamic addresses to connected devices.
 *
 * @param[in] instance  Instance of the I3C controller.
 * @return None.
 */
static void i3c_init_bus(uint8_t instance)
{
    uint8_t defByte = 0;
    bool requireDAA = false;
    struct i3c_CmdPayload cmdPayload;
    bool isI2c = false;

    /* reset all connected devices, send ccc broadcast */
    INFO("Resetting all connected devices");
    defByte = I3C_CCC_RSTACT_RESET_WHOLE_TARGET;

    cmdPayload.cmdId         = I3C_CCC_RSTACT(true);    /* reset action command*/
    cmdPayload.read          = false;                   /* send(write) action*/
    cmdPayload.data          = &defByte;
    cmdPayload.dataLength    = sizeof(defByte);
    cmdPayload.targetAddress = 0;

    if (i3c_ll_SendXferCommand(instance, &cmdPayload, 1, isI2c) != 0)
    {
        /* if failed to reset the whole device, try resetting only
         * the peripheral*/
        defByte = I3C_CCC_RSTACT_PERIPHERAL_ONLY;
        if (i3c_ll_SendXferCommand(instance, &cmdPayload, 1, isI2c) != 0)
        {
            return;
        }
    }

    /* reset current DAA assignments*/
    INFO("Resetting dynamic address assignments");
    cmdPayload.cmdId        = I3C_CCC_RSTDAA;      /* reset DAA command*/
    cmdPayload.read         = false;               /* send(write) action*/
    cmdPayload.data         = NULL;
    cmdPayload.dataLength   = 0;

    if (i3c_ll_SendXferCommand(instance, &cmdPayload, 1, isI2c) != 0)
    {
        return;
    }

    /* disable all events*/
    INFO("Disabling all events");
    defByte = I3C_CCC_EVT_ALL;

    cmdPayload.cmdId = I3C_CCC_DISEC(true); /* disable all events broadcast command*/
    cmdPayload.read  = false;               /* send(write) action*/
    cmdPayload.data  = &defByte;
    cmdPayload.dataLength = sizeof(defByte);

    if (i3c_ll_SendXferCommand(instance, &cmdPayload, 1, isI2c) != 0)
    {
        return;
    }


    /* perform DASA*/
    i3c_PerformDASA(instance, &requireDAA);

    if (requireDAA == true)
    {
        i3c_PerformDAA(instance);
    }
}

/**
 * @brief Get the assigned dynamic address of the device identified by its PID.
 *
 * @param[in] instance     Instance of the I3C controller.
 * @param[in] pDevice      Pointer to the device object. dynamicAddress field of this struct
 *                         would be updated if a matching deviceID is found in the controller.
 * @return 0               If success.
 * @return -EINVAL         If deviceID is not found in the list of devices managed by controller.
 */
static int32_t i3c_ValidateI2cDeviceAddress(uint8_t instance,
        struct i3c_I3cDevice *pI3cDevice)
{
    uint8_t idx;
    int32_t ret = 0;


    for (idx = 0; idx < i3c_Obj[instance].numAttachedDev; idx++)
    {
        if (i3c_Obj[instance].i3cDevDescList[idx].device.deviceId == 0U)
        {
            if (i3c_Obj[instance].i3cDevDescList[idx].device.staticAddress ==
                    pI3cDevice->staticAddress)
            {
                break;
            }
        }
    }
    if (idx >= i3c_Obj[instance].numAttachedDev)
    {
	ERROR("Invalid device");
        ret = -EINVAL;
    }

    return ret;
}

/**
 * @brief Retrieve the dynamic address assigned to the device identified by its PID.
 *
 * @param[in] instance     Instance of the I3C controller.
 * @param[in] pDevice      Pointer to the device object. dynamicAddress field of this struct
 *                         would be updated if a matching deviceID is found in the controller.
 * @return 0               If success.
 * @return -EINVAL         If deviceID is not found in the list of devices managed by controller.
 */
static int32_t i3c_GetDeviceDynAddress( uint8_t instance,
        struct i3c_I3cDevice *pDevice )
{
    uint8_t idx;
    int32_t ret = 0;

    for (idx = 0; idx < i3c_Obj[instance].numAttachedDev; idx++)
    {
        if (i3c_Obj[instance].i3cDevDescList[idx].device.deviceId ==
                pDevice->deviceId)
        {
            pDevice->dynamicAddress =
                    i3c_Obj[instance].i3cDevDescList[idx].device.dynamicAddress;
            break;
        }

    }
    if (idx >= i3c_Obj[instance].numAttachedDev)
    {
	ERROR("Invalid device");
        ret = -EINVAL;
    }

    return ret;
}


int32_t i3c_open(uint8_t instance)
{
    if (instance >= I3C_NUM_INSTANCES)
    {
        return -EINVAL;
    }

    /* initialize the controller instance( i3c_controller[instance]) */
    (void) memset((void *)&i3c_Obj[instance], 0, sizeof(struct i3c_DriverObj));

    /* initialise the address allotment table */
    i3c_InitAddressTable(instance);

    /* initialise the Transfer semaphore*/
    i3c_Obj[instance].xferComplete    =
            osal_semaphore_create(&(i3c_Obj[instance].xferSemMem));

    /* mutex for Mutual exclusion for controller operations */
    i3c_Obj[instance].lock =
            osal_semaphore_create(&(i3c_Obj[instance].xLockMem));

    if (i3c_AssignOwnDA(instance) != 0)
    {
        return -EBUSY;
    }

    /* set up the pin mux and other resources,
     * also enable power and clock to the controller*/
    i3c_ll_init(instance);

    return 0;
}

int32_t i3c_ioctl(uint8_t instance, enum i3c_IoctlRequest ioctl, void *pArgs )
{
    int32_t status = 0;

    switch (ioctl)
    {
    case I3C_IOCTL_TARGET_ATTACH:
	INFO("Attaching I3C devices");
        struct i3c_DevList *pDevices = (struct i3c_DevList *)pArgs;

        status = i3c_AddDevices( instance, pDevices );
        break;

    case I3C_IOCTL_BUS_INIT:
	INFO("Initializing the bus");
        i3c_init_bus(instance);
        break;

    case I3C_IOCTL_DO_DAA:
	INFO("Performing DAA");
        i3c_PerformDAA(instance);
        break;

    case I3C_IOCTL_GET_DYNADDRESS:
	INFO("Fetching I3C dynamic address");
        struct i3c_I3cDevice *pDevice = (struct i3c_I3cDevice *)pArgs;

        status = i3c_GetDeviceDynAddress( instance, pDevice );
        break;

    case I2C_IOCTL_ADDRESS_VALID:
	INFO("Vaildating I2C address");
        struct i3c_I3cDevice *pI3cDevice = (struct i3c_I3cDevice *)pArgs;
        status = i3c_ValidateI2cDeviceAddress(instance, pI3cDevice);
        break;

    default:
	INFO("Invalid IOCTL command");
        status = -EINVAL;
        break;
    }

    return status;
}

int32_t i3c_TransferSync(uint8_t instance, uint8_t address,
        struct i3c_XferRequest *pXferRequest, uint8_t numXfers,
        bool isI2c )
{
    int32_t ret = 0;
    uint16_t totalRx = 0, totalTx = 0;
    uint8_t idx,i;

    struct i3c_CmdPayload cmdPayload[I3C_MAX_XFER];

    if ((numXfers > I3C_MAX_XFER) ||
            (numXfers > i3c_Obj[instance].cmdFIFODepth))
    {
	ERROR("Invalid transfer argument");
        ret = -EINVAL;
    }

    for (idx = 0; (ret == 0) && (idx < i3c_Obj[instance].numAttachedDev);
            idx++)
    {
        if (i3c_Obj[instance].attachedDevAddrList[idx] == address)
        {
            break;
        }
    }

    /* if the device is part of the attach device of the controller, perform the transaction*/
    if ((ret == 0) && (idx <= i3c_Obj[instance].numAttachedDev))
    {
        for (i = 0; i < numXfers; i++)
        {
            /* prepare the command payload*/
            cmdPayload[i].cmdId         = 0;
            cmdPayload[i].targetAddress = address;
            cmdPayload[i].read          = pXferRequest[i].read;               /* read transfer if true, write otherwise*/
            cmdPayload[i].data          = pXferRequest[i].buffer;
            cmdPayload[i].dataLength    = pXferRequest[i].length;

            if (pXferRequest[i].read == true)
            {
                totalRx += pXferRequest[i].length;
            }
            else
            {
                totalTx += pXferRequest[i].length;
            }
        }
	INFO("Data transfer started");
        ret = i3c_ll_SendXferCommand(instance, &cmdPayload[0],
                numXfers /* number of commands*/, isI2c);
	if(ret != 0)
	{
		ERROR("Data Transfer failed");
		return -EIO;
	}
	INFO("Data transfer completed");

        for (i = 0; i < numXfers; i++)
        {
            if (cmdPayload[i].read == true)
            {
                /* update the actual legnth read */
                pXferRequest[i].length = cmdPayload[i].dataLength;
            }
        }
    }
    return ret;
}


/* end of file */
