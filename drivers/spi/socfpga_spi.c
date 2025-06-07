/*
 * SPDX-FileCopyrightText: Copyright (C) 2025 Altera Corporation
 *
 * SPDX-License-Identifier: MIT-0
 *
 * HAL driver implementation for SPI
 */

#include <string.h>
#include "socfpga_defines.h"
#include "socfpga_spi.h"
#include "socfpga_spi_ll.h"
#include "socfpga_spi_reg.h"
#include "socfpga_interrupt.h"
#include "osal.h"
#include "osal_log.h"

#define MAX_INSTANCES    2U

struct SpiDescriptor
{
    BaseType_t xIsOpen;
    BaseType_t xIsRxBusy;
    BaseType_t xIsTxBusy;
    BaseType_t xIsRxAsync;
    BaseType_t xIsTxAsync;
    BaseType_t xIsRxOn;
    BaseType_t xIsTxOn;
    uint32_t lInstance;
    uint32_t ulBaseAddress;
    uint32_t slave_id;
    uint16_t usTxSize;
    uint16_t usRxSize;
    uint16_t usTxBytesLeft;
    uint16_t usRxBytesLeft;
    uint8_t *pucTxBuf;
    uint8_t *pucRxBuf;
    SpiCallback_t xCallbackFn;
    void *cbUserContext;
    osal_mutex_def_t xMutexMem;
    osal_semaphore_def_t xSemMem;
    osal_mutex_t xMutex;
    osal_semaphore_t xSem;
};

static struct SpiDescriptor spi_descriptor[ MAX_INSTANCES ];

void spi_isr( void *pvParam );

/**
 * @brief Check if the SPI handle is valid.
 */
static BaseType_t spi_is_handle_valid( SpiHandle_t handle )
{
    if ((handle == &spi_descriptor[ 0 ]) || (handle == &spi_descriptor[ 1 ]))
    {
        return true;
    }
    return false;
}

SpiHandle_t spi_open( uint32_t lSPIinstance )
{
    SpiHandle_t pxHandle;
    SocfpgaHpuInterrupt_t xIntId;
    SocfpgaInterruptErr_t xIntRet;

    if (lSPIinstance >= MAX_INSTANCES)
    {
        ERROR("Invalid SPI Instance");
        return NULL;
    }

    pxHandle = &spi_descriptor[ lSPIinstance ];
    if (pxHandle->xIsOpen != 0)
    {
        ERROR("SPI instance already open");
        return NULL;
    }

    (void) memset(pxHandle, 0, sizeof(struct SpiDescriptor));
    pxHandle->xIsOpen = 1;
    pxHandle->lInstance = lSPIinstance;
    pxHandle->ulBaseAddress = GET_BASE_ADDRESS(lSPIinstance);

    xIntId = SPI_GET_INT_ID(lSPIinstance);
    xIntRet = interrupt_register_isr(xIntId, spi_isr, pxHandle);
    if (xIntRet != eErrOk)
    {
        ERROR("Failed to register SPI interrupt");
        return NULL;
    }
    xIntRet = interrupt_enable(xIntId, GIC_INTERRUPT_PRIORITY_SPI);
    if (xIntRet != eErrOk)
    {
        ERROR("Failed to enable SPI interrupt");
        return NULL;
    }

    pxHandle->xMutex = osal_mutex_create(&pxHandle->xMutexMem);
    pxHandle->xSem = osal_semaphore_create(&pxHandle->xSemMem);

    spi_init(lSPIinstance);

    spi_disable_interrupt(pxHandle->ulBaseAddress, SPI_ALL_INTERRUPTS);

    return pxHandle;
}

int32_t spi_set_callback( SpiHandle_t const pxSPIPeripheral,
        SpiCallback_t xCallback, void *pvUserContext )
{
    if (pxSPIPeripheral == NULL)
    {
        ERROR("Invalid SPI handle");
        return -EINVAL;
    }

    if (pxSPIPeripheral->xIsRxBusy || pxSPIPeripheral->xIsTxBusy)
    {
        ERROR("SPI bus is busy");
        return -EBUSY;
    }

    pxSPIPeripheral->xCallbackFn = xCallback;
    pxSPIPeripheral->cbUserContext = pvUserContext;

    return 0;
}

