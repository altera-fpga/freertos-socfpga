/*
 * SPDX-FileCopyrightText: Copyright (C) 2025-2026 Altera Corporation
 *
 * SPDX-License-Identifier: MIT-0
 *
 * Sample application implementation for i3c temperature sensor
 */

/**
 * @file i3c_temp_sample.c
 * @brief Sample Application for I3C using a temperature sensor
 */

#include <FreeRTOS.h>
#include <FreeRTOSConfig.h>
#include <task.h>
#include <stdbool.h>
#include <string.h>
#include <socfpga_i3c.h>
#include "osal.h"
#include "osal_log.h"

/**
 * @defgroup i3c_temp_sample I3C Temperature Sensor
 * @ingroup samples
 *
 * Sample application for the I3C driver using a temperature sensor
 *
 * @details
 * @section i3c_temp_desc Description
 * This is a sample application to demonstrate the usage of I3C driver.
 * It communicates with an NXP P3T1755 (LM75-style) temperature sensor on the
 * I3C bus, either as a native I3C device (enumerated by dynamic address
 * assignment) or as a legacy I2C device, selected by @c SENSOR_MODE_I3C_NATIVE.
 * It programs the THIGH/TLOW limit registers, reads them back to confirm, then
 * reads the ambient temperature a few times.
 *
 * @section i3c_temp_pre Prerequisites
 * - A P3T1755 (or compatible) sensor connected to the I3C bus. In legacy I2C
 *   mode, the slave address shall be updated in this file if different.
 *
 * @section i3c_temp_param Configurable Parameters
 * - The connected_devices structure shall be initialized with the correct device properties
 * - The sensor transport can be selected with @c SENSOR_MODE_I3C_NATIVE.
 * - The sensor slave address can be configured in @c SENSOR_ADDRESS macro.
 * - The number of readings can be configured in @c NUM_READINGS macro.
 *
 * @section i3c_temp_how_to How to Run
 * 1. Build with the @c i3c_temp_sample target and flash the application.
 * 2. Run the application on the board with the sensor connected.
 * 3. Observe the UART terminal for status messages.
 *
 * @section i3c_temp_result Expected Results
 * - THIGH 80 C and TLOW 75 C are written, confirmed, and a plausible ambient
 *   temperature is printed.
 */

/* Access the P3T1755 as a native I3C device (1) or a legacy I2C device (0) */
#define SENSOR_MODE_I3C_NATIVE    1

/*
 * P3T1755 48-bit I3C Provisional ID. Manufacturer 0x011B, device 0x152A; the
 * low bits depend on the address-pin strapping. Only used in native I3C mode;
 * the value only needs to be non-zero and consistent for dynamic-address
 * lookup (DAA assigns the address; the driver matches this device_id).
 */
#define SENSOR_I3C_PID    0x0236152A0098ULL

/* I3C controller instance */
#define I3C_INSTANCE    1

/* P3T1755 sensor I2C slave address */
#define SENSOR_ADDRESS    0x4C

/* P3T1755 pointer register values */
#define REG_TEMP      0x00
#define REG_CONFIG    0x01
#define REG_TLOW      0x02
#define REG_THIGH     0x03

/* limit defaults: 80 C and 75 C (12-bit left-justified, 0.0625 C/LSB) */
#define THIGH_MSB    0x50
#define THIGH_LSB    0x00
#define TLOW_MSB     0x4B
#define TLOW_LSB     0x00

/* number of temperature samples to print */
#define NUM_READINGS    10U

/* delay between samples in ms */
#define READ_DELAY_MS   500U

static i3c_handle_t g_i3c_handle;

/* sensor transport, set up in i3c_temp_task(): static I2C address or dynamic I3C */
static uint16_t g_sensor_addr;
static bool g_sensor_is_i2c;

/* Structure to describe the devices connected to the I3C bus. I3C controller supports both
 * I3C devices and legacy I2C devices. For I3C devices both the static address and
 * preferred dynamic address is set as zero and  device_id is set with the PID of the slave device.
 * For legacy I2C device the PID is set a zero and the static address field is set with the address
 * of the device.
 */
