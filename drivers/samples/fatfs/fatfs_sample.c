/*
 * SPDX-FileCopyrightText: Copyright (C) 2025 Altera Corporation
 *
 * SPDX-License-Identifier: MIT-0
 *
 * Sample application for FAT FS operation with SD card storage
 */

/**
 * @file fatfs_sample.c
 * @brief Sample Application for fatfs operations.
 */

/**
 * @defgroup fatfs_sample Fatfs Sample Applications
 * @ingroup samples
 *
 * Sample Application for fatfs opertaions.
 *
 * @details
 * @subsubsection description Description
 * This is a sample application to demonstrate the fatfs operations on SD card. The sample performs the following operations sequentially on the first available fat partition.
 * - Identify all the fatfs partition in the sdcard.
 * - Mount the first fat partition.
 * - Write data to a text file.
 * - Create a directory in the partition.
 * - Delete the directry.
 * - Read and display data from the text file.
 * - Delete the file.
 * - Unmount the fat partition.
 *
 * @subsubsection prerequisites Prerequisites
 * The SD card should be partitioned as FAT partition. In Linux based systems, you may use the below commands :
 *  - Use fdisk to partition the SD card.
 *        - Use mkfs tool to format the sd card partition into FAT.
 *            - sudo mkfs.fat /dev/<partition>
 *
 * @subsubsection how_to_run How to Run
 * 1. Follow the common README for build and flashing instructions.
 * 2. Insert the partitioned SD card into the device, and run the sample.
 *
 * @subsubsection expected_results Expected Results
 * - The result of fat operations are displayed in the console.
 * @{
 */
/** @} */

#include "ff_sddisk.h"
#include "FreeRTOS.h"
#include "ff_sys.h"
#include "ff_stdio.h"
#include "fatfs_sample.h"
#include <string.h>
#include "osal_log.h"

#define MOUNT_POINT    "/sdcard"

void fatfs_task()
{
    FF_Disk_t *pxDisk;
    FF_FILE *pxFile;
    FF_Error_t xError;
    FF_SPartFound_t partsFound;
    partsFound.iCount = 0;
    const char *pcFileName = "/sample.txt";
    char *pcDataToWrite =
            "Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor";
    const char *pcSampleDir = "/Test1";
    uint8_t pcReadBuffer[ 128 ];
    size_t xBytesWritten, xBytesRead;

    pxDisk = FF_SDDiskInit(MOUNT_POINT,-1);
    if (pxDisk == NULL)
    {
        ERROR("Failed to initialize disk");
        return;
    }

    /* Get the Fatfs partition count */
    int fat_pcount = FF_PartitionSearch(pxDisk->pxIOManager, &partsFound);

    INFO("Fatfs partitions available : %d", fat_pcount);

    if (fat_pcount == 0)
    {
        ERROR("SD card does not contain any fat partition!!!");
        ERROR("Exiting fatfs sample");
        return;
    }

    INFO("Running fat operations on first partition");

    xError = FF_Mount(pxDisk, 0);
    if (xError != FF_ERR_NONE)
    {
        ERROR("Failed to mount filesystem");
        FF_SDDiskDelete(pxDisk);
        return;
    }
    INFO("\rFile system mounted successfully\n");

    pxFile = FF_Open(pxDisk->pxIOManager, pcFileName,
            FF_MODE_WRITE | FF_MODE_CREATE, &xError);
    if ((pxFile == NULL) || (xError != FF_ERR_NONE))
    {
        ERROR("Failed to open file for writing");
        FF_Unmount(pxDisk);
        FF_SDDiskDelete(pxDisk);
        return;
    }
    xBytesWritten = FF_Write(pxFile, 1, strlen((char*) pcDataToWrite),
            (uint8_t*) pcDataToWrite);
    if ((xBytesWritten != strlen((char*) pcDataToWrite)) ||
            (xError != FF_ERR_NONE))
    {
        ERROR("Failed to write data to file");
        FF_Close(pxFile);
        FF_Unmount(pxDisk);
        FF_SDDiskDelete(pxDisk);
        return;
    }
    FF_Close(pxFile);
    INFO("Data written to file successfully");

    xError = FF_MkDir(pxDisk->pxIOManager, pcSampleDir);
    if (xError != FF_ERR_NONE)
    {
        ERROR("Failed to create directory");
        FF_Unmount(pxDisk);
        FF_SDDiskDelete(pxDisk);
        return;
    }
    INFO("Directory created successfully");

    xError = FF_RmDir(pxDisk->pxIOManager, pcSampleDir);
    if (xError != FF_ERR_NONE)
    {
        ERROR("Failed to remove directory");
        FF_Unmount(pxDisk);
        FF_SDDiskDelete(pxDisk);
        return;
    }
    INFO("Directory removed successfully");

    pxFile = FF_Open(pxDisk->pxIOManager, pcFileName, FF_MODE_READ, &xError);
    if ((pxFile == NULL) || (xError != FF_ERR_NONE))
    {
        ERROR("Failed to open file for reading");
        FF_Unmount(pxDisk);
        FF_SDDiskDelete(pxDisk);
        return;
    }
    xBytesRead = FF_Read(pxFile, 1, sizeof(pcReadBuffer) - 1, pcReadBuffer);
    if ((xBytesRead <= 0) || (xError != FF_ERR_NONE))
    {
        ERROR("Failed to read data from file");
        FF_Close(pxFile);
        FF_Unmount(pxDisk);
        FF_SDDiskDelete(pxDisk);
        return;
    }
    pcReadBuffer[ xBytesRead ] = '\0';
    FF_Close(pxFile);
    INFO("Data read from file: %s", pcReadBuffer);

    xError = FF_RmFile(pxDisk->pxIOManager, pcFileName);
    if (xError != FF_ERR_NONE)
    {
        ERROR("Failed to delete file");
        FF_Unmount(pxDisk);
        FF_SDDiskDelete(pxDisk);
        return;
    }
    INFO("File deleted successfully");

    xError = FF_Unmount(pxDisk);
    if (xError != FF_ERR_NONE)
    {
        ERROR("Failed to unmount filesystem");
        FF_SDDiskDelete(pxDisk);
        return;
    }
    INFO("Filesystem unmounted successfully");

    FF_SDDiskDelete(pxDisk);
    INFO("Disk deleted successfully");
}