int32_t spi_ioctl( SpiHandle_t const pxSPIPeripheral,
        SpiIoctlRequest_t xSPIRequest, void *const pvBuffer )
{
    SpiMasterConfig_t xConfig = {0};
    int32_t result = 0;

    if (!(spi_is_handle_valid(pxSPIPeripheral)))
    {
        ERROR("SPI Handle is invalid");
        return -EINVAL;
    }

    switch (xSPIRequest)
    {
    case eSPISetMasterConfig:
        if (pxSPIPeripheral->xIsTxBusy || pxSPIPeripheral->xIsRxBusy)
        {
            ERROR("SPI bus is busy");
            return -EBUSY;
        }
        if (pvBuffer == NULL)
        {
            ERROR("Buffer cannot be NULL");
            return -EINVAL;
        }

        xConfig = *(SpiMasterConfig_t*) pvBuffer;
        spi_disable(pxSPIPeripheral->ulBaseAddress);
        spi_set_config(pxSPIPeripheral->ulBaseAddress, xConfig.ulFreq,
                xConfig.eMode);
        spi_set_transfermode(pxSPIPeripheral->ulBaseAddress, SPI_TX_RX_MOD);
        spi_enable(pxSPIPeripheral->ulBaseAddress);
        break;

    case eSPIGetMasterConfig:
        if (pvBuffer == NULL)
        {
            ERROR("Buffer cannot be NULL");
            result = -EINVAL;
            break;
        }
        spi_get_config(pxSPIPeripheral->ulBaseAddress, &xConfig.ulFreq,
                &xConfig.eMode);
        *(SpiMasterConfig_t*) pvBuffer = xConfig;
        break;

    case eSPIGetTxNoOfbytes:
        if (pvBuffer == NULL)
        {
            ERROR("Buffer cannot be NULL");
            result = -EINVAL;
            break;
        }
        *(uint16_t*) pvBuffer = pxSPIPeripheral->usTxSize -
                pxSPIPeripheral->usTxBytesLeft;
        break;

    case eSPIGetRxNoOfbytes:
        if (pvBuffer == NULL)
        {
            ERROR("Buffer cannot be NULL");
            result = -EINVAL;
            break;
        }
        *(uint16_t*) pvBuffer = pxSPIPeripheral->usRxSize -
                pxSPIPeripheral->usRxBytesLeft;
        break;

    case eSPIActivateSlave:
        spi_select_chip(pxSPIPeripheral->lInstance, pxSPIPeripheral->slave_id);
        break;

    case eSPIDeactivateSlave:
        spi_select_chip(pxSPIPeripheral->lInstance, 0U);
        break;

    default:
        ERROR("Invalid IOCTL request");
        result = -EINVAL;
        break;
    }
    return result;
}

/**
 * @brief Start SPI transfer
 */
static void spi_transfer( SpiHandle_t pxSPIPeripheral, uint8_t *buffer,
        uint16_t xBytes )
{
    uint16_t usByteCount = 0;

    pxSPIPeripheral->usTxBytesLeft = xBytes;
    pxSPIPeripheral->usRxBytesLeft = xBytes;

    spi_enable_interrupt(pxSPIPeripheral->ulBaseAddress, SPI_RX_FULL_INT);

    if ((pxSPIPeripheral->xIsTxOn == true)
            && (pxSPIPeripheral->xIsRxOn == false))
    {
        pxSPIPeripheral->pucTxBuf += usByteCount;
    }

    else if ((pxSPIPeripheral->xIsTxOn == true)
            && (pxSPIPeripheral->xIsRxOn == true))
    {
        spi_select_chip(pxSPIPeripheral->lInstance, pxSPIPeripheral->slave_id);

        pxSPIPeripheral->pucTxBuf += usByteCount;
    }

    else
    {
        /* do nothing */
    }

    usByteCount = spi_write_fifo(pxSPIPeripheral->ulBaseAddress, buffer,
            xBytes);
    pxSPIPeripheral->usTxBytesLeft = pxSPIPeripheral->usTxBytesLeft -
            usByteCount;

    spi_enable_interrupt(pxSPIPeripheral->ulBaseAddress, SPI_TX_EMPTY_INT);
}

