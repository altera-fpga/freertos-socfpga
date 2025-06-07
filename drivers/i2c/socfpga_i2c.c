/*
 * SPDX-FileCopyrightText: Copyright (C) 2025 Altera Corporation
 *
 * SPDX-License-Identifier: MIT-0
 *
 * HAL driver implementation for i2c
 */

#include "socfpga_interrupt.h"
#include "socfpga_i2c.h"
#include "socfpga_i2c_ll.h"
#include "socfpga_i2c_reg.h"
#include "socfpga_defines.h"
#include "socfpga_rst_mngr.h"
#include "osal_log.h"

struct I2cDescriptor
{
    uint32_t ulBaseAddress;
    uint32_t ulInstance;
    uint16_t usSlaveAddress;
    size_t usBytesLeft;
    size_t usRdCmdsLeft;
    size_t usXferSize;
    uint8_t *puBuffer;
    BaseType_t xIsAsync;
    BaseType_t xIsOpen;
    BaseType_t xIsBusy;
    BaseType_t xNoStopFlag;
    BaseType_t xIsXferAbort;
    I2cCallback_t xCallbackFn;
    void *cb_usercontext;
    osal_mutex_def_t xMutexMem;
    osal_semaphore_def_t xSemMem;
    osal_mutex_t xMutex;
    osal_semaphore_t xSem;
};

static struct I2cDescriptor i2cDesc[ MAX_I2C_INSTANCES ];

/**
 * @brief handle the interrupt
 */
void i2c_isr( void *vData );

/** 
 * @brief Get the reset instance for the I2C peripheral
 */
static ResetPeriphrl_t i2c_get_rst_instance( uint32_t lI2CInstance )
{
    ResetPeriphrl_t rst_instance = RST_PERIPHERAL_END;
    switch (lI2CInstance)
    {
    case 0U:
        rst_instance = RST_I2C0;
        break;
    case 1U:
        rst_instance = RST_I2C1;
        break;
    case 2U:
        rst_instance = RST_I2C2;
        break;
    case 3U:
        rst_instance = RST_I2C3;
        break;
    case 4U:
        rst_instance = RST_I2C4;
        break;
    default:
        rst_instance = RST_PERIPHERAL_END;
        break;
    }
    return rst_instance;
}

/** 
 * @brief Get the interrupt instance for the I2C peripheral
 */
static SocfpgaHpuInterrupt_t i2c_get_interrupt_instance( uint32_t lI2CInstance )
{
    SocfpgaHpuInterrupt_t interrupt_instance = eMaxHpuSpiInterrupt;
    switch (lI2CInstance)
    {
    case 0U:
        interrupt_instance = eI2c0Irq;
        break;
    case 1U:
        interrupt_instance = eI2c1Irq;
        break;
    case 2U:
        interrupt_instance = eI2c2Irq;
        break;
    case 3U:
        interrupt_instance = eI2c3Irq;
        break;
    case 4U:
        interrupt_instance = eI2c4Irq;
        break;
    default:
        interrupt_instance = eMaxHpuSpiInterrupt;
        break;
    }
    return interrupt_instance;
}

