/*
 * Common IO - basic V1.0.0
 * Copyright (C) 2020 Amazon.com, Inc. or its affiliates.  All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * http://aws.amazon.com/freertos
 * http://www.FreeRTOS.org
 */

/*
 * SPDX-FileCopyrightText: Copyright (C) 2025 Altera Corporation
 *
 * SPDX-License-Identifier: MIT-0
 *
 * Header file for gpio driver
 */

#ifndef __SOCFPGA_GPIO_H__
#define __SOCFPGA_GPIO_H__

/**
 * @file    socfpga_gpio.h
 * @brief   This file contains the GPIO HAL API definitions.
 */

#include "socfpga_defines.h"
#include <errno.h>

/**
 * @defgroup gpio GPIO
 * @ingroup drivers
 * @{
 */
/**
 * @defgroup gpio_fns Functions
 * @ingroup gpio
 * GPIO HAL APIs
 */

/**
 * @defgroup gpio_structs Structures
 * @ingroup gpio
 * GPIO Specific Structures
 */

/**
 * @defgroup gpio_enums Enumerations
 * @ingroup gpio
 * GPIO Specific Enumerations
 */

/**
 * @defgroup gpio_macros Macros
 * @ingroup gpio
 * GPIO Specific Macros
 */

/**
 * @addtogroup gpio_macros
 * @{
 */
#define GPIO_MAX_INSTANCE     (48U)    /*!< Number of GPIO pins supported. */
#define GPIO_PINS_PER_REG     (24U)    /*!< Number of GPIO pins having same base address . */
#define GPIO_INSTANCE_ZERO    (0U)     /*!< GPIO instance 0. */
#define GPIO_INSTANCE_ONE     (1U)     /*!< GPIO instance 1. */
#define PINMUX_REG(p)    ((((p) < \
    40U) ? (0x10D13000U) : (0x10D13100U + (4U * (p - 40)))))    /*!< Returns the pinmux register of GPIO pin . */
#define PINMUX_GPIO           (0x08U)    /*!< GPIO functionality bit. */
#define PINMUX_MASK           (0x0FU)    /*!< Pin functionality mask. */
#define GPIO0_BASE_ADDRESS             (0x10C03200U)    /*!< GPIO instance 0 base address. */
#define GPIO1_BASE_ADDRESS             (0x10C03300U)    /*!< GPIO instance 1 base address. */
#define GET_GPIO_BASE_ADDRESS(instance)    (((instance) < \
    24U) ? GPIO0_BASE_ADDRESS : GPIO1_BASE_ADDRESS)     /*!< Returns the base address of GPIO pin. */
/**
 * @}
 *
 */

/**
 * @addtogroup gpio_enums
 * @{
 */
/**
 * @brief Available gpio pins.
 * @ingroup gpio_enums
 */