int32_t spi_write_sync( SpiHandle_t const pxSPIPeripheral,
        uint8_t *const pvBuffer, uint16_t xBytes )
{
    BaseType_t xTxSemReturn;

    if (!(spi_is_handle_valid(pxSPIPeripheral)) || (pvBuffer == NULL)
            || (xBytes == 0U))
    {
        ERROR("Invalid SPI handle or buffer");
        return -EINVAL;
    }

    if (osal_mutex_lock(pxSPIPeripheral->xMutex, OSAL_TIMEOUT_WAIT_FOREVER))
    {
        if (!pxSPIPeripheral->xIsOpen)
        {
            if (osal_mutex_unlock(pxSPIPeripheral->xMutex) == false)
            {
                ERROR("Failed to unlock mutex");
                return -EIO;
            }
            ERROR("SPI instance not open");
            return -EINVAL;
        }
        if (pxSPIPeripheral->xIsTxBusy != 0)
        {
            if (osal_mutex_unlock(pxSPIPeripheral->xMutex) == false)
            {
                ERROR("Failed to unlock mutex");
                return -EIO;
            }
            ERROR("SPI bus is busy");
            return -EBUSY;
        }
        pxSPIPeripheral->xIsTxBusy = true;
        pxSPIPeripheral->xIsTxAsync = false;
        pxSPIPeripheral->xIsTxOn = true;
        pxSPIPeripheral->xIsRxOn = false;
        if (osal_mutex_unlock(pxSPIPeripheral->xMutex) == false)
        {
            ERROR("Failed to unlock mutex");
            return -EIO;
        }
    }

    pxSPIPeripheral->pucTxBuf = pvBuffer;
    pxSPIPeripheral->usTxSize = xBytes;
    pxSPIPeripheral->usRxSize = 0;

    INFO("Starting SPI write in sync mode");
    spi_transfer(pxSPIPeripheral, pvBuffer, xBytes);

    xTxSemReturn = osal_semaphore_wait(pxSPIPeripheral->xSem, OSAL_TIMEOUT_WAIT_FOREVER);

    if (xTxSemReturn != 0)
    {
        pxSPIPeripheral->xIsTxBusy = false;
    }

    INFO("Completed SPI write in sync mode");
    return 0;
}

int32_t spi_write_async( SpiHandle_t const pxSPIPeripheral,
        uint8_t *const pvBuffer, uint16_t xBytes )
{
    if (!(spi_is_handle_valid(pxSPIPeripheral)) || (pvBuffer == NULL)
            || (xBytes == 0U))
    {
        ERROR("Invalid SPI handle or buffer");
        return -EINVAL;
    }

    if (osal_mutex_lock(pxSPIPeripheral->xMutex, OSAL_TIMEOUT_WAIT_FOREVER))
    {
        if (!pxSPIPeripheral->xIsOpen)
        {
            if (osal_mutex_unlock(pxSPIPeripheral->xMutex) == false)
            {
                ERROR("Failed to unlock mutex");
                return -EIO;
            }
            ERROR("SPI instance not open");
            return -EINVAL;
        }
        if (pxSPIPeripheral->xIsTxBusy != 0)
        {
            if (osal_mutex_unlock(pxSPIPeripheral->xMutex) == false)
            {
                ERROR("Failed to unlock mutex");
                return -EIO;
            }
            ERROR("SPI bus is busy");
            return -EBUSY;
        }
        pxSPIPeripheral->xIsTxBusy = true;
        pxSPIPeripheral->xIsTxAsync = true;
        pxSPIPeripheral->xIsTxOn = true;
        pxSPIPeripheral->xIsRxOn = false;
        if (osal_mutex_unlock(pxSPIPeripheral->xMutex) == false)
        {
            ERROR("Failed to unlock mutex");
            return -EIO;
        }
    }

    pxSPIPeripheral->pucTxBuf = pvBuffer;
    pxSPIPeripheral->usTxSize = xBytes;
    pxSPIPeripheral->usRxSize = 0;

    INFO("Starting SPI write in async mode");
    spi_transfer(pxSPIPeripheral, pvBuffer, xBytes);

    return 0;
}

