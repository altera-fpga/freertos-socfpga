/*
 * SPDX-FileCopyrightText: Copyright (C) 2025 Altera Corporation
 *
 * SPDX-License-Identifier: MIT-0
 *
 * Header file for QSPI HAL
 */

#ifndef __SOCFPGA_QSPI_H__
#define __SOCFPGA_QSPI_H__

/**
 * @file socfpga_qspi.h
 * @brief This file contains all the QSPI HAL API definitions
 */

#include <stdint.h>
#include <stdbool.h>
#include "socfpga_defines.h"

#define QSPI_ENABLE_INT_MODE       1U

#define QSPI_BUSY                       1
#define QSPI_OK                         0
#define QSPI_ERROR                      -1
#define QSPI_TIMEOUT                    0x10000000U
#define QSPI_WRITE_IN_PROGRESS_FLAG     0x01
#define QSPI_INDIRECT_OP_DONE_FLAG      0x02

#define QSPI_INDDONE                 1U
#define QSPI_INDDONE_AND_XFERBRCH    2U

#define QSPI_WRITE_WATER_LVL              100U

#define QSPI_ALL_INT_MASK               0x1ffeU

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

#define SFDP_NSS_DELAY                  0x14U
#define SFDP_BTWN_DELAY                 0x14U
#define SFDP_AFTER_DELAY                0xffU
#define SFDP_INIT_DELAY                 0xc8U
#define SFDP_BAUDDIV                    0xfU
#define SFDP_INST_WIDTH                 0U
#define SFDP_DATA_WIDTH                 0U
#define SFDP_ADDR_WIDTH                 0U
#define SFDP_DUMMY_DELAY                0U

#define QSPI_IND_OPDONE                 0x4U
#define QSPI_XFER_LVLBRCH               0x40U

/**
 * @brief This is the FlashCallback
 *
 */
typedef void (*FlashCallback_t)( uint32_t xOpStatus, void *pvUserContext );

/**
 * @brief This is the QspiCallback
 *
 */
typedef FlashCallback_t *QspiCallback_t;

/**
 * @brief This is the structure used to hold the flash
 *        descriptor variables
 */
typedef struct QpsiDescriptor
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

    osal_semaphore_t xSem;
    osal_semaphore_t xMutex;
    void *cb_usercontext;
    uint32_t start_addr;
    uint32_t usBytesLeft;
    uint32_t usXferSize;
    uint8_t *puBuffer;
    BaseType_t isWrOp;
    BaseType_t xIsAsync;
    BaseType_t xIsBusy;
    QspiCallback_t xqspiCallback;
}QspiDescriptor_t;

/**
 * @brief Enable the interrupt.
 *
 * @param[in] type Flag to indicate the interrupts to be enabled
 *
 * @return none
 */
void QSPI_Enable_Int(uint32_t type);

/**
 * @brief Disable interrupts.
 *
 * @param[in] mask for interrupts to be disabled
 *
 * @return none
 */
void QSPI_Disable_Int(uint32_t mask);

/**
 * @brief Read the SFDP parameters.
 *
 * @param[in] address Address to read from.
 * @param[in] size Length of data to be read.
 * @param[out] data Pointer to the data.
 *
 * @return
 * - QSPI_ERROR if the operation is not successful.
 * - QSPI_OK if the operation is successful.
 */
int32_t QSPI_ReadSFDP(uint32_t address, uint8_t size, uint32_t *data);

/**
 * @brief Send command to flash to read data.
 *
 * @param[in] cmd Flash handle to do the operation on.
 * @param[in] numbytes Number of bytes to read.
 * @param[out] data Pointer to buffer to write data to.
 *
 * @return
 * - QSPI_ERROR if the operation is not successful.
 * - QSPI_OK if the operation is successful.
 */
int32_t QSPI_SendFlashReadcmd(uint8_t cmd, uint8_t numbytes, uint32_t *data);

/**
 * @brief Erase 4K sectors.
 *
 * @param[in] cmd Command to be sent to the flash device.
 *
 * @return
 * - QSPI_ERROR if the operation is not successful.
 * - QSPI_OK if the operation is successful.
 */
int32_t QSPI_SendFlashWritecmd(uint8_t cmd, uint8_t numbytes, uint32_t *data);

/**
 * @brief To select the 16MB bank.
 *
 * @param[in] bankAddr The bank address to be selected.
 *
 * @return
 * - QSPI_ERROR if the operation is not successful.
 * - QSPI_OK if the operation is successful.
 */
int32_t QSPI_Select_Bank( uint32_t *bankAddr);

/**
 * @brief Send a command to the flash chip.
 *
 * @param[in] cmd Command to be sent to the flash chip.
 *
 * @return
 * - QSPI_ERROR if the operation is not successful.
 * - QSPI_OK if the operation is successful.
 */
int32_t QSPI_SendFlashcmd(uint32_t cmd);

/**
 * @brief Verify if the write operation is completed in the flash chip.
 *
 * @param none
 *
 * @return
 * - QSPI_ERROR if the operation is not successful.
 * - QSPI_OK if the operation is successful.
 */
int32_t QSPI_Write_Finish(void);

/**
 * @brief Write data to the flash chip in pages (256 bytes).
 *
 * @param[in] offset Starting page offset.
 * @param[in] data Pointer to the buffer containing the data.
 * @param[in] The size of the data.
 *
 * @return
 * - QSPI_ERROR if the operation is not successful.
 * - QSPI_OK if the operation is successful.
 */