typedef enum
{
    eGPIO0Pin0 = 0,       /*!< GPIO 0th instance pin 0. */
    eGPIO0Pin1,           /*!< GPIO 0th instance pin 1. */
    eGPIO0Pin2,           /*!< GPIO 0th instance pin 2. */
    eGPIO0Pin3,           /*!< GPIO 0th instance pin 3. */
    eGPIO0Pin4,           /*!< GPIO 0th instance pin 4. */
    eGPIO0Pin5,           /*!< GPIO 0th instance pin 5. */
    eGPIO0Pin6,           /*!< GPIO 0th instance pin 6. */
    eGPIO0Pin7,           /*!< GPIO 0th instance pin 7. */
    eGPIO0Pin8,           /*!< GPIO 0th instance pin 8. */
    eGPIO0Pin9,           /*!< GPIO 0th instance pin 9. */
    eGPIO0Pin10,          /*!< GPIO 0th instance pin 10. */
    eGPIO0Pin11,          /*!< GPIO 0th instance pin 11. */
    eGPIO0Pin12,          /*!< GPIO 0th instance pin 12. */
    eGPIO0Pin13,          /*!< GPIO 0th instance pin 13. */
    eGPIO0Pin14,          /*!< GPIO 0th instance pin 14. */
    eGPIO0Pin15,          /*!< GPIO 0th instance pin 15. */
    eGPIO0Pin16,          /*!< GPIO 0th instance pin 16. */
    eGPIO0Pin17,          /*!< GPIO 0th instance pin 17. */
    eGPIO0Pin18,          /*!< GPIO 0th instance pin 18. */
    eGPIO0Pin19,          /*!< GPIO 0th instance pin 19. */
    eGPIO0Pin20,          /*!< GPIO 0th instance pin 20. */
    eGPIO0Pin21,          /*!< GPIO 0th instance pin 21. */
    eGPIO0Pin22,          /*!< GPIO 0th instance pin 22. */
    eGPIO0Pin23,          /*!< GPIO 0th instance pin 23. */
    eGPIO1Pin0,           /*!< GPIO 1st instance pin 0. */
    eGPIO1Pin1,           /*!< GPIO 1st instance pin 1. */
    eGPIO1Pin2,           /*!< GPIO 1st instance pin 2. */
    eGPIO1Pin3,           /*!< GPIO 1st instance pin 3. */
    eGPIO1Pin4,           /*!< GPIO 1st instance pin 4. */
    eGPIO1Pin5,           /*!< GPIO 1st instance pin 5. */
    eGPIO1Pin6,           /*!< GPIO 1st instance pin 6. */
    eGPIO1Pin7,           /*!< GPIO 1st instance pin 7. */
    eGPIO1Pin8,           /*!< GPIO 1st instance pin 8. */
    eGPIO1in9,            /*!< GPIO 1st instance pin 9. */
    eGPIO1Pin10,          /*!< GPIO 1st instance pin 10. */
    eGPIO1Pin11,          /*!< GPIO 1st instance pin 11. */
    eGPIO1Pin12,          /*!< GPIO 1st instance pin 12. */
    eGPIO1Pin13,          /*!< GPIO 1st instance pin 13. */
    eGPIO1Pin14,          /*!< GPIO 1st instance pin 14. */
    eGPIO1Pin15,          /*!< GPIO 1st instance pin 15. */
    eGPIO1Pin16,          /*!< GPIO 1st instance pin 16. */
    eGPIO1Pin17,          /*!< GPIO 1st instance pin 17. */
    eGPIO1Pin18,          /*!< GPIO 1st instance pin 18. */
    eGPIO1Pin19,          /*!< GPIO 1st instance pin 19. */
    eGPIO1Pin20,          /*!< GPIO 1st instance pin 20. */
    eGPIO1Pin21,          /*!< GPIO 1st instance pin 21. */
    eGPIO1Pin22,          /*!< GPIO 1st instance pin 22. */
    eGPIO1Pin23,          /*!< GPIO 1st instance pin 23. */
    eGPIOMaxPins,         /*!< GPIO pins available. */
    eGPIOPinsPerReg = 24  /*!< GPIO pins available per register . */
} GpioPinNumber_t;

/**
 * @brief GPIO operation direction.
 * @ingroup gpio_enums
 */
typedef enum
{
    eGpioDirectionInput, /*!< Input direction. */
    eGpioDirectionOutput /*!< Output direction. */
} GpioDirection_t;

/**
 * @brief GPIO interrupts.
 * @ingroup gpio_enums
 */
typedef enum
{
    eGpioInterruptNone, /*!< No interrupt. */
    eGpioInterruptRising, /*!< Interrupt on rising edge. */
    eGpioInterruptFalling, /*!< Interrupt on falling edge. */
    eGpioInterruptLow, /*!< Interrupt on low level. */
    eGpioInterruptHigh, /*!< Interrupt on high level. */
} GpioInterrupt_t;

/**
 * @brief Ioctl requests for GPIO.
 * @ingroup gpio_enums
 */
typedef enum
{
    eSetGpioDirection, /*!< Set GPIO Direction. Takes input type GpioDirection_t */
    eSetGpioInterrupt, /*!< Set GPIO Interrupt type. Takes input type GpioInterrupt_t */
    eGetGpioDirection, /*!< Get GPIO Direction setting. Returns GpioDirection_t */
    eGetGpioInterrupt, /*!< Get GPIO Interrupt config. Returns GpioInterrupt_t type */
} GpioIoctlRequest_t;
/**
 * @}
 */

/**
 * @addtogroup gpio_structs
 * @{
 */
