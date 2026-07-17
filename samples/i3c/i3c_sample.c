/*
 * SPDX-FileCopyrightText: Copyright (C) 2025 Altera Corporation
 *
 * SPDX-License-Identifier: MIT-0
 *
 * Sample application for I3c
 */

/**
 * @file i3c_sample.c
 * @brief Sample Application for I3C
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
 * @defgroup i3c_sample I3C
 * @ingroup samples
 *
 * Sample Application for I3C
 *
 * @details
 * @section i3c_desc Description
 * This is a sample application to demonstrate the usage of I3C driver.
 * It communicates with an NXP P3T1755 (LM75-style) temperature sensor on the
 * I3C bus, either as a native I3C device (enumerated by dynamic address
 * assignment) or as a legacy I2C device, selected by @c SENSOR_MODE_I3C_NATIVE.
 * It programs the THIGH/TLOW limit registers, reads them back to confirm, then
 * reads the ambient temperature a few times.
 *
 * @section i3c_pre Prerequisites
 * - A P3T1755 (or compatible) sensor connected to the I3C bus. In legacy I2C
 *   mode, the slave address shall be updated in this file if different.
 *
 * @section i3c_param Configurable Parameters
 * - The connected_devices structure shall be initialized with the correct device properties
 * - The sensor transport can be selected with @c SENSOR_MODE_I3C_NATIVE.
 * - The sensor slave address can be configured in @c SENSOR_ADDRESS macro.
 * - The I2C test can be disabled or enabled with macro @c ENABLE_I2C_I3C_TEST .
 *
 * @section i3c_how_to How to Run
 * 1. Follow the common README instructions to build and flash the application.
 * 2. Run the application on the board with the sensor connected.
 * 3. Observe the UART terminal for status messages.
 *
 * @section i3c_result Expected Results
 * - THIGH 80 C and TLOW 75 C are written, confirmed, and a plausible ambient
 *   temperature is printed.
 */

/* I2C test macro */
#define ENABLE_I2C_I3C_TEST    1
#define ENABLE_I3C_IBI_SAMPLE 0

/* Native LPS27HHW I3C sensor test (kept for reference, disabled by default) */
#define ENABLE_I3C_NATIVE_SENSOR_TEST 0

/* Access the P3T1755 as a native I3C device (1) or a legacy I2C device (0) */
#define SENSOR_MODE_I3C_NATIVE  1

/*
 * P3T1755 48-bit I3C Provisional ID. Manufacturer 0x011B, device 0x152A; the
 * low bits depend on the address-pin strapping. Only used in native I3C mode;
 * the value only needs to be non-zero and consistent for dynamic-address
 * lookup (DAA assigns the address; the driver matches this device_id).
 */
#define SENSOR_I3C_PID   0x0236152A0098ULL

#define I3C_INSTANCE      1
#define DEV_ADDRESS       0
#define NUM_TEST_BYTES    64

/* LPS27HHW register addresses */
#define WHOAMI    0x0F
#define IF_CTRL    0x0E

#define CTRL_1    0x10
#define CTRL_2    0x11
#define CTRL_3    0x12

#define TEMP_OUT_L    0x2B
#define TEMP_OUT_H    0x2C

#define STATUS    0x27

/* LPS27HHW Device ID */
#define LPS27HHW_DEV_ID      0xB3

#define IBI_PAYLOAD_MAX            32U

/* P3T1755 sensor I2C slave address */
#define SENSOR_ADDRESS       0x4C

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

#if ENABLE_I3C_IBI_SAMPLE
static volatile bool ibi_pending;
static uint8_t ibi_payload[IBI_PAYLOAD_MAX];
static volatile uint8_t ibi_payload_len;
static volatile uint8_t ibi_status;
static volatile uint8_t ibi_id;
static osal_semaphore_def_t ibi_event_sem_mem;
static osal_semaphore_t ibi_event_sem;
#endif

static i3c_handle_t g_i3c_handle;

/* sensor transport, set up in i3c_task(): static I2C address or dynamic I3C */
static uint16_t g_sensor_addr;
static bool g_sensor_is_i2c;

/* Structure to describe the devices connected to the I3C bus. I3C controller supports both
 * I3C devices and legacy I2C devices. For I3C devices both the static address and
 * preferred dynamic address is set as zero and  device_id is set with the PID of the slave device.
 * For legacy I2C device the PID is set a zero and the static address field is set with the address
 * of the device.
 */
struct i3c_device connected_devices[] =
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

