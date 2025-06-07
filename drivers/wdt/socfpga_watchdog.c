/*
 * SPDX-FileCopyrightText: Copyright (C) 2025 Altera Corporation
 *
 * SPDX-License-Identifier: MIT-0
 *
 * HAL driver implementation for Watchdog timer
 */

 #include <errno.h>
#include "socfpga_watchdog.h"
#include "socfpga_watchdog_reg.h"
#include "socfpga_defines.h"
#include "socfpga_interrupt.h"
#include "osal_log.h"

struct WdtDescriptor
{
    uint32_t ulBaseAddress;
    uint32_t ulInstance;
    BaseType_t xIsOpen;
    BaseType_t xIsConfigDone;
    WdtCallback_t xCallBackFn;
    void *CbUsercontext;
};


static struct WdtDescriptor wdt_descriptors[ MAX_WATCHDOG_INSTANCES ];

static void de_assert_reset_signal( uint32_t instance );

void wdt_isr( void *pvhandle );

static bool set_response_mode(WdtHandle_t handle, WdtBiteConfig_t xConfig);

SocfpgaHpuInterrupt_t get_intr_id(uint32_t instance);

WdtHandle_t wdt_open( uint32_t lWatchdogInstance )
{
    WdtHandle_t handle;

    if (!(lWatchdogInstance < MAX_WATCHDOG_INSTANCES))
    {
        ERROR("Invalid watchdog instance");
        return NULL;
    }

    handle = &wdt_descriptors[ lWatchdogInstance ];
    handle->ulInstance = lWatchdogInstance;
    de_assert_reset_signal(lWatchdogInstance);

    if ((handle->xIsOpen) == 1)
    {
        ERROR("Watchdog instance is already open");
        return NULL;
    }
    else
    {
        handle->ulBaseAddress = GET_WDT_BASE_ADDRESS(lWatchdogInstance);
        handle->xIsOpen = 1;
        return handle;
    }
}

int32_t wdt_start( const WdtHandle_t pxWdtHandle )
{
    uint32_t ulVal;
    if (pxWdtHandle == NULL)
    {
        ERROR("Invalid watchdog handle");
        return -EINVAL;
    }

    if (!(pxWdtHandle->xIsConfigDone))
    {
        ERROR("Configuration is not done");
        return -ENODATA;
    }

    INFO("Starting Watchdog timer");
    ulVal = RD_REG32(pxWdtHandle->ulBaseAddress + WDT_CR);
    ulVal |= (1U << WDT_CR_WDT_EN_POS);
    WR_REG32(pxWdtHandle->ulBaseAddress + WDT_CR, ulVal);
    return 0;
}

int32_t wdt_stop( const WdtHandle_t pxWdtHandle )
{
    uint32_t ulVal;
    if (pxWdtHandle == NULL)
    {
        ERROR("Invalid watchdog handle");
        return -EINVAL;
    }

    ulVal = RD_REG32((uint32_t) pxWdtHandle->ulBaseAddress + WDT_CR);
    ulVal &= ~(1U << WDT_CR_WDT_EN_POS);
    WR_REG32( pxWdtHandle->ulBaseAddress + WDT_CR, ulVal);

    INFO("Watchdog timer stopped");
    return 0;
}

int32_t wdt_restart( const WdtHandle_t pxWdtHandle )
{
    uint32_t ulVal;

    if (pxWdtHandle == NULL)
    {
        ERROR("Invalid watchdog handle");
        return -EINVAL;
    }

    if (!(pxWdtHandle->xIsConfigDone))
    {
        ERROR("Configuration is not done");
        return -ENODATA;
    }

    INFO("Restarting Watchdog timer");
    ulVal = (WDT_CRR_RESTART_VAL << WDT_CRR_WDT_CRR_POS);
    WR_REG32(pxWdtHandle->ulBaseAddress + WDT_CRR, ulVal);

    return 0;
}

