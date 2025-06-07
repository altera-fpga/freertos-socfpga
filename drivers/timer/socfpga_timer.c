/*
 * SPDX-FileCopyrightText: Copyright (C) 2025 Altera Corporation
 *
 * SPDX-License-Identifier: MIT-0
 *
 * HAL driver implementation for timer
 */

 #include <errno.h>
#include "socfpga_timer.h"
#include "socfpga_timer_reg.h"
#include "socfpga_interrupt.h"
#include "socfpga_clk_mngr.h"
#include "socfpga_rst_mngr.h"
#include "osal_log.h"

#define TIMER_DISABLE    0x0U
#define TIMER_ENABLE     0x1U

/*Macro to convert microsecond delay to tick value*/
#define TIMER_US_TO_TICKS(xClkHz, ulTimeUs)  \
    ((uint32_t) ((((uint64_t)(ulTimeUs)) *  \
            ((uint64_t)(xClkHz))) / (1000U * 1000U)))

/*Macro to convert ticks to miro second*/
#define TIMER_TICKS_TO_US(xClkHz, ulTicks) \
    ((uint32_t) ((((uint64_t)(ulTicks)) * \
            ((uint64_t)(1000U * 1000U))) / (xClkHz)))

#define TIMER_FREE_RUNNING_PERIOD    0xFFFFFFFFU

struct TimerDescriptor
{
    uint32_t ulBaseAddress;
    BaseType_t xIsRunning;
    BaseType_t xIsOpen;
    BaseType_t xIsConfigured;
    SocfpgaHpuInterrupt_t ulInterruptId;
    uint32_t ulClkHz;
    TimerCallback_t xCallbackFunc;
    void *pvUserContext;

};

static struct TimerDescriptor timer_descriptor[ TIMER_MAX_INSTANCE ];

void prvTimerIrqHandler( void *xData );

static ResetPeriphrl_t timer_get_rst_instance (TimerInstance_t xTimerInstance)
{
    ResetPeriphrl_t xRstInstance = RST_PERIPHERAL_END;
    switch (xTimerInstance)
    {
    case eSocfpgaOsc1Timer0:
        xRstInstance = RST_L4SYSTIMER0;
        break;
    case eSocfpgaOsc1Timer1:
        xRstInstance = RST_L4SYSTIMER1;
        break;
    case eSocfpgaSpTimer0:
        xRstInstance = RST_SPTIMER0;
        break;
    case eSocfpgaSpTimer1:
        xRstInstance = RST_SPTIMER1;
        break;
    default:
        ERROR("Invalid Timer instance");
        break;
    }
    return xRstInstance;
}

Timer_Handle_t timer_open( TimerInstance_t xTimerInstance )
{
    uint32_t ulClkHz;
    uint32_t ulVal;
    uint8_t uRes;
    int32_t status;
    ClockBlock_t xClockBlockID = eClockInvalid;
    SocfpgaInterruptErr_t xIntRet;
    ResetPeriphrl_t xRstInstance;
    int32_t ret = 0;

    /*Check Instance Validity*/
    if (xTimerInstance >= eSocfpgaMaxTimerInstance)
    {
        ERROR("Invalid Timer instance");
        return NULL;
    }
    Timer_Handle_t xHandle = &timer_descriptor[ xTimerInstance ];
    if (xHandle->xIsOpen == 1)
    {
        ERROR("Timer instance is already running");
        return NULL;
    }
    /*Update Timer base address to the context variable */
    switch (xTimerInstance)
    {

    case eSocfpgaOsc1Timer0:
        xHandle->ulBaseAddress = OSC1_TIMER0_BASE_ADDR;
        xHandle->ulInterruptId = eTimerOsc10irq;
        xClockBlockID = eClockSpTimer;
        break;
    case eSocfpgaOsc1Timer1:
        xHandle->ulBaseAddress = OSC1_TIMER1_BASE_ADDR;
        xHandle->ulInterruptId = eTimerOsc11irq;
        xClockBlockID = eClockSpTimer;
        break;
    case eSocfpgaSpTimer0:
        xHandle->ulBaseAddress = SPTIMER0_BASE_ADDR;
        xHandle->ulInterruptId = eTimerL4sp0irq;
        xClockBlockID = eClockOsc1Timer;
        break;
    case eSocfpgaSpTimer1:
        xHandle->ulBaseAddress = SPTIMER0_BASE_ADDR;
        xHandle->ulInterruptId = eTimerL4sp1irq;
        xClockBlockID = eClockOsc1Timer;
        break;
    default:
        ERROR("Invalid Timer instance");
        ret = -EINVAL;
        break;

    }
    if (ret == -EINVAL)
    {
        return NULL;
    }

    xRstInstance = timer_get_rst_instance(xTimerInstance);
    if (xRstInstance == RST_PERIPHERAL_END)
    {
        ERROR("Invalid reset instance");
        return NULL;
    }

    status = rstmgr_get_reset_status(xRstInstance, &uRes);
    if (status != RSTMGR_SUCCESS)
    {
        ERROR("Timer block get reset status failed. ");
        return NULL;
    }
    if (uRes != 0U)
    {
        status =
                rstmgr_toggle_reset(xRstInstance);
        if (status != RSTMGR_SUCCESS)
        {
            ERROR("Failed to reset release Timer block. ");
            return NULL;
        }
    }
    /*Get the source clock value in Hz*/
    if (clk_mngr_get_clk(xClockBlockID, &ulClkHz) == 1U)
    {
        ERROR("Error while getting clock");
        return NULL;
    }
    xHandle->ulClkHz = ulClkHz;

    /*Setup and enable interrupts in GIC*/
    xIntRet = interrupt_register_isr(xHandle->ulInterruptId, prvTimerIrqHandler,
            xHandle);
    if (xIntRet != eErrOk)
    {
        ERROR("Error while registering ISR");
        return NULL;
    }
    xIntRet = interrupt_enable(xHandle->ulInterruptId,
            GIC_INTERRUPT_PRIORITY_TIMER);
    if (xIntRet != eErrOk)
    {
        ERROR("Error while enabling ISR");
        return NULL;
    }

    /*Enable the interrupt*/
    ulVal = RD_REG32((xHandle->ulBaseAddress + TIMER_TIMER1CONTROLREG));
    ulVal &= ~TIMER_TIMER1CONTROLREG_TIMER_INTERRUPT_MASK_MASK;
    WR_REG32((xHandle->ulBaseAddress + TIMER_TIMER1CONTROLREG), ulVal);

    xHandle->xIsOpen = 1;

    return xHandle;

}

