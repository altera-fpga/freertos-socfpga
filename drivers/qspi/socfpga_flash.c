/*
 * SPDX-FileCopyrightText: Copyright (C) 2025 Altera Corporation
 *
 * SPDX-License-Identifier: MIT-0
 *
 * HAL driver implementation for Flash
 */

#include <stdlib.h>
#include <stdio.h>
#include "osal_log.h"
#include "socfpga_flash_adapter.h"
#include "socfpga_flash.h"
#include "socfpga_qspi.h"


#define FLASH_MAX_WAIT_TIME    0xFFFFFFFFU        /*!< Maximum wait time for mutex lock. */

/*The Flash handle*/
struct FlashHandle {
    uint8_t slaveSelect;
    QspiDescriptor_t desc;
    FlashAdapter_t        *adapter;
    osal_mutex_def_t xMutexMem;
    osal_mutex_def_t xSemMem;
    BaseType_t xIsOpen;
    FlashCallback_t xflashCallback;
};

static struct FlashHandle gflash_handle = {
    .xIsOpen = 0,
    .desc.xIsBusy = 0,
    .desc.xSem    = NULL,
    .desc.xMutex  = NULL,
};

static struct FlashAdapter m25qAdapter = {
    .deviceId = 0,
    .sfdp = { { { 0 } } },
    .parseSfdp =  parseM25QParameters,
};

static struct FlashAdapter *adapterList[] = {
    &m25qAdapter
    /*Add other adapters*/
};

static int flash_read_sfdp(FlashHandle_t pFlash)
{
    uint64_t sfdp_header_raw, param_header_raw;
    uint32_t sfdp_header[2] = {0}, param_header[2] = {0}, signature;
    int32_t ret;
    struct SFDPObject *sfdp = &pFlash->adapter->sfdp;

    ret = QSPI_ReadSFDP(SFDP_START_ADDR, SFDP_HEADER_SIZE, &sfdp_header[0]);

    if (ret != QSPI_OK)
    {
        ERROR("SFDP header read failed.");
        return -EIO;
    }

    sfdp_header_raw =
            (uint64_t)((((uint64_t)sfdp_header[1]) <<
            SFDP_HEADER_MSB_POS) | (uint64_t)sfdp_header[0]);
    signature =
            (uint32_t) ((sfdp_header_raw >>
            SFDP_SGN_START_POS) & SFDP_HEADER_MASK);
    (void) memcpy(sfdp->stdHeader.signature, &signature, SFDP_SGN_NUM_BYTES);
    sfdp->stdHeader.minRev               =
            (uint8_t)(sfdp_header_raw >> (SFDP_MINREV_POS & SFDP_MINREV_MASK));
    sfdp->stdHeader.majorRev             =
            (uint8_t)(sfdp_header_raw >>
            (SFDP_MAJORREV_POS & SFDP_MAJORREV_MASK));
    sfdp->stdHeader.numParameterTables   =
            (uint8_t)(sfdp_header_raw >>
            (SFDP_NUM_PARAM_TABLES_POS & SFDP_NUM_PARAM_TABLES_MASK));


    ret =
            QSPI_ReadSFDP(SFDP_PARAM_START_ADDR, PARAM_HEADER_SIZE,
            &param_header[0]);

    if (ret != QSPI_OK)
    {
        ERROR("SFDP parameter header read failed");
        return -EIO;
    }

    param_header_raw =
            (uint64_t)((((uint64_t)param_header[1]) <<
            SFDP_PARAM_HEADER_MSB_POS) | (uint64_t)param_header[0]);
    for (uint8_t i = 0; i <  sfdp->stdHeader.numParameterTables; i++)
    {
        sfdp->paramHeader[i].parameterLength        =
                (uint8_t) (param_header_raw >>
                (SFDP_PARAM_LEN_POS & SFDP_PARAM_LEN_MASK));
        sfdp->paramHeader[i].parameterTableOffset   =
                (uint32_t) (param_header_raw >>
                (SFDP_PARAM_TABLE_OFFSET_POS & SFDP_PARAM_TABLE_OFFSET_MASK));
    }

    return 0;
}

