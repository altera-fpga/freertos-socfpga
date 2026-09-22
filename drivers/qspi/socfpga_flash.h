/*
 * SPDX-FileCopyrightText: Copyright (C) 2025-2026 Altera Corporation
 *
 * SPDX-License-Identifier: MIT-0
 *
 * Header file for Flash driver
 */

#ifndef __SOCFPGA_FLASH_H__
#define __SOCFPGA_FLASH_H__

/*
    File: QSPI Flash Driver

    This file contains the flash driver definitions

    APIs for SoC FPGA QSPI Flash driver.

    This driver provides interfaces to perform flash operations with a flash
    device connected to the SoC FPGA QSPI interface.

    The driver supports blocking (sync) and non-blocking (async) functions.
    The async mode supports registering a callback to get notified on
    completion.

    The flash driver uses an adaptation layer which uses the SFDP protocol to
    fetch vendor specific information for different devices and uses this
    information for erase, read and write. To see example usage, see QSPI
    Sample Application.
*/

#include <stdint.h>
#include <errno.h>
#include "socfpga_qspi.h"
#include "socfpga_flash_adapter.h"


/*
    Group: Macros

    Types for the QSPI Flash driver.
*/


#define FLASH_SECTOR_SIZE    4096U                /* Flash  sector size. */
#define MAX_FLASH_DEV        4U                   /* Maximum number of flash devices supported. */
#define QSPI_DEV0            0U                   /* QSPI device number */

/*
    Type: flash_handle_t

    This is the structure used to hold the flash handle variables
*/
typedef struct flash_handle *flash_handle_t;
/*
    Group: Functions

    Entry points for the QSPI Flash driver.
*/
/*
    Function: flash_open

    Obtain a flash handle.

    Parameters:

        flash_num - Instance of the QSPI controller.

    Returns:

        - NULL if invalid argument is passed.
        - flash_handle_t if open is successful.
*/
flash_handle_t flash_open(uint32_t flash_num);

/*
    Function: flash_erase_sectors

    Erase flash sectors.

    Parameters:

        flash_handle - Flash handle.
        address - Start address.
        size - Total size in bytes.

    Returns:

        - -EINVAL: if invalid handle is passed.
        - -EIO:    if erase failed.
        - count   upon success returns the number of sectors erased.
*/
int flash_erase_sectors(flash_handle_t flash_handle, uint32_t address, uint32_t
        size);

/*
    Function: flash_write_sync

    Write data to the QSPI in indirect write mode synchronously.

    Parameters:

        flash_handle - Flash handle.
        address - Start address.
        data - Pointer to the data buffer.
        size - Total size in bytes.

    Returns:

        - -EINVAL:    if invalid arguments are passed.
        - -EBUSY:     if the device is in busy state.
        - -ETIMEDOUT: if failed to get release lock or semaphore.
        - -EIO:       if write operation failed.
        - 0:          if operation succeeded.
*/
int flash_write_sync(flash_handle_t flash_handle, uint32_t address,
        uint8_t *data, uint32_t size);

/*
    Function: flash_read_sync

    Read data from the QSPI in indirect read mode synchronously.

    Parameters:

        flash_handle - Flash handle.
        address - Start address.
        buffer - Pointer to the data buffer.
        size - Total size in bytes.

    Returns:

        - -EINVAL:    if invalid arguments are passed.
        - -ETIMEDOUT: if failed to get release lock or semaphore.
        - -EBUSY:     if handle is in use.
        - -EIO:       if the read operation failed.
        - 0:          if operation succeeded.
*/
int flash_read_sync(flash_handle_t flash_handle, uint32_t address,
        uint8_t *buffer, uint32_t size);

/*
    Function: flash_set_callback

    Set the callback function.

    Sets the callback function for QSPI async transfer.

    Parameters:

        flash_handle - Flash handle returned by open API.
        callback - Callback function pointer.
        puser_context - User defined context variable.

    Returns:

        - -EINVAL: if invalid arguments are used.
        - 0:       on success.
*/
int flash_set_callback(flash_handle_t const flash_handle, flash_callback_t
        callback, void *puser_context);

/*
    Function: flash_close

    Close the flash descriptor.

    Parameters:

        flash_handle - Flash handle.

    Returns:

        - -EINVAL: if invalid flash handle is used.
        - -EIO:    if the close operation failed.
        - -EFAULT: if failed to deinitialize mutex and semaphore.
        - 0:       if operation succeeded.
*/
int flash_close(flash_handle_t flash_handle);

#if QSPI_ENABLE_INT_MODE
/*
    Function: flash_write_async

    Write data to the QSPI in indirect write mode asynchronously.

    Parameters:

        flash_handle - Flash handle.
        address - Start address.
        data - Pointer to the data buffer.
        size - Total size in bytes.

    Returns:

        - -EINVAL:    if invalid arguments are passed.
        - -EBUSY:     if handle is in use.
        - -ETIMEDOUT: if failed to get release lock.
        - -EIO:       if the write  operation failed.
        - 0:          if operation succeeded.
*/
int flash_write_async(flash_handle_t flash_handle, uint32_t address,
        uint8_t *data, uint32_t size);

/*
    Function: flash_read_async

    Read data from the QSPI in indirect read mode asynchronously.

    Parameters:

        flash_handle - Flash handle.
        address - Start address.
        buffer - Pointer to the data buffer.
        size - Total size in bytes.

    Returns:

        - -EINVAL:    if invalid arguments are passed.
        - -EBUSY:     if handle is in use.
        - -ETIMEDOUT: if failed to get release lock.
        - -EIO:       if the read operation failed.
        - 0:          if operation succeeded.
*/
int flash_read_async(flash_handle_t flash_handle, uint32_t address,
        uint8_t *buffer, uint32_t size);
#endif

#endif // SOCFPGA_FLASH_H