static struct i3c_device connected_devices[] =
{
#if SENSOR_MODE_I3C_NATIVE
    {
        .static_address = 0,
        .device_id = SENSOR_I3C_PID,    /*native I3C device, enumerated by DAA*/
        .preferred_dynamic_address = 0
    },
#else
    {
        .static_address = SENSOR_ADDRESS,
        .device_id = 0x0,               /*PID as zero for legacy I2C devices*/
        .preferred_dynamic_address = 0
    },
#endif
};

/**
 * @brief Decode and print a temperature register value.
 *
 * Interprets @p b as a 12-bit left-justified two's-complement value with a
 * resolution of 0.0625 C per LSB.
 *
 * @param[in] label Text label for the printed line.
 * @param[in] b     Two register bytes, most significant first.
 */
static void print_temp(const char *label, const uint8_t *b)
{
    int16_t raw = (int16_t)(((uint16_t)b[0] << 8) | (uint16_t)b[1]);
    int32_t u = (int32_t)(raw >> 4) * 625;   /* tenths of a milli-degree */
    const char *sign = "";

    if (u < 0)
    {
        u = -u;
        sign = "-";
    }
    PRINT("%s = %s%ld.%04ld C", label, sign, (long)(u / 10000),
            (long)(u % 10000));
}

/**
 * @brief Read a sensor register.
 *
 *    Two transfer requests are sent. The first writes the 1-byte pointer
 *    (register address), the second reads the register content. The 'is_i2c'
 *    flag marks the device as a legacy I2C device on the I3C bus.
 *
 * @param[in]  reg_address Pointer register value.
 * @param[out] data        Destination buffer.
 * @param[in]  num_bytes   Number of bytes to read.
 * @return 0 on success, negative error code otherwise.
 */
static int sensor_read_reg(uint8_t reg_address, uint8_t *data,
        uint32_t num_bytes)
{
    int retval = 0;
    struct i3c_xfer_request xfer_request[2];
    uint8_t xfer_cmd;

    xfer_cmd = reg_address;

    xfer_request[0].buffer = &xfer_cmd;
    xfer_request[0].length = 1;
    xfer_request[0].read = false;

    memset(data, 0, num_bytes);
    xfer_request[1].buffer = data;
    xfer_request[1].length = num_bytes;
    xfer_request[1].read = true;

    retval = i3c_transfer_sync(g_i3c_handle, g_sensor_addr, xfer_request, 2,
            g_sensor_is_i2c);

    return retval;
}

/**
 * @brief Write to a sensor register.
 *
 *    One transfer request carries the 1-byte pointer (register address)
 *    followed by the data bytes. The 'is_i2c' flag marks the device as a
 *    legacy I2C device on the I3C bus.
 *
 * @param[in] reg_address Pointer register value.
 * @param[in] data        Data bytes to write.
 * @param[in] num_bytes   Number of data bytes.
 * @return 0 on success, negative error code otherwise.
 */
static int sensor_write_reg(uint8_t reg_address, uint8_t *data,
        uint8_t num_bytes)
{
    int32_t retval = 0;
    uint8_t xfer_data[num_bytes + 1], i;

    struct i3c_xfer_request xfer_request;

    xfer_data[0] = reg_address;

    for (i = 0; i < num_bytes; i++)
    {
        xfer_data[1 + i] = data[i];
    }
    xfer_request.buffer = xfer_data;
    xfer_request.length = num_bytes + 1;
    xfer_request.read = false;

    retval = i3c_transfer_sync(g_i3c_handle, g_sensor_addr, &xfer_request, 1,
            g_sensor_is_i2c);
    return retval;
}

/**
 * @brief Program a temperature limit register and read it back to confirm.
 *
 * @param[in] reg   Limit pointer register (@c REG_THIGH or @c REG_TLOW).
 * @param[in] label Text label for the printed line.
 * @param[in] msb   Expected most significant byte.
 * @param[in] lsb   Expected least significant byte.
 */