#if ENABLE_I3C_IBI_SAMPLE
static void i3c_ibi_callback(uint8_t ibi_id_cb, uint8_t ibi_status_cb,
        const uint8_t *payload, uint8_t payload_len, void *param)
{
    (void)param;
    uint8_t copy_len = payload_len;
    if (copy_len > IBI_PAYLOAD_MAX)
    {
        copy_len = IBI_PAYLOAD_MAX;
    }

    for (uint8_t i = 0; i < copy_len; i++)
    {
        ibi_payload[i] = payload[i];
    }

    ibi_payload_len = copy_len;
    ibi_status = ibi_status_cb;
    ibi_id = ibi_id_cb;
    ibi_pending = true;
    (void)osal_semaphore_post(ibi_event_sem);
}
#endif

/**
 * @brief Read a sensor register over the legacy I2C path.
 *
 *    Two transfer requests are sent. The first writes the 1-byte pointer
 *    (register address), the second reads the register content. The 'is_i2c'
 *    flag marks this as a legacy I2C device on the I3C bus.
 *
 * @param[in]  reg_address Pointer register value.
 * @param[out] data        Destination buffer.
 * @param[in]  num_bytes   Number of bytes to read.
 * @return 0 on success, negative error code otherwise.
 */
int sensor_i3c_i2c_read(uint8_t reg_address, uint8_t *data, uint32_t num_bytes)
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

/*
 * @brief Write to a sensor register over the legacy I2C path.
 *
 *    One transfer request carries the 1-byte pointer (register address)
 *    followed by the data bytes. The 'is_i2c' flag marks this as a legacy
 *    I2C device on the I3C bus.
 *
 * @param[in] reg_address Pointer register value.
 * @param[in] data        Data bytes to write.
 * @param[in] num_bytes   Number of data bytes.
 * @return 0 on success, negative error code otherwise.
 */
int sensor_i3c_i2c_write(uint8_t reg_address, uint8_t *data, uint8_t num_bytes)
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

#if ENABLE_I3C_NATIVE_SENSOR_TEST
/*
 *  @brief Read lps27hhw device register.
 *
 *    The lps27hhw sensor is an I3C slave. The I3C controller communicates with the
 *    sensor using the dynamic address allocated to the device by the I3C controller.
 *    The read operation is carried out using two transfer requests. The first one is
 *    a write request to send the register address. The second request is a read
 *    transaction to get the register value. To indicate that the device is not a
 *    legacy I2C device the 'is_i2c' flag is set as false.
 */
int lps27hhw_read_register(uint8_t reg_address, uint8_t *data,
        uint8_t num_bytes)
{
    int32_t retval = 0;
    struct i3c_xfer_request xfer_request[2];
    bool is_i2c = false;

    xfer_request[0].buffer = &reg_address;
    xfer_request[0].length = sizeof(reg_address);
    xfer_request[0].read = 0;

    memset(data, 0, num_bytes);
    xfer_request[1].buffer = data;
    xfer_request[1].length = num_bytes;
    xfer_request[1].read = 1;

    retval = i3c_transfer_sync(g_i3c_handle,
            connected_devices[0].dynamic_address,
            xfer_request, 2, is_i2c);

    return retval;
}

/*
 * @brief Write to lps27hhw device.
 *
 *    The lps27hhw sensor is an I3C slave. The I3C controller communicates with the
 *    sensor using the dynamic address  allocated to the device by the I3C controller.
 *    The write operation is carried out using one write transfer request. This is a
 *    write request with register address and write data. To indicate that the
 *    device is not a legacy I2C device the 'is_i2c' flag is to be set as false.
 */
int lps27hhw_write_register(uint8_t reg_address, uint8_t *data,
        uint8_t num_bytes)
{
    int32_t retval = 0;
    uint8_t xfer_data[10], i;
    bool is_i2c = false;

    struct i3c_xfer_request xfer_request;

    xfer_data[0] = reg_address;
    for (i = 0; i < num_bytes; i++)
    {
        xfer_data[1 + i] = data[i];
    }

    xfer_request.buffer = xfer_data;
    xfer_request.length = num_bytes + 1; /* for register address */
    xfer_request.read = 0; /* write request */

    retval = i3c_transfer_sync(g_i3c_handle,
            connected_devices[0].dynamic_address,
            &xfer_request, 1, is_i2c);

    return retval;
}
#endif /* ENABLE_I3C_NATIVE_SENSOR_TEST */

