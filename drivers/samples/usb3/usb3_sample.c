/*
 * SPDX-FileCopyrightText: Copyright (C) 2025 Altera Corporation
 *
 * SPDX-License-Identifier: MIT-0
 *
 * Sample fatfs application for agilex5 USB3.1
 */

/**
 * @file usb3_sample.c
 * @brief Sample Application for usb3.1 driver.
 */

/**
 * @defgroup usb3_sample USB3.1 Sample Applications
 * @ingroup samples
 *
 * Sample Application for USB3.1 operations
 *
 * @details
 * @subsubsection description Description
 * This is a sample application to demonstrate the fatfs operations on USB3.1 disk. The sample performs the following operations sequentially on the first fat partitions:
 * - Identify the total fatfs partition in the disk. 
 * - Mount the first fat partition.
 * - Write data to a text file.
 * - Create a directory in the partition.
 * - Delete the directry.
 * - Read and display data from the text file.
 * - Delete the file.
 * - Unmount the fat partition.
 *
 * @note The sample supports both USB2 and USB3 devices.
 *
 * @subsubsection prerequisites Prerequisites
 * The USB disk should be partitioned as FAT partition. For Linux based systems, you may use the below commands :
 *  - Use fdisk to partition the USB mass storage device.
 *        - Use mkfs tool to format the USB disk partition into FAT.
 *            - sudo mkfs.fat /dev/<partition>
 *
 * @subsubsection how_to_run How to Run
 * 1. Follow the common README for build and flashing instructions.
 * 2. Insert the partitioned USB device into the device, and run the sample..
 *
 * @subsubsection expected_results Expected Results
 * The result of fat operations are displayed in the console.
 *
 * @note Due to a fatfs limitation, all the rbf file names should be limited to 8 characters.
 * @{
 */
/** @} */


#include "ff_sddisk.h"
#include "ff_sys.h"
#include "usb_main.h"
#include "osal_log.h"

#define TASK_PRIORITY      (configMAX_PRIORITIES - 2)

#define USB_MOUNT_POINT    "/usb"
#define USB_MOUNT_ADDR     0

void run_usb_fatfs(void);

void usb3_sample_task(void)
{
    BaseType_t xReturn;

    xReturn = xTaskCreate(usb3_task, "usb3_sample",
            configMINIMAL_STACK_SIZE * 20,
            NULL, TASK_PRIORITY - 1, NULL);
    if (xReturn != 1)
    {
        //can not create task
        while (1);
    }

    run_usb_fatfs();
}

void run_usb_fatfs(void)
{
    const char *pcFileName = "/b.txt";
    const char *pcSampleDir = "/Test1";
    char *write_data1 =
            "Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor";
    uint8_t pcReadBuffer[ 512 ] ={0};
    size_t xBytesWritten,xBytesRead;

    FF_Disk_t *pxDisk;
    FF_FILE *pxFile;
    FF_Error_t xError;
    FF_SPartFound_t partsFound;
	partsFound.iCount = 0;

    INFO("Waiting for the USB3 disk to be mounted");

    /* wait for the usb3 disk to be mounted */
    wait_for_usb_mount_complete();

    INFO("Mass Storage Device Mounted");
    /***********************************************************************************/
    /*                          USB Disk Init                                          */
    /***********************************************************************************/
    pxDisk = FF_SDDiskInit(USB_MOUNT_POINT, USB_MOUNT_ADDR);
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
        ERROR("USB disk does not contain any fat partition!!!");
        return;
    }

    INFO("Running fat operations on first partition");

    /***********************************************************************************/
    /*                          Mount the USB                                          */
    /***********************************************************************************/
    xError = FF_Mount(pxDisk, 0);
    if (xError != FF_ERR_NONE)
    {
        ERROR("Failed to mount filesystem");
        FF_SDDiskDelete(pxDisk);
        return;
    }
    INFO("File system mounted successfully");

    /***********************************************************************************/
    /*                          create a directory                                     */
    /***********************************************************************************/
    xError = FF_MkDir(pxDisk->pxIOManager, pcSampleDir);
    if (xError != FF_ERR_NONE)
    {
        ERROR("Failed to create directory");
        FF_Unmount(pxDisk);
        FF_SDDiskDelete(pxDisk);
        return;
    }
    INFO("Directory created successfully");

    /***********************************************************************************/
    /*                         delete the directory                                    */
    /***********************************************************************************/
    xError = FF_RmDir(pxDisk->pxIOManager, pcSampleDir);
    if (xError != FF_ERR_NONE)
    {
        ERROR("Failed to remove directory");
        FF_Unmount(pxDisk);
        FF_SDDiskDelete(pxDisk);
        return;
    }
    INFO("Directory removed successfully");

    /***********************************************************************************/
    /*                         write to a file                                         */
    /***********************************************************************************/

    pxFile = FF_Open(pxDisk->pxIOManager, pcFileName,
            FF_MODE_WRITE | FF_MODE_CREATE, &xError);
    if ((pxFile == NULL) || (xError != FF_ERR_NONE))
    {
        ERROR("Failed to open file for writing");
        FF_Unmount(pxDisk);
        FF_SDDiskDelete(pxDisk);
        return;
    }
    xBytesWritten = FF_Write(pxFile, 1, strlen((char*) write_data1),
            (uint8_t*) write_data1);
    if ((xBytesWritten != strlen((char*)write_data1)) ||
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

    /***********************************************************************************/
    /*                         read from a file                                        */
    /***********************************************************************************/
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

    /***********************************************************************************/
    /*                         delete a file                                           */
    /***********************************************************************************/
    xError = FF_RmFile(pxDisk->pxIOManager, pcFileName);
    if (xError != FF_ERR_NONE)
    {
        ERROR("Failed to delete file");
        FF_Unmount(pxDisk);
        FF_SDDiskDelete(pxDisk);
        return;
    }
    INFO("File deleted successfully");

    /***********************************************************************************/
    /*                         unmount the usb drive                                   */
    /***********************************************************************************/
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
