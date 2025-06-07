/*
 * SPDX-FileCopyrightText: Copyright (C) 2025 Altera Corporation
 *
 * SPDX-License-Identifier: MIT-0
 *
 * Driver implementation for QSPI
 */

#include <stdint.h>
#include <stdio.h>
#include "socfpga_qspi.h"
#include "socfpga_interrupt.h"
#include "socfpga_qspi_reg.h"
#include "socfpga_qspi_ll.h"
#include "socfpga_flash.h"

#define DEFAULT_REMAP_ADDR     0U
static uint32_t prevBankAddr = 0x00;

/**
 * @brief Enable interrupt
 */
void QSPI_Enable_Int( uint32_t type )
{
    if (type == QSPI_INDDONE)
    {
        qspi_enable_interrupt(QSPI_IND_OPDONE);
    }
    else
    {
        qspi_enable_interrupt(QSPI_IND_OPDONE | QSPI_XFER_LVLBRCH);
    }
}

/**
 * @brief Disable interrupt
 */
void QSPI_Disable_Int( uint32_t mask )
{
    qspi_disable_interrupt( mask );
}

/**
 * @brief Set QSPI callback
 */
int32_t  QSPI_set_callback(QspiDescriptor_t *qspi_handle, QspiCallback_t xCallback,
        void *pvUserContext)
{
    qspi_handle->xqspiCallback   = xCallback;
    qspi_handle->cb_usercontext  = pvUserContext;
    return QSPI_OK;
}


/**
 * @brief QSPI ISR
 */
#if QSPI_ENABLE_INT_MODE
void QSPI_isr(void *pvParam){

    uint32_t status;
    QspiDescriptor_t *pxQspiPeripheral;
    pxQspiPeripheral = (QspiDescriptor_t *)pvParam;

    status            = qspi_get_int_status();
    qspi_set_int_status(status);

    if (((status & QSPI_IND_OPDONE) != 0U) || ((status & QSPI_XFER_LVLBRCH) != 0U))
    {
        if (pxQspiPeripheral->isWrOp != 0U)
        {
            if (((status & QSPI_XFER_LVLBRCH) != 0U) || ((status & QSPI_IND_OPDONE) != 0U))
            {
                /*Write data to SRAM if bytes left*/
                if (pxQspiPeripheral->usBytesLeft > 0U)
                {
                    uint32_t bytesWritten = 0U;
		    (void)QSPI_IndirectWrite(
                            pxQspiPeripheral->start_addr,
                            pxQspiPeripheral->puBuffer,
                            pxQspiPeripheral->usBytesLeft,
			    &bytesWritten);
                    pxQspiPeripheral->usBytesLeft   -= bytesWritten;
                    pxQspiPeripheral->puBuffer      += bytesWritten;
                    pxQspiPeripheral->start_addr    += bytesWritten;
                }
                else
                {
                    if ((status & QSPI_IND_OPDONE) != 0U)
                    {
                        qspi_clear_indwr_op_status();
                    }
                    if (pxQspiPeripheral->xIsAsync != 0U)
                    {
                        (*pxQspiPeripheral->xqspiCallback)(QSPI_OK,
                                pxQspiPeripheral->cb_usercontext);
                        QSPI_Disable_Int(QSPI_XFER_LVLBRCH | QSPI_IND_OPDONE);
                        pxQspiPeripheral->xIsBusy = false;
                    }
                    else
                    {
                        if (osal_semaphore_post(pxQspiPeripheral->xSem) == false)
                        {
                            return;
                        }
                    }
                }
            }
        }
        else
        {
            if (((status & QSPI_XFER_LVLBRCH) != 0U) || ((status & QSPI_IND_OPDONE) != 0U))
            {
                if (pxQspiPeripheral->usBytesLeft > 0U)
                {
                    uint32_t bytesRead = 0U;
		    (void)QSPI_Read_Indirect(
                            pxQspiPeripheral->start_addr,
                            pxQspiPeripheral->puBuffer,
                            pxQspiPeripheral->usBytesLeft,
			    &bytesRead);

                    pxQspiPeripheral->usBytesLeft   -= bytesRead;
                    pxQspiPeripheral->puBuffer      += bytesRead;
                    pxQspiPeripheral->start_addr    += bytesRead;
                }
                else
                {
                    if ((status & QSPI_IND_OPDONE) != 0U)
                    {
                        qspi_clear_indrd_op_status();
                    }
                    if (pxQspiPeripheral->xIsAsync != 0U)
                    {
                        (*pxQspiPeripheral->xqspiCallback)(QSPI_OK,
                                pxQspiPeripheral->cb_usercontext);
                        QSPI_Disable_Int(QSPI_XFER_LVLBRCH | QSPI_IND_OPDONE);
                        pxQspiPeripheral->xIsBusy = false;
                    }
                    else
                    {
                        if (osal_semaphore_post(pxQspiPeripheral->xSem) == true)
                        {
                            return;
                        }
                    }
                }
            }
        }
    }
}
#endif

