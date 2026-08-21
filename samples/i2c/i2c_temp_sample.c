/*
 * SPDX-FileCopyrightText: Copyright (C) 2025-2026 Altera Corporation
 *
 * SPDX-License-Identifier: MIT-0
 *
 * Sample application implementation for i2c temperature sensor
 */
#include <string.h>
#include "osal.h"
#include "osal_log.h"
#include <task.h>
#include "socfpga_i2c.h"

/**
 * @defgroup i2c_temp_sample I2C Temperature Sensor
 * @ingroup samples
 *
 * Sample application for the I2C driver using a temperature sensor
 *
 * @details
 * @section i2c_temp_desc Description
 * This sample reads temperature from an NXP P3T1755 (LM75-style) sensor over
 * the I2C bus. It sets continuous conversion, writes the THIGH/TLOW limit
 * registers to 80/75 C and reads them back to confirm, then prints the
 * ambient temperature a few times.
 *
 * @section i2c_temp_pre Prerequisites
 * - A P3T1755 (or LM75-compatible) sensor on the I2C bus.
 *
 * @section i2c_temp_param Configurable Parameters
 * - I2C bus instance: @c I2CBUS
 * - Sensor slave address: @c DEV_ADDR
 * - Number of readings: @c NUM_READINGS
 *
 * @section i2c_temp_how_to How to Run
 * 1. Build with the @c i2c_temp_sample target and flash the application.
 * 2. Run the application on the board.
 * 3. Observe the results in the console.
 *
 * @section i2c_temp_result Expected Results
 * - THIGH 80 C and TLOW 75 C are written, confirmed, and a plausible ambient
 *   temperature is printed.
 */

/* I2C controller instance */
#define I2CBUS    0

/* P3T1755 slave address */
#define DEV_ADDR    0x4C

/* pointer register values */
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

/**
 * @brief Read a sensor register.
 *
 * Sets the pointer register to @p reg, then reads @p n bytes.
 *
 * @param[in]  handle Open I2C handle.
 * @param[in]  reg    Pointer register value.
 * @param[out] buf    Destination buffer.
 * @param[in]  n      Number of bytes to read.
 * @return 0 on success, negative error code otherwise.
 */
static int temp_read_reg(i2c_handle_t handle, uint8_t reg, uint8_t *buf,
        uint32_t n)
{
    int ret = i2c_write_sync(handle, &reg, 1U);
    if (ret != 0)
    {
        return ret;
    }
    return i2c_read_sync(handle, buf, n);
}

/**
 * @brief Write a 16-bit value to a sensor register.
 *
 * @param[in] handle Open I2C handle.
 * @param[in] reg    Pointer register value.
 * @param[in] msb    Most significant byte.
 * @param[in] lsb    Least significant byte.
 * @return 0 on success, negative error code otherwise.
 */
static int temp_write_reg(i2c_handle_t handle, uint8_t reg, uint8_t msb,
        uint8_t lsb)
{
    uint8_t b[3];

    b[0] = reg;
    b[1] = msb;
    b[2] = lsb;
    return i2c_write_sync(handle, b, 3U);
}

/**
 * @brief Decode and print a temperature register value.
 *
 * Interprets @p b as a 12-bit left-justified two's-complement value with a
 * resolution of 0.0625 C per LSB.
 *
 * @param[in] label Text label for the printed line.
 * @param[in] b     Two register bytes, most significant first.
 */
static void temp_print(const char *label, const uint8_t *b)
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
 * @brief Program a limit register and read it back to confirm the write.
 *
 * @param[in] handle Open I2C handle.
 * @param[in] reg    Pointer register value.
 * @param[in] label  Text label for the printed line.
 * @param[in] msb    Most significant byte to write.
 * @param[in] lsb    Least significant byte to write.
 * @return 0 on success, negative error code otherwise.
 */
static int temp_set_limit(i2c_handle_t handle, uint8_t reg, const char *label,
        uint8_t msb, uint8_t lsb)
{
    uint8_t buf[2];
    int ret;

    ret = temp_write_reg(handle, reg, msb, lsb);
    if (ret != 0)
    {
        return ret;
    }
    ret = temp_read_reg(handle, reg, buf, 2U);
    if (ret != 0)
    {
        return ret;
    }
    temp_print(label, buf);
    if ((buf[0] != msb) || (buf[1] != lsb))
    {
        ERROR("%s write mismatch", label);
        return -1;
    }
    PRINT("%s write confirmed", label);
    return 0;
}

/**
 * @brief I2C sample task: read temperature from a P3T1755 sensor.
 */
void i2c_temp_task(void)
{
    int retval;
    i2c_handle_t handle;
    i2c_config_t config;
    uint16_t slave_addr;
    uint8_t buf[2];
    uint32_t i;

    PRINT("P3T1755 temperature sensor sample (I2C)");

    PRINT("Configuring the i2c as master ...");

    handle = i2c_open(I2CBUS);
    if (handle == NULL)
    {
        ERROR("Cannot open the i2c instance");
        return;
    }

    config.clk = I2C_STANDARD_MODE_BPS;
    retval = i2c_ioctl(handle, I2C_SET_MASTER_CFG, &config);
    if (retval != 0)
    {
        ERROR("Configuring i2c speed failed");
        i2c_close(handle);
        return;
    }

    slave_addr = DEV_ADDR;
    retval = i2c_ioctl(handle, I2C_SET_SLAVE_ADDR, (void *)(&slave_addr));
    if (retval != 0)
    {
        ERROR("Configuring slave address failed");
        i2c_close(handle);
        return;
    }

    PRINT("Configuration done.");

    /* continuous conversion mode */
    buf[0] = REG_CONFIG;
    buf[1] = 0x00;
    retval = i2c_write_sync(handle, buf, 2U);
    if (retval != 0)
    {
        ERROR("Config write failed");
        i2c_close(handle);
        return;
    }

    /* program the alert limits and read them back to confirm */
    (void)temp_set_limit(handle, REG_THIGH, "THIGH", THIGH_MSB, THIGH_LSB);
    (void)temp_set_limit(handle, REG_TLOW, "TLOW ", TLOW_MSB, TLOW_LSB);

    for (i = 0; i < NUM_READINGS; i++)
    {
        retval = temp_read_reg(handle, REG_TEMP, buf, 2U);
        if (retval != 0)
        {
            ERROR("Temperature read failed");
            break;
        }
        temp_print("Temperature", buf);
        osal_task_delay(READ_DELAY_MS);
    }

    i2c_close(handle);
    PRINT("I2c sample application completed.");
}
