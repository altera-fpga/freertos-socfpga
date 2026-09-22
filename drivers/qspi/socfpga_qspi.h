/*
 * SPDX-FileCopyrightText: Copyright (C) 2025-2026 Altera Corporation
 *
 * SPDX-License-Identifier: MIT-0
 *
 * Header file for QSPI HAL
 */

#ifndef __SOCFPGA_QSPI_H__
#define __SOCFPGA_QSPI_H__

#include <stdint.h>
#include <stdbool.h>
#include "socfpga_defines.h"

#define QSPI_ENABLE_INT_MODE    1U

#define QSPI_BUSY                      1
#define QSPI_OK                        0
#define QSPI_ERROR                     -1
#define QSPI_TIMEOUT                   0x10000000U
#define QSPI_WRITE_IN_PROGRESS_FLAG    0x01
#define QSPI_INDIRECT_OP_DONE_FLAG     0x02

#define QSPI_INDDONE                 1U
#define QSPI_INDDONE_AND_XFERBRCH    2U

#define QSPI_WRITE_WATER_LVL    100U

#define QSPI_ALL_INT_MASK    0x1ffeU

#define QSPI_WRITE_ENABLE_CMD           0x06U
#define QSPI_WRITE_DISABLE_CMD          0x04U
#define QSPI_SECTOR_ERASE_CMD           0x20U
#define QSPI_READ_SFDP_CMD              0x5aU
#define QSPI_READ_STATUS_CMD            0x5U
#define QSPI_READ_STATUS_POS            0x1U
#define QSPI_READ_FLAG_STATUS_CMD       0x70U
#define QSPI_READ_FLAG_STATUS_POS       0x80U
#define QSPI_CLEAR_FLAG_STATUS_CMD      0x9fU
#define QSPI_EXT_REG_CMD                0xc5U
#define QSPI_PAGE_SIZE                  0x00000100U
#define QSPI_BANK_SIZE                  0x01000000U
#define QSPI_BANK_ADDR_POS              24U
#define QSPI_BANK_ADDR_OFFSET           0xff000000U
#define QSPI_INDONE_OPSTATUS_MASK       0x1U
#define QSPI_SRAMFILL_WR_STATUS_POS     16U
#define QSPI_SRAMFILL_RD_STATUS_POS     0U
#define QSPI_SRAMFILL_WR_STATUS_MASK    0x00FFFFU
#define QSPI_SRAMFILL_RD_STATUS_MASK    0x00FFFFU
#define QSPI_SRAM_RD_CAP_MASK           0x3FFU
#define QSPI_TOTAL_SRAM_SIZE            1024U

#define SFDP_NSS_DELAY      0x14U
#define SFDP_BTWN_DELAY     0x14U
#define SFDP_AFTER_DELAY    0xffU
#define SFDP_INIT_DELAY     0xc8U
#define SFDP_BAUDDIV        0xfU
#define SFDP_INST_WIDTH     0U
#define SFDP_DATA_WIDTH     0U
#define SFDP_ADDR_WIDTH     0U
#define SFDP_DUMMY_DELAY    0U

#define QSPI_IND_OPDONE      0x4U
#define QSPI_XFER_LVLBRCH    0x40U

/*
    File: QSPI Driver

    Declares the QSPI driver interface: 21 functions, 3 types and 43
    constants.
*/

/*
    Callback: flash_callback_t

    This is the FlashCallback
*/
typedef void (*flash_callback_t)(uint32_t op_status, void *puser_context);

/*
    Type: qspi_callback_t

    This is the QspiCallback
*/
typedef flash_callback_t *qspi_callback_t;

/*
    Struct: qspi_descriptor_t

    This is the structure used to hold the flash descriptor variables
*/
typedef struct qpsi_descriptor
{
    uint32_t inst_width;
    uint32_t addr_width;
    uint32_t data_width;
    uint32_t baud_div;
    uint64_t flash_size;
    uint32_t sector_size;
    uint32_t page_size;
    uint32_t clock_freq;
    uint32_t nss_delay;
    uint32_t init_delay;
    uint32_t after_delay;
    uint32_t btwn_delay;
    uint32_t num_addr_bytes;
    uint8_t dummy_cycles;
    uint8_t qspi_mode;

    osal_semaphore_t sem;
    osal_semaphore_t mutex;
    void *cb_usercontext;
    uint32_t start_addr;
    uint32_t bytes_left;
    uint32_t xfer_size;
    uint8_t *buffer;
    BaseType_t is_wr_op;
    BaseType_t is_async;
    BaseType_t is_busy;
    qspi_callback_t xqspi_callback;
} qspi_descriptor_t;

