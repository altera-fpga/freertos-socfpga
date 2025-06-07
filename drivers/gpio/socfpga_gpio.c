/*
 * SPDX-FileCopyrightText: Copyright (C) 2025 Altera Corporation
 *
 * SPDX-License-Identifier: MIT-0
 *
 * HAL driver implementation for gpio
 */

#include <stdbool.h>
#include <string.h>
#include "socfpga_gpio.h"
#include "socfpga_interrupt.h"
#include "socfpga_gpio_reg.h"
#include "socfpga_defines.h"
#include "socfpga_rst_mngr.h"
#include "osal_log.h"

struct GpioDescriptor
{
    BaseType_t xIsOpen;
    GpioPinNumber_t lInstance;
    GpioCallback_t xCallbackFn;
    void *cb_usercontext;
};

static struct GpioDescriptor gpio_descriptors[ GPIO_MAX_INSTANCE ];

void gpio_irq_handler( void *vData );

/*
 * @brief Check if the pin is configured as gpio in pinmux
 */
static bool is_pin_gpio( uint32_t pin )
{
    uint32_t val;
    val = RD_REG32(PINMUX_REG(pin));
    val &= PINMUX_MASK;
    return (val == PINMUX_GPIO);
}

/*
 * @brief Set the bit value in the given gpio block register
 */
static void set_gpio_reg_bit( uint32_t reg, uint32_t pin, uint8_t val )
{
    uint32_t regVal;
    uint32_t baseAdd = GET_GPIO_BASE_ADDRESS(pin);
    if (pin >= GPIO_PINS_PER_REG)
    {
        pin -= GPIO_PINS_PER_REG;
    }
    regVal = RD_REG32(baseAdd + reg);
    regVal &= ~(1U << pin);
    regVal |= ((uint32_t) val << pin);
    WR_REG32(baseAdd + reg, regVal);
}

/*
 * @brief Get the bit value in the given gpio block register
 */
static uint8_t get_gpio_reg_bit( uint32_t reg, uint8_t pin )
{
    uint32_t val;
    val = RD_REG32(GET_GPIO_BASE_ADDRESS(pin) + reg);
    if (pin >= GPIO_PINS_PER_REG)
    {
        pin -= GPIO_PINS_PER_REG;
    }
    val = (val >> pin) & 0x01U;
    return ((uint8_t) val);
}

/*
 * @brief set the direction of gpio pin
 */
static void gpio_set_direction( GpioPinNumber_t pin, GpioDirection_t dir )
{
    if (dir == eGpioDirectionInput)
    {
        set_gpio_reg_bit(GPIO_SWPORTA_DDR, (uint32_t) pin, 0U);
    }
    else
    {
        set_gpio_reg_bit(GPIO_SWPORTA_DDR, (uint32_t) pin, 1U);
    }
}

/*
 * @brief handle the interrupt
 */
void gpio_irq_handler( void *vData )
{
    uint8_t gpioInst = *(uint8_t*) vData;
    uint8_t pinState;
    uint8_t pinOffset;
    uint32_t intStat;
    uint32_t gpioBase;
    uint8_t pin;
    uint8_t i;

    gpioBase = (gpioInst == GPIO_INSTANCE_ZERO) ?
            GPIO0_BASE_ADDRESS :
            GPIO1_BASE_ADDRESS;

    /* get the active interrupts */
    intStat = RD_REG32(gpioBase + GPIO_INTSTATUS);

    /* clear all the edge interrupts */
    WR_REG32(gpioBase + GPIO_PORTA_EOI, intStat);

    pinOffset = ((gpioInst == GPIO_INSTANCE_ZERO) ? 0U : GPIO_PINS_PER_REG);

    /* Invoke callback for all active interrupts if it is registered */
    for (i = 0; i < GPIO_PINS_PER_REG; i++)
    {
        if ((intStat & ((uint32_t) 1 << i)) == ((uint32_t) 1U << i))
        {
            pin = i + pinOffset;
            if (gpio_descriptors[ pin ].xCallbackFn != NULL)
            {
                pinState = get_gpio_reg_bit(GPIO_SWPORTA_DR, pin);
                gpio_descriptors[ pin ].xCallbackFn(pinState,
                        gpio_descriptors[ pin ].cb_usercontext);
            }
        }
    }
}

/*
 * @brief set the interrupt configuration
 */