int32_t spi_read_sync( SpiHandle_t const pxSPIPeripheral,
        uint8_t *const pvBuffer, uint16_t xBytes )
{
    BaseType_t xRxSemReturn;

    if (!(spi_is_handle_valid(pxSPIPeripheral)) || (pvBuffer == NULL)
            || (xBytes == 0U))
    {
        ERROR("Invalid SPI handle or buffer");
        return -EINVAL;
    }

    if (osal_mutex_lock(pxSPIPeripheral->xMutex, OSAL_TIMEOUT_WAIT_FOREVER))
    {
        if (!pxSPIPeripheral->xIsOpen)
        {
            if (osal_mutex_unlock(pxSPIPeripheral->xMutex) == false)
            {
                ERROR("Failed to unlock mutex");
                return -EIO;
            }
            ERROR("SPI instance not open");
            return -EINVAL;
        }
        if (pxSPIPeripheral->xIsRxBusy != 0)
        {
            if (osal_mutex_unlock(pxSPIPeripheral->xMutex) == false)
            {
                ERROR("Failed to unlock mutex");
                return -EIO;
            }
            ERROR("SPI bus is busy");
            return -EBUSY;
        }
        pxSPIPeripheral->xIsRxBusy = true;
        pxSPIPeripheral->xIsRxAsync = false;
        pxSPIPeripheral->xIsTxOn = false;
        pxSPIPeripheral->xIsRxOn = true;
        if (osal_mutex_unlock(pxSPIPeripheral->xMutex) == false)
        {
            ERROR("Failed to unlock mutex");
            return -EIO;
        }
    }

    pxSPIPeripheral->pucRxBuf = pvBuffer;
    pxSPIPeripheral->usRxSize = xBytes;
    pxSPIPeripheral->usTxSize = 0;

    INFO("Starting SPI read in sync mode");
    spi_transfer(pxSPIPeripheral, NULL, xBytes);

    xRxSemReturn = osal_semaphore_wait(pxSPIPeripheral->xSem, OSAL_TIMEOUT_WAIT_FOREVER);

    if (xRxSemReturn != 0)
    {
        pxSPIPeripheral->xIsRxBusy = false;
    }

    INFO("Completed SPI read in sync mode");
    return 0;
}

int32_t spi_read_async( SpiHandle_t const pxSPIPeripheral,
        uint8_t *const pvBuffer, uint16_t xBytes )
{
    if (!(spi_is_handle_valid(pxSPIPeripheral)) || (pvBuffer == NULL)
            || (xBytes == 0U))
    {
        ERROR("Invalid SPI handle or buffer");
        return -EINVAL;
    }

    if (osal_mutex_lock(pxSPIPeripheral->xMutex, OSAL_TIMEOUT_WAIT_FOREVER))
    {
        if (!pxSPIPeripheral->xIsOpen)
        {
            if (osal_mutex_unlock(pxSPIPeripheral->xMutex) == false)
            {
                ERROR("Failed to unlock mutex");
                return -EIO;
            }
            ERROR("SPI instance not open");
            return -EINVAL;
        }
        if (pxSPIPeripheral->xIsRxBusy != 0)
        {
            if (osal_mutex_unlock(pxSPIPeripheral->xMutex) == false)
            {
                ERROR("Failed to unlock mutex");
                return -EIO;
            }
            ERROR("SPI bus is busy");
            return -EBUSY;
        }
        pxSPIPeripheral->xIsRxBusy = true;
        pxSPIPeripheral->xIsRxAsync = true;
        if (osal_mutex_unlock(pxSPIPeripheral->xMutex) == false)
        {
            ERROR("Failed to unlock mutex");
            return -EIO;
        }
    }

    pxSPIPeripheral->pucRxBuf = pvBuffer;
    pxSPIPeripheral->usRxSize = xBytes;
    pxSPIPeripheral->usTxSize = 0;

    INFO("Starting SPI read in async mode");
    spi_transfer(pxSPIPeripheral, NULL, xBytes);

    return 0;
}