I2cHandle_t i2c_open( uint32_t lI2CInstance )
{
    I2cHandle_t handle;
    SocfpgaInterruptErr_t xIntRet;
    SocfpgaHpuInterrupt_t xIntId;
    ResetPeriphrl_t rst_instance;
    uint8_t reset_status;
    int32_t status;

    if (!(lI2CInstance < MAX_I2C_INSTANCES))
    {
        ERROR("Invalid I2C instance.");
        return NULL;
    }

    handle = &i2cDesc[ lI2CInstance ];
    if ((handle->xIsOpen) == 1)
    {
        ERROR("I2C instance already open.");
        return NULL;
    }
    else
    {
        (void) memset(handle, 0, sizeof(struct I2cDescriptor));

        handle->ulBaseAddress = GET_I2C_BASE_ADDRESS(lI2CInstance);
        handle->ulInstance = lI2CInstance;
        rst_instance = i2c_get_rst_instance(lI2CInstance);
        if (rst_instance == RST_PERIPHERAL_END)
        {
            ERROR("Invalid Reset Manager instance. ");
            return NULL;
        }
        status = rstmgr_get_reset_status(rst_instance, &reset_status);
        if (status != RSTMGR_SUCCESS)
        {
            ERROR("I2C block get reset status failed. ");
            return NULL;
        }
        if (reset_status != 0U)
        {
            status = rstmgr_toggle_reset(rst_instance);
            if (status != RSTMGR_SUCCESS)
            {
                ERROR("Failed to reset release I2C block. ");
                return NULL;
            }
        }
        i2c_init(handle->ulBaseAddress);

        handle->xIsOpen = true;
        handle->xMutex = osal_mutex_create(&handle->xMutexMem);
        handle->xSem = osal_semaphore_create(&handle->xSemMem);
        if (handle->xSem == NULL)
        {
            ERROR("Failed to create semaphore for I2C instance. ");
            return NULL;
        }
        xIntId = i2c_get_interrupt_instance(lI2CInstance);
        if (xIntId == eMaxHpuSpiInterrupt)
        {
            ERROR("Invalid interrupt instance. ");
            return NULL;
        }
        xIntRet = interrupt_register_isr(xIntId, i2c_isr, handle);
        if (xIntRet != eErrOk)
        {
            ERROR("Failed to register I2C interrupt handler. ");
            return NULL;
        }
        xIntRet = interrupt_enable(xIntId, GIC_INTERRUPT_PRIORITY_I2C);
        if (xIntRet != eErrOk)
        {
            ERROR("Failed to enable I2C interrupt");
            return NULL;
        }

        return handle;
    }
}

int32_t i2c_close( I2cHandle_t const pxI2CPeripheral )
{

    if ((pxI2CPeripheral == NULL) || !(pxI2CPeripheral->xIsOpen))
    {
        ERROR("Invalid I2C handle or I2C instance not open.");
        return -EINVAL;
    }
    else
    {
        i2c_disable_interrupt(pxI2CPeripheral->ulBaseAddress,
                I2C_TX_EMPTY_INT);
        i2c_disable_interrupt(pxI2CPeripheral->ulBaseAddress,
                I2C_RX_FULL_INT);
        pxI2CPeripheral->xIsOpen = false;
        return 0;
    }
}

void i2c_set_callback( I2cHandle_t const pxI2CPeripheral,
        I2cCallback_t xCallback, void *pvUserContext )
{
    if (pxI2CPeripheral == NULL)
    {
        ERROR("Invalid I2C handle.");
        return;
    }

    pxI2CPeripheral->xCallbackFn = xCallback;
    pxI2CPeripheral->cb_usercontext = pvUserContext;
}