/**
 * @brief Initialize QSPI interface
 */
int32_t QSPI_Init(QspiDescriptor_t *qspi_handle)
{

    QSPI_Disable_Int(QSPI_ALL_INT_MASK);
#if QSPI_ENABLE_INT_MODE
    SocfpgaHpuInterrupt_t xIntId;
    SocfpgaInterruptErr_t xIntRet;
    xIntId = eSdmQspiIntr;
    xIntRet = interrupt_register_isr(xIntId, QSPI_isr, qspi_handle);
    if (xIntRet != eErrOk)
    {
        return QSPI_ERROR;
    }
    xIntRet = interrupt_enable(xIntId, GIC_INTERRUPT_PRIORITY_QSPI);
    if (xIntRet != eErrOk)
    {
        return QSPI_ERROR;
    }
#endif
    uint32_t status;

    if (qspi_is_nonidle() != 0U)
    {
        return QSPI_BUSY;
    }

    qspi_disable();

    qspi_set_nss_delay(qspi_handle->nss_delay);
    qspi_set_btwn_delay(qspi_handle->btwn_delay);
    qspi_set_after_delay(qspi_handle->after_delay);
    qspi_set_init_delay(qspi_handle->init_delay);
    qspi_set_remap_address(DEFAULT_REMAP_ADDR);
    qspi_set_bytes_per_page(qspi_handle->page_size);
    qspi_enable_fast_read_mode();
    qspi_set_instruction_width(qspi_handle->inst_width);
    qspi_set_addr_width(qspi_handle->addr_width);
    qspi_set_data_width(qspi_handle->data_width);
    qspi_enable_mode_bit(1);
    qspi_set_dummy_delay(qspi_handle->dummy_cycles);
    qspi_cfg_write_mode();
    qspi_set_baud_divisor(qspi_handle->baud_div);
    qspi_enable();

    status = qspi_get_int_status();
    qspi_set_int_status(status);

    return QSPI_OK;
}

/**
 * @brief Enable flash command send
 */
static int32_t QSPI_FlashCmdHelper( void )
{
    uint32_t count = 0U;

    qspi_flashcmd_exec();
    while (count < QSPI_TIMEOUT)
    {
        if (qspi_get_flashcmd_stat() == 0U)
        {
            break;
        }
        count++;
    }

    if (count >= QSPI_TIMEOUT)
    {
        return QSPI_ERROR;
    }
    return QSPI_OK;
}

/**
 * @brief Send flash command
 */
int32_t QSPI_SendFlashcmd( uint32_t cmd )
{
    uint32_t cs = 0U;
    int32_t ret = QSPI_OK;

    qspi_select_chip(cs);

    qspi_set_flashcmd(cmd);

    ret = QSPI_FlashCmdHelper();
    if (ret != QSPI_OK)
    {
        return QSPI_ERROR;
    }
    return ret;
}

