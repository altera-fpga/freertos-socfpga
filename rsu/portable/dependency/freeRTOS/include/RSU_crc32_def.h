/*
 * SPDX-FileCopyrightText: Copyright (C) 2025 Altera Corporation
 *
 * SPDX-License-Identifier: MIT-0
 */

/*
    File: RSU CRC32 Definitions Library

    contains macros and functions used for CRC calculation
*/
#ifndef RSU_PLAT_CRC32_DEF_H
#define RSU_PLAT_CRC32_DEF_H

#include <stdint.h>
#include <stdio.h>

#define CRC_BATCH_SZ                     3990
#define CRC_BATCH_ZEROES                 0xa10d3d0c
#define CRC_BATCH_MIN_SZ                 800
#define CRC_POLY                         0xedb88320


/*
    Function: calculate_crc32

    calculate crc32.

    Parameters:

        initial - CRC value.
        pointer - to the data buffer.
        size - of the data to calculate CRC

    Returns:

        crc value.
*/
unsigned long calculate_crc32(unsigned long int ulCrc, void *vData,
        unsigned long int ulDataSize);
#endif