int32_t i2c_ioctl( I2cHandle_t const pxI2CPeripheral,
        I2cIoctlRequest_t xI2CRequest, void *const pvBuffer )
{
    I2cConfig_t *config;
    int32_t ret = 0;
    uint16_t bytes_left;

    if ((pxI2CPeripheral == NULL) || (pxI2CPeripheral->xIsOpen == false))
    {
        ERROR("Invalid I2C handle or I2C instance not open.");
        return -EINVAL;
    }

    switch (xI2CRequest)
    {
    case eI2CSendNoStopFlag:
        pxI2CPeripheral->xNoStopFlag = true;
        break;

    case eI2CSetSlaveAddr:
        if ((pvBuffer == NULL))
        {
            ERROR("Buffer cannot be null");
            return -EINVAL;
        }
        pxI2CPeripheral->usSlaveAddress = *((uint16_t*) pvBuffer);
        i2c_set_target_addr(pxI2CPeripheral->ulBaseAddress,
                pxI2CPeripheral->usSlaveAddress);
        break;

    case eI2CSetMasterConfig:
        if ((pvBuffer == NULL))
        {
            ERROR("Buffer cannot be null");
            return -EINVAL;
        }
        config = (I2cConfig_t*) pvBuffer;
        if (i2c_config_master(pxI2CPeripheral->ulBaseAddress,
                config->ulBusFreq) == 0U)
        {
            ERROR("Failed to set I2C master configuration.");
            ret = -EINVAL;
        }
        break;

    case eI2CGetMasterConfig:
        if ((pvBuffer == NULL))
        {
            ERROR("Buffer cannot be null");
            return -EINVAL;
        }
        config = (I2cConfig_t*) pvBuffer;
        config->ulBusFreq = i2c_get_config(pxI2CPeripheral->ulBaseAddress);
        break;

    case eI2CGetBusState:
        if ((pvBuffer == NULL))
        {
            ERROR("Buffer cannot be null");
            return -EINVAL;
        }
        break;

    case eI2CGetTxNoOfbytes:
    /* fall through */
    case eI2CGetRxNoOfbytes:
        if ((pvBuffer == NULL))
        {
            ERROR("Buffer cannot be null");
            return -EINVAL;
        }
        bytes_left = (uint16_t) ((pxI2CPeripheral->usXferSize -
                pxI2CPeripheral->usBytesLeft) & 0xFFFFU);
        *(uint16_t*) pvBuffer = bytes_left;
        break;

    default:
        ERROR("Invalid IOCTL request");
        ret = -EINVAL;
        break;
    }
    return ret;
}

int32_t i2c_write_sync( I2cHandle_t const pxI2CPeripheral,
        uint8_t *const pucBuffer, size_t xBytes )
{
    if ((pxI2CPeripheral == NULL) || (pucBuffer == NULL) || (xBytes == 0U)
            || (!pxI2CPeripheral->xIsOpen))
    {
        ERROR("Invalid parameters");
        return -EINVAL;
    }

    if (pxI2CPeripheral->usSlaveAddress == 0U)
    {
        ERROR("Slave address not set");
        return -ENXIO;
    }

    if (osal_mutex_lock(pxI2CPeripheral->xMutex, 0xFFFFFFFFU))
    {
        if (!(pxI2CPeripheral->xIsOpen))
        {
            if (osal_mutex_unlock(pxI2CPeripheral->xMutex) == false)
            {
                return -EIO;
            }
            ERROR("Instance is not open");
            return -EINVAL;
        }

        if (pxI2CPeripheral->xIsBusy == 1)
        {
            if (osal_mutex_unlock(pxI2CPeripheral->xMutex) == false)
            {
                return -EIO;
            }
            ERROR("Instance is busy");
            return -EBUSY;
        }

        pxI2CPeripheral->xIsBusy = true;
        if (osal_mutex_unlock(pxI2CPeripheral->xMutex) == false)
        {
            return -EIO;
        }
    }

    pxI2CPeripheral->xIsAsync = false;
    pxI2CPeripheral->puBuffer = pucBuffer;
    pxI2CPeripheral->usXferSize = xBytes;
    pxI2CPeripheral->usBytesLeft = xBytes;

    INFO("Starting I2C sync write");
    xBytes = i2c_write_fifo(pxI2CPeripheral->ulBaseAddress, pucBuffer,
            pxI2CPeripheral->usXferSize, pxI2CPeripheral->xNoStopFlag);

    pxI2CPeripheral->usBytesLeft -= xBytes;
    pxI2CPeripheral->puBuffer += xBytes;
    i2c_enable_interrupt(pxI2CPeripheral->ulBaseAddress,
            I2C_TX_ABORT_INT | I2C_TX_EMPTY_INT);

    if (osal_semaphore_wait(pxI2CPeripheral->xSem, 0xFFFFFFFFU) == false)
    {
        return -EIO;
    }

    if (pxI2CPeripheral->xIsXferAbort == 1)
    {
        ERROR("Transfer aborted");
        return -EIO;
    }
    pxI2CPeripheral->xIsBusy = false;
    pxI2CPeripheral->xNoStopFlag = false;

    INFO("I2C write transfer completed");
    return 0;
}