/*
    Function: qspi_enable_int

    Enable the interrupt.

    Parameters:

        type - Flag to indicate the interrupts to be enabled
*/
void qspi_enable_int(uint32_t type);

/*
    Function: qspi_disable_int

    Disable interrupts.

    Parameters:

        mask - for interrupts to be disabled
*/
void qspi_disable_int(uint32_t mask);

/*
    Function: qspi_read_sfdp

    Read the SFDP parameters.

    Parameters:

        address - Address to read from.
        size - Length of data to be read.
        data - Pointer to the data.

    Returns:

        - QSPI_ERROR: if the operation is not successful.
        - QSPI_OK:    if the operation is successful.
*/
int32_t qspi_read_sfdp(uint32_t address, uint8_t size, uint32_t *data);

/*
    Function: qspi_send_flash_readcmd

    Send command to flash to read data.

    Parameters:

        cmd - Flash handle to do the operation on.
        numbytes - Number of bytes to read.
        data - Pointer to buffer to write data to.

    Returns:

        - QSPI_ERROR: if the operation is not successful.
        - QSPI_OK:    if the operation is successful.
*/
int32_t qspi_send_flash_readcmd(uint8_t cmd, uint8_t numbytes, uint32_t *data);

/*
    Function: qspi_send_flash_writecmd

    Send a flash write or erase command.

    Parameters:

        cmd - Command to be sent to the flash device.
        numbytes - Number of bytes to write.
        data - Pointer to the data buffer.

    Returns:

        - QSPI_ERROR: if the operation is not successful.
        - QSPI_OK:    if the operation is successful.
*/
int32_t qspi_send_flash_writecmd(uint8_t cmd, uint8_t numbytes, uint32_t *data);

/*
    Function: qspi_select_bank

    To select the 16MB bank.

    Parameters:

        bank_addr - The bank address to be selected.

    Returns:

        - QSPI_ERROR: if the operation is not successful.
        - QSPI_OK:    if the operation is successful.
*/
int32_t qspi_select_bank(uint32_t *bank_addr);

/*
    Function: qspi_send_flashcmd

    Send a command to the flash chip.

    Parameters:

        cmd - Command to be sent to the flash chip.

    Returns:

        - QSPI_ERROR: if the operation is not successful.
        - QSPI_OK:    if the operation is successful.
*/
int32_t qspi_send_flashcmd(uint32_t cmd);

/*
    Function: qspi_write_finish

    Verify if the write operation is completed in the flash chip.

    Returns:

        - QSPI_ERROR: if the operation is not successful.
        - QSPI_OK:    if the operation is successful.
*/
int32_t qspi_write_finish(void);

/*
    Function: qspi_page_write_indirect

    Write data to the flash chip in pages (256 bytes).

    Parameters:

        offset - Starting page offset.
        data - Pointer to the buffer containing the data.
        size - size of the data.
        count - number of bytes written.

    Returns:

        - QSPI_ERROR: if the operation is not successful.
        - QSPI_OK:    if the operation is successful.
*/
int32_t qspi_page_write_indirect(uint32_t offset, uint8_t *data, uint32_t size,
        uint32_t *count);

/*
    Function: qspi_bank_write_indirect

    Write data to the flash chip in banks (16 MB).

    Parameters:

        bank_offset - Starting offset.
        data - Pointer to the buffer containing the data.
        size - The size of the data.
        w_count - number of bytes written.

    Returns:

        - QSPI_ERROR: if the operation is not successful.
        - QSPI_OK:    if the operation is successful.
*/
int32_t qspi_bank_write_indirect(uint32_t bank_offset, uint8_t *data, uint32_t
        size, uint32_t *w_count);