FlashHandle_t flash_open(uint32_t flash_num)
{

    int status;
    FlashHandle_t flash_handle = &gflash_handle;
    if (flash_handle == NULL)
    {
        return NULL;
    }
    flash_handle->slaveSelect  = (uint8_t) flash_num;

    if (flash_num > (MAX_FLASH_DEV - 1U))
    {
        ERROR("Invalid device number");
        return NULL;
    }

    if (flash_handle->xIsOpen != 0)
    {
        ERROR("Device is already open");
        return NULL;
    }

    if (flash_num > ((sizeof(adapterList) / sizeof(adapterList[0])) - 1U))
    {
        ERROR("Invalid device number");
        return NULL;
    }

    flash_handle->adapter = adapterList[flash_num];
    if (flash_handle->adapter == NULL)
    {
        ERROR("Adapter layer not available");
        return NULL;
    }

    status = flash_read_sfdp(flash_handle);

    if (status != QSPI_OK)
    {
        ERROR("SFDP parametes read failed");
        return NULL;
    }
    status =  flash_handle->adapter->parseSfdp(&flash_handle->desc,
            &flash_handle->adapter->sfdp);
    if (status != 0)
    {
        ERROR("SFDP parameters parsing failed");
        return NULL;
    }
    status  = QSPI_Init(&flash_handle->desc);

    if (status != QSPI_OK)
    {
        ERROR("QSPI init failed" );
        return NULL;
    }

    if (flash_handle->desc.xMutex == NULL)
    {
        flash_handle->desc.xMutex   =
                osal_mutex_create(&flash_handle->xMutexMem);
        if (flash_handle->desc.xMutex == NULL)
        {
            ERROR("Mutex create failed");
            return NULL;
        }
    }
    if (flash_handle->desc.xSem == NULL)
    {
        flash_handle->desc.xSem = osal_semaphore_create(&flash_handle->xSemMem);
        if (flash_handle->desc.xSem == NULL)
        {
            ERROR("Semaphore create failed");
            return NULL;
        }
    }

    flash_handle->xIsOpen = 1;

    return flash_handle;
}

int  flash_set_callback( FlashHandle_t const flash_handle,
        FlashCallback_t xCallback, void *pvUserContext )
{
    int ret;
    if (flash_handle == NULL)
    {
        ERROR("Invalid flash handle");
        return -EINVAL;
    }

    /*Register the call back in the Flash layer*/
    flash_handle->xflashCallback   = xCallback;
    ret = QSPI_set_callback(&flash_handle->desc, &flash_handle->xflashCallback,
            pvUserContext);

    if (ret != QSPI_OK)
    {
        ERROR("Failed to set callback");
        return -EINVAL;
    }

    return 0;

}

int flash_erase_sectors(FlashHandle_t flash_handle, uint32_t address,
        uint32_t size)
{
    const uint32_t SECTOR_SIZE = 4096U;
    int32_t erase_count = 0;
    uint32_t sectorOffset = address & (SECTOR_SIZE - 1U);
    uint32_t remainLen;

    if (flash_handle == NULL)
    {
        ERROR("Invalid flash handle");
        return -EINVAL;
    }
    INFO("Erase started");

    /*If the size crosses to the next sector and the size is only 4k*/
    if (((sectorOffset + flash_handle->desc.page_size) >= SECTOR_SIZE) &&
            (size <= SECTOR_SIZE))
    {
        remainLen = SECTOR_SIZE - sectorOffset;
        if (QSPI_Erase(address) != 0)
        {
            ERROR("Erase failed");
            return -EIO;
        }
        address += remainLen;
        size    += remainLen;
        erase_count++;
    }

    while (size > 0U)
    {
        if (QSPI_Erase(address) != 0)
        {
            ERROR("Erase failed");
            return -EIO;
        }
        if (size <= SECTOR_SIZE)
        {
            erase_count++;
            break;
        }
        /*Update the next sector size*/
        address += SECTOR_SIZE;
        size     = (size > SECTOR_SIZE) ? size - SECTOR_SIZE : SECTOR_SIZE;
        erase_count++;
    }
    INFO("Erase completed");
    return erase_count;
}

