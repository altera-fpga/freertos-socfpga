/*
 * SPDX-FileCopyrightText: Copyright (C) 2025 Altera Corporation
 *
 * SPDX-License-Identifier: MIT-0
 *
 * Implementation of CLI commands for I3C bus
 */

/**
 * @defgroup cli_i3c I3C
 * @ingroup cli
 *
 * Perform operations on I3C bus
 *
 * @details
 * It supports the following commands:
 * - i3c list
 * - i3c attach <instance> <device_id>
 * - i3c write <instance> <device_id> <target_address> <data>...
 * - i3c read  <instance> <device_id> <target_address> <size>
 * - i3c help
 *
 * Typical usage:
 * - Use 'i3c list' once to get the slave device IDs.
 * - Use 'i3c attach' to attach the desired slave to the I3C bus.
 * - Use 'i3c write' to perform data write operations.
 * - Use 'i3c read' to perform data read operations.
 *
 * @subsection commands Commands
 * @subsubsection i3c_list i3c list
 * List I3C slave devices  <br>
 *
 * Usage:  <br>
 *   i3c list  <br>
 *
 * No arguments.  <br>
 *
 * @subsubsection i3c_attach i3c attach
 * Attach slave to I3C bus  <br>
 *
 * Usage:  <br>
 *   i3c attach <instance> <device_id>  <br>
 *
 * It requires the following arguments:
 * - instance -     The instance of the I3C driver. Valid values are 0 and 1.  <br>
 * - device_id -    The ID of the slave device. Use `i3c list` to get the ID.  <br>
 *
 * @subsubsection i3c_write i3c write
 * Write data to a given memory location  <br>
 *
 * Usage:  <br>
 *   i3c write <instance> <device_id> <target_address> <data>...  <br>
 *
 * It requires the following arguments:
 * - instance -        The instance of the I3C driver. Valid values are 0 and 1.  <br>
 * - device_id -       The ID of the slave device. Use `i3c list` to get the ID.  <br>
 * - target_address -  The address of the memory location (hex 0x00–0x3FFFF).  <br>
 * - data -            One or more data bytes to write (hex 0x00–0xFF).  <br>
 *
 * @subsubsection i3c_read i3c read
 * Read data from a given memory location  <br>
 *
 * Usage:  <br>
 *   i3c read <instance> <device_id> <target_address> <size>  <br>
 *
 * It requires the following arguments:
 * - instance -        The instance of the I3C driver. Valid values are 0 and 1.  <br>
 * - device_id -       The ID of the slave device. Use `i3c list` to get the ID.  <br>
 * - target_address -  The address of the memory location (hex 0x00–0x3FFFF).  <br>
 * - size -            Number of bytes to read.  <br>
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <FreeRTOS.h>
#include <socfpga_uart.h>
#include <FreeRTOS_CLI.h>
#include "cli_app.h"
#include "socfpga_i3c.h"

#define LIST_CMD             ("list")
#define ADD_I2C_CMD          ("add_i2c")
#define ADD_I3C_CMD          ("add_i3c")
#define READ_CMD             ("read")
#define WRITE_CMD            ("write")
#define INIT_CMD             ("init")
#define HELP_CMD             ("help")

#define CLI_I3C_FAIL         1

#define CTRL_1               0x10
#define CTRL_2               0x11
#define CTRL_3               0x12

#define DEV_ADDRESS          0
#define LPS27HHW_DEV_ID      0xB3
#define EEPROM_ADDRESS       0x50
#define EEPROM_START_ADDR    0x00
#define WHOAMI               0x0F
#define MAX_48BIT            ((1ULL << 48) - 1)

uint8_t i3c_wrBuf[100] = {0};
uint8_t i3c_rdBuf[100] = {0};
uint8_t XferData[100] = {0};
struct i3c_XferRequest XferCmd[2] = {0};
int numDev = 0;
int initialized = 0;

struct i3c_I3cDevice connected_devices[] =
{
    {
        .staticAddress = DEV_ADDRESS,
        .deviceId = 0x0,
        .preferredDynamicAddress = 0
    },

    {
        .staticAddress = 0x0,
        .deviceId = 0x0,
        .preferredDynamicAddress = 0
    },
};

struct i3c_DevList DevList = {0};

int is_48bit_unsigned_hex(const char *hex_str)
{
    char *end;
    unsigned long long val = strtoull(hex_str, &end, 0);

    if (*end != '\0')
    {
        return 0;
    }
    return val <= MAX_48BIT;
}

BaseType_t cmd_i3c( char *pcWriteBuffer, size_t xWriteBufferLen,
        const char *pcCommandString )
{
    (void) xWriteBufferLen;
    (void) pcWriteBuffer;

    const char *pcParam1;
    const char *pcParam2;
    const char *pcParam3;
    const char *pcParam4;
    const char *pcParam5;
    BaseType_t Param1StrLen;
    BaseType_t Param2StrLen;
    BaseType_t Param3StrLen;
    BaseType_t Param4StrLen;
    BaseType_t Param5StrLen;

    char tempStr[8] = { 0 };
    char helpStr[5] = { 0 };
    uint32_t instance;
    uint8_t targ_addr;
    uint32_t size = 0;
    uint32_t idx = 0;
    uint64_t devId = 0;
    uint16_t dynamicAddr = 0;

    DevList.numDevices = 2;
    DevList.list = connected_devices;

    pcParam1 = FreeRTOS_CLIGetParameter(pcCommandString, 1, &Param1StrLen);
    strncpy(tempStr, pcParam1, Param1StrLen);

    if (strcmp(tempStr, LIST_CMD) == 0)
    {
        pcParam2 = FreeRTOS_CLIGetParameter(pcCommandString, 2, &Param2StrLen);
        strncpy(tempStr, pcParam2, Param2StrLen);
        if (strcmp(tempStr, HELP_CMD) == 0)
        {
            printf("\rList the available slave devices."
                    "\r\n\nUsage:"
                    "\r\n  i3c list");
            return pdFALSE;
        }
        printf("I3C device list:\r\n");
        if ((DevList.list[0].deviceId == 0) &&
                (DevList.list[1].staticAddress == 0))
        {
            printf("Devices not added");
            return pdFALSE;
        }
        if (DevList.list[1].staticAddress != 0)
        {
            printf(" Legacy I2C Device %d: \r\n", 1);
            printf(
                    "\tDevice ID: 0x%x\r\n\tStatic Address: 0x%x\r\n\tPreferred Dynamic Address: 0x%x\r\n",
                    (uint32_t) DevList.list[1].deviceId, DevList.list[1].staticAddress,
                    DevList.list[1].preferredDynamicAddress);
        }
        if ((DevList.list[0].deviceId != 0))
        {
            printf("I3C Device %d: \r\n", 0);
            printf(
                    "\tDevice ID: 0x%x\r\n\tStatic Address: 0x%x\r\n\tPreferred Dynamic Address: 0x%x\r\n",
                    (uint32_t) DevList.list[0].deviceId, DevList.list[0].staticAddress,
                    DevList.list[0].preferredDynamicAddress);
        }
        return pdFALSE;
    }
    else if ((strcmp(tempStr, ADD_I2C_CMD)) == 0)
    {
        if ((numDev >= 2))
        {
            printf(
                    "\r\nThe CLI supports maximun two devices(one I2C slave and one I3C slave");
        }
        if (connected_devices[1].staticAddress != 0x0)
        {
            printf("\r\nOne I2C device already added");
            return pdFAIL;
        }
        pcParam2 = FreeRTOS_CLIGetParameter(pcCommandString, 2, &Param2StrLen); //static address

        strncpy(helpStr, pcParam2, Param2StrLen);
        if (strcmp(helpStr, HELP_CMD) == 0)
        {
            printf("\rAdd I2C slave to I3C bus"
                    "\r\n\nUsage:"
                    "\r\n  i3c add_i2c <address>"
                    "\r\n\nIt requires the following arguments:"
                    "\r\n  address     The static address of the I2C device in hex.");
            return pdFALSE;
        }
        else if (pcParam2 == NULL)
        {
            printf("\r\nERROR: Incorrect number of arguments."
                    "\r\nEnter 'i3c add_i2c help' for more information.");
            return pdFAIL;
        }
        int8_t addr = strtol(pcParam2, NULL, 16);
        if (addr < 0)
        {
            printf("\r\nERROR: Invalid address"
                    "\r\nEnter 'i3c add_i2c  help' for more information.");
            return pdFAIL;
        }
        connected_devices[1].staticAddress = strtol(pcParam2, NULL, 16);
        numDev++;
        printf ("\r\nAdded the I2C device");
    }
    else if ((strcmp(tempStr, ADD_I3C_CMD)) == 0)
    {
        if ((numDev >= 2))
        {
            printf(
                    "\r\nThe CLI supports maximun two devices(one I2C slave and one I3C slave");
        }
        if (connected_devices[0].deviceId != 0x0)
        {
            printf("\r\nOne I3C device already added");
            return pdFAIL;
        }
        pcParam2 = FreeRTOS_CLIGetParameter(pcCommandString, 2, &Param2StrLen); //deviceId

        strncpy(helpStr, pcParam2, Param2StrLen);
        if (strcmp(helpStr, HELP_CMD) == 0)
        {
            printf("\rAdd I3C slave to I3C bus"
                    "\r\n\nUsage:"
                    "\r\n  i3c add_i3c <deviceId>"
                    "\r\n\nIt requires The following arguments:"
                    "\r\n  deviceId    48 bit provisional id in hex format");
            return pdFALSE;
        }
        else if (pcParam2 == NULL)
        {
            printf("\r\nERROR: Incorrect number of arguments."
                    "\r\nEnter 'i3c add_i3c help' for more information.");
            return pdFAIL;
        }
        int64_t id = strtol(pcParam2, NULL, 16);
        if (id < 0)
        {
            printf("\r\nERROR: Invalid device id"
                    "\r\nEnter 'i3c add_i3c  help' for more information.");
            return pdFAIL;
        }
        int lret = is_48bit_unsigned_hex(pcParam2);
        if (lret == 0)
        {
            printf("\r\nERROR: Invalid device id"
                    "\r\n Enter 'i3c add_i3c  help' for more information.");
            return pdFAIL;
        }
        connected_devices[0].deviceId = strtol(pcParam2, NULL, 16);
        numDev++;
        printf ("\r\nAdded the I3C device");
    }
    else if (strcmp(tempStr, INIT_CMD) == 0)
    {
        pcParam2 = FreeRTOS_CLIGetParameter(pcCommandString, 2, &Param2StrLen); //instance
        pcParam3 = FreeRTOS_CLIGetParameter(pcCommandString, 3, &Param3StrLen); //devID

        if (numDev == 0)
        {
            printf(
                    "No devices added to the I3C bus. Use add_i3c or add_i2c to add devices");
            return pdFAIL;
        }
        strncpy(helpStr, pcParam2, Param2StrLen);
        if (strcmp(helpStr, HELP_CMD) == 0)
        {
            printf("\rAttach slave to I3C bus"
                    "\r\n\nUsage:"
                    "\r\n  i3c init <instance>"
                    "\r\n\nIt requires the following arguments:"
                    "\r\n  instance     The instance of the I3C driver. Valid values are 0 and 1.");
            return pdFALSE;
        }
        else if (pcParam2 == NULL)
        {
            printf("\r\nERROR: Incorrect number of arguments."
                    "\r\nEnter 'i3c init help' for more information.");
            return pdFAIL;
        }

        int inst = strtol(pcParam2, NULL, 10);
        if (inst < 0)
        {
            printf("\r\nERROR: Incorrect instance number"
                    "\r\nEnter 'i3c init help' for more information.");
            return pdFAIL;

        }
        instance = strtol(pcParam2, NULL, 10);
        if (instance > 2)
        {
            printf("\r\nERROR: Invalid arguments"
                    "\r\nEnter 'i3c init  help' for more information.");
            return pdFAIL;
        }
        if (i3c_open(instance) != 0)
        {
            printf("\r\nI3C instance not initialized");
            return pdFAIL;
        }

        if (i3c_ioctl(instance, I3C_IOCTL_TARGET_ATTACH, &DevList) != 0)
        {
            printf("\r\nI3C Target not attached");
            return pdFAIL;
        }

        if (i3c_ioctl(instance, I3C_IOCTL_BUS_INIT, &connected_devices[0]) != 0)
        {
            printf("\r\nDynamic address not assigned to devices");
            return pdFAIL;
        }
        initialized = 1;
        printf ("\r\nInitialized the I3C bus");
        return pdFALSE;
    }
    else if (strcmp(tempStr, WRITE_CMD) == 0)
    {

        pcParam2 = FreeRTOS_CLIGetParameter(pcCommandString, 2, &Param2StrLen); //instance
        pcParam3 = FreeRTOS_CLIGetParameter(pcCommandString, 3, &Param3StrLen); //devID
        pcParam4 = FreeRTOS_CLIGetParameter(pcCommandString, 4, &Param4StrLen); //target address
        strncpy(helpStr, pcParam2, Param2StrLen);
        if (strcmp(helpStr, HELP_CMD) == 0)
        {
            printf("\rWrite data to a given memory location"
                    "\r\n\nUsage:"
                    "\r\n  i3c write <instance> <device_id> <target_address> <data>..."
                    "\r\n\nIt requires the following arguments:"
                    "\r\n  instance          The instance of the I3C driver. Valid values are 0 and 1."
                    "\r\n  device_id         The ID of the slave device. Use list command to get device ID."
                    "\r\n  target_address    The address of the memory location. It is a hexadecimal value from 0x00 to 0x3FFFF."
                    "\r\n  data              Data written to the memory. User can provide multiple values. Valid data range is from 0x00 to 0xFF.");
            return pdFALSE;
        }
        if (initialized == 0)
        {
            printf("\r\nI3C bus not initilazed. Use i3c init before read/write");
            return pdFAIL;
        }
        else if ((pcParam2 == NULL) || (pcParam3 == NULL) || (pcParam4 == NULL))
        {
            printf("\r\nIncorrect number of arguments."
                    "\r\nEnter 'i3c write help' for more information.");
            return pdFAIL;
        }

        instance = strtol(pcParam2, NULL, 10);
        if (instance > 2)
        {
            printf("\r\nInvalid I3C instance"
                    "\r\nEnter 'i3c write help' for more information.");
            return pdFAIL;
        }
        devId = strtol(pcParam3, NULL, 16);
        int8_t addr = strtol(pcParam4, NULL, 16);
        if (addr < 0)
        {
            printf("\r\nERROR:Invalid address");
            return pdFAIL;
        }
        targ_addr = strtol(pcParam4, NULL, 16);
        idx = 5;
        while (true)
        {
            pcParam5 = FreeRTOS_CLIGetParameter(pcCommandString, idx,
                    &Param5StrLen);
            if (pcParam5 == NULL)
            {
                break;
            }
            i3c_wrBuf[size] = strtol(pcParam5, NULL, 10);
            size++;
            idx++;
        }
        if (devId == 0)
        {
            /* Legacy I2C */
            XferData[0] = (targ_addr >> 8) & 0xFF;
            XferData[1] = targ_addr & 0xFF;
            for (uint32_t i = 0; i < size; i++)
            {
                XferData[2 + i] = i3c_wrBuf[i];
            }
            XferCmd[0].buffer = XferData;
            XferCmd[0].length = size + 2;
            XferCmd[0].read = false;
            if (i3c_TransferSync(instance, EEPROM_ADDRESS, &XferCmd[0], 1,
                    true) != 0)
            {
                printf("\r\nWrite failed. Make sure the device is attached.");
                return pdFAIL;
            }
        }
        else
        {
            /* I3C device */
            for (int i = 0; i < DevList.numDevices; i++)
            {
                if (DevList.list[i].deviceId == devId)
                {
                    dynamicAddr = DevList.list[i].dynamicAddress;
                    break;
                }
            }
            if (dynamicAddr == 0)
            {
                printf("\r\nDevice not found");
                return pdFAIL;
            }

            XferData[0] = targ_addr;
            for (idx = 0; idx < size; idx++)
            {
                XferData[1 + idx] = i3c_wrBuf[idx];
            }
            XferCmd[0].buffer = XferData;
            XferCmd[0].length = size + 1;
            XferCmd[0].read = 0;

            if (i3c_TransferSync(instance, dynamicAddr, &XferCmd[0], 1,
                    false) != 0)
            {
                printf("\r\nWrite failed. Make sure the device is attached.");
                return pdFAIL;
            }
        }
        printf("\r\nWrite successful");
        return pdFALSE;
    }

    else if (strcmp(tempStr, READ_CMD) == 0)
    {
        pcParam2 = FreeRTOS_CLIGetParameter(pcCommandString, 2, &Param2StrLen); //instance
        pcParam3 = FreeRTOS_CLIGetParameter(pcCommandString, 3, &Param3StrLen); //devID
        pcParam4 = FreeRTOS_CLIGetParameter(pcCommandString, 4, &Param4StrLen); //target address
        pcParam5 = FreeRTOS_CLIGetParameter(pcCommandString, 5, &Param5StrLen); //size
        strncpy(helpStr, pcParam2, Param2StrLen);
        if (strcmp(helpStr, HELP_CMD) == 0)
        {
            printf("\rRead data from a given memory location"
                    "\r\n\nUsage:"
                    "\r\n  i3c read <instance> <device_id> <target_address> <size>"
                    "\r\n\nIt requires the following arguments:"
                    "\r\n  instance          The instance of the I3C driver. Valid values are 0 and 1."
                    "\r\n  device_id         The ID of the slave device. Use list command to get device ID."
                    "\r\n  target_address    The address of the memory location. It is a hexadecimal value from 0x00 to 0x3FFFF."
                    "\r\n  size              Number of bytes to read.");
            return pdFALSE;
        }
        if (initialized == 0)
        {
            printf("\r\nI3C bus not initilazed. Use i3c init before read/write");
            return pdFAIL;
        }
        else if ((pcParam2 == NULL) || (pcParam3 == NULL) ||
                (pcParam4 == NULL) || (pcParam5 == NULL))
        {
            printf("\r\nIncorrect number of arguments"
                    "\r\nEnter 'i3c read help' for more information.");
            return pdFAIL;
        }

        int inst = strtol(pcParam2, NULL, 10);
        if (inst < 0)
        {
            printf("\r\nInvalid instance"
                    "\r\nEnter 'i3c read help' for more information.");
            return pdFAIL;
        }
        instance = strtol(pcParam2, NULL, 10);
        if (instance > 2)
        {
            printf("\r\nInvalid I3C instance"
                    "\r\nEnter 'i3c read help' for more information.");
            return pdFAIL;
        }
        int64_t id = strtol(pcParam3, NULL, 16);
        if (id < 0)
        {
            printf("\r\nInvalid device id"
                    "\r\nEnter 'i3c read help' for more information.");
            return pdFAIL;

        }
        devId = strtol(pcParam3, NULL, 16);
        int16_t sz =  strtol(pcParam5, NULL, 10);
        if (sz < 0)
        {
            printf("\r\nInvalid size"
                    "\r\nEnter 'i3c read help' for more information.");
            return pdFAIL;
        }
        size = strtol(pcParam5, NULL, 10);
        int8_t addr =  strtol(pcParam5, NULL, 16);
        if (addr < 0)
        {
            printf("\r\nInvalid address"
                    "\r\nEnter 'i3c read help' for more information.");
            return pdFAIL;
        }
        targ_addr = strtol(pcParam4, NULL, 16);

        if (devId == 0)
        {
            /* Legacy I2C */
            XferData[0] = (targ_addr >> 8) & 0xFF;
            XferData[1] = targ_addr & 0xFF;

            XferCmd[0].buffer = XferData;
            XferCmd[0].length = 2;
            XferCmd[0].read = false;

            XferCmd[1].buffer = i3c_rdBuf;
            XferCmd[1].length = size;
            XferCmd[1].read = true;

            if (i3c_TransferSync(instance, EEPROM_ADDRESS, XferCmd, 2,
                    true) != 0)
            {
                printf("\r\nRead failed. Make sure the device is attached.");
                return pdFAIL;
            }
        }
        else
        {
            /* I3C device */
            int lRet = i3c_ioctl(instance, I3C_IOCTL_GET_DYNADDRESS,
                    &connected_devices[ 0 ]);
            if (lRet != 0)
            {
                printf("\r\nFailed to get the dynamic address");
                return pdFAIL;
            }
            for (int i = 0; i < DevList.numDevices; i++)
            {
                if (DevList.list[i].deviceId == devId)
                {
                    dynamicAddr = DevList.list[i].dynamicAddress;
                    break;
                }
            }
            if (dynamicAddr == 0)
            {
                printf("\r\nDevice not found");
                return pdFAIL;
            }

            XferCmd[0].buffer = &targ_addr;
            XferCmd[0].length = sizeof(targ_addr);
            XferCmd[0].read = 0;

            memset(i3c_rdBuf, 0, size);
            XferCmd[1].buffer = i3c_rdBuf;
            XferCmd[1].length = size;
            XferCmd[1].read = 1;

            if (i3c_TransferSync(instance, dynamicAddr, XferCmd, 2, false) != 0)
            {
                printf("\r\nRead failed. Make sure the device is attached.");
                return pdFAIL;
            }
        }
        printf("\r\n%d bytes data at 0x%x: \n\r", size, targ_addr);
        for (uint32_t i = 0; i < size; i++)
        {
            printf(" %d", i3c_rdBuf[i]);
        }
        return pdFALSE;
    }

    else if (strcmp(tempStr, HELP_CMD) == 0)
    {
        printf("\rPerform operations on I3C bus(CLI supports a maximum of one i2c device and a i3c device )"
                "\r\n\nIt supports the following commands:"
                "\r\n  i3c add_i2c <device_address>"
                "\r\n  i3c add_i3c <device_id>"
                "\r\n  i3c init <instance>"
                "\r\n  i3c list"
                "\r\n  i3c write <instance> <device_id> <target_address> <data>..."
                "\r\n  i3c read <instance> <device_id> <target_address> <size>"
                "\r\n  i3c help"
                "\r\n\nTypical usage:"
                "\r\n- Use add_i2c command once to add I2C slave."
                "\r\n- Use add_i3c command once to add I3C slave."
                "\r\n- Use init command once to initialiaze to I3C bus."
                "\r\n- Use list command once to get the slave device ID."
                "\r\n- Use write/read command to perform desired write/read operations");
        return pdFALSE;
    }
    else
    {
        printf("\r\nERROR: Invalid command for i3c");
        printf("\r\nEnter 'i3c help' for more information.");
        return pdFAIL;
    }
    return pdFALSE;
}