/**
 * @brief Read SFDP parametes from flash
 */
int32_t QSPI_ReadSFDP( uint32_t address, uint8_t size, uint32_t *data )
{
    int32_t ret = QSPI_OK;

    qspi_set_nss_delay(SFDP_NSS_DELAY);
    qspi_set_btwn_delay(SFDP_BTWN_DELAY);
    qspi_set_after_delay(SFDP_AFTER_DELAY);
    qspi_set_init_delay(SFDP_INIT_DELAY);
    qspi_set_remap_address(DEFAULT_REMAP_ADDR);
    qspi_set_baud_divisor(SFDP_BAUDDIV);
    qspi_enable();

    qspi_enable_fast_read_mode();
    qspi_set_instruction_width(SFDP_INST_WIDTH);
    qspi_set_addr_width(SFDP_ADDR_WIDTH);
    qspi_set_data_width(SFDP_DATA_WIDTH);
    qspi_enable_mode_bit(1);
    qspi_set_dummy_delay(SFDP_DUMMY_DELAY);
    qspi_cfg_write_mode();

    qspi_select_chip(0);

    qspi_set_flashcmd(QSPI_READ_SFDP_CMD);
    qspi_set_flashcmdaddr(address);
    qspi_set_enablecmdaddr();
    qspi_set_flashcmdaddrbytes(3);
    qspi_flashcmd_read_data();
    qspi_flashcmd_read_bytes(size);

    ret = QSPI_FlashCmdHelper();
    if (ret != QSPI_OK)
    {
        return QSPI_ERROR;
    }

    data[ 0 ] = qspi_flash_cmd_rddata0();
    if (size > 4U)
    {
        data[ 1 ] = qspi_flash_cmd_rddata1();
    }

    return ret;
}

/**
 * @brief Send flash write command
 */
int32_t QSPI_SendFlashWritecmd( uint8_t cmd, uint8_t numbytes, uint32_t *data )
{
    int32_t ret = QSPI_OK;
    qspi_select_chip(0);
    qspi_set_flashcmd(cmd);
    qspi_flashcmd_write_data();
    qspi_flashcmd_write_bytes(numbytes);

    qspi_flash_cmd_wrdata0(data[ 0 ]);

    if (numbytes > 4U)
    {
        qspi_flash_cmd_wrdata1(data[ 1 ]);
    }

    ret = QSPI_FlashCmdHelper();
    if (ret != QSPI_OK)
    {
        return QSPI_ERROR;
    }

    return ret;
}

/**
 * @brief Send flash read command
 */
int32_t QSPI_SendFlashReadcmd( uint8_t cmd, uint8_t numbytes, uint32_t *data )
{
    int32_t ret = QSPI_OK;

    qspi_select_chip(0);

    qspi_set_flashcmd(cmd);
    qspi_flashcmd_read_data();
    qspi_flashcmd_read_bytes(numbytes);

    ret = QSPI_FlashCmdHelper();
    if (ret != QSPI_OK)
    {
        return QSPI_ERROR;
    }

    data[ 0 ] = qspi_flash_cmd_rddata0();

    if (numbytes > 4U)
    {
        data[ 1 ] = qspi_flash_cmd_rddata1();
    }

    return ret;
}

/**
 * @brief Wait for flash erase or program
 */