int flash_write_sync(FlashHandle_t flash_handle, uint32_t address,
        uint8_t *data, uint32_t size)
{

    int ret = 0;
    uint32_t wCount = 0;

    if ((flash_handle == NULL) || (data == NULL) || (size == 0U))
    {
        ERROR("Inavlid arguments");
        return -EINVAL;
    }
    if (osal_mutex_lock(flash_handle->desc.xMutex, FLASH_MAX_WAIT_TIME))
    {
        if ((flash_handle->xIsOpen) == 0)
        {
            ERROR("Device is not open");
            return -EINVAL;
        }
        if (flash_handle->desc.xIsBusy != 0)
        {
            ERROR("Device is busy");
            return -EBUSY;
        }
        flash_handle->desc.xIsBusy = true;
        if (osal_mutex_unlock(flash_handle->desc.xMutex) == false)
        {
            ERROR("Mutex unlock failed");
            return -ETIME;
        }
    }
    flash_handle->desc.isWrOp         = true;
    flash_handle->desc.xIsAsync       = false;
#if QSPI_ENABLE_INT_MODE
    flash_handle->desc.puBuffer       = data;
    flash_handle->desc.usXferSize     = size;
    flash_handle->desc.usBytesLeft    = size;
    flash_handle->desc.start_addr     = address;

    INFO("Write sync transfer started");
    ret                       = QSPI_IndirectWrite(address, data,
            size, &wCount);

    if (ret != QSPI_OK)
    {
        ERROR("Write failed");
        return -EIO;
    }

    flash_handle->desc.usBytesLeft   -= wCount;
    flash_handle->desc.puBuffer      += wCount;
    flash_handle->desc.start_addr    += wCount;

    if (wCount < size)
    {
        if (size < QSPI_WRITE_WATER_LVL)
        {
            QSPI_Enable_Int(QSPI_INDDONE);
        }
        else
        {
            QSPI_Enable_Int(QSPI_INDDONE_AND_XFERBRCH);
        }
        if (osal_semaphore_wait(flash_handle->desc.xSem,
                FLASH_MAX_WAIT_TIME) == false)
        {
            ERROR("Write failed due to timeout");
            return -ETIMEDOUT;
        }
        QSPI_Disable_Int(QSPI_XFER_LVLBRCH | QSPI_IND_OPDONE);
    }
    INFO("Write sync transfer completed");
#else
    INFO("Write sync transfer started");
    ret  = QSPI_IndirectWrite(address, data, size, &wCount);
    if (ret != QSPI_OK)
    {
        ERROR("Write failed");
        return -EIO;
    }
    INFO("Write sync transfer completed");
#endif
    flash_handle->desc.xIsBusy = false;
    return ret;
}

#if QSPI_ENABLE_INT_MODE
int flash_write_async(FlashHandle_t flash_handle, uint32_t address,
        uint8_t *data, uint32_t size)
{
    uint32_t wCount = 0;
    int ret = 0;

    if ((flash_handle == NULL) || (data == NULL) || (size == 0U))
    {
        ERROR("Invalid arguments");
        return -EINVAL;
    }
    if (osal_mutex_lock(flash_handle->desc.xMutex, FLASH_MAX_WAIT_TIME))
    {
        if (!(flash_handle->xIsOpen))
        {
            ERROR("Device is not open");
            return -EINVAL;
        }
        if (flash_handle->desc.xIsBusy != 0)
        {
            ERROR("Devie is busy");
            return -EBUSY;
        }
        flash_handle->desc.xIsBusy = true;
        if (osal_mutex_unlock(flash_handle->desc.xMutex) == false)
        {
            ERROR("Mutex unlock failed");
            return -ETIME;
        }
    }

    flash_handle->desc.isWrOp         = true;
    flash_handle->desc.xIsAsync       = true;

    flash_handle->desc.puBuffer       = data;
    flash_handle->desc.usXferSize     = size;
    flash_handle->desc.usBytesLeft    = size;
    flash_handle->desc.start_addr     = address;

    INFO("Write async transfer started");
    ret                       =
            QSPI_IndirectWrite(address, data, size, &wCount);

    if (ret != QSPI_OK)
    {
        ERROR("Write failed");
        return -EIO;
    }

    flash_handle->desc.usBytesLeft   -= wCount;
    flash_handle->desc.puBuffer      += wCount;
    flash_handle->desc.start_addr    += wCount;

    if (wCount <= size)
    {
        if (size < QSPI_WRITE_WATER_LVL)
        {
            QSPI_Enable_Int(QSPI_INDDONE);
        }
        else
        {
            QSPI_Enable_Int(QSPI_INDDONE_AND_XFERBRCH);
        }
    }
    return 0;
}
#endif