static void sensor_set_limit(uint8_t reg, const char *label, uint8_t msb,
        uint8_t lsb)
{
    uint8_t buf[2];

    buf[0] = msb;
    buf[1] = lsb;
    if ((sensor_write_reg(reg, buf, 2) == 0) &&
            (sensor_read_reg(reg, buf, 2) == 0))
    {
        print_temp(label, buf);
        if ((buf[0] == msb) && (buf[1] == lsb))
        {
            PRINT("%s write confirmed", label);
        }
        else
        {
            ERROR("%s write mismatch", label);
        }
    }
}

void i3c_temp_task(void)
{
    struct i3c_dev_list dev_list =
    {
        .num_devices = 1,
        .list = connected_devices,
    };
    bool sensor_ready = false;
    uint8_t buf[2];
    int32_t retval;
    i3c_handle_t hi3c;
    uint32_t i;

    PRINT("Starting I3C temperature sensor sample application.");

    /*
     * Configure and initialize the i3c controller instances
     * Setup the role of the instance(primary/secondary)
     * Initializes the address allotment table
     */
    hi3c = i3c_open(I3C_INSTANCE);
    if (hi3c == NULL)
    {
        ERROR("i3c instance not initialized");
        return;
    }
    g_i3c_handle = hi3c;

    /* Add the devices connected to the bus */
    PRINT("Attaching the devices to the bus ...");
    retval = i3c_ioctl(hi3c, I3C_IOCTL_TARGET_ATTACH, &dev_list);
    if (retval != 0)
    {
        ERROR("I3C Target not attached");
        (void)i3c_close(hi3c);
        return;
    }
    PRINT("Done.");

    /* Initialize the i3c bus. Performs dynamic address assignment  */
    PRINT("Initializing the I3C bus ...");
    retval = i3c_ioctl(hi3c, I3C_IOCTL_BUS_INIT, &connected_devices[0]);
    if (retval != 0)
    {
        PRINT("Dynamic address not assigned to devices");
    }
    PRINT("Done.");

    PRINT("Starting sensor test.");
#if SENSOR_MODE_I3C_NATIVE
    PRINT("Getting I3C dynamic address ...");
    retval = i3c_ioctl(hi3c, I3C_IOCTL_GET_DYNADDRESS, &connected_devices[0]);
    if (retval != 0)
    {
        PRINT("Dynamic address not assigned");
    }
    else
    {
        g_sensor_addr = connected_devices[0].dynamic_address;
        g_sensor_is_i2c = false;
        PRINT("Done. I3C dynamic address = 0x%02X", g_sensor_addr);
        sensor_ready = true;
    }
#else
    PRINT("Checking if I2C device is valid ...");
    retval = i3c_ioctl(hi3c, I2C_IOCTL_ADDRESS_VALID, &connected_devices[0]);
    if (retval != 0)
    {
        PRINT("Invalid I2C device address");
    }
    else
    {
        g_sensor_addr = SENSOR_ADDRESS;
        g_sensor_is_i2c = true;
        PRINT("Done.");
        sensor_ready = true;
    }
#endif

    if (sensor_ready)
    {
        /* continuous conversion mode */
        buf[0] = 0x00;
        retval = sensor_write_reg(REG_CONFIG, buf, 1);
        if (retval != 0)
        {
            ERROR("Config write failed");
        }

        sensor_set_limit(REG_THIGH, "THIGH", THIGH_MSB, THIGH_LSB);
        sensor_set_limit(REG_TLOW, "TLOW ", TLOW_MSB, TLOW_LSB);

        PRINT("Reading temperature for %d iterations ...", NUM_READINGS);
        for (i = 0; i < NUM_READINGS; i++)
        {
            retval = sensor_read_reg(REG_TEMP, buf, 2);
            if (retval != 0)
            {
                ERROR("Temperature read failed");
                break;
            }
            print_temp("Temperature", buf);
            osal_task_delay(READ_DELAY_MS);
        }
    }
    PRINT("Sensor test completed.");

    (void)i3c_close(hi3c);
    PRINT("I3C temperature sensor sample completed.");
}
