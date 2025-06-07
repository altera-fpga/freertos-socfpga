/*
 * SPDX-FileCopyrightText: Copyright (C) 2025 Altera Corporation
 *
 * SPDX-License-Identifier: MIT-0
 *
 * HAL driver implementation for UART
 */
#include <string.h>
#include <errno.h>
#include "socfpga_uart.h"
#include "socfpga_uart_ll.h"
#include "socfpga_uart_reg.h"
#include "socfpga_interrupt.h"
#include "osal.h"

#define GET_INT_ID(instance)    (((instance) == 1U)?eUart1Irq:eUart0Irq)
struct UartDescriptor
{
    BaseType_t xIsOpen;
    uint32_t lInstance;
    BaseType_t xTxIsBusy;
    BaseType_t xRxIsBusy;
    BaseType_t xTxIsAsync;
    BaseType_t xRxIsAsync;
    uint32_t ulBaseAddress;
    size_t usTxBytesLeft;
    size_t usRxBytesLeft;
    size_t usTxSize;
    size_t usRxSize;
    uint8_t *pucTxBuf;
    uint8_t *pucRxBuf;
    UartCallback_t xCallbackFn;
    void *cbUserContext;
    osal_mutex_def_t xMutexMem;
    osal_semaphore_def_t xWrSemMem;
    osal_semaphore_def_t xRdSemMem;
    osal_mutex_t xMutex;
    osal_semaphore_t xRdSem;
    osal_semaphore_t xWrSem;
};

static struct UartDescriptor uart_descriptors[ UART_MAX_INSTANCE ];

void uart_isr( void *pvParam );

/**
 * @brief Check if the UART handle is valid
 */
static BaseType_t uart_isHandleValid( UartHandle_t handle )
{
    if ((handle == &uart_descriptors[ 0 ]) ||
            (handle == &uart_descriptors[ 1 ]))
    {
        return true;
    }
    return false;
}

UartHandle_t uart_open( uint32_t lUartInstance )
{
    UartHandle_t pxHandle;
    SocfpgaHpuInterrupt_t xIntId;
    SocfpgaInterruptErr_t xIntRet;

    if (lUartInstance >= UART_MAX_INSTANCE)
    {
        return NULL;
    }

    pxHandle = &(uart_descriptors[ lUartInstance ]);

    if (pxHandle->xIsOpen == true)
    {
        return NULL;
    }

    (void) memset(pxHandle, 0, sizeof(struct UartDescriptor));
    pxHandle->xIsOpen = 1;
    pxHandle->lInstance = lUartInstance;
    pxHandle->ulBaseAddress = GET_UART_BASE_ADDRESS(lUartInstance);

    xIntId = GET_INT_ID(lUartInstance);
    xIntRet = interrupt_register_isr(xIntId, uart_isr, pxHandle);
    if (xIntRet != eErrOk)
    {
        return NULL;
    }

    xIntRet = interrupt_enable(xIntId, GIC_INTERRUPT_PRIORITY_UART);
    if (xIntRet != eErrOk)
    {
        return NULL;
    }

    pxHandle->xMutex = osal_mutex_create(&pxHandle->xMutexMem);
    pxHandle->xRdSem = osal_semaphore_create(&pxHandle->xRdSemMem);
    pxHandle->xWrSem = osal_semaphore_create(&pxHandle->xWrSemMem);
    uart_init(lUartInstance);
    return pxHandle;
}

int32_t uart_ioctl( UartHandle_t const pxUartPeripheral,
        UartIoctlRequest_t xUartRequest, void *const pvBuffer )
{
    UartConfig_t xConfig = { 0 };
    int32_t res = 0;
    uint16_t bytesLeft;

    if (!(uart_isHandleValid(pxUartPeripheral)))
    {
        return -EINVAL;
    }
    if (pvBuffer == NULL)
    {
        return -EINVAL;
    }

    switch (xUartRequest)
    {
    case eUartSetConfig:
        if (pxUartPeripheral->xTxIsBusy || pxUartPeripheral->xRxIsBusy)
        {
            res = -EBUSY;
            break;
        }
        xConfig = *(UartConfig_t*) pvBuffer;

        uart_set_config(pxUartPeripheral->ulBaseAddress, xConfig.xParity,
                xConfig.xStopbits, xConfig.ucWordlength);

        if (uart_set_baud(pxUartPeripheral->ulBaseAddress, xConfig.ulBaudrate)
                != 1U)
        {
            res = -EINVAL;
        }
        break;

    case eUartGetConfig:
        uart_get_config(pxUartPeripheral->ulBaseAddress, &xConfig.ulBaudrate,
                &xConfig.xParity, &xConfig.xStopbits, &xConfig.ucWordlength);
        *(UartConfig_t*) pvBuffer = xConfig;
        break;

    case eGetTxNoOfbytes:
        bytesLeft = (uint16_t) ((pxUartPeripheral->usTxSize -
                pxUartPeripheral->usTxBytesLeft) & 0xFFFFU);
        *(uint16_t*) pvBuffer = bytesLeft;
        break;

    case eGetRxNoOfbytes:
    bytesLeft = (uint16_t) ((pxUartPeripheral->usRxSize -
                pxUartPeripheral->usRxBytesLeft) & 0xFFFFU);
        *(uint16_t*) pvBuffer = bytesLeft;
        break;

    case eGetTxState:
        *(int32_t *) pvBuffer = 0; 
        if (pxUartPeripheral->xTxIsBusy)
        {
            *(int32_t *) pvBuffer = -EBUSY;
        }
        break;

    case eGetRxState:
        *(int32_t *) pvBuffer = 0;
        if (pxUartPeripheral->xRxIsBusy)
        {
            *(int32_t *) pvBuffer = -EBUSY;
        }
        break;

    default:
        res = -EINVAL;
        break;
    }
    return res;
}