int32_t QSPI_Wait_for_EraseandProgram( void )
{
    int32_t ret;
    uint32_t status = 0, count = 0;

    /*Read the status of the  flash device*/
    while (count < QSPI_TIMEOUT)
    {
        ret = QSPI_SendFlashReadcmd(QSPI_READ_STATUS_CMD, 1, &status);
        if (ret != QSPI_OK)
        {
            return QSPI_ERROR;
        }
        if ((status & QSPI_READ_STATUS_POS) == 0U)
        {
            break;
        }
        count++;
    }

    if (count >= QSPI_TIMEOUT)
    {
        return QSPI_ERROR;
    }
    /*Read the flag status register*/
    count = 0;
    status = 0;
    while (count < QSPI_TIMEOUT)
    {
        ret = QSPI_SendFlashReadcmd(QSPI_READ_FLAG_STATUS_CMD, 1, &status);
        if (ret != QSPI_OK)
        {
            return QSPI_ERROR;
        }
        if ((status & QSPI_READ_FLAG_STATUS_POS) != 0U)
        {
            break;
        }
        count++;
    }
    if (count >= QSPI_TIMEOUT)
    {
        return QSPI_ERROR;
    }

    /*Clear the flag*/
    if ((status & QSPI_READ_FLAG_STATUS_POS) != 0U)
    {
        ret = QSPI_SendFlashcmd(QSPI_CLEAR_FLAG_STATUS_CMD);
        if (ret != QSPI_OK)
        {
            return QSPI_ERROR;
        }
    }
    return QSPI_OK;
}

/**
 * @brief Send command for QSPI sector erase
 */
int32_t QSPI_Erase( uint32_t address )
{

    int32_t ret = QSPI_OK;

    ret = QSPI_SendFlashcmd(QSPI_WRITE_ENABLE_CMD);
    if (ret != QSPI_OK)
    {
        return QSPI_ERROR;
    }

    /*Send the command to set the bank*/
    uint32_t bankAddr = (address >> QSPI_BANK_ADDR_POS);
    ret = QSPI_SendFlashWritecmd(QSPI_EXT_REG_CMD, 1, &bankAddr);
    if (ret != QSPI_OK)
    {
        return QSPI_ERROR;
    }

    ret = QSPI_SendFlashcmd(QSPI_WRITE_DISABLE_CMD);
    if (ret != QSPI_OK)
    {

        return QSPI_ERROR;
    }

    /*Start of sequence for 4k erase*/
    ret = QSPI_SendFlashcmd(QSPI_WRITE_ENABLE_CMD);
    if (ret != QSPI_OK)
    {
        return QSPI_ERROR;
    }

    qspi_select_chip(0);
    qspi_set_flashcmd(QSPI_SECTOR_ERASE_CMD);
    qspi_set_enablecmdaddr();
    qspi_set_flashcmdaddrbytes(3);
    qspi_set_flashcmdaddr(address);

    ret = QSPI_FlashCmdHelper();
    if (ret != QSPI_OK)
    {
        return QSPI_ERROR;
    }

    ret = QSPI_Wait_for_EraseandProgram();
    if (ret != QSPI_OK)
    {
        return QSPI_ERROR;
    }

    ret = QSPI_SendFlashcmd(QSPI_WRITE_DISABLE_CMD);

    if (ret != QSPI_OK)
    {
        return QSPI_ERROR;
    }
    return QSPI_OK;
}

/**
 * @brief Wait for QSPI erase and program
 */
int32_t QSPI_Write_Finish( void )
{
    uint32_t count = 0U, status = 0U;

    while (count < QSPI_TIMEOUT)
    {
        status = qspi_get_indwr_multiop_status();
        if ((status & QSPI_INDONE_OPSTATUS_MASK) != 0U)
        {
            qspi_clear_indwr_op_status();
            break;
        }
        count++;
    }
    if (count >= QSPI_TIMEOUT)
    {
        return QSPI_ERROR;
    }

    int32_t ret = QSPI_Wait_for_EraseandProgram();
    if (ret != QSPI_OK)
    {
        return QSPI_ERROR;
    }

    return QSPI_OK;
}

/**
 * @brief Select QPSI flash bank
 */