int32_t i2c_write_async( I2cHandle_t const pxI2CPeripheral,
        uint8_t *const pucBuffer, size_t xBytes )
{
    if ((pxI2CPeripheral == NULL) || (pucBuffer == NULL) || (xBytes == 0U)
            || (!pxI2CPeripheral->xIsOpen))
    {
        ERROR("Invalid parameters");
        return -EINVAL;
    }

    if (pxI2CPeripheral->usSlaveAddress == 0U)
    {
        ERROR("Slave address not set");
        return -ENXIO;
    }

    if (osal_mutex_lock(pxI2CPeripheral->xMutex, 0xFFFFFFFFU))
    {
        if (!(pxI2CPeripheral->xIsOpen))
        {
            if (osal_mutex_unlock(pxI2CPeripheral->xMutex) == false)
            {
                return -EIO;
            }
            ERROR("Instance is not open");
            return -EINVAL;
        }

        if (pxI2CPeripheral->xIsBusy == 1)
        {
            if (osal_mutex_unlock(pxI2CPeripheral->xMutex) == false)
            {
                return -EIO;
            }
            ERROR("Instance is busy");
            return -EBUSY;
        }

        pxI2CPeripheral->xIsBusy = true;
        if (osal_mutex_unlock(pxI2CPeripheral->xMutex) == false)
        {
            return -EIO;
        }
    }
    pxI2CPeripheral->xIsAsync = true;
    pxI2CPeripheral->puBuffer = pucBuffer;
    pxI2CPeripheral->usXferSize = xBytes;
    pxI2CPeripheral->usBytesLeft = xBytes;

    INFO("Starting I2C async write");
    xBytes = i2c_write_fifo(pxI2CPeripheral->ulBaseAddress, pucBuffer,
            pxI2CPeripheral->usXferSize, pxI2CPeripheral->xNoStopFlag);

    pxI2CPeripheral->usBytesLeft -= xBytes;
    pxI2CPeripheral->puBuffer += xBytes;

    i2c_enable_interrupt(pxI2CPeripheral->ulBaseAddress,
            I2C_TX_ABORT_INT | I2C_TX_EMPTY_INT);

    return 0;
}

int32_t i2c_read_sync( I2cHandle_t const pxI2CPeripheral,
        uint8_t *const pucBuffer, size_t xBytes )
{
    if ((pxI2CPeripheral == NULL) || (pucBuffer == NULL) || (xBytes == 0U)
            || (!pxI2CPeripheral->xIsOpen))
    {
        ERROR("Invalid parameters");
        return -EINVAL;
    }

    if (pxI2CPeripheral->usSlaveAddress == 0U)
    {
        ERROR("Slave address not set");
        return -ENXIO;
    }

    if (osal_mutex_lock(pxI2CPeripheral->xMutex, 0xFFFFFFFFU))
    {
        if (!(pxI2CPeripheral->xIsOpen))
        {
            if (osal_mutex_unlock(pxI2CPeripheral->xMutex) == false)
            {
                return -EIO;
            }
            ERROR("Instance is not open");
            return -EINVAL;
        }

        if (pxI2CPeripheral->xIsBusy == 1)
        {
            if (osal_mutex_unlock(pxI2CPeripheral->xMutex) == false)
            {
                return -EIO;
            }
            ERROR("Instance is busy");
            return -EBUSY;
        }

        pxI2CPeripheral->xIsBusy = true;
        if (osal_mutex_unlock(pxI2CPeripheral->xMutex) == false)
        {
            return -EIO;
        }
    }

    pxI2CPeripheral->xIsAsync = false;
    pxI2CPeripheral->puBuffer = pucBuffer;
    pxI2CPeripheral->usXferSize = xBytes;
    pxI2CPeripheral->usBytesLeft = xBytes;
    pxI2CPeripheral->usRdCmdsLeft = xBytes;

    INFO("Starting I2C sync read");

    /* Enqueue the read commands */
    pxI2CPeripheral->usRdCmdsLeft -= i2c_enq_read_cmd(
            pxI2CPeripheral->ulBaseAddress, xBytes,
            pxI2CPeripheral->xNoStopFlag);

    i2c_enable_interrupt(pxI2CPeripheral->ulBaseAddress,
            I2C_TX_ABORT_INT | I2C_RX_FULL_INT);

    if (osal_semaphore_wait(pxI2CPeripheral->xSem, 0xFFFFFFFFU) == false)
    {
        return -EIO;
    }
    if (pxI2CPeripheral->xIsXferAbort == 1)
    {
        ERROR("Transfer aborted");
        return -EIO;
    }
    pxI2CPeripheral->xIsBusy = false;
    pxI2CPeripheral->xNoStopFlag = false;

    INFO("I2C read transfer completed");
    return 0;
}