/*
    Function: qspi_indirect_write

    Write to the flash chip in indirect mode.

    Parameters:

        address - Start address for indirect write.
        data - Data buffer containing write data.
        size - Size in bytes of the data to be written to the flash
            device.
        w_count - number of bytes written.

    Returns:

        - QSPI_ERROR: if the write operation failed.
        - QSPI_OK:   if the write is successful.
*/
int32_t qspi_indirect_write(uint32_t address, uint8_t *data, uint32_t size,
        uint32_t *w_count);

/*
    Function: qspi_bank_read_indirect

    Read data from the flash chip in banks.

    Parameters:

        bank_offset - Starting offset.
        data - Output buffer used to hold the read data.
        size - Size in bytes of the data to be read from the flash device.
        rd_count - number of bytes written.

    Returns:

        - QSPI_ERROR: if the operation is not successful.
        - QSPI_OK:    if the operation is successful.
*/
int32_t qspi_bank_read_indirect(uint32_t bank_offset, uint8_t *data, uint32_t
        size, uint32_t *rd_count);

/*
    Function: qspi_read_data

    Read from the flash chip in indirect mode.

    Parameters:

        size - Size in bytes of the data to be read from the flash device.
        data - Data buffer containing read data.
        count - Number of bytes read.

    Returns:

        - QSPI_ERROR: if the operation is not successful.
        - QSPI_OK: if the operation is successful.
*/
int32_t qspi_read_data(uint32_t size, uint8_t *data, uint32_t *count);

/*
    Function: qspi_read_indirect

    Read from the flash chip in indirect mode(used in ISR).

    Parameters:

        address - Start address for indirect read.
        data - Data buffer containing read data.
        size - Bytes of the data to be read from the flash device.
        r_count - Number of bytes read.

    Returns:

        - QSPI_ERROR: if the operation is not successful.
        - QSPI_OK: if the operation is successful.
*/
int32_t qspi_read_indirect(uint32_t address, uint8_t *data, uint32_t size,
        uint32_t *r_count);

/*
    Function: qspi_indirect_read

    Read from the flash chip in indirect mode.

    Parameters:

        address - Start address for indirect read.
        buffer - Data buffer containing read data.
        size - Bytes of the data to be read from the flash device.
        r_count - Number of bytes read.

    Returns:

        - QSPI_ERROR: if the read operation failed.
        - QSPI_OK: if the read is successful.
*/
int32_t qspi_indirect_read(uint32_t address, uint8_t *buffer, uint32_t size,
        uint32_t *r_count);

/*
    Function: qspi_init

    Initialize the flash handle.

    Parameters:

        qspi_handle - Pointer to the QSPI descriptor.

    Returns:

        - QSPI_BUSY if QSPI chip is busy.
        - QSPI_OK:   if the initialization is successful.
*/
int32_t qspi_init(qspi_descriptor_t *qspi_handle);

/*
    Function: qspi_wait_for_eraseand_program

    Wait for erase and program.

    Returns:

        - QSPI_ERROR: if the erase or program failed.
        - QSPI_OK:    if the erase or program is successful.
*/
int32_t qspi_wait_for_eraseand_program(void);

/*
    Function: qspi_erase

    Erase 4K sectors.

    Parameters:

        address - Address to be erased.

    Returns:

        - QSPI_ERROR: if the erase is not successful.
        - QSPI_OK:    if the erase is successful.
*/
int32_t qspi_erase(uint32_t address);

/*
    Function: qspi_deinit

    Deinitialize the flash handle.

    Returns:

        - QSPI_ERROR: if the deinitialization fails.
        - QSPI_OK:    if the deinitialization is successful.
*/
int32_t qspi_deinit(void);

/*
    Function: qspi_set_callback

    Set callback function.

    Parameters:

        flash_handle - Flash handle to be deinitialized.
        callback - Callback function from flash layer.

    Returns:

        - QSPI_ERROR: if the callback register is not successful.
        - QSPI_OK:    if the callback register is successful.
*/
int32_t qspi_set_callback(qspi_descriptor_t *qspi_handle, qspi_callback_t
        callback, void *puser_context);

/*
    Function: qspi_isr

    QSPI isr function.

    Parameters:

        pparam - Pointer to flash handle.
*/
void qspi_isr(void *pparam);

#endif // __SOCFPGA_QSPI_H__