int32_t uart_set_callback( UartHandle_t const pxUartPeripheral,
        UartCallback_t xCallback, void *pvUserContext )
{
    if (pxUartPeripheral == NULL)
    {
        return -EINVAL;
    }

    if (pxUartPeripheral->xTxIsBusy || pxUartPeripheral->xRxIsBusy)
    {
        return -EBUSY;
    }

    pxUartPeripheral->xCallbackFn = xCallback;
    pxUartPeripheral->cbUserContext = pvUserContext;

    return 0;
}

/**
 * @brief Start writing data to transmit FIFO and enable tx interrupt
 * Update the number of bytes
 */
static void uart_write( UartHandle_t pxUartPeripheral, uint8_t *const pucBuffer,
        uint32_t xBytes )
{
    uint16_t usByteCount = 0U;
    pxUartPeripheral->usTxBytesLeft = xBytes;

    usByteCount = uart_write_fifo(pxUartPeripheral->ulBaseAddress, pucBuffer,
            xBytes);

    pxUartPeripheral->usTxBytesLeft = pxUartPeripheral->usTxBytesLeft -
            usByteCount;
    pxUartPeripheral->pucTxBuf += usByteCount;

    uart_enable_interrupt(pxUartPeripheral->ulBaseAddress, eInterruptTx);
}

/**
 * @brief Start reading data from receive FIFO
 * Enable rx interrupt if required
 */
static int32_t uart_read( UartHandle_t pxUartPeripheral,
        uint8_t *const pucBuffer, uint32_t xBytes )
{
    uint16_t usByteCount = 0U;
    pxUartPeripheral->usRxBytesLeft = xBytes;

    usByteCount = uart_read_fifo(pxUartPeripheral->ulBaseAddress, pucBuffer,
            xBytes);

    pxUartPeripheral->usRxBytesLeft -= usByteCount;
    pxUartPeripheral->pucRxBuf += usByteCount;

    if (pxUartPeripheral->usRxBytesLeft > 0U)
    {
        uart_enable_interrupt(pxUartPeripheral->ulBaseAddress, eInterruptRx);
    }
    else
    {
        if (pxUartPeripheral->xRxIsAsync == true)
        {

            if (pxUartPeripheral->xCallbackFn != NULL)
            {
                pxUartPeripheral->xCallbackFn(eUartReadCompleted,
                        pxUartPeripheral->cbUserContext);
            }
            pxUartPeripheral->xRxIsAsync = false;
            pxUartPeripheral->xRxIsBusy = false;
        }
        else
        {
            if (osal_semaphore_post(pxUartPeripheral->xRdSem) == false)
            {
                return -EIO;
            }
        }
    }

    return 0;
}

int32_t uart_write_async( UartHandle_t const pxUartPeripheral,
        uint8_t *const pvBuffer, uint32_t xBytes )
{
    if (!(uart_isHandleValid(pxUartPeripheral)) || (pvBuffer == NULL)
            || (xBytes == 0U))
    {
        return -EINVAL;
    }

    if (osal_mutex_lock(pxUartPeripheral->xMutex, OSAL_TIMEOUT_WAIT_FOREVER))
    {
        if (!(pxUartPeripheral->xIsOpen))
        {
            if (osal_mutex_unlock(pxUartPeripheral->xMutex) == false)
            {
                return -EIO;
            }
            return -EINVAL;
        }

        if (pxUartPeripheral->xTxIsBusy == true)
        {
            if (osal_mutex_unlock(pxUartPeripheral->xMutex) == false)
            {
                return -EIO;
            }
            return -EBUSY;
        }

        pxUartPeripheral->xTxIsBusy = true;
        pxUartPeripheral->xTxIsAsync = true;
        if (osal_mutex_unlock(pxUartPeripheral->xMutex) == false)
        {
            return -EIO;
        }
    }

    pxUartPeripheral->pucTxBuf = pvBuffer;
    pxUartPeripheral->usTxSize = xBytes;

    uart_write(pxUartPeripheral, pvBuffer, xBytes);

    return 0;
}