/**
 * @brief   GPIO descriptor type defined in the source file.
 */
struct GpioDescriptor;

/**
 * @brief   GpioHandle_t type is the GPIO handle returned by calling gpio_open()
 *          this is initialized in open and returned to caller. Caller must pass this pointer
 *          to the rest of the APIs.
 */
typedef struct GpioDescriptor *GpioHandle_t;
/**
 * @}
 */

/**
 * @addtogroup gpio_fns
 * @{
 */
/**
 * @brief   GPIO interrupt callback type. This callback is passed to the
 *          driver by using gpio_set_callback API.
 */
typedef void (*GpioCallback_t)( uint8_t ucPinState, void *pvUserContext );

/**
 * @brief   Initializes the GPIO pin instance.
 *          The application must call this function to open the desired GPIO pin
 *          before using it for any functionality.
 *
 * @param[in]   lGpioNumber The GPIO pin number to open. Shall use the constants
 *              defined by GpioPinNumber_t
 *
 * @return
 *   - handle to the GPIO peripheral if everything succeeds
 *   - NULL, if
 *    - invalid instance number
 *    - open same instance more than once before closing it.
 */
GpioHandle_t gpio_open( GpioPinNumber_t lGpioNumber );

/**
 * @brief   Sets the application callback to be invoked on interrupt trigger.
 *          The callback is triggered by a hardware interrupt.
 *          To receive the callback, the user must configure an interrupt type
 *          for the GPIO pin using gpio_ioctl().
 *
 * @note This callback is per handle. Each instance has its own callback.
 *
 * @warning If input handle or if callback function is NULL, this function silently takes no action.
 *
 * @param[in] pxGpio    The GPIO handle returned in the open() call.
 * @param[in] xGpioCallback The callback function to be called on interrupt.
 * @param[in] pvUserContext The user context to be passed back when callback is called.
 */
void gpio_set_callback( GpioHandle_t const pxGpio,
        GpioCallback_t xGpioCallback, void *pvUserContext );

/**
 * @brief Read the gpio pin in blocking mode
 *
 * @param[in]   pxGpio      The GPIO handle returned in the open() call.
 * @param[out]  pucPinState The the pin state.
 *
 * @return
 *   - 0 on success
 *   - -EINVAL, if pxGpio or pucPinState are NULL or handle is not open
 */
int32_t gpio_read_sync( GpioHandle_t const pxGpio,
        uint8_t *pucPinState );

/**
 * @brief   Write the gpio pin in blocking mode
 *
 * @param[in]   pxGpio      The GPIO handle returned in the open() call.
 * @param[in]   ucPinState    Desired pin state
 *
 * @return
 *   - 0 on success
 *   - -EINVAL, if pxGpio is NULL or handle is not open
 */
int32_t gpio_write_sync( GpioHandle_t const pxGpio,
        uint8_t ucPinState );

/**
 * @brief   Deinitializes the GPIO pin and closes the handle.
 *          The application should call this function to reset the GPIO pin to its default state,
 *          disable any associated interrupts, and release the handle.
 *
 * @param[in]   pxGpio The GPIO handle returned in the open() call.
 *
 * @return
 *   - 0 on success (gpio pin is deinitialized)
 *   - -EINVAL, if
 *      - pxGpio handle is NULL
 *      - if is not in open state (already closed).
 */
int32_t gpio_close( GpioHandle_t const pxGpio );

/**
 * @brief   gpio_ioctl is used to configure GPIO pin options.
 *          The application should call this function to configure various GPIO
 *          pin options: I/O direction, interrupt options etc
 *
 * @param[in] pxGpio        The GPIO handle returned in the open() call.
 * @param[in] xRequest      One of GpioIoctlRequest_t enum
 * @param[in,out] pvBuffer  Buffer holding IOCTL argument
 *
 * @return
 *   - 0 on success
 *   - -EINVAL, on NULL handle, invalid request, or NULL buffer when required.
 */
int32_t gpio_ioctl( GpioHandle_t const pxGpio,
        GpioIoctlRequest_t xRequest, void *const pvBuffer );

/**
 * @}
 */
/* end of group gpio_fns */
/**
 * @}
 */
/* end of group gpio */

#endif /* __SOCFPGA_GPIO_H__ */

