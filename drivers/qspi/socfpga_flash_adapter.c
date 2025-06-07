/*
 * SPDX-FileCopyrightText: Copyright (C) 2025 Altera Corporation
 *
 * SPDX-License-Identifier: MIT-0
 *
 * Adapter layer implementation for QSPI
 */

#include "socfpga_flash.h"
#include "socfpga_qspi.h"
#include "socfpga_flash_adapter.h"

int parseM25QParameters(void *pHandle, struct SFDPObject *sfdp)
{
    int ret = 0;
    uint32_t flash_size[2] = {0}, flash_page_size[2] = {0}, flash_pageSizeBytes;
    uint64_t flash_size_raw, flash_sizeBits, flash_page_size_raw;
    uint8_t flash_pageSizeBits;
    QspiDescriptor_t *qspi_handle = (QspiDescriptor_t  *) pHandle;
    for (uint8_t i = 0; i < sfdp->stdHeader.numParameterTables; i++)
    {
        if (i == 0U)
        {               /*Get the flash size*/
            ret = QSPI_ReadSFDP(SFDP_PARAM_FLASHSIZE_ADDR,
                    SFDP_PARAM_FLASHSIZE_SIZE, &flash_size[0]);
            if (ret != QSPI_OK)
            {
                return -EIO;
            }

            flash_size_raw                  =
                    (uint64_t)((((uint64_t)flash_size[1]) <<
                    SFDP_PARAM_FLASHSIZE_MSB_POS) | (uint64_t)flash_size[0]);
            flash_sizeBits                  =
                    ((flash_size_raw >>
                    SFDP_PARAM_FLASHSIZE_POS) & SFDP_PARAM_FLASHSIZE_MASK) + 1U;
            qspi_handle->flash_size         =  (flash_sizeBits >> 30);
            sfdp->paramTable[i].flashSize   =  (flash_sizeBits >> 30);

            /*Get the page size*/
            ret = QSPI_ReadSFDP(SFDP_PARAM_PAGESIZE_ADDR,
                    SFDP_PARAM_PAGESIZE_SIZE, &flash_page_size[0]);
            if (ret != QSPI_OK)
            {
                return -EIO;
            }

            flash_page_size_raw             =
                    (uint64_t)((((uint64_t)flash_page_size[1]) <<
                    32) | (uint64_t)flash_page_size[0]);
            flash_pageSizeBits              =
                    (uint8_t)((flash_page_size_raw >>
                    SFDP_PARAM_PAGESIZE_POS) & SFDP_PARAM_PAGESIZE_MASK);
            flash_pageSizeBytes             =
                    ((uint32_t) 1U << (uint32_t) flash_pageSizeBits);
            qspi_handle->page_size          = flash_pageSizeBytes;
            sfdp->paramTable[i].pageSize    =
                    (uint8_t) qspi_handle->page_size;
        }
    }
    /*These parameters are to be used with the
     * Micron M25Q flash chip
     */
    qspi_handle->inst_width       = M25Q_INST_WIDTH;
    qspi_handle->data_width       = M25Q_DATA_WIDTH;
    qspi_handle->addr_width       = M25Q_ADDR_WIDTH;
    qspi_handle->baud_div         = M25Q_BAUDDIV;
    qspi_handle->sector_size      = M25Q_SECTOR_SIZE;
    qspi_handle->clock_freq       = M25Q_CLOCK_FREQ;
    qspi_handle->nss_delay        = M25Q_NSS_DEALY;
    qspi_handle->init_delay       = M25Q_INIT_DELAY;
    qspi_handle->btwn_delay       = M25Q_BTWN_DELAY;
    qspi_handle->after_delay      = M25Q_AFTER_DELAY;
    qspi_handle->num_addr_bytes   = M25Q_NUM_ADDR_BYTES;
    qspi_handle->dummy_cycles     = M25Q_DUMMY_CYCLES;
    qspi_handle->qspi_mode        = M25Q_QSPI_MODE;

    return ret;
}