int32_t timer_set_period_us( Timer_Handle_t const pxTimerPeripheral,
        uint32_t period )
{
    uint32_t volatile ulVal, ulCountVal;
    if (pxTimerPeripheral == NULL)
    {
        ERROR("Timer handle cannot be NULL");
        return -EINVAL;
    }
    if (pxTimerPeripheral->xIsRunning == 1)
    {
        ERROR("Timer instance already running");
        return -EBUSY;
    }
    if ((TIMER_US_TO_TICKS(pxTimerPeripheral->ulClkHz,
            period) >= TIMER_FREE_RUNNING_PERIOD))
    {
        WARN("Timer period too long");
        period = TIMER_FREE_RUNNING_PERIOD;
    }
    /* Timer needs to be disabled to configure */
    WR_REG32(pxTimerPeripheral->ulBaseAddress + TIMER_TIMER1CONTROLREG,
            TIMER_DISABLE);
    ulVal = RD_REG32(pxTimerPeripheral->ulBaseAddress + TIMER_TIMER1CONTROLREG);
    ulVal |= TIMER_TIMER1CONTROLREG_TIMER_MODE_MASK;
    WR_REG32(pxTimerPeripheral->ulBaseAddress + TIMER_TIMER1CONTROLREG, ulVal);
    ulCountVal = TIMER_US_TO_TICKS(pxTimerPeripheral->ulClkHz, period);
    WR_REG32(pxTimerPeripheral->ulBaseAddress + TIMER_TIMER1LOADCOUNT,
            ulCountVal);
    pxTimerPeripheral->xIsConfigured = 1;

    return 0;
}

int32_t timer_start( Timer_Handle_t const pxTimerPeripheral )
{
    uint32_t ulVal;
    if (pxTimerPeripheral == NULL)
    {
        ERROR("Timer handle cannot be NULL");
        return -EINVAL;
    }

    if (!(pxTimerPeripheral->xIsConfigured))
    {
        ERROR("Timer needs to be configured before starting");
        return -EINVAL;
    }

    if ((pxTimerPeripheral->xIsRunning) == 1)
    {
        ERROR("Timer instance already running");
        return -EBUSY;
    }

    INFO("Starting timer instance");
    /*Enable the timer*/
    ulVal = RD_REG32(pxTimerPeripheral->ulBaseAddress + TIMER_TIMER1CONTROLREG);
    ulVal |= TIMER_ENABLE;
    WR_REG32(pxTimerPeripheral->ulBaseAddress + TIMER_TIMER1CONTROLREG, ulVal);

    pxTimerPeripheral->xIsRunning = 1;

    return 0;
}

