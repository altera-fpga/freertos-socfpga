/*
 * SPDX-FileCopyrightText: Copyright (C) 2025 Altera Corporation
 *
 * SPDX-License-Identifier: MIT-0
 *
 * Header file for Flash adapter layer
 */

#ifndef __SOCFPGA_FLASH_ADAPTER_H__
#define __SOCFPGA_FLASH_ADAPTER_H__

#include <stdint.h>

#define MAX_PARAM_HEADERS               4U
#define MAX_PARAM_TABLES                4U
#define SFDP_START_ADDR                 0x0U
#define SFDP_PARAM_START_ADDR           0x8U
#define SFDP_HEADER_SIZE                8U
#define PARAM_HEADER_SIZE               8U

#define SFDP_HEADER_MSB_POS             32U
#define SFDP_HEADER_MASK                0xffffffffU
#define SFDP_SGN_START_POS              8U
#define SFDP_SGN_NUM_BYTES              4U
#define SFDP_MINREV_POS                 40U
#define SFDP_MINREV_MASK                0xffU
#define SFDP_MAJORREV_POS               48U
#define SFDP_MAJORREV_MASK              0xffU
#define SFDP_NUM_PARAM_TABLES_POS       56U
#define SFDP_NUM_PARAM_TABLES_MASK      0xffU
#define SFDP_PARAM_HEADER_MSB_POS       32U
#define SFDP_PARAM_LEN_POS              32U
#define SFDP_PARAM_LEN_MASK             0xffU
#define SFDP_PARAM_TABLE_OFFSET_POS     40U
#define SFDP_PARAM_TABLE_OFFSET_MASK    0xffffffU


#define SFDP_PARAM_FLASHSIZE_ADDR       0x34U
#define SFDP_PARAM_FLASHSIZE_SIZE       5U
#define SFDP_PARAM_FLASHSIZE_MSB_POS    32U
#define SFDP_PARAM_FLASHSIZE_POS        8U
#define SFDP_PARAM_FLASHSIZE_MASK       0xffffffffU
#define SFDP_PARAM_PAGESIZE_ADDR        0x58U
#define SFDP_PARAM_PAGESIZE_SIZE        5U
#define SFDP_PARAM_PAGESIZE_POS         12U
#define SFDP_PARAM_PAGESIZE_MASK        0x0fU


#define M25Q_INST_WIDTH                 0U
#define M25Q_DATA_WIDTH                 0U
#define M25Q_ADDR_WIDTH                 0U
#define M25Q_BAUDDIV                    0xfU
#define M25Q_SECTOR_SIZE                4096U
#define M25Q_CLOCK_FREQ                 100000000U
#define M25Q_NSS_DEALY                  0x14U
#define M25Q_INIT_DELAY                 0xc8U
#define M25Q_BTWN_DELAY                 0x14U
#define M25Q_AFTER_DELAY                0xffU
#define M25Q_NUM_ADDR_BYTES             2U
#define M25Q_DUMMY_CYCLES               0U
#define M25Q_QSPI_MODE                  4U

/*@brief The SFDP header structure
 *
 */
struct SFDPHeader
{
    uint32_t signature[4U];
    uint8_t minRev;
    uint8_t majorRev;
    uint8_t numParameterTables;
    uint8_t reserved[1U];
};


/*@brief The SFDP parameter header structure
 *
 */
struct SFDPParamHeader
{
    uint8_t paramId;
    uint8_t minRev;
    uint8_t majorRev;
    uint8_t parameterLength;
    uint32_t parameterTableOffset;
    uint32_t reserved;
};

/*@brief The SFDP parameter table structure
 *
 */
struct SFDPParamTable
{
    uint8_t minSectorEraseSize;
    uint8_t addressBytes;
    uint8_t pageSize;
    uint8_t EraseCmd4K;
    uint8_t addressMode;
    uint8_t DTRMode;
    uint8_t flashDensity;
    uint8_t readModeInterface;
    uint8_t modeBits;
    uint8_t dummyCycle;
    uint64_t flashSize;

};

/*@brief The Flash Adapter structure
 *
 */
struct SFDPObject
{
    struct SFDPHeader stdHeader;
    struct SFDPParamHeader paramHeader[MAX_PARAM_HEADERS];
    struct SFDPParamTable paramTable[MAX_PARAM_TABLES];
};

/*@brief Function pointer to the SFDP parsing
 *       logic
 */
typedef int (*parseSFPD_pf)(void *pQspi, struct SFDPObject *sfdp);

/*@brief The Flash Adapter structure
 *
 */
typedef struct FlashAdapter
{
    uint8_t deviceId;
    struct SFDPObject sfdp;
    parseSFPD_pf parseSfdp;
}FlashAdapter_t;

/**
 * @brief Read the SFDP parameters for the Micron M25Q flash chip.
 *
 * @param[in] pHandle Void pointer which has to be type casted to flash handle.
 *
 * @return
 * - FLASH_OK if the operation is successful.
 * - FLASH_ERROR if the operation failed.
 */
int  parseM25QParameters(void *pHandle,  struct SFDPObject *sfdp);


#endif
