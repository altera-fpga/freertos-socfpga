/*
 * SPDX-FileCopyrightText: Copyright (C) 2025 Altera Corporation
 *
 * SPDX-License-Identifier: MIT-0
 *
 * Header file for Flash driver
 */

#ifndef __SOCFPGA_FLASH_H__
#define __SOCFPGA_FLASH_H__

/**
 * @file  socfpga_flash.h
 * @brief This file contains the flash driver definitions 
 *
 * This driver provides interfaces to perform flash operations with a flash
 * device connected to the SoC FPGA QSPI interface.
 *
 * The driver supports blocking (sync) and non-blocking (async) functions. The
 * driver can be used in both polling mode and interrupt mode. In polling mode
 * only sync functions are available. In interrupt mode both sync and async
 * functions can be used. The async mode supports registering a callback to
 * get notified on completion. The macro QSPI_ENABLE_INT_MODE should be set as
 * 1 for interrupt mode and 0 for polling mode
 *
 * The flash driver uses an adapter layer which uses the SFDP protocol to fetch
 * vendor specific information for different devices and uses this information
 * for erase, read and write.
 *
 */

#include <stdint.h>
#include <errno.h>
#include "socfpga_qspi.h"
#include "socfpga_flash_adapter.h"

/**
 * @defgroup flash QSPI Flash
 * @ingroup drivers
 * @{
 */
/**
 * @defgroup flash_fns Functions
 * @ingroup flash
 * QSPI Flash HAL APIs
 */

/**
 * @defgroup flash_structs Structures 
 * @ingroup flash
 * QSPI Flash Specific Structures
 */

/**
 * @defgroup flash_macros Macros
 * @ingroup flash
 * QSPI Flash Specific Macros
 */

 /**
  * @addtogroup flash_macros
  * @{
  */


#define FLASH_SECTOR_SIZE            4096U        /*!< Flash  sector size. */
#define MAX_FLASH_DEV                4U           /*!< Maximum number of flash devices supported. */
#define QSPI_DEV0                    0U           /*!< QSPI device number */

/**
 * @}
 */

/**
 * @brief This is the structure used to hold the flash
 *       handle variables
 * @ingroup flash_structs
 */
typedef struct FlashHandle *FlashHandle_t;
/**
 * @addtogroup flash_fns
 * @{
 */
/**
 * @brief Obtain a flash handle.
 *
 * @param[in] flash_num Instance of the QSPI controller.
 *
 * @return
 * - NULL if invalid argument is passed.
 * - FlashHandle_t if open is successful.
 */
FlashHandle_t flash_open(uint32_t flash_num);

/**
 * @brief Obtain a flash descriptor.
 *
 * @param[in] flash_handle Flash handle.
 * @param[in] address Start address.
 * @param[in] size Total size in bytes.
 *
 * @return
 * - -EINVAL   if invalid handle is passed.
 * - -EIO      if erase failed.
 * - count    upon success returns the number of sectors erased.
 */
int flash_erase_sectors(FlashHandle_t flash_handle, uint32_t address,
        uint32_t size);

/**
 * @brief Write data to the QSPI in indirect write mode synchronously.
 *
 * @param[in] flash_handle Flash handle.
 * @param[in] address Start address.
 * @param[in] data Pointer to the data buffer.
 * @param[in] size Total size in bytes.
 *
 * @return
 * - -EINVAL    if invalid arguments are passed.
 * - -EBUSY     if the device us in busy state.
 * - -ETIME     if failed to get release lock.
 * - -EIO       if write operation failed.
 * - 0          if operation succeeded.
 */
int flash_write_sync(FlashHandle_t flash_handle, uint32_t address,
        uint8_t *data, uint32_t size);

/**
 * @brief Read data from the QSPI in indirect read mode synchronously.
 *
 * @param[in] flash_handle Flash handle.
 * @param[in] address Start address.
 * @param[out] buffer Pointer to the data buffer.
 * @param[in] size Total size in bytes.
 *
 * @return
 * - -EINVAL   if invalid arguments are passed.
 * - -EBUSY    if handle is in use.
 * - -EIO      if the read operation failed.
 * - 0         if operation succeeded.
 */
int flash_read_sync(FlashHandle_t flash_handle, uint32_t address,
        uint8_t *buffer, uint32_t size);

/**
 * @brief Set the callback function.
 *
 * Sets the callback function for QSPI async transfer.
 *
 * @param[in] flash_handle Flash handle returned by open API.
 * @param[in] xCallback Callback function pointer.
 * @param[in] pvUserContext User defined context variable.
 *
 * @return
 * - -EINVAL    if invalid arguements are used.
 * - 0          on success.
 */
int flash_set_callback(FlashHandle_t const flash_handle,
        FlashCallback_t xCallback, void *pvUserContext);

/**
 * @brief Close the flash descriptor.
 *
 * @param[in] flash_handle Flash handle.
 *
 * @return
 * - -EINVAL    if invalid flash handle is used.
 * - -EIO       if the close operation failed.
 * - -EFAULT    if failed to deinitialize mutex and semaphore.
 * - 0          if operation succeeded.
 */
int flash_close(FlashHandle_t flash_handle);

#if QSPI_ENABLE_INT_MODE
/**
 * @brief Write data to the QSPI in indirect write mode asynchronously.
 *
 * @param[in] flash_handle Flash handle.
 * @param[in] address Start address.
 * @param[in] data Pointer to the data buffer.
 * @param[in] size Total size in bytes.
 *
 * @return
 * - -EINVAL   if invalid arguments are passed.
 * - -EBUSY    if handle is in use.
 * - -EIO      if the write  operation failed.
 * - 0         if operation succeeded.
 */
int flash_write_async(FlashHandle_t flash_handle, uint32_t address,
        uint8_t *data, uint32_t size);

/**
 * @brief Read data from the QSPI in indirect read mode asynchronously.
 *
 * @param[in] flash_handle Flash handle.
 * @param[in] address Start address.
 * @param[out] buffer Pointer to the data buffer.
 * @param[in] size Total size in bytes.
 *
 * @return
 * - -EINVAL   if invalid arguments are passed.
 * - -EBUSY    if handle is in use.
 * - -EIO      if the read operation failed.
 * - 0         if operation succeeded.
 */
int flash_read_async(FlashHandle_t flash_handle, uint32_t address,
        uint8_t *buffer, uint32_t size);
#endif
/**
 * @}
 */
/**
 * @}
 */

#endif // SOCFPGA_FLASH_H