int32_t QSPI_Select_Bank( uint32_t *bankAddr)
{
    int32_t ret = QSPI_SendFlashcmd(QSPI_WRITE_ENABLE_CMD);
    if (ret != QSPI_OK)
    {
        return QSPI_ERROR;
    }

    ret = QSPI_SendFlashWritecmd(QSPI_EXT_REG_CMD, 1, bankAddr);
    if (ret != QSPI_OK)
    {
        return QSPI_ERROR;
    }

    ret = QSPI_SendFlashcmd(QSPI_WRITE_DISABLE_CMD);
    if (ret != QSPI_OK)
    {
        return QSPI_ERROR;
    }
    return QSPI_OK;
}

/**
 * @brief Write indirectly to the QSPI flash page
 */
int32_t QSPI_Page_Write_Indirect( uint32_t offset, uint8_t *data,
        uint32_t size, uint32_t *count )
{
    uint32_t sramPartition =
            ((qspi_reg_get_data((uint32_t) QSPI_SRAMPART)) &
            QSPI_SRAM_RD_CAP_MASK);
    uint32_t wCapacity = (uint32_t) QSPI_TOTAL_SRAM_SIZE - sramPartition;
    volatile uint32_t *destAddr     = (uint32_t*) QSPI_DATA_BASE;
    volatile uint8_t *destAddrByte = (uint8_t*) QSPI_DATA_BASE;
    uint32_t wFillLvl, space;
    uint8_t *wDataByte = NULL;
    uint32_t wCount = 0, *wData = NULL;
    if (destAddr == NULL)
    {
        return QSPI_ERROR;
    }

    qspi_set_indwrstaddr(offset);
    qspi_set_indwrcnt((uint32_t) size);
    qspi_start_indwr();

    while (wCount < size)
    {

        wFillLvl =
                ((qspi_get_sramfill() >>
                QSPI_SRAMFILL_WR_STATUS_POS) & QSPI_SRAMFILL_WR_STATUS_MASK);

        if ((wCapacity - wFillLvl) < 4U)
        {
            break;
        }
        space = ((wCapacity - wFillLvl)
                < (((uint32_t) size - (uint32_t) wCount) /
                (uint32_t) sizeof(uint32_t))) ?
                (wCapacity - wFillLvl) :
                (((uint32_t) size - (uint32_t) wCount) /
                (uint32_t) sizeof(uint32_t));

        wData = (uint32_t*) (data + wCount);
        if (wData == NULL)
        {
            return QSPI_ERROR;
        }

        for (uint32_t i = 0; i < space; ++i)
        {
            *destAddr = *wData;
            wData++;
        }
        wCount += (space * sizeof(uint32_t));
        if ((size - wCount) < 4U)
        {
            wDataByte = (uint8_t *) wData;
            if (wDataByte == NULL)
            {
                return QSPI_ERROR;
            }
            while (wCount != size)
            {
                *destAddrByte = *wDataByte;
                wDataByte++;
                wCount++;
            }
        }
    }
#if QSPI_ENABLE_INT_MODE
    *count = wCount;
    return QSPI_OK;
#else
    int32_t ret = 0;
    (void)count;
    ret = QSPI_Write_Finish();
    if (ret != QSPI_OK)
    {
        return QSPI_ERROR;
    }
    if (wCount != size)
    {
        return QSPI_ERROR;
    }
    return QSPI_OK;
#endif
}

/**
 * @brief Write in indirect mode to QSPI flash bank
 */