int32_t wdt_ioctl( const WdtHandle_t pxWdtHandle,
        WdtIoctlRequest_t xRequest, void *const pvBuffer )
{
    uint32_t ulVal;
    uint32_t uTimeIndex;
    int32_t ret = 0;
    if ((pxWdtHandle == NULL) || (pvBuffer == NULL))
    {
        ERROR("Invalid parameters");
        return -EINVAL;
    }

    switch (xRequest)
    {
    case eSetWatchdogBarkTime:
        uTimeIndex = *(uint32_t*) pvBuffer;
        if (uTimeIndex > WDT_TIMEOUT_20000MSEC)
        {
            ERROR("Invalid bark time index");
            return -EINVAL;
        }
        ulVal = RD_REG32(pxWdtHandle->ulBaseAddress + WDT_TORR);
        ulVal &= ~(WDT_TORR_TOP_INIT_MASK);
        ulVal |= (uTimeIndex << WDT_TORR_TOP_INIT_POS);
        WR_REG32(pxWdtHandle->ulBaseAddress + WDT_TORR, ulVal);
        pxWdtHandle->xIsConfigDone = 1;
        break;

    case eGetWatchdogBarkTime:
        ulVal = RD_REG32(pxWdtHandle->ulBaseAddress + WDT_TORR);
        ulVal = ((ulVal & WDT_TORR_TOP_INIT_MASK)
                >> WDT_TORR_TOP_INIT_POS);
        *(uint32_t*) pvBuffer = ulVal;
        break;

    case eSetWatchdogBiteTime:
        uTimeIndex = *(uint32_t*) pvBuffer;
        if (uTimeIndex > WDT_TIMEOUT_20000MSEC)
        {
            ERROR("Invalid bite time index");
            return -EINVAL;
        }

        ulVal = RD_REG32(pxWdtHandle->ulBaseAddress + WDT_TORR);
        ulVal &= ~(WDT_TORR_TOP_MASK);
        ulVal |= (uTimeIndex << WDT_TORR_TOP_POS);
        WR_REG32(pxWdtHandle->ulBaseAddress + WDT_TORR, ulVal);
        pxWdtHandle->xIsConfigDone = 1;

        break;

    case eGetWatchdogBiteTime:
        ulVal = RD_REG32(pxWdtHandle->ulBaseAddress + WDT_TORR);
        ulVal &= WDT_TORR_TOP_MASK;
        *(uint32_t*) pvBuffer = ulVal;
        break;

    case eGetWatchdogStatus:

        uint32_t ulStat = RD_REG32(
                pxWdtHandle->ulBaseAddress + WDT_STAT);
        if (ulStat == 1U)
        {
            *(WdtStatus_t*) pvBuffer = eWdtTimerBarkExpired;
        }
        else
        {
            *(WdtStatus_t*) pvBuffer = eWdtTimerRunning;
        }
        break;

    case eSetWatchdogBiteBehaviour:
        if (set_response_mode(pxWdtHandle,
                *(WdtBiteConfig_t*) pvBuffer) == false)
        {
            ERROR("Failed to set response mode");
            ret = -EINVAL;
        }
        break;

    default:
        ERROR("Invalid IOCTL request");
        ret = -EINVAL;
        break;
    }
    return ret;
}

void wdt_set_callback( const WdtHandle_t pxWdtHandle,
        WdtCallback_t xCallback, void *pvUserContext )
{
    if (pxWdtHandle == NULL)
    {
        ERROR("Invalid watchdog handle");
        return;
    }
    pxWdtHandle->xCallBackFn = xCallback;
    pxWdtHandle->CbUsercontext = pvUserContext;
}

int32_t wdt_close( const WdtHandle_t pxWdtHandle )
{
    if (pxWdtHandle == NULL)
    {
        ERROR("Invalid watchdog handle");
        return -EINVAL;
    }
    if (!pxWdtHandle->xIsOpen)
    {
        ERROR("Watchdog instance is not open");
        return -EINVAL;
    }

    int32_t ulStop = wdt_stop(pxWdtHandle);
    if (ulStop != 0)
    {
        ERROR("Failed to stop watchdog timer");
        return -EINVAL;
    }
    pxWdtHandle->xIsOpen = 0;
    pxWdtHandle->xIsConfigDone = 0;
    return 0;
}

/** 
 * @brief Interrupt Service Routine for Watchdog timer
 */