void i3c_task(void)
{
    PRINT("Starting I3C sample application.");
    struct i3c_dev_list dev_list =
    {
        .num_devices = 1, .list =
                connected_devices,
    };

    int32_t retval;
    i3c_handle_t hi3c;
#if ENABLE_I3C_NATIVE_SENSOR_TEST
    uint8_t reg_address = 0x0F;
    uint8_t whoam_i = 0, data[4];
    uint32_t itr = 0;
#endif

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
#ifdef ENABLE_I2C_I3C_TEST
    {
        bool sensor_ready = false;
        uint8_t buf[2];
        uint32_t i;

        PRINT("Starting sensor test.");
#if SENSOR_MODE_I3C_NATIVE
        PRINT("Getting I3C dynamic address ...");
        retval = i3c_ioctl(hi3c, I3C_IOCTL_GET_DYNADDRESS,
                &connected_devices[0]);
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
        retval = i3c_ioctl(hi3c, I2C_IOCTL_ADDRESS_VALID,
                &connected_devices[0]);
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
        retval = sensor_i3c_i2c_write(REG_CONFIG, buf, 1);
        if (retval != 0)
        {
            ERROR("Config write failed");
        }

        /* program THIGH = 80 C and read back to confirm */
        buf[0] = THIGH_MSB;
        buf[1] = THIGH_LSB;
        retval = sensor_i3c_i2c_write(REG_THIGH, buf, 2);
        if ((retval == 0) &&
                (sensor_i3c_i2c_read(REG_THIGH, buf, 2) == 0))
        {
            print_temp("THIGH", buf);
            if ((buf[0] == THIGH_MSB) && (buf[1] == THIGH_LSB))
            {
                PRINT("THIGH write confirmed");
            }
            else
            {
                ERROR("THIGH write mismatch");
            }
        }

        /* program TLOW = 75 C and read back to confirm */
        buf[0] = TLOW_MSB;
        buf[1] = TLOW_LSB;
        retval = sensor_i3c_i2c_write(REG_TLOW, buf, 2);
        if ((retval == 0) &&
                (sensor_i3c_i2c_read(REG_TLOW, buf, 2) == 0))
        {
            print_temp("TLOW ", buf);
            if ((buf[0] == TLOW_MSB) && (buf[1] == TLOW_LSB))
            {
                PRINT("TLOW write confirmed");
            }
            else
            {
                ERROR("TLOW write mismatch");
            }
        }

        PRINT("Reading temperature for %d iterations ...", NUM_READINGS);
        for (i = 0; i < NUM_READINGS; i++)
        {
            retval = sensor_i3c_i2c_read(REG_TEMP, buf, 2);
            if (retval != 0)
            {
                ERROR("Temperature read failed");
                break;
            }
            print_temp("Temperature", buf);
            osal_task_delay(500);
        }
        }
    }
    PRINT("Sensor test completed.");