int32_t QSPI_Bank_Write_Indirect( uint32_t bankOffset, uint8_t *data,
        uint32_t size, uint32_t *wCount )
{
    uint32_t pageOffset = bankOffset & (QSPI_PAGE_SIZE - 1U);
    uint32_t wSize =
            size < (QSPI_PAGE_SIZE - pageOffset) ?
            size : (QSPI_PAGE_SIZE - pageOffset);
#if QSPI_ENABLE_INT_MODE
    uint32_t intStatus = 0U;
    int32_t ret = QSPI_OK;

    intStatus = qspi_get_int_status();
    qspi_set_int_status(intStatus);

    /*set the water mark level*/
    if (size > QSPI_WRITE_WATER_LVL)
    {
        qspi_set_indwrwater(QSPI_WRITE_WATER_LVL);
    }
    else
    {
        qspi_set_indwrwater(size);
    }

    ret = QSPI_Page_Write_Indirect(bankOffset, data, wSize, wCount);
    if (ret != QSPI_OK)
    {
	    return QSPI_ERROR;
    }
    return QSPI_OK;
#else
    int32_t status = 0;
    while (size > 0)
    {
        status = QSPI_Page_Write_Indirect(bankOffset, data, wSize, wCount);
        if (status != QSPI_OK)
        {
            break;
        }
        bankOffset    += wSize;
        data          += wSize;
        size          -= wSize;
        wSize          = size < QSPI_PAGE_SIZE? size: QSPI_PAGE_SIZE;

    }
    return status;
#endif
}

/**
 * @brief Write indirectly to QSPI flash
 */
int32_t QSPI_IndirectWrite( uint32_t address, uint8_t *data, uint32_t size, uint32_t *wCount )
{
    uint32_t bankAddr, bankWriteLen, bankOffset;
    int32_t ret = 0;
    uint8_t *wData = data;

    bankAddr = ((address & QSPI_BANK_ADDR_OFFSET) >> QSPI_BANK_ADDR_POS);
    bankOffset = address & (QSPI_BANK_SIZE - 1U);
    bankWriteLen =
            size < (QSPI_BANK_SIZE - bankOffset) ?
            size : (QSPI_BANK_SIZE - bankOffset);

#if QSPI_ENABLE_INT_MODE
    ret = QSPI_Select_Bank(&bankAddr);
    if (ret != QSPI_OK)
    {
        return QSPI_ERROR;
    }
    ret = QSPI_Bank_Write_Indirect(bankOffset, wData, bankWriteLen, wCount);
    if(ret != QSPI_OK)
    {
	    return QSPI_ERROR;
    }
    return QSPI_OK;

#else
    uint32_t bankCount        =
            (((address + size - 1) >>
            QSPI_BANK_ADDR_POS) - ((address) >> QSPI_BANK_ADDR_POS)) + 1;

    for (uint32_t i = 0; i < bankCount; i++)
    {
        ret = QSPI_Select_Bank( &bankAddr );
        if (ret != QSPI_OK)
        {
            return QSPI_ERROR;
        }

        ret = QSPI_Bank_Write_Indirect(bankOffset, wData, bankWriteLen, wCount);
        if (ret != QSPI_OK)
        {
            return QSPI_ERROR;
        }

        bankAddr      += QSPI_BANK_SIZE;
        wData         += bankWriteLen;
        size          -= bankWriteLen;
        bankOffset     = 0;

        bankWriteLen   = size < (QSPI_BANK_SIZE)? size: (QSPI_BANK_SIZE);
    }
    return QSPI_OK;
#endif
}

/**
 * @brief Read data in indirect mode from QSPI flash
 */