static bool gpio_set_interrupt( GpioPinNumber_t pin, GpioInterrupt_t type )
{
    SocfpgaHpuInterrupt_t id;
    uint8_t gpioInst;
    SocfpgaInterruptErr_t xIntRet;
    if (pin < eGPIOPinsPerReg)
    {
        id = eGpio0Irq;
        gpioInst = 0;
    }
    else
    {
        id = eGpio1Irq;
        gpioInst = 1;
    }
    xIntRet = interrupt_register_isr(id, gpio_irq_handler, &gpioInst);
    if (xIntRet != eErrOk)
    {
        return false;
    }

    /* clear the edge interrupt if set already */
    set_gpio_reg_bit(GPIO_PORTA_EOI, (uint32_t) pin, 1U);

    /* Configure the level and polarity and enable the interrupt */
    switch (type)
    {
    case eGpioInterruptNone:
        set_gpio_reg_bit(GPIO_INTEN, (uint32_t) pin, 0U);
        break;
    case eGpioInterruptRising:
        set_gpio_reg_bit(GPIO_INTTYPE_LEVEL, (uint32_t) pin, 1U);
        set_gpio_reg_bit(GPIO_INT_POLARITY, (uint32_t) pin, 1U);
        set_gpio_reg_bit(GPIO_INTEN, (uint32_t) pin, 1U);
        break;
    case eGpioInterruptFalling:
        set_gpio_reg_bit(GPIO_INTTYPE_LEVEL, (uint32_t) pin, 1U);
        set_gpio_reg_bit(GPIO_INT_POLARITY, (uint32_t) pin, 0U);
        set_gpio_reg_bit(GPIO_INTEN, (uint32_t) pin, 1U);
        break;
    case eGpioInterruptLow:
        set_gpio_reg_bit(GPIO_INTTYPE_LEVEL, (uint32_t) pin, 0U);
        set_gpio_reg_bit(GPIO_INT_POLARITY, (uint32_t) pin, 0U);
        set_gpio_reg_bit(GPIO_INTEN, (uint32_t) pin, 1U);
        break;
    case eGpioInterruptHigh:
        set_gpio_reg_bit(GPIO_INTTYPE_LEVEL, (uint32_t) pin, 0U);
        set_gpio_reg_bit(GPIO_INT_POLARITY, (uint32_t) pin, 1U);
        set_gpio_reg_bit(GPIO_INTEN, (uint32_t) pin, 1U);
        break;

    default:
        /* do nothing */
        break;
    }
    if (type != eGpioInterruptNone)
    {
        xIntRet = interrupt_enable(id, GIC_INTERRUPT_PRIORITY_GPIO);
        if (xIntRet != eErrOk)
        {
            return false;
        }
    }
    return true;
}

/*
 * @brief get the direction of the gpio pin
 */
static uint8_t gpio_get_direction( GpioPinNumber_t pin )
{
    uint8_t retVal;
    retVal = get_gpio_reg_bit(GPIO_SWPORTA_DDR, pin);
    return retVal;
}

/*
 * @brief get the interrupt configuration of the gpio pin
 */

static GpioInterrupt_t gpio_get_interrupt( GpioPinNumber_t pin )
{
    GpioInterrupt_t retVal = eGpioInterruptNone;
    uint8_t res1;
    uint8_t res2;

    /* get the level/edge and polarity */
    res1 = get_gpio_reg_bit(GPIO_INTTYPE_LEVEL, pin);
    res2 = get_gpio_reg_bit(GPIO_INT_POLARITY, pin);
    res1 = (res1 << 1U) | res2;

    switch (res1)
    {
    case 0:
        retVal = eGpioInterruptLow;
        break;
    case 1:
        retVal = eGpioInterruptHigh;
        break;
    case 2:
        retVal = eGpioInterruptFalling;
        break;
    case 3:
        retVal = eGpioInterruptRising;
        break;
    default:
        retVal = eGpioInterruptNone;
        break;
    }
    res1 = get_gpio_reg_bit(GPIO_INTEN, pin);
    if (res1 == 0U)
    {
        retVal = eGpioInterruptNone;
    }
    return retVal;
}

/**
 * @brief Get the reset instance for the GPIO peripheral
 */
static ResetPeriphrl_t gpio_get_rst_instance( uint32_t lGpioNumber )
{
    ResetPeriphrl_t rst_instance = RST_PERIPHERAL_END;
    if(lGpioNumber <= eGPIO0Pin23)
    {
        rst_instance = RST_GPIO0;
    }
    else if(lGpioNumber <= eGPIO1Pin23)
    {
        rst_instance = RST_GPIO1;
    }
    return rst_instance;
}

/*
 * @brief read the current state of the gpio pin
 */
static uint8_t gpio_read_state( GpioPinNumber_t pin )
{
    uint8_t retVal;
    retVal = get_gpio_reg_bit(GPIO_EXT_PORTA, pin);
    return retVal;
}