int flash_read_sync(FlashHandle_t flash_handle, uint32_t address,
        uint8_t *buffer, uint32_t size)
{

    int ret = 0;
    uint32_t rCount = 0;
    if ((flash_handle == NULL) || (size == 0U) || (buffer == NULL))
    {
        ERROR("Invalid arguments");
        return -EINVAL;
    }
    if (osal_mutex_lock(flash_handle->desc.xMutex, FLASH_MAX_WAIT_TIME))
    {
        if ((flash_handle->xIsOpen) == 0)
        {
            ERROR("Device is not open");
            return -EINVAL;
        }
        if (flash_handle->desc.xIsBusy != 0)
        {
            ERROR("Device is busy");
            return -EBUSY;
        }
        flash_handle->desc.xIsBusy = true;
        if (osal_mutex_unlock(flash_handle->desc.xMutex) == false)
        {
            ERROR("Failed to unlock mutex");
            return -ETIME;
        }
    }
    flash_handle->desc.isWrOp         = false;
    flash_handle->desc.xIsAsync       = false;
#if QSPI_ENABLE_INT_MODE
    flash_handle->desc.puBuffer       = buffer;
    flash_handle->desc.usXferSize     = size;
    flash_handle->desc.usBytesLeft    = size;
    flash_handle->desc.start_addr     = address;

    INFO("Read sync transfer started");
    QSPI_Enable_Int(QSPI_INDDONE_AND_XFERBRCH);
    ret                       = QSPI_IndirectRead(address, buffer,
            size, &rCount);
    if (ret != QSPI_OK)
    {
        ERROR("Read failed");
        return -EIO;
    }

    flash_handle->desc.usBytesLeft   -= rCount;
    flash_handle->desc.puBuffer      += rCount;
    flash_handle->desc.start_addr    += rCount;

    if (rCount < size)
    {
        if (osal_semaphore_wait(flash_handle->desc.xSem,
                FLASH_MAX_WAIT_TIME) == false)
        {
            ERROR("Read failed due to timesout");
            return -ETIMEDOUT;
        }
    }
    QSPI_Disable_Int(QSPI_XFER_LVLBRCH | QSPI_IND_OPDONE);
    INFO("Read sync transfer completed");
#else
    INFO("Read sync transfer started");
    ret   = QSPI_IndirectRead(address, buffer, size, &rCount);
    if (ret != QSPI_OK)
    {
        ERROR("Read failed");
        return -EIO;
    }
    INFO("Read sync transfer completed");
#endif
    flash_handle->desc.xIsBusy = false;
    return ret;
}

#if QSPI_ENABLE_INT_MODE
int flash_read_async(FlashHandle_t flash_handle, uint32_t address,
        uint8_t *buffer, uint32_t size)
{
    uint32_t rCount = 0;
    int ret;

    if ((flash_handle == NULL) || (size == 0U) || (buffer == NULL))
    {
        ERROR("Invalid arguments");
        return -EINVAL;
    }
    if (osal_mutex_lock(flash_handle->desc.xMutex, FLASH_MAX_WAIT_TIME))
    {
        if (!(flash_handle->xIsOpen))
        {
            ERROR("Device is not open");
            return -EINVAL;
        }
        if (flash_handle->desc.xIsBusy != 0)
        {
            ERROR("Device is busy");
            return -EBUSY;
        }
        flash_handle->desc.xIsBusy = true;
        if (osal_mutex_unlock(flash_handle->desc.xMutex) == false)
        {
            ERROR("Mutex lock failed");
            return -ETIME;
        }
    }
    flash_handle->desc.isWrOp         = false;
    flash_handle->desc.xIsAsync       = true;
    flash_handle->desc.puBuffer       = buffer;
    flash_handle->desc.usXferSize     = size;
    flash_handle->desc.usBytesLeft    = size;
    flash_handle->desc.start_addr     = address;

    QSPI_Enable_Int(QSPI_INDDONE_AND_XFERBRCH);

    INFO("Read async transfer started");
    ret                       =
            QSPI_IndirectRead(address, buffer, size, &rCount);
    if (ret != QSPI_OK)
    {
        ERROR("Read failed");
        return -EIO;
    }

    flash_handle->desc.usBytesLeft   -= rCount;
    flash_handle->desc.puBuffer      += rCount;
    flash_handle->desc.start_addr    += rCount;

    return 0;
}
#endif

int flash_close(FlashHandle_t flash_handle)
{
    if ((flash_handle == NULL))
    {
        ERROR("Invalid flash handle");
        return -EINVAL;
    }

    if (!(flash_handle->xIsOpen))
    {
        ERROR("Device is not open");
        return -EINVAL;
    }

    if (QSPI_Deinit() != QSPI_OK)
    {
        ERROR("QSPI deinit failed");
        return -EIO;
    }

    if (osal_mutex_delete(flash_handle->desc.xMutex) == false)
    {
        ERROR("Mutex destroy failed");
        return -EFAULT;
    }
    if (osal_semaphore_delete(flash_handle->desc.xSem) == false)
    {
        ERROR("Semaphore destroy failed");
        return -EFAULT;
    }
    flash_handle->adapter = NULL;
    (void) memset(flash_handle, 0, sizeof(*flash_handle));

    return 0;
}