int32_t spi_transfer_sync( SpiHandle_t const pxSPIPeripheral,
        uint8_t *const pvTxBuffer, uint8_t *const pvRxBuffer, uint16_t xBytes )
{
    BaseType_t xRxSemReturn;

    if (!(spi_is_handle_valid(pxSPIPeripheral)) || (pvTxBuffer == NULL)
            || (pvRxBuffer == NULL) || (xBytes == 0U))
    {
        ERROR("Invalid SPI handle or buffer");
        return -EINVAL;
    }

    if (osal_mutex_lock(pxSPIPeripheral->xMutex, OSAL_TIMEOUT_WAIT_FOREVER))
    {
        if (!pxSPIPeripheral->xIsOpen)
        {
            if (osal_mutex_unlock(pxSPIPeripheral->xMutex) == false)
            {
                ERROR("Failed to unlock mutex");
                return -EIO;
            }
            ERROR("SPI instance not open");
            return -EINVAL;
        }
        if (pxSPIPeripheral->xIsTxBusy || pxSPIPeripheral->xIsRxBusy)
        {
            if (osal_mutex_unlock(pxSPIPeripheral->xMutex) == false)
            {
                ERROR("Failed to unlock mutex");
                return -EIO;
            }
            ERROR("SPI bus is busy");
            return -EBUSY;
        }
        pxSPIPeripheral->xIsTxBusy = true;
        pxSPIPeripheral->xIsRxBusy = true;
        pxSPIPeripheral->xIsTxAsync = false;
        pxSPIPeripheral->xIsRxAsync = false;
        pxSPIPeripheral->xIsRxOn = true;
        pxSPIPeripheral->xIsTxOn = true;
        if (osal_mutex_unlock(pxSPIPeripheral->xMutex) == false)
        {
            ERROR("Failed to unlock mutex");
            return -EIO;
        }
    }

    pxSPIPeripheral->pucRxBuf = pvRxBuffer;
    pxSPIPeripheral->pucTxBuf = pvTxBuffer;
    pxSPIPeripheral->usRxSize = xBytes;
    pxSPIPeripheral->usTxSize = xBytes;

    INFO("Starting SPI transfer in sync mode");
    spi_transfer(pxSPIPeripheral, pvTxBuffer, xBytes);

    xRxSemReturn = osal_semaphore_wait(pxSPIPeripheral->xSem, OSAL_TIMEOUT_WAIT_FOREVER);

    if (xRxSemReturn != 0)
    {
        pxSPIPeripheral->xIsRxBusy = false;
    }

    INFO("Completed SPI transfer in sync mode");
    return 0;
}

int32_t spi_transfer_async( SpiHandle_t const pxSPIPeripheral,
        uint8_t *const pvTxBuffer, uint8_t *const pvRxBuffer, uint16_t xBytes )
{
    if (!(spi_is_handle_valid(pxSPIPeripheral)) || (pvTxBuffer == NULL)
            || (pvRxBuffer == NULL) || (xBytes == 0U))
    {
        ERROR("Invalid SPI handle or buffer");
        return -EINVAL;
    }

    if (osal_mutex_lock(pxSPIPeripheral->xMutex, OSAL_TIMEOUT_WAIT_FOREVER))
    {
        if (!pxSPIPeripheral->xIsOpen)
        {
            if (osal_mutex_unlock(pxSPIPeripheral->xMutex) == false)
            {
                ERROR("Failed to unlock mutex");
                return -EIO;
            }
            ERROR("SPI instance not open");
            return -EINVAL;
        }
        if (pxSPIPeripheral->xIsTxBusy || pxSPIPeripheral->xIsRxBusy)
        {
            if (osal_mutex_unlock(pxSPIPeripheral->xMutex) == false)
            {
                ERROR("Failed to unlock mutex");
                return -EIO;
            }
            ERROR("SPI bus is busy");
            return -EBUSY;
        }
        pxSPIPeripheral->xIsTxBusy = true;
        pxSPIPeripheral->xIsRxBusy = true;
        pxSPIPeripheral->xIsTxAsync = true;
        pxSPIPeripheral->xIsRxAsync = true;
        if (osal_mutex_unlock(pxSPIPeripheral->xMutex) == false)
        {
            return -EIO;
        }
    }

    pxSPIPeripheral->pucRxBuf = pvRxBuffer;
    pxSPIPeripheral->pucTxBuf = pvTxBuffer;
    pxSPIPeripheral->usRxSize = xBytes;
    pxSPIPeripheral->usTxSize = xBytes;

    INFO("Starting SPI transfer in async mode");
    spi_set_transfermode(pxSPIPeripheral->ulBaseAddress, SPI_TX_RX_MOD);
    spi_transfer(pxSPIPeripheral, pvTxBuffer, xBytes);

    return 0;
}

int32_t spi_select_slave( SpiHandle_t const pxSPIPeripheral, uint32_t lSPISlave )
{
    if ((lSPISlave < 1U) || (lSPISlave > 4U))
    {
        ERROR("Invalid instance or slave ID");
        return -EINVAL;
    }

    pxSPIPeripheral->slave_id = lSPISlave;

    return 0;
}