int32_t timer_stop( Timer_Handle_t const pxTimerPeripheral )
{

    if ((pxTimerPeripheral == NULL))
    {
        ERROR("Timer Handle cannot be NULL");
        return -EINVAL;
    }
    if (!pxTimerPeripheral->xIsRunning)
    {
        ERROR("Timer instance not running");
        return -EPERM;
    }
    /* Disable timer instances
     *
     * Writing 0x0 directly without read modify as disabling timer asynchronously
     * resets all associated registers
     */
    INFO("Stopping timer instance");
    WR_REG32(pxTimerPeripheral->ulBaseAddress + TIMER_TIMER1CONTROLREG,
            TIMER_DISABLE);
    pxTimerPeripheral->xIsRunning = 0;
    /*Set xIsConfigured variable to zero as disabling the timer instance resets all registers*/
    pxTimerPeripheral->xIsConfigured = 0;
    return 0;
}

int32_t timer_close( Timer_Handle_t const pxTimerPeripheral )
{
    SocfpgaInterruptErr_t xIntRet;

    if ((pxTimerPeripheral == NULL))
    {
        ERROR("Timer Handle cannot be NULL");
        return -EINVAL;
    }
    if (pxTimerPeripheral->xIsOpen == 0)
    {
        ERROR("Invalid handle");
        return -EINVAL;
    }
    /* Disable timer instances
     *
     * Writing 0x0 directly without read modify as disabling timer asynchronously
     * resets all associated registers
     */
    WR_REG32(pxTimerPeripheral->ulBaseAddress + TIMER_TIMER1CONTROLREG,
            TIMER_DISABLE);
    /*Disable timer interrupt at GIC level*/
    xIntRet = interrupt_spi_disable(pxTimerPeripheral->ulInterruptId);
    if (xIntRet != eErrOk)
    {
        ERROR("Error while disabling ISR");
        return -EFAULT;
    }

    pxTimerPeripheral->ulBaseAddress = 0;
    pxTimerPeripheral->xIsRunning = 0;
    pxTimerPeripheral->xIsOpen = 0;
    pxTimerPeripheral->xIsConfigured = 0;
    /* setting unsupported interrupt id to prevent interrupts */
    pxTimerPeripheral->ulInterruptId = eMaxHpuSpiInterrupt;
    pxTimerPeripheral->ulClkHz = 0;
    pxTimerPeripheral->xCallbackFunc = NULL;

    return 0;
}

int32_t timer_get_value_raw( Timer_Handle_t const pxTimerPeripheral,
        uint32_t *ulCounterVal )
{
    if ((pxTimerPeripheral == NULL))
    {
        ERROR("Timer Handle cannot be NULL");
        return -EINVAL;
    }
    if (!pxTimerPeripheral->xIsRunning)
    {
        ERROR("Timer instance not running");
        return -EPERM;
    }
    *ulCounterVal = RD_REG32(
            pxTimerPeripheral->ulBaseAddress + TIMER_TIMER1CURRENTVAL);
    return 0;
}

int32_t timer_get_value_us( Timer_Handle_t const pxTimerPeripheral,
        uint32_t *ulMicroSeconds )
{
    if ((pxTimerPeripheral == NULL) || (ulMicroSeconds == NULL))
    {
        ERROR("Timer Handle cannot be NULL");
        return -EINVAL;
    }
    if (!pxTimerPeripheral->xIsRunning)
    {
        ERROR("Timer instance not running");
        return -EPERM;
    }

    uint32_t ulCountVal = 0U;
    ulCountVal = RD_REG32(
            pxTimerPeripheral->ulBaseAddress + TIMER_TIMER1CURRENTVAL);

    if (pxTimerPeripheral->ulClkHz == 0U)
    {
        ERROR("Denominator is 0");
        return -EINVAL;
    }
    *ulMicroSeconds = TIMER_TICKS_TO_US(pxTimerPeripheral->ulClkHz, ulCountVal);
    return 0;
}

int32_t timer_set_callback( Timer_Handle_t const pxTimerPeripheral,
        TimerCallback_t pxCallback, void *pvUserContext )
{
    if (pxTimerPeripheral == NULL)
    {
        ERROR("Timer handle cannot be NULL");
        return -EINVAL;
    }
    (void) pvUserContext;
    /*Assign user callback to context variable */
    pxTimerPeripheral->xCallbackFunc = pxCallback;
    pxTimerPeripheral->pvUserContext = pvUserContext;
    return 0;
}

void prvTimerIrqHandler( void *xData )
{
    uint32_t ulVal;
    Timer_Handle_t xHandle = (Timer_Handle_t) xData;
    /* Reading TIMER_TIMER1EOI register clears the interrupt and it returns all zeros */
    ulVal = RD_REG32(xHandle->ulBaseAddress + TIMER_TIMER1EOI);
    (void) ulVal;
    if (xHandle->xCallbackFunc != NULL)
    {
        xHandle->xCallbackFunc(xData);
    }
}