int32_t uart_write_sync( UartHandle_t const pxUartPeripheral,
        uint8_t *const pvBuffer, uint32_t xBytes )
{
    BaseType_t xTxSemReturn;
    if (!(uart_isHandleValid(pxUartPeripheral)) || (pvBuffer == NULL)
            || (xBytes == 0U))
    {
        return -EINVAL;
    }

    if (osal_mutex_lock(pxUartPeripheral->xMutex, OSAL_TIMEOUT_WAIT_FOREVER))
    {
        if (!(pxUartPeripheral->xIsOpen))
        {
            if (osal_mutex_unlock(pxUartPeripheral->xMutex) == false)
            {
                return -EIO;
            }
            return -EINVAL;
        }

        if (pxUartPeripheral->xTxIsBusy == true)
        {
            if (osal_mutex_unlock(pxUartPeripheral->xMutex) == false)
            {
                return -EIO;
            }
            return -EBUSY;
        }

        pxUartPeripheral->xTxIsBusy = true;
        pxUartPeripheral->xTxIsAsync = false;
        if (osal_mutex_unlock(pxUartPeripheral->xMutex) == false)
        {
            return -EIO;
        }
    }

    pxUartPeripheral->pucTxBuf = pvBuffer;
    pxUartPeripheral->usTxSize = xBytes;

    uart_write(pxUartPeripheral, pvBuffer, xBytes);

    xTxSemReturn = osal_semaphore_wait(pxUartPeripheral->xWrSem, OSAL_TIMEOUT_WAIT_FOREVER);
    if (xTxSemReturn == 1)
    {
        pxUartPeripheral->xTxIsBusy = false;
    }

    return 0;
}

int32_t uart_read_async( UartHandle_t const pxUartPeripheral,
        uint8_t *const pvBuffer, uint32_t xBytes )
{
    if (!(uart_isHandleValid(pxUartPeripheral)) || (pvBuffer == NULL)
            || (xBytes == 0U))
    {
        return -EINVAL;
    }

    if (osal_mutex_lock(pxUartPeripheral->xMutex, OSAL_TIMEOUT_WAIT_FOREVER))
    {
        if (!(pxUartPeripheral->xIsOpen))
        {
            if (osal_mutex_unlock(pxUartPeripheral->xMutex) == false)
            {
                return -EIO;
            }
            return -EINVAL;
        }

        if (pxUartPeripheral->xRxIsBusy == true)
        {
            if (osal_mutex_unlock(pxUartPeripheral->xMutex) == false)
            {
                return -EIO;
            }
            return -EBUSY;
        }

        pxUartPeripheral->xRxIsBusy = true;
        pxUartPeripheral->xRxIsAsync = true;
        if (osal_mutex_unlock(pxUartPeripheral->xMutex) == false)
        {
            return -EIO;
        }
    }

    pxUartPeripheral->pucRxBuf = pvBuffer;
    pxUartPeripheral->usRxSize = xBytes;

    if (uart_read(pxUartPeripheral, pvBuffer, xBytes) != 0)
    {
        return -EIO;
    }

    return 0;
}

int32_t uart_read_sync( UartHandle_t const pxUartPeripheral,
        uint8_t *const pvBuffer, uint32_t xBytes )
{
    BaseType_t xRxSemReturn;

    if (!(uart_isHandleValid(pxUartPeripheral)) || (pvBuffer == NULL)
            || (xBytes == 0U))
    {
        return -EINVAL;
    }
    if (osal_mutex_lock(pxUartPeripheral->xMutex, OSAL_TIMEOUT_WAIT_FOREVER))
    {
        if (!(pxUartPeripheral->xIsOpen))
        {
            if (osal_mutex_unlock(pxUartPeripheral->xMutex) == false)
            {
                return -EIO;
            }
            return -EINVAL;
        }

        if (pxUartPeripheral->xRxIsBusy == true)
        {
            if (osal_mutex_unlock(pxUartPeripheral->xMutex) == false)
            {
                return -EIO;
            }
            return -EBUSY;
        }

        pxUartPeripheral->xRxIsBusy = true;
        pxUartPeripheral->xRxIsAsync = false;
        if (osal_mutex_unlock(pxUartPeripheral->xMutex) == false)
        {
            return -EIO;
        }
    }
    pxUartPeripheral->pucRxBuf = pvBuffer;
    pxUartPeripheral->usRxSize = xBytes;

    if (uart_read(pxUartPeripheral, pvBuffer, xBytes) != 0)
    {
        return -EIO;
    }

    xRxSemReturn = osal_semaphore_wait(pxUartPeripheral->xRdSem, OSAL_TIMEOUT_WAIT_FOREVER);

    if (xRxSemReturn == 0)
    {
        uart_disable_interrupt(pxUartPeripheral->ulBaseAddress, eInterruptRx);
    }
    else
    {
        pxUartPeripheral->xRxIsBusy = false;
    }

    return 0;
}