#endif
#if ENABLE_I3C_NATIVE_SENSOR_TEST
    PRINT("Starting I3C test");
    /*Get the assigned dynamic address of the I3C device*/
    PRINT("Getting I3C slave dynamic address ...");
    retval = i3c_ioctl(hi3c, I3C_IOCTL_GET_DYNADDRESS,
            &connected_devices[0]);
    if (retval != 0)
    {
        PRINT("Dynamic address not assigned");
    }
    else
    {
        PRINT("Done.");
        /* Performs read transfer of the WHOAMI register of LPS27HHW device
         * The read is synchronous in nature*/
        reg_address = WHOAMI;
        whoam_i = 0;
        PRINT("Getting device ID ...");
        retval = lps27hhw_read_register(reg_address, &whoam_i, 1);
        if (retval != 0)
        {
            ERROR("Read Transfer not successful");
        }
        else
        {
            PRINT("LPS27HHW Device ID received : 0x%x, Actual= 0x%x", whoam_i,
                    LPS27HHW_DEV_ID);
        }

        /*
         * perform SW reset
         */

        reg_address = CTRL_2;
        memset(data, 0, sizeof(data));
        retval = lps27hhw_read_register(reg_address, &data[0], 1);

        reg_address = CTRL_2;
        data[0] |= 0x4; /* CTRL 2 val */
        retval = lps27hhw_write_register(reg_address, &data[0], 1);

        do
        {
            reg_address = CTRL_2;
            data[0] = 0;
            retval = lps27hhw_read_register(reg_address, &data[0], 1);
        } while (data[0] & 0x4);   /* wait until SW bit is cleared */

        reg_address = IF_CTRL;
        data[0] = (uint8_t)((1U << 7) | (1U << 2) | (1U << 0));
        retval = lps27hhw_write_register(reg_address, &data[0], 1);
        if (retval != 0)
        {
            ERROR("Failed to configure IF_CTRL");
        }

        reg_address = CTRL_1;
        data[0] = 0x12; /* SET BDU and ODR */
        retval = lps27hhw_write_register(reg_address, &data[0], 1);

        /* read back ctrl_1 */
        osal_task_delay(10);
        reg_address = CTRL_1;
        data[0] = 0;
        retval = lps27hhw_read_register(reg_address, &data[0], 1);

        PRINT("Reading temperature sensor value for 10 iterations ...");
        /* read temperature continuously for 10 times */
        while (itr < 10)
        {
            /* read the status register to check if new temp data is generated */
            reg_address = STATUS;
            data[0] = 0;
            retval = lps27hhw_read_register(reg_address, &data[0], 1);
            osal_task_delay(100);
            if ((retval == 0) && (data[0] & 0x2))   /* bit 1 indicates new temp data */
            {
                /* read temp reading*/
                reg_address = TEMP_OUT_L;
                data[0] = 0;
                retval = lps27hhw_read_register(reg_address, &data[0], 1);
                osal_task_delay(100);
                if (retval != 0)
                {
                    ERROR("Write Transfer not successful");
                }
                else
                {
                    reg_address = TEMP_OUT_H;
                    data[1] = 0;
                    retval = lps27hhw_read_register(reg_address, &data[1], 1);
                    if (retval != 0)
                    {
                        ERROR("Write Transfer not successful");
                    }
                    else
                    {
                        PRINT("Temperature reading =  %d *C",
                                ((data[1] << 8) | (data[0])) / 100);
                    }
                }
            }
            osal_task_delay(1000);
            itr++;
        }
        PRINT("Done.");

#if ENABLE_I3C_IBI_SAMPLE
        bool ibi_ok = true;

        reg_address = CTRL_3;
        data[0] = 0x04; /* DRDY enable */
        retval = lps27hhw_write_register(reg_address, &data[0], 1);
        if (retval != 0)
        {
            ERROR("Failed to configure DRDY");
            ibi_ok = false;
        }

        if (ibi_ok)
        {
            ibi_event_sem = osal_semaphore_create(&ibi_event_sem_mem);
            if (ibi_event_sem == NULL)
            {
                ERROR("Failed to create IBI semaphore");
                ibi_ok = false;
            }
        }

        if (ibi_ok)
        {
            struct i3c_ibi_config ibi_cfg = {
                .dev = &connected_devices[0],
                .enable = true
            };
            i3c_set_ibi_callback(hi3c, i3c_ibi_callback, NULL);
            retval = i3c_ioctl(hi3c, I3C_IOCTL_CONFIG_IBI, &ibi_cfg);
            if (retval != 0)
            {
                ERROR("Failed to enable IBI");
                ibi_ok = false;
            }
        }

        if (ibi_ok)
        {
            itr = 0;
            ibi_pending = false;
            PRINT("Reading temperature via IBI for 10 events ...");
            while (itr < 10)
            {
                if (osal_semaphore_wait(ibi_event_sem, 2000U) == false)
                {
                    ERROR("Timed out waiting for IBI event");
                    break;
                }

                if (ibi_pending)
                {
                    ibi_pending = false;
                    PRINT("IBI: id=0x%02X status=0x%02X len=%u",
                            ibi_id, ibi_status, ibi_payload_len);
                    osal_task_delay(1U);
                    reg_address = STATUS;
                    data[0] = 0;
                    retval = lps27hhw_read_register(reg_address, &data[0], 1);
                    if (retval != 0)
                    {
                        ERROR("Status read failed after IBI (%d)", retval);
                        continue;
                    }

                    if ((retval == 0) && (data[0] & 0x2))
                    {
                        reg_address = TEMP_OUT_L;
                        data[0] = 0;
                        retval = lps27hhw_read_register(reg_address, &data[0], 1);
                        if (retval == 0)
                        {
                            reg_address = TEMP_OUT_H;
                            data[1] = 0;
                            retval = lps27hhw_read_register(reg_address, &data[1], 1);
                        }
                        if (retval != 0)
                        {
                            ERROR("Read Transfer not successful (%d)", retval);
                        }
                        else
                        {
                            PRINT("Temperature reading =  %d *C",
                                    ((data[1] << 8) | (data[0])) / 100);
                        }
                        itr++;
                    }
                }
            }
            (void)osal_semaphore_delete(ibi_event_sem);
            ibi_event_sem = NULL;
            PRINT("Done.");
        }
#endif
        PRINT("I3C test completed");
    }
#endif /* ENABLE_I3C_NATIVE_SENSOR_TEST */
    (void)i3c_close(hi3c);
    PRINT("I3C sample completed.");
}