/*
 * @brief write the sate of the gpio pin
 */
static void gpio_write_state( GpioPinNumber_t pin, uint8_t state )
{
    set_gpio_reg_bit(GPIO_SWPORTA_DR, (uint32_t) pin, state);
}

GpioHandle_t gpio_open( GpioPinNumber_t lGpioNumber )
{
    GpioHandle_t pxHandle;
    ResetPeriphrl_t rst_instance;
    uint8_t reset_status;
    int32_t status;

    if ((lGpioNumber >= eGPIOMaxPins) || (lGpioNumber < eGPIO0Pin0))
    {
        return NULL;
    }
    if (!is_pin_gpio((uint32_t) lGpioNumber))
    {
        ERROR("The pin trying to open is allocated for different interface in Pinmux REG");
        return NULL;
    }

    pxHandle = &(gpio_descriptors[ lGpioNumber ]);
    if (pxHandle->xIsOpen == true)
    {
        return NULL;
    }
    rst_instance = gpio_get_rst_instance((uint32_t) lGpioNumber);
    status = rstmgr_get_reset_status(rst_instance, &reset_status);
    if (status != RSTMGR_SUCCESS)
    {
        ERROR("Failed to get GPIO reset status. ");
        return NULL;
    }
    if (reset_status != 0U)
    {
        status = rstmgr_toggle_reset(rst_instance);
        if (status != RSTMGR_SUCCESS)
        {
            ERROR("Failed to reset release GPIO block. ");
            return NULL;
        }
    }

    (void) memset(pxHandle, 0, sizeof(struct GpioDescriptor));
    pxHandle->xIsOpen = 1;
    pxHandle->lInstance = lGpioNumber;
    return pxHandle;
}

int32_t gpio_ioctl( GpioHandle_t const pxGpio, GpioIoctlRequest_t xRequest,
        void *const pvBuffer )
{
    GpioDirection_t xDirConfig;
    GpioInterrupt_t xIntrConfig;
    int32_t ret = 0;

    if (pxGpio == NULL)
    {
        return -EINVAL;
    }
    if (pvBuffer == NULL)
    {
        return -EINVAL;
    }
    switch (xRequest)
    {
    case eSetGpioDirection:
        xDirConfig = *(GpioDirection_t*) pvBuffer;
        gpio_set_direction(pxGpio->lInstance, xDirConfig);
        break;
    case eSetGpioInterrupt:
        xIntrConfig = *(GpioInterrupt_t*) pvBuffer;
        if (gpio_set_interrupt(pxGpio->lInstance, xIntrConfig) == false)
        {
            ret = -EINVAL;
        }
        break;
    case eGetGpioDirection:
        *(uint8_t*) pvBuffer = gpio_get_direction(pxGpio->lInstance);
        break;
    case eGetGpioInterrupt:
        *(GpioInterrupt_t*) pvBuffer = gpio_get_interrupt(pxGpio->lInstance);
        break;
    default:
        ret = -EINVAL;
        break;
    }
    return ret;
}

void gpio_set_callback( GpioHandle_t const pxGpio, GpioCallback_t xGpioCallback,
        void *pvUserContext )
{
    if (pxGpio == NULL)
    {
        return;
    }
    pxGpio->xCallbackFn = xGpioCallback;
    pxGpio->cb_usercontext = pvUserContext;
}

int32_t gpio_read_sync( GpioHandle_t const pxGpio, uint8_t *pucPinState )
{
    if ((pxGpio == NULL) || (pucPinState == NULL))
    {
        return -EINVAL;
    }
    if (!(pxGpio->xIsOpen))
    {
        return -EINVAL;
    }
    *pucPinState = gpio_read_state(pxGpio->lInstance);
    return 0;
}

int32_t gpio_write_sync( GpioHandle_t const pxGpio, uint8_t ucPinState )
{
    if (pxGpio == NULL)
    {
        return -EINVAL;
    }
    if (!(pxGpio->xIsOpen))
    {
        return -EINVAL;
    }
    gpio_write_state(pxGpio->lInstance, ucPinState);
    return 0;
}

int32_t gpio_close( GpioHandle_t const pxGpio )
{
    if ((pxGpio == NULL) || (pxGpio->xIsOpen == 0))
    {
        return -EINVAL;
    }
    if (gpio_get_interrupt(pxGpio->lInstance) != eGpioInterruptNone)
    {
        if (gpio_set_interrupt(pxGpio->lInstance, eGpioInterruptNone) == false)
        {
            return -EINVAL;
        }
        pxGpio->xCallbackFn = NULL;
    }
    pxGpio->xIsOpen = 0;

    return 0;
}