#if QSPI_ENABLE_INT_MODE
int32_t QSPI_Read_Data(uint32_t size, uint8_t *data, uint32_t *count)
{
    uint32_t rCount = 0;
    uint32_t *rData     = (uint32_t *) data;
    uint32_t rFillLvl;
    volatile uint32_t *srcWord = (uint32_t*) QSPI_DATA_BASE;
    volatile uint8_t *srcByte = (uint8_t*) QSPI_DATA_BASE;
    uint8_t  *rDataByte = (uint8_t *)data;
    uint32_t rBytes;

    if (srcWord == NULL)
    {
        return QSPI_ERROR;
    }
    if (size >= sizeof(uint32_t))
    {
        rFillLvl =
                ((qspi_get_sramfill() >>
                QSPI_SRAMFILL_RD_STATUS_POS) & QSPI_SRAMFILL_RD_STATUS_MASK);
        if (rFillLvl > 512U)
        {
            rFillLvl = 512U;
        }
        rBytes = rFillLvl <
                (size / (sizeof(uint32_t))) ? rFillLvl: (size /
                (sizeof(uint32_t)));
        for (uint32_t i = 0; i < rBytes; i++)
        {
            *rData = *srcWord;
            rData++;
        }
        rCount = (rBytes * (sizeof(uint32_t)));
        rDataByte = (uint8_t *)rData;
        if (rDataByte == NULL)
        {
            return QSPI_ERROR;
        }
        size -= rCount;
    }
    if (size < 4U)
    {
        rFillLvl =
                ((qspi_get_sramfill() >>
                QSPI_SRAMFILL_RD_STATUS_POS) & QSPI_SRAMFILL_RD_STATUS_MASK);
        if (rFillLvl > 512U)
        {
            rFillLvl = 512U;
        }
        rBytes = (rFillLvl * (sizeof(uint32_t))) <
                size ? (rFillLvl * (sizeof(uint32_t))): size;
        for (uint32_t i = 0; i < rBytes; i++)
        {
            *rDataByte = *srcByte;
            rDataByte++;
        }
        rCount += rBytes;
    }
    *count = rCount;
    return QSPI_OK;
}

/**
 * @brief Read data indirectly form QSPI flash(used in ISR)
 */
int32_t QSPI_Read_Indirect( uint32_t address, uint8_t *data, uint32_t size,
		uint32_t *rCount)
{
    uint32_t bankAddr, bankOffset;
    uint32_t intStatus;
    uint32_t sramPartition;
    int32_t ret = QSPI_OK;

    bankAddr = ((address & QSPI_BANK_ADDR_OFFSET) >> QSPI_BANK_ADDR_POS);

    /*New bank means new transfer*/
    if (prevBankAddr != bankAddr)
    {
        size = size < QSPI_BANK_SIZE ? size: QSPI_BANK_SIZE;
        bankOffset = (address & (QSPI_BANK_SIZE - 1U));
        ret = QSPI_Select_Bank(&bankAddr);
        if (ret != QSPI_OK)
        {
            return 0;
        }
        qspi_set_indrdstaddr(bankOffset);
        qspi_set_indrdcnt(size);

        intStatus = qspi_get_int_status();
        qspi_set_int_status(intStatus);
        sramPartition     =
                ((qspi_reg_get_data((uint32_t)QSPI_SRAMPART)) &
                QSPI_SRAM_RD_CAP_MASK);

        if ((size / 4U) > sramPartition)
        {
            qspi_set_indrdwater((sramPartition * 4U));
        }
        else
        {
            qspi_set_indrdwater(size);
        }
        qspi_start_indrd();
        prevBankAddr = bankAddr;
    }
    ret = QSPI_Read_Data(size, data, rCount);
    if(ret != QSPI_OK)
    {
	    return QSPI_ERROR;
    }
    return ret;
}
#endif

/**
 * @brief Read data indirectly form QSPI flash(to be not used in ISR)
 */