int32_t uart_cancel( UartHandle_t const pxUartPeripheral )
{
    if (uart_isHandleValid(pxUartPeripheral) || (pxUartPeripheral == NULL))
    {
        return -EINVAL;
    }
    return -ENOSYS;
}

int32_t uart_close( UartHandle_t const pxUartPeripheral )
{

    if ((pxUartPeripheral == NULL) || (pxUartPeripheral->xIsOpen == 0))
    {
        return -EINVAL;
    }

    if (osal_semaphore_delete(pxUartPeripheral->xRdSem) == false)
    {
        return -EFAULT;
    }
    if (osal_semaphore_delete(pxUartPeripheral->xWrSem) == false)
    {
        return -EFAULT;
    }

    uart_deinit(pxUartPeripheral->lInstance);
    pxUartPeripheral->xIsOpen = 0;

    return 0;
}

/**
 * @brief Interrupt handler for UART
 */
void uart_isr( void *pvParam )
{
    UartHandle_t pxUartPeripheral;
    uint32_t id;
    uint16_t usRxByteCount;
    uint16_t usTxByteCount;

    pxUartPeripheral = (UartHandle_t) pvParam;
    if (uart_isHandleValid(pxUartPeripheral) == 0)
    {
        return;
    }

    id = get_int_status(pxUartPeripheral->ulBaseAddress);

    switch (id)
    {
    case UART_RXBUF_RDY_INT:
        if (pxUartPeripheral->usRxBytesLeft > 0U)
        {
            usRxByteCount = uart_read_fifo(pxUartPeripheral->ulBaseAddress,
                    pxUartPeripheral->pucRxBuf,
                    pxUartPeripheral->usRxBytesLeft);

            pxUartPeripheral->usRxBytesLeft -= usRxByteCount;
            pxUartPeripheral->pucRxBuf += usRxByteCount;
        }
        if (pxUartPeripheral->usRxBytesLeft == 0U)
        {
            uart_disable_interrupt(pxUartPeripheral->ulBaseAddress,
                    eInterruptRx);

            if (pxUartPeripheral->xRxIsAsync == true)
            {

                if (pxUartPeripheral->xCallbackFn != NULL)
                {
                    pxUartPeripheral->xCallbackFn(eUartReadCompleted,
                            pxUartPeripheral->cbUserContext);
                }
                pxUartPeripheral->xRxIsAsync = false;
                pxUartPeripheral->xRxIsBusy = false;
            }
            else
            {
                (void) osal_semaphore_post(pxUartPeripheral->xRdSem);
            }
        }

        break;

    case UART_TXBUF_EMPTY_INT:
        if (pxUartPeripheral->usTxBytesLeft > 0U)
        {
            usTxByteCount = uart_write_fifo(pxUartPeripheral->ulBaseAddress,
                    pxUartPeripheral->pucTxBuf,
                    pxUartPeripheral->usTxBytesLeft);

            pxUartPeripheral->usTxBytesLeft = pxUartPeripheral->usTxBytesLeft -
                    usTxByteCount;
            pxUartPeripheral->pucTxBuf += usTxByteCount;
        }
        else
        {
            uart_disable_interrupt(pxUartPeripheral->ulBaseAddress,
                    eInterruptTx);
            if (pxUartPeripheral->xTxIsAsync == true)
            {

                if (pxUartPeripheral->xCallbackFn != NULL)
                {
                    pxUartPeripheral->xCallbackFn(eUartWriteCompleted,
                            pxUartPeripheral->cbUserContext);
                }
                pxUartPeripheral->xTxIsAsync = false;
                pxUartPeripheral->xTxIsBusy = false;
            }
            else
            {
                (void) osal_semaphore_post(pxUartPeripheral->xWrSem);
            }
        }
        break;

    case UART_HW_ERR_INT:
        break;

    default:
        /* do nothing */
        break;
    }
}