int32_t spi_cancel( SpiHandle_t const pxSPIPeripheral )
{
    (void) pxSPIPeripheral;
    ERROR("Function not supported");
    return -ENOSYS;
}

int32_t spi_close( SpiHandle_t const pxSPIPeripheral )
{
    if (!spi_is_handle_valid(pxSPIPeripheral))
    {
        ERROR("Invalid SPI handle");
        return -EINVAL;
    }
    if (!pxSPIPeripheral->xIsOpen)
    {
        ERROR("SPI instance not open");
        return -EINVAL;
    }

    pxSPIPeripheral->xIsOpen = false;
    spi_deinit(pxSPIPeripheral->lInstance);

    return 0;
}

/**
 * @brief Interrupt handler for SPI
 */
void spi_isr( void *pvParam )
{
    SpiHandle_t pxSPIPeripheral;
    uint8_t id;
    uint16_t usRxByteCount;
    uint16_t usTxByteCount;

    pxSPIPeripheral = (SpiHandle_t) pvParam;
    if (pxSPIPeripheral == NULL)
    {
        return;
    }
    id = spi_get_interrupt_status(pxSPIPeripheral->ulBaseAddress);

    switch (id)
    {
    case SPI_RX_FULL_INT:
        if (pxSPIPeripheral->usRxBytesLeft > 0U)
        {
            if (pxSPIPeripheral->xIsRxOn == true)
            {
                usRxByteCount = spi_read_fifo(pxSPIPeripheral->ulBaseAddress,
                        pxSPIPeripheral->pucRxBuf,
                        pxSPIPeripheral->usRxBytesLeft);
                pxSPIPeripheral->usRxBytesLeft -= usRxByteCount;
                pxSPIPeripheral->pucRxBuf += usRxByteCount;
            }
            else
            {
                usRxByteCount = spi_read_fifo(pxSPIPeripheral->ulBaseAddress,
                        NULL, pxSPIPeripheral->usRxBytesLeft);
                pxSPIPeripheral->usRxBytesLeft -= usRxByteCount;
            }
        }
        if (pxSPIPeripheral->usRxBytesLeft == 0U)
        {
            spi_disable_interrupt(pxSPIPeripheral->ulBaseAddress,
                    SPI_RX_FULL_INT);

            if ((pxSPIPeripheral->xIsTxOn == true)
                    && (pxSPIPeripheral->xIsRxOn == true))
            {
                spi_select_chip(pxSPIPeripheral->lInstance, 0U);
            }

            if ((pxSPIPeripheral->xIsRxAsync == true)
                    || (pxSPIPeripheral->xIsTxAsync == true))
            {
                pxSPIPeripheral->xIsTxAsync = false;
                pxSPIPeripheral->xIsRxAsync = false;
                pxSPIPeripheral->xIsTxBusy = false;
                pxSPIPeripheral->xIsRxBusy = false;

                if (pxSPIPeripheral->xCallbackFn != NULL)
                {
                    pxSPIPeripheral->xCallbackFn(eSPISuccess,
                            pxSPIPeripheral->cbUserContext);
                }
            }
            else
            {
                (void) osal_semaphore_post(pxSPIPeripheral->xSem);
            }
        }
        break;

    case SPI_TX_EMPTY_INT:
        if (pxSPIPeripheral->usTxBytesLeft > 0U)
        {
            if (pxSPIPeripheral->xIsTxOn == true)
            {
                usTxByteCount = spi_write_fifo(pxSPIPeripheral->ulBaseAddress,
                        pxSPIPeripheral->pucTxBuf,
                        pxSPIPeripheral->usTxBytesLeft);
                pxSPIPeripheral->usTxBytesLeft -= usTxByteCount;
                pxSPIPeripheral->pucTxBuf += usTxByteCount;
            }
            else
            {
                usTxByteCount = spi_write_fifo(pxSPIPeripheral->ulBaseAddress,
                        NULL, pxSPIPeripheral->usTxBytesLeft);
                pxSPIPeripheral->usTxBytesLeft -= usTxByteCount;
            }
        }
        else
        {
            spi_disable_interrupt(pxSPIPeripheral->ulBaseAddress,
                    SPI_TX_EMPTY_INT);
        }

        break;

    default:
        /* do nothing */
        break;
    }
}