int32_t QSPI_Page_Write_Indirect(uint32_t offset, uint8_t *data, uint32_t size,
		uint32_t *count);

/**
 * @brief Write data to the flash chip in banks (16 MB).
 *
 * @param[in] bankOffset Starting offset.
 * @param[in] data Pointer to the buffer containing the data.
 * @param[in] size The size of the data.
 * @param[out] The number of bytes written.
 *
 * @return
 * - QSPI_ERROR if the operation is not successful.
 * - QSPI_OK if the operation is successful.
 */
int32_t QSPI_Bank_Write_Indirect(uint32_t bankOffset, uint8_t *data,
        uint32_t size, uint32_t *wCount);

/**
 * @brief Write to the flash chip in indirect mode.
 *
 * @param[in] address Start address for indirect write.
 * @param[in] data Data buffer containing write data.
 * @param[in] size Size in bytes of the data to be written to the flash device.
 * @param[out] The number of bytes written.
 *
 * @return
 * - QSPI_ERROR if the write operation failed.
 * - QSPI_OK if the write is successful.
 */
int32_t QSPI_IndirectWrite(uint32_t address, uint8_t *data, uint32_t size,
		uint32_t *wCount);

/**
 * @brief Read data from the flash chip in banks.
 *
 * @param[in] bankOffset Starting offset.
 * @param[out] data Output buffer used to hold the read data.
 * @param[in] size Size in bytes of the data to be read from the flash device.
 * @param[out] The number of bytes written.
 *
 * @return
 * - QSPI_ERROR if the operation is not successful.
 * - QSPI_OK if the operation is successful.
 */
int32_t QSPI_Bank_Read_Indirect(uint32_t bankOffset, uint8_t *data,
        uint32_t size, uint32_t *rdCount);

/**
 * @brief Read from the flash chip in indirect mode.
 *
 * @param[in] Offset to start read.
 * @param[out] buffer Data buffer containing read data.
 * @param[out] The number of bytes read.
 *
 * @return
 * - QSPI_ERROR if the operation is not successful.
 * - QSPI_OK if the operation is successful.
 *
 */
int32_t QSPI_Read_Data(uint32_t size, uint8_t *data, uint32_t *count);

/**
 * @brief Read from the flash chip in indirect mode(used in ISR).
 *
 * @param[in] address Start address for indirect read.
 * @param[out] buffer Data buffer containing read data.
 * @param[in] size  in bytes of the data to be read from the flash device.
 * @param[out] The number of bytes read.
 *
 * @return
 * - QSPI_ERROR if the operation is not successful.
 * - QSPI_OK if the operation is successful.
 */
int32_t QSPI_Read_Indirect( uint32_t address, uint8_t *data, uint32_t size,
		uint32_t *rCount);

/**
 * @brief Read from the flash chip in indirect mode.
 *
 * @param[in] address Start address for indirect read.
 * @param[out] buffer Data buffer containing read data.
 * @param[in] size Size in bytes of the data to be read from the flash device.
 * @param[out] The number of bytes read.
 *
 * @return
 * - QSPI_ERROR if the read operation failed.
 * - QSPI_OK if the read is successful.
 */
int32_t QSPI_IndirectRead(uint32_t address, uint8_t *buffer, uint32_t size,
		uint32_t *rCount);

/**
 * @brief Initialize the flash handle.
 *
 * @param[in] address Start address for indirect read.
 * @param[out] buffer Data buffer containing read data.
 * @param[in] size Size in bytes of the data to be read from the flash device.
 * @param[out] The number of bytes read.
 *
 * @return
 * - QSPI_BUSY if QSPI chip is busy.
 * - QSPI_OK if the initialization is successful.
 */
int32_t QSPI_Init(QspiDescriptor_t *qspi_handle);

/**
 * @brief Wait for erase and program.
 *
 * @param none
 *
 * @return
 * - QSPI_ERROR if the erase or program failed.
 * - QSPI_OK if the erase or program is successful.
 */
int32_t QSPI_Wait_for_EraseandProgram(void);

/**
 * @brief Erase 4K sectors.
 *
 * @param[in] address Address to be erased.
 *
 * @return
 * - QSPI_ERROR if the erase is not successful.
 * - QSPI_OK if the erase is successful.
 */
int32_t QSPI_Erase(uint32_t address);

/**
 * @brief Deinitialize the flash handle.
 *
 * @param[in] flash_handle Flash handle to be deinitialized.
 *
 * @return none
 */
int32_t QSPI_Deinit(void);

/**
 * @brief Set callback function.
 *
 * @param[in] flash_handle Flash handle to be deinitialized.
 * @param[in] xCallback Callback function from flash layer.
 *
 * @return
 * - QSPI_ERROR if the callback register is not successful.
 * - QSPI_OK if the callback register is successful.
 */
int32_t QSPI_set_callback(QspiDescriptor_t *qspi_handle, QspiCallback_t xCallback,
        void *pvUserContext);

/**
 * @brief QSPI isr function.
 *
 * @param[in] pointer to flash handle
 *
 * return
 *      NONE
 */
void QSPI_isr( void *pvParam );

#endif // __SOCFPGA_QSPI_H__