int32_t QSPI_Bank_Read_Indirect( uint32_t bankOffset, uint8_t *data,
        uint32_t size, uint32_t *rdCount )
{

    qspi_set_indrdstaddr(bankOffset);
    qspi_set_indrdcnt(size);
#if QSPI_ENABLE_INT_MODE
    uint32_t intStatus = qspi_get_int_status();
    int32_t ret = QSPI_OK;
    qspi_set_int_status(intStatus);

    uint32_t sramPartition     =
            ((qspi_reg_get_data((uint32_t)QSPI_SRAMPART)) &
            QSPI_SRAM_RD_CAP_MASK);

    if ((size / 4U) > sramPartition)
    {
        qspi_set_indrdwater((sramPartition * 4U));
    }
    else
    {
        qspi_set_indrdwater(size);
    }

#endif
    qspi_start_indrd();
#if QSPI_ENABLE_INT_MODE
    ret = QSPI_Read_Data(size, data, rdCount);
    if(ret != QSPI_OK)
    {
	    return QSPI_ERROR;
    }
    return QSPI_OK;
#else
    (void)rdCount;
    uint32_t numrByte = size % 4;
    uint32_t numrWord = size / 4;
    uint32_t rCount = 0;
    uint32_t *rData     = (uint32_t *) data;
    volatile uint32_t *srcWord = (uint32_t*) QSPI_DATA_BASE;
    if (srcWord == NULL)
    {
        return QSPI_ERROR;
    }
    uint32_t rFillLvl;
    while (rCount < size)
    {
        do
        {
            rFillLvl  =
                    ((qspi_get_sramfill() >>
                    QSPI_SRAMFILL_RD_STATUS_POS) &
                    QSPI_SRAMFILL_RD_STATUS_MASK);
            rData = (uint32_t *)(data + rCount);

            for (uint32_t i = 0; i < rFillLvl; ++i)
            {
                if (numrWord > 0)
                {
                    *rData++ = *srcWord;
                    numrWord--;
                }
                else
                {
                    uint32_t temp = *srcWord;
                    memcpy(rData, &temp, numrByte);
                }
            }
            rCount += rFillLvl * sizeof(uint32_t);
        } while(rFillLvl > 0);
    }
    if (rCount != size)
    {
        return QSPI_ERROR;
    }
    return QSPI_OK;
#endif
}

/**
 * @brief Read directly from QSPI flash
 */
int32_t QSPI_IndirectRead( uint32_t address, uint8_t *buffer, uint32_t size,
		uint32_t *rCount )
{
    uint32_t bankReadLen, bankAddr, bankOffset;
    int32_t ret = QSPI_OK;

    bankAddr = ((address & QSPI_BANK_ADDR_OFFSET) >> QSPI_BANK_ADDR_POS);
    prevBankAddr = bankAddr;
    bankOffset = (address & (QSPI_BANK_SIZE - 1U));
    bankReadLen =
            size < (QSPI_BANK_SIZE - bankOffset) ?
            size : (QSPI_BANK_SIZE - bankOffset);
#if QSPI_ENABLE_INT_MODE
    ret = QSPI_Select_Bank(&bankAddr);
    if (ret != QSPI_OK)
    {
        return QSPI_ERROR;
    }

    ret = QSPI_Bank_Read_Indirect(bankOffset, buffer, bankReadLen, rCount);
    if(ret != QSPI_OK)
    {
	    return QSPI_ERROR;
    }
    return QSPI_OK;
#else

    uint32_t bankCount                 =
            (((address + size - 1) >>
            QSPI_BANK_ADDR_POS) - ((address) >> QSPI_BANK_ADDR_POS)) + 1;

    for (uint32_t i = 0; i < bankCount; i++)
    {
        ret = QSPI_Select_Bank( &bankAddr );
        if (ret != QSPI_OK)
        {
            return QSPI_ERROR;
        }

        ret   =  QSPI_Bank_Read_Indirect(bankOffset, buffer, bankReadLen, rCount);
        if (ret != QSPI_OK)
        {
            return QSPI_ERROR;
        }

        bankAddr      += QSPI_BANK_SIZE;
        buffer        += bankReadLen;
        size          -= bankReadLen;
        bankOffset     = 0;

        bankReadLen    = size < QSPI_BANK_SIZE? size: QSPI_BANK_SIZE;
    }
    return QSPI_OK;
#endif
}

/**
 * @brief Deinitialize QSPI interface
 */
int32_t QSPI_Deinit(void)
{
#if QSPI_ENABLE_INT_MODE
    if (qspi_is_nonidle() != 0U)
    {
        return QSPI_BUSY;
    }

    qspi_disable_interrupt(QSPI_ALL_INT_MASK);
#endif

    qspi_disable();
    if (qspi_is_nonidle() != 0U)
    {
        return QSPI_BUSY;
    }
    return QSPI_OK;
}