void wdt_isr( void *pvhandle )
{
    WdtHandle_t pxWdtHandle = (WdtHandle_t) pvhandle;
    if (pxWdtHandle->xCallBackFn != NULL)
    {
        pxWdtHandle->xCallBackFn(pxWdtHandle->CbUsercontext);
    }
}



/**
 * @brief Set the response mode as system reset or generate interrupt and reset the system
 */
static bool set_response_mode(WdtHandle_t handle, WdtBiteConfig_t xConfig)
{
    uint32_t ulVal;
    SocfpgaHpuInterrupt_t InterruptId;
    SocfpgaInterruptErr_t xIntRet;

    if (xConfig == eWdtBiteTimerInterrupt)
    {
        ulVal = RD_REG32(handle->ulBaseAddress + WDT_CR);
        ulVal |= (1U << WDT_CR_RMOD_POS);
        WR_REG32(handle->ulBaseAddress + WDT_CR, ulVal);

        InterruptId = get_intr_id(handle->ulInstance);
        xIntRet = interrupt_enable(InterruptId, GIC_INTERRUPT_PRIORITY_WDOG);
        if (xIntRet != eErrOk)
        {
            return false;
        }
        xIntRet = interrupt_register_isr(InterruptId, wdt_isr,
                handle);
        if (xIntRet != eErrOk)
        {
            return false;
        }
    }
    else
    {
        ulVal = RD_REG32(handle->ulBaseAddress + WDT_CR);
        ulVal &= ~(1U << WDT_CR_RMOD_POS);
        WR_REG32(handle->ulBaseAddress + WDT_CR, ulVal);
    }
    return true;
}



/**
 * @brief Get the interrupt id based on instance
 */

SocfpgaHpuInterrupt_t get_intr_id(uint32_t instance)
{
    SocfpgaHpuInterrupt_t intr_id = eWdog0Irq;
    switch (instance)
    {
    case 0:
        intr_id = eWdog0Irq;
        break;

    case 1:
        intr_id = eWdog1Irq;
        break;

    case 2:
        intr_id = eWdog2Irq;
        break;

    case 3:
        intr_id = eWdog3Irq;
        break;

    case 4:
        intr_id = eWdog4Irq;
        break;

    default:
        /*Do nothing*/
        break;
    }

    return intr_id;
}

/** 
 * @brief Deassert the reset signal for the Watchdog instance
 */
static void de_assert_reset_signal( uint32_t instance )
{
    uint32_t val;
    if (instance == 0U)
    {
        val = RD_REG32(SOCFPGA_RESET_MANAGER_BASE + WDT_PER1MODRST_OFFSET);
        val &= ~(1U << WDT0_RESET_BIT);
        WR_REG32(SOCFPGA_RESET_MANAGER_BASE + WDT_PER1MODRST_OFFSET, val);
    }

    if (instance == 1U)
    {
        val = RD_REG32(SOCFPGA_RESET_MANAGER_BASE + WDT_PER1MODRST_OFFSET);
        val &= ~(1U << WDT1_RESET_BIT);
        WR_REG32(SOCFPGA_RESET_MANAGER_BASE + WDT_PER1MODRST_OFFSET, val);
    }

    if (instance == 2U)
    {
        val = RD_REG32(SOCFPGA_RESET_MANAGER_BASE + WDT_PER1MODRST_OFFSET);
        val &= ~(1U << WDT2_RESET_BIT);
        WR_REG32(SOCFPGA_RESET_MANAGER_BASE + WDT_PER1MODRST_OFFSET, val);
    }

    if (instance == 3U)
    {
        val = RD_REG32(SOCFPGA_RESET_MANAGER_BASE + WDT_PER1MODRST_OFFSET);
        val &= ~(1U << WDT3_RESET_BIT);
        WR_REG32(SOCFPGA_RESET_MANAGER_BASE + WDT_PER1MODRST_OFFSET, val);
    }

    if (instance == 4U)
    {
        val = RD_REG32(SOCFPGA_RESET_MANAGER_BASE + WDT_PER1MODRST_OFFSET);
        val &= ~((uint32_t)1U << WDT4_RESET_BIT);
        WR_REG32(SOCFPGA_RESET_MANAGER_BASE + WDT_PER1MODRST_OFFSET, val);
    }
}