int32_t i2c_read_async( I2cHandle_t const pxI2CPeripheral,
        uint8_t *const pucBuffer, size_t xBytes )
{
    if ((pxI2CPeripheral == NULL) || (pucBuffer == NULL) || (xBytes == 0U)
            || (!pxI2CPeripheral->xIsOpen))
    {
        ERROR("Invalid parameters");
        return -EINVAL;
    }

    if (pxI2CPeripheral->usSlaveAddress == 0U)
    {
        ERROR("Slave address not set");
        return -ENXIO;
    }

    if (osal_mutex_lock(pxI2CPeripheral->xMutex, 0xFFFFFFFFU))
    {
        if (!(pxI2CPeripheral->xIsOpen))
        {
            if (osal_mutex_unlock(pxI2CPeripheral->xMutex) == false)
            {
                return -EIO;
            }
            ERROR("Instance is not open");
            return -EINVAL;
        }

        if (pxI2CPeripheral->xIsBusy == 1)
        {
            if (osal_mutex_unlock(pxI2CPeripheral->xMutex) == false)
            {
                return -EIO;
            }
            ERROR("Instance is busy");
            return -EBUSY;
        }

        pxI2CPeripheral->xIsBusy = true;
        if (osal_mutex_unlock(pxI2CPeripheral->xMutex) == false)
        {
            return -EIO;
        }
    }

    pxI2CPeripheral->xIsAsync = true;
    pxI2CPeripheral->puBuffer = pucBuffer;
    pxI2CPeripheral->usXferSize = xBytes;
    pxI2CPeripheral->usBytesLeft = xBytes;
    pxI2CPeripheral->usRdCmdsLeft = xBytes;

    INFO("Starting I2C async read");
    /* Enqueue the read commands */
    pxI2CPeripheral->usRdCmdsLeft -= i2c_enq_read_cmd(
            pxI2CPeripheral->ulBaseAddress, xBytes,
            pxI2CPeripheral->xNoStopFlag);

    i2c_enable_interrupt(pxI2CPeripheral->ulBaseAddress,
            I2C_TX_ABORT_INT | I2C_RX_FULL_INT);

    return 0;
}

int32_t i2c_cancel( I2cHandle_t const pxI2CPeripheral )
{
    if ((pxI2CPeripheral == NULL) || !(pxI2CPeripheral->xIsOpen))
    {
        ERROR("Invalid parameters");
        return -EINVAL;
    }
    if (!pxI2CPeripheral->xIsBusy)
    {
        ERROR("I2C instance is not busy");
        return -EPERM;
    }
    i2c_ll_cancel(pxI2CPeripheral->ulBaseAddress);
    pxI2CPeripheral->xIsXferAbort = false;
    pxI2CPeripheral->xIsBusy = false;
    pxI2CPeripheral->xNoStopFlag = false;
    return 0;
}

