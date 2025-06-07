/*
 * SPDX-FileCopyrightText: Copyright (C) 2025 Altera Corporation
 *
 * SPDX-License-Identifier: MIT-0
 *
 * Implementation of CLI commands for GPIO
 */


/**
 * @defgroup cli_gpio GPIO
 * @ingroup cli
 *
 * Perform operations on gpio pins.
 *
 * @details
 * It supports the following commands:
 * - gpio set <pin_num> <pin_state>
 * - gpio get <pin_num>
 * - gpio help
 *
 * Typical usage:
 * - Use 'gpio set' command to change the output state.
 * - Use 'gpio get' command to read the input state.
 *
 * @subsection commands Commands
 * @subsubsection gpio_set gpio set
 * Configure a gpio pin and change its state <br>
 *
 * Usage: <br>
 *   gpio set <pin_num> <pin_state> <br>
 *
 * It requires the following arguments:
 * - pin_num -            GPIO pin number. Valid values are 0 to 47
 * - pin_state -          Pin state as 0 or 1.
 *
 * @subsubsection gpio_get gpio get
 * Read the state of a gpio pin <br>
 *
 * Usage: <br>
 *   gpio get <pin_num> <br>
 *
 * It requires the following argument:
 * - pin_num -             GPIO pin number. Valid values are 0 to 47
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <FreeRTOS.h>
#include <task.h>
#include "semphr.h"
#include <socfpga_uart.h>
#include <socfpga_timer.h>
#include "FreeRTOS_CLI.h"
#include "cli_app.h"
#include <socfpga_gpio.h>
#include "osal_log.h"

SemaphoreHandle_t xSemCliGpio;
BaseType_t xHigherPriorityTaskWoken = pdFALSE;

typedef struct
{
    uint8_t pin;
    GpioInterrupt_t intType;
} gpioIntEvent;

gpioIntEvent gpioIrqEvent;
BaseType_t cmd_gpio( char *pcWriteBuffer, size_t xWriteBufferLen,
        const char *pcCommandString );
TaskHandle_t gpioEventTaskHdl = NULL;

void gpio_callback( uint8_t ucPinState, void *pvUserContext )
{
    (void) ucPinState;
    if (strcmp(pvUserContext, "gpio_cli") == 0)
    {
        xSemaphoreGiveFromISR(xSemCliGpio, NULL);
    }
}

static GpioInterrupt_t intrpt_type_char_to_val( char *ctype )
{
    GpioInterrupt_t res = eGpioInterruptNone;
    switch (ctype[ 0 ])
    {
    case 'l':
        res = eGpioInterruptLow;
        break;
    case 'h':
        res = eGpioInterruptHigh;
        break;
    case 'r':
        res = eGpioInterruptRising;
        break;
    case 'f':
        res = eGpioInterruptFalling;
        break;
    default:
        res = eGpioInterruptNone;
    }
    return res;
}

/*
 * @brief Check if the pin is configured as gpio in pinmux
 */
static bool is_pin_gpio( uint8_t pin )
{
    uint32_t val;
    val = RD_REG32(PINMUX_REG(pin));
    val &= PINMUX_MASK;
    return (val == PINMUX_GPIO);
}

/*
 * @brief Configure the pin as gpio in pinmux
 */
static void config_pinmux(uint8_t pin)
{
    uint32_t regVal;
    if (is_pin_gpio(pin) == false)
    {
        printf("\n\r Configuring pinmux reg");
        /* Change pin functionality to gpio */
        regVal = RD_REG32(PINMUX_REG(pin));
        regVal = (regVal & ~PINMUX_MASK) | PINMUX_GPIO;
        WR_REG32(PINMUX_REG(pin), regVal);
    }
}

/**
 * @func : gpioEvent
 * @brief : prints the gpio event occurred.
 */

void gpioEvent( void *params )
{

    gpioIntEvent *event = (gpioIntEvent*) params;
    while (1)
    {
        osal_semaphore_wait(xSemCliGpio, portMAX_DELAY);
        switch (event->intType)
        {
        case eGpioInterruptHigh:
            printf("Level high interrupt is detected for gpio %d \r\n",
                    event->pin);
            break;
        case eGpioInterruptLow:
            printf("Level low interrupt is detected for gpio %d \r\n",
                    event->pin);
            break;
        case eGpioInterruptRising:
            printf("Rising edge interrupt is detected for gpio %d \r\n",
                    event->pin);
            break;
        case eGpioInterruptFalling:
            printf("Falling edge interrupt is detected for gpio %d \r\n",
                    event->pin);
            break;
        default:
            break;
        }
    }
}

/**
 * @func : cmd_gpio
 * @brief : callback function to set or get gpio pinstate
 */

BaseType_t cmd_gpio( char *pcWriteBuffer, size_t xWriteBufferLen,
        const char *pcCommandString )
{
    int intType;
    int configType;
    uint8_t pinState;
    GpioPinNumber_t lGpioNumber;
    char retBuffer[ 50 ] = { 0 };
    char tempString[ 4 ] = { 0 };
    const char *pcParameter1;
    const char *pcParameter2;
    const char *pcParameter3;
    BaseType_t xParameter1StringLength;
    BaseType_t xParameter2StringLength;
    BaseType_t xParameter3StringLength;
    char *uContext = "gpio_cli";
    const char *getCmd = "get";
    const char *setCmd = "set";
    const char *setIntCmd = "setint";
    const char *closeCmd = "close";
    const char *helpCmd = "help";
    GpioHandle_t gpioPinHdl;
    static GpioHandle_t gpioHandleLive;
    char *endptr;
    char *spacePos;

    pcParameter1 = FreeRTOS_CLIGetParameter(pcCommandString, 1,
            &xParameter1StringLength);
    pcParameter2 = FreeRTOS_CLIGetParameter(pcCommandString, 2,
            &xParameter2StringLength);
    strncpy(tempString, pcParameter1, xParameter1StringLength);
    if (strcmp(tempString, setCmd) == 0)
    {
        if (strncmp( pcParameter2, "help", 4) == 0)
        {
            printf("\r\nConfigure a gpio and its state"
                    "\r\n\nUsage:"
                    "\r\n  gpio set <pin_no> <pin_state>"
                    "\r\n\nIt requires the following arguments:"
                    "\r\n pin_num     GPIO pin number. Valid values are 0 to 47"
                    "\r\n pin_state   Valid values are 0 and 1. "
                    "\r\n             0 for LOW,"
                    "\r\n             1 for HIGH."
                    );

            return pdFALSE;
        }
        pcParameter3 = FreeRTOS_CLIGetParameter(pcCommandString, 3,
                &xParameter3StringLength);

        /* terminate string at the particular commands */
        spacePos = strchr(pcParameter2, ' ');
        if (spacePos != NULL)
        {
            *spacePos = '\0';
        }

        spacePos = strchr(pcParameter3, ' ');
        if (spacePos != NULL)
        {
            *spacePos = '\0';
        }

        if (pcParameter3 == NULL)
        {
            ERROR(
                    "Incorrect Command \r\n Enter 'help' to view a list of available commands.");
            return pdFAIL;
        }
        lGpioNumber = strtol(pcParameter2, &endptr, 10);
        if (endptr == pcParameter2)
        {
            ERROR(
                    "Incorrect Command \r\n Enter 'help' to view a list of available commands.");
            return pdFAIL;
        }
        else if (*endptr != '\0')
        {
            ERROR(
                    "Incorrect Command \r\n Enter 'help' to view a list of available commands.");
            return pdFAIL;
        }
        else
        {
            /* valid pin argument */
            pinState = strtol(pcParameter3, NULL, 10);
            if (lGpioNumber > 47)
            {
                strncpy(pcWriteBuffer, "Invalid GPIO number",
                        strlen("Invalid GPIO number "));
                return pdFAIL;
            }
            if ((pinState != 0) && (pinState != 1))
            {
                strncpy(pcWriteBuffer, "Invalid GPIO pin state",
                        strlen("Invalid GPIO pin state "));
                return pdFAIL;
            }
            config_pinmux(lGpioNumber);
            gpioPinHdl = gpio_open(lGpioNumber);
            if (gpioPinHdl == NULL)
            {
                ERROR("Failed opening the Pin. Maybe already in use.");
                return pdFAIL;
            }
            configType = eGpioDirectionOutput;
            gpio_ioctl(gpioPinHdl, eSetGpioDirection, &configType);
            gpio_write_sync(gpioPinHdl, pinState);
            gpio_close(gpioPinHdl);
        }
    }
    else if (strcmp(tempString, getCmd) == 0)
    {
        if (strncmp( pcParameter2, "help", 4) == 0)
        {
            printf("\r\nRead the current state of a gpio pin"
                    "\r\n\nUsage:"
                    "\r\n  gpio get <pin_no>"
                    "\r\n\nIt requires the following arguments:"
                    "\r\n pin_num    GPIO pin number. Valid values are 0 to 47"
                    );

            return pdFALSE;
        }
        lGpioNumber = strtol(pcParameter2, &endptr, 10);
        if (endptr == tempString)
        {
            ERROR(
                    "Incorrect Command \r\n Enter 'help' to view a list of available commands.");
            return pdFAIL;
        }
        else if (*endptr != '\0')
        {
            ERROR(
                    "Incorrect Command \r\n Enter 'help' to view a list of available commands.");
            return pdFAIL;
        }
        else
        {
            if (lGpioNumber >= GPIO_MAX_INSTANCE)
            {
                strncpy(pcWriteBuffer, "Invalid GPIO number",
                        strlen("Invalid GPIO number "));
                return pdFAIL;
            }
            config_pinmux(lGpioNumber);
            gpioPinHdl = gpio_open(lGpioNumber);
            if (gpioPinHdl == NULL)
            {
                ERROR("Failed opening the Pin. Maybe already in use.");
                return pdFAIL;
            }
            configType = eGpioDirectionInput;
            gpio_ioctl(gpioPinHdl, eSetGpioDirection, &configType);
            gpio_read_sync(gpioPinHdl, &pinState);
            gpio_close(gpioPinHdl);
            itoa(pinState, tempString, 10);
            strncpy(pcWriteBuffer, tempString, xWriteBufferLen);
        }
    }
    else if (strcmp(tempString, setIntCmd) == 0)
    {
        xSemCliGpio = osal_semaphore_create(NULL);
        pcParameter3 = FreeRTOS_CLIGetParameter(pcCommandString, 3,
                &xParameter3StringLength);
        strncpy(tempString, pcParameter3, xParameter3StringLength);
        intType = intrpt_type_char_to_val(tempString);
        lGpioNumber = strtol(pcParameter2, NULL, 10);
        if (lGpioNumber > GPIO_MAX_INSTANCE)
        {
            strncpy(pcWriteBuffer, "Invalid GPIO number",
                    strlen("Invalid GPIO number "));
            return pdFAIL;
        }
        if ((intType < eGpioInterruptNone) || (intType > eGpioInterruptHigh))
        {
            strncpy(pcWriteBuffer, "Invalid interrupt type",
                    strlen("Invalid interrupt type "));
            return pdFAIL;
        }
        config_pinmux(lGpioNumber);
        gpioPinHdl = gpio_open(lGpioNumber);
        if (gpioPinHdl != NULL)
        {
            gpio_ioctl(gpioPinHdl, eSetGpioDirection,
                    eGpioDirectionInput);
            gpio_set_callback(gpioPinHdl, gpio_callback, uContext);

            gpioHandleLive = gpioPinHdl;
            switch (intType)
            {
            case eGpioInterruptHigh:
                printf("Level high interrupt is set for gpio %d \r\n",
                        lGpioNumber);
                break;
            case eGpioInterruptLow:
                printf("Level low interrupt is set for gpio %d \r\n",
                        lGpioNumber);
                break;
            case eGpioInterruptRising:
                printf("Rising edge interrupt is set for gpio %d \r\n",
                        lGpioNumber);
                break;
            case eGpioInterruptFalling:
                printf("Falling edge interrupt is set for gpio %d \r\n",
                        lGpioNumber);
                break;
            default:
                break;
            }
            gpioIrqEvent.intType = intType;
            gpioIrqEvent.pin = lGpioNumber;
            gpio_ioctl(gpioPinHdl, eSetGpioInterrupt, &intType);
            if (gpioEventTaskHdl == NULL)
            {
                if (xTaskCreate(gpioEvent, "gpioEventTask",
                        configMINIMAL_STACK_SIZE, &gpioIrqEvent,
                        tskIDLE_PRIORITY,
                        &gpioEventTaskHdl) != pdPASS)
                {
                    ERROR("Task creation failed.");
                }
            }
            strncpy(pcWriteBuffer, retBuffer, strlen(retBuffer));
        }
        else
        {
            switch (intType)
            {
            case eGpioInterruptHigh:
                printf("Level high interrupt is set for gpio %d \n\r",
                        lGpioNumber);
                break;
            case eGpioInterruptLow:
                printf("Level low interrupt is set for gpio %d \n\r",
                        lGpioNumber);
                break;
            case eGpioInterruptRising:
                printf("Rising edge interrupt is set for gpio %d \n\r",
                        lGpioNumber);
                break;
            case eGpioInterruptFalling:
                printf("Falling edge interrupt is set for gpio %d \n\r",
                        lGpioNumber);
                break;
            default:
                break;
            }
            gpio_ioctl(gpioHandleLive, eSetGpioInterrupt, &intType);
            if (intType == eGpioInterruptNone)
            {
                sprintf(retBuffer, "No interrupt is set for gpio %d",
                        lGpioNumber);
                vTaskDelete(gpioEventTaskHdl);
            }
            else
            {
                gpioIrqEvent.intType = intType;
                gpioIrqEvent.pin = lGpioNumber;
                if (xTaskCreate(gpioEvent, "gpioEventTask",
                        configMINIMAL_STACK_SIZE, &gpioIrqEvent,
                        tskIDLE_PRIORITY,
                        &gpioEventTaskHdl) != pdPASS)
                {
                    ERROR("Task creation failed.");
                }
            }
            strncpy(pcWriteBuffer, retBuffer, strlen(retBuffer));
        }
    }
    else if (strcmp(tempString, closeCmd) == 0)
    {
        strncpy(tempString, pcParameter2, xParameter2StringLength);
        lGpioNumber = strtol(tempString, NULL, 10);
        if (lGpioNumber > GPIO_MAX_INSTANCE)
        {
            strncpy(pcWriteBuffer, "Invalid GPIO number",
                    strlen("Invalid GPIO number "));
            strncpy(pcWriteBuffer, retBuffer, strlen(retBuffer));
            return pdFAIL;
        }

        gpio_close(gpioHandleLive);

    }
    else if (strcmp(tempString, helpCmd) == 0)
    {
        printf("\rPerform operations on GPIOs"
                "\r\n\nIt supports the following commands:"
                "\r\n  gpio set <pin_num> <pin_state>"
                "\r\n  gpio get <pin_num>"
                "\r\n  gpio help"
                "\r\n\nTypical usage:"
                "\r\n- Use 'gpio set' to change the output state."
                "\r\n- Use 'gpio get' to read the input state."
                "\r\n\nFor help on the specific commands please do:"
                "\r\n  gpio <command> help\r\n"
                );
        return pdFAIL;
    }
    else
    {
        strncpy(pcWriteBuffer, "Invalid gpio command",
                strlen("Invalid gpio command "));
        return pdFAIL;
    }
    return pdFALSE;
}