void i2c_isr( void *vData )
{
    uint16_t xBytes;
    uint32_t baseAddr;
    BaseType_t noStopFlag;
    uint32_t status;

    I2cHandle_t pxI2cPeripheral = (I2cHandle_t) vData;
    if (pxI2cPeripheral == NULL)
    {
        return;
    }

    baseAddr = pxI2cPeripheral->ulBaseAddress;

    status = i2c_get_interrupt_status(baseAddr);
    i2c_clear_interrupt(pxI2cPeripheral->ulBaseAddress);
    noStopFlag = pxI2cPeripheral->xNoStopFlag;
    if ((status & I2C_TX_ABORT_INT) == I2C_TX_ABORT_INT)
    {
        i2c_disable_interrupt(baseAddr,
                I2C_TX_ABORT_INT | I2C_TX_EMPTY_INT | I2C_RX_FULL_INT);

        pxI2cPeripheral->xIsXferAbort = true;
        if ((pxI2cPeripheral->xIsAsync) == 1)
        {
            pxI2cPeripheral->xCallbackFn(eI2CNackFromSlave,
                    pxI2cPeripheral->cb_usercontext);
        }
        else
        {
            (void) osal_semaphore_post(pxI2cPeripheral->xSem);
        }
    }
    else
    {
        if ((status & I2C_TX_EMPTY_INT) == 1U)
        {
            if (pxI2cPeripheral->usBytesLeft > 0U)
            {
                xBytes = i2c_write_fifo(baseAddr, pxI2cPeripheral->puBuffer,
                        pxI2cPeripheral->usBytesLeft, noStopFlag);
                pxI2cPeripheral->usBytesLeft -= xBytes;
                pxI2cPeripheral->puBuffer += xBytes;
            }
            else
            {
                i2c_disable_interrupt(baseAddr, I2C_TX_EMPTY_INT);
                if ((pxI2cPeripheral->xIsAsync) == 1)
                {
                    pxI2cPeripheral->xIsBusy = false;
                    pxI2cPeripheral->xNoStopFlag = false;
                    pxI2cPeripheral->xCallbackFn(eI2CCompleted,
                            pxI2cPeripheral->cb_usercontext);
                }
                else
                {
                    (void) osal_semaphore_post(pxI2cPeripheral->xSem);
                }
            }
        }
        if ((status & I2C_RX_FULL_INT) == I2C_RX_FULL_INT)
        {
            /*
             * Before reading the received byte(s), enqueue next set of
             * read commands if pending
             */
            if (pxI2cPeripheral->usRdCmdsLeft > 0U)
            {
                pxI2cPeripheral->usRdCmdsLeft -= i2c_enq_read_cmd(baseAddr,
                        pxI2cPeripheral->usRdCmdsLeft, noStopFlag);
            }

            /* read the received bytes */
            if (pxI2cPeripheral->usBytesLeft > 0U)
            {
                xBytes = i2c_read_fifo(baseAddr, pxI2cPeripheral->puBuffer,
                        pxI2cPeripheral->usBytesLeft);
                pxI2cPeripheral->usBytesLeft -= xBytes;
                pxI2cPeripheral->puBuffer += xBytes;
            }
            if (pxI2cPeripheral->usBytesLeft == 0U)
            {
                i2c_disable_interrupt(baseAddr, I2C_RX_FULL_INT);

                if (pxI2cPeripheral->xIsAsync == 1)
                {
                    pxI2cPeripheral->xIsBusy = false;
                    pxI2cPeripheral->xNoStopFlag = false;
                    pxI2cPeripheral->xCallbackFn(eI2CCompleted,
                            pxI2cPeripheral->cb_usercontext);
                }
                else
                {
                    (void) osal_semaphore_post(pxI2cPeripheral->xSem);
                }
            }
        }
    }
}
