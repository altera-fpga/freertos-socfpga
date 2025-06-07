/*
 * Copyright (c) 2024, Intel Corporation.
 *
 * SPDX-License-Identifier: MIT
 *
 * test for SoC FPGA fatfs
 */

#include "ff_sddisk.h"
#include "unity.h"
#include "unity_fixture.h"
#include "FreeRTOS.h"
#include "task.h"
#include "ff_sys.h"
#include "ff_stdio.h"
#include <assert.h>

#define MOUNTED      1
#define UNMOUNTED    0

int disk_no = -2;
FF_Disk_t *DiskObject = NULL;
uint8_t mount_state = UNMOUNTED;
uint8_t rbuf[ 5150 ] =
{ 0 };
uint8_t wbuf[ 5150 ] =
{ 0 };

TEST_GROUP(TEST_FAT);

TEST_SETUP( TEST_FAT )
{
    if (mount_state == UNMOUNTED)
    {
        DiskObject = FF_SDDiskInit("/usb", disk_no);
        TEST_ASSERT_NOT_NULL(DiskObject);
        mount_state = MOUNTED;
    }
}
TEST_TEAR_DOWN( TEST_FAT )
{
    if (mount_state == MOUNTED)
    {
        FF_SDDiskDelete(DiskObject);
        mount_state = UNMOUNTED;
    }
}

TEST_GROUP_RUNNER( TEST_FAT )
{
    RUN_TEST_CASE(TEST_FAT, test_FF_Mount);
    RUN_TEST_CASE(TEST_FAT, test_FF_Unmount);
    RUN_TEST_CASE(TEST_FAT, test_FF_Open_Write_Read_Close);
    RUN_TEST_CASE(TEST_FAT, test_Mount_Mount);
    RUN_TEST_CASE(TEST_FAT, test_FF_MultiBlock_MultiFile_Write);
    RUN_TEST_CASE(TEST_FAT, test_FF_NestedDirectories);
    RUN_TEST_CASE(TEST_FAT, test_FF_NestedRewrite);
}

TEST( TEST_FAT, test_Mount_Mount )
{
    TEST_ASSERT_EQUAL(FF_ERR_NONE, FF_Mount(DiskObject, 0));
    TEST_ASSERT_EQUAL(FF_ERR_NONE, FF_Mount(DiskObject, 0));
}

TEST( TEST_FAT, test_Mount_UnMount_Open )
{
    FF_FILE *pxFile;
    const char *pcFileName = "/testfile.txt";
    FF_Error_t xError;

    TEST_ASSERT_EQUAL(FF_ERR_NONE, FF_Mount(DiskObject, 0));
    TEST_ASSERT_EQUAL(FF_ERR_NONE, FF_Unmount(DiskObject));
    pxFile = FF_Open(DiskObject->pxIOManager, pcFileName,
            FF_MODE_WRITE | FF_MODE_CREATE, &xError);
    TEST_ASSERT_NULL(pxFile);
    TEST_ASSERT_NOT_EQUAL(FF_ERR_NONE, xError);
}

TEST( TEST_FAT, test_FF_Mount )
{
    TEST_ASSERT_EQUAL(FF_ERR_NONE, FF_Mount(DiskObject, 0));
}

TEST( TEST_FAT, test_FF_Unmount )
{
    TEST_ASSERT_EQUAL(FF_ERR_NONE, FF_Unmount(DiskObject));
}

TEST( TEST_FAT, test_FF_Open_Write_Read_Close )
{
    FF_FILE *pxFile;
    const char *pcFileName = "/testfile.txt";
    const char *pcDataToWrite = "Hello, FreeRTOS+FAT!";
    char pcReadBuffer[ 50 ];
    size_t xBytesWritten, xBytesRead;
    FF_Error_t xError;

    /*Open file for writing*/
    pxFile = FF_Open(DiskObject->pxIOManager, pcFileName,
            FF_MODE_WRITE | FF_MODE_CREATE, &xError);
    TEST_ASSERT_NOT_NULL(pxFile);
    TEST_ASSERT_EQUAL(FF_ERR_NONE, xError);

    /*Write data to the file*/
    xBytesWritten = FF_Write(pxFile, 1, strlen(pcDataToWrite) + 1,
            (uint8_t*) pcDataToWrite);
    TEST_ASSERT_EQUAL((strlen(pcDataToWrite) + 1), xBytesWritten);
    TEST_ASSERT_EQUAL(FF_ERR_NONE, xError);

    /*Close the file*/
    TEST_ASSERT_EQUAL(FF_ERR_NONE, FF_Close(pxFile));

    /*Open file for reading*/
    pxFile = FF_Open(DiskObject->pxIOManager, pcFileName, FF_MODE_READ,
            &xError);
    TEST_ASSERT_NOT_NULL(pxFile);
    TEST_ASSERT_EQUAL(FF_ERR_NONE, xError);

    /*Read data from the file*/
    xBytesRead = FF_Read(pxFile, 1, strlen(pcDataToWrite) + 1,
            (uint8_t*) pcReadBuffer);
    TEST_ASSERT_GREATER_THAN(0, xBytesRead);
    TEST_ASSERT_EQUAL(FF_ERR_NONE, xError);

    /*Null-terminate the read buffer*/
    pcReadBuffer[ xBytesRead ] = '\0';

    /*Verify the read data*/
    TEST_ASSERT_EQUAL_STRING(pcDataToWrite, pcReadBuffer);

    /*Close the file*/
    TEST_ASSERT_EQUAL(FF_ERR_NONE, FF_Close(pxFile));

    /*Delete the file*/
    TEST_ASSERT_EQUAL(FF_ERR_NONE,
            FF_RmFile(DiskObject->pxIOManager, pcFileName));
}

/* *************************
 * Test configurations for:
 * test_FF_MultiBlock_MultiFile_Write
 * test_FF_NestedDirectories
 * test_FF_NestedRewrite */

#define FF_BLOCK_SIZE         (512)
#define FF_TEST_FILE_COUNT    (10)
#define FF_BLOCK_COUNT        (10)
#define FF_TOTAL_FILE_SIZE    (FF_BLOCK_COUNT * FF_BLOCK_SIZE)
/**************************************/

/*Test to force the file content to be scattered in different blocks of memory*/
TEST( TEST_FAT, test_FF_MultiBlock_MultiFile_Write )
{
    FF_FILE *pxFile;
    char file_name[ 15 ] =
    { 0 };
    uint8_t fill_byte[ FF_BLOCK_COUNT ][ FF_TEST_FILE_COUNT ];
    uint32_t b = 0, f = 0;
    FF_Error_t xError;

    /*Initialize fill_byte table. 0x<f><b> is used as the byte to fill
     * where f is the file number and b is the block number*/
    for (b = 0; b < FF_BLOCK_COUNT; b++)
    {
        for (f = 0; f < FF_TEST_FILE_COUNT; f++)
        {
            fill_byte[ b ][ f ] = ((f + 1) << 4) + (b + 1);
        }
    }

    /*
     * Write multiple blocks, one block at a time to all the files.
     * The file is closed after writing one block. This is repeated for all the files.
     * Then the files are open in append mode and next block is written.
     * This sequence is repeated for all the blocks.
     */
    for (b = 0; b < FF_BLOCK_COUNT; b++)
    {
        for (f = 0; f < FF_TEST_FILE_COUNT; f++)
        {
            /*open file in append mode*/
            sprintf(file_name, "/f%x.txt", f);
            pxFile = FF_Open(DiskObject->pxIOManager, file_name,
                    FF_MODE_APPEND | FF_MODE_WRITE | FF_MODE_CREATE, &xError);
            TEST_ASSERT_NOT_NULL(pxFile);
            TEST_ASSERT_EQUAL(FF_ERR_NONE, xError);

            /*write one block of data*/
            memset(wbuf, fill_byte[ b ][ f ], FF_BLOCK_SIZE);
            FF_Write(pxFile, 1, FF_BLOCK_SIZE, wbuf);
            TEST_ASSERT_EQUAL(FF_ERR_NONE, xError);

            /*close the file after writing one block*/
            TEST_ASSERT_EQUAL(FF_ERR_NONE, FF_Close(pxFile));
        }
    }

    /*
     * open file and read it
     * compare the first and last byte of a block with the bytes written.
     * repeat for all the blocks.
     * close and remove the file. Repeat for all files.
     */
    for (f = 0; f < FF_TEST_FILE_COUNT; f++)
    {
        /*open file in read mode*/
        sprintf(file_name, "/f%x.txt", f);
        pxFile = FF_Open(DiskObject->pxIOManager, file_name, FF_MODE_READ,
                &xError);
        TEST_ASSERT_NOT_NULL(pxFile);
        TEST_ASSERT_EQUAL(FF_ERR_NONE, xError);

        /*read the total number of bytes written*/
        FF_Read(pxFile, 1, FF_TOTAL_FILE_SIZE, rbuf);
        TEST_ASSERT_EQUAL(FF_ERR_NONE, xError);

        /*compare the first and last byte of each block*/
        for (b = 0; b < FF_BLOCK_COUNT; b++)
        {
            TEST_ASSERT_EQUAL(fill_byte[ b ][ f ], rbuf[b * FF_BLOCK_SIZE]);
            TEST_ASSERT_EQUAL(fill_byte[ b ][ f ],
                    rbuf[((b + 1) * FF_BLOCK_SIZE) - 1]);
        }

        /*close the file after verifying*/
        TEST_ASSERT_EQUAL(FF_ERR_NONE, FF_Close(pxFile));

        /*remove file so it does not fill up the memory*/
        TEST_ASSERT_EQUAL(FF_ERR_NONE,
                FF_RmFile(DiskObject->pxIOManager, file_name));
    }
}

/*test to check functionalities in longer path names and nested directories*/
TEST( TEST_FAT, test_FF_NestedDirectories )
{
    FF_FILE *pxFile;
    char file_name[ 15 ] =
    { 0 };
    char dir_name[ 15 ] =
    { 0 };
    char dir_path[ 100 ] =
    { 0 };
    char file_path[ 100 ] =
    { 0 };
    uint8_t fill_byte[ 15 ] =
    { 0 };
    uint32_t f = 0;
    FF_Error_t xError;

    /*Initializing fill_byte array
     * fill_byte[f] to fill file f, where f is the file number
     */
    for (f = 0; f < FF_TEST_FILE_COUNT; f++)
    {
        fill_byte[ f ] = (f + 1) * 0x11;
    }

    /*
     * create a directory within the current directory
     * open a file within the directory
     * write into the file and close the file
     * repeat it for FF_TEST_FILE_COUNT
     */
    for (f = 0; f < FF_TEST_FILE_COUNT; f++)
    {
        /*create directory and add it to directory path*/
        sprintf(dir_name, "/d%x", f);
        strcat(dir_path, dir_name);
        FF_MkDir(DiskObject->pxIOManager, dir_path);

        /* open a file in write mode in the directory path and update it in file path */
        memcpy(file_path, dir_path, strlen(dir_path));
        sprintf(file_name, "/f%x.txt", f);
        strcat(file_path, file_name);
        pxFile = FF_Open(DiskObject->pxIOManager, file_path,
                FF_MODE_WRITE | FF_MODE_CREATE, &xError);

        /*fill 10 blocks of data in each file*/
        memset(wbuf, fill_byte[ f ], FF_TOTAL_FILE_SIZE);
        FF_Write(pxFile, 1, FF_TOTAL_FILE_SIZE, wbuf);
        TEST_ASSERT_EQUAL(FF_ERR_NONE, xError);

        /* close the file after writing one block */
        TEST_ASSERT_EQUAL(FF_ERR_NONE, FF_Close(pxFile));

        /* clear file_path so it can be used to update the next file's path */
        memset(file_path, 0, strlen(file_path));
    }

    /*directory path is reset before starting to read back from files*/
    memset(dir_path, 0, strlen(dir_path));

    /* open the file in the nested directory
     * read the file
     * verify the first and last byte of each file
     * close the file
     */
    for (f = 0; f < FF_TEST_FILE_COUNT; f++)
    {
        /*update the directory path*/
        sprintf(dir_name, "/d%x", f);
        strcat(dir_path, dir_name);
        /*update the file path, open the file in read mode and read from it*/
        memcpy(file_path, dir_path, strlen(dir_path));
        sprintf(file_name, "/f%x.txt", f);
        strcat(file_path, file_name);
        pxFile = FF_Open(DiskObject->pxIOManager, file_path,
                FF_MODE_READ, &xError);

        FF_Read(pxFile, 1, FF_TOTAL_FILE_SIZE, rbuf);
        TEST_ASSERT_EQUAL(FF_ERR_NONE, xError);

        /*compare first and last byte of each file with the fill_byte array*/
        TEST_ASSERT_EQUAL(fill_byte[ f ], rbuf[ 0 ]);
        TEST_ASSERT_EQUAL(fill_byte[ f ], rbuf[ 5119 ]);

        /*close the file after reading*/
        TEST_ASSERT_EQUAL(FF_ERR_NONE, FF_Close(pxFile));
        /*reset file_path to update the path to next file*/
        memset(file_path, 0, strlen(file_path));
    }

    memset(rbuf, 0, FF_TOTAL_FILE_SIZE);
}

/* test to rewrite and verify files in the nested directories */
TEST( TEST_FAT, test_FF_NestedRewrite )
{
    FF_FILE *pxFile;
    char file_name[ 15 ] =
    { 0 };
    char dir_name[ 15 ] =
    { 0 };
    char dir_path[ 100 ] =
    { 0 };
    char file_path[ 100 ] =
    { 0 };
    uint8_t fill_byte[ 15 ] =
    { 0 };

    uint32_t f = 0, size = 0;
    FF_Error_t xError;

    /*Initializing fill_byte array
     * fill_byte[f] to fill file f, where f is the file number
     */
    for (f = 0; f < FF_TEST_FILE_COUNT; f++)
    {
        fill_byte[ f ] = (f * 0x10) + 0x19;
    }

    /*
     * open the file in the nested directory
     * rewrite the file
     * close the file
     * repeat it for all the files
     */
    for (f = 0; f < FF_TEST_FILE_COUNT; f++)
    {
        /*update the directory path*/
        sprintf(dir_name, "/d%x", f);
        strcat(dir_path, dir_name);
        /*update the file path, open the file in write mode*/
        memcpy(file_path, dir_path, strlen(dir_path));
        sprintf(file_name, "/f%x.txt", f);
        strcat(file_path, file_name);
        pxFile = FF_Open(DiskObject->pxIOManager, file_path,
                FF_MODE_WRITE | FF_MODE_CREATE, &xError);

        /*rewrite to the file for 10 blocks*/
        memset(wbuf, fill_byte[ f ], FF_TOTAL_FILE_SIZE);
        FF_Write(pxFile, 1, FF_TOTAL_FILE_SIZE, wbuf);
        TEST_ASSERT_EQUAL(FF_ERR_NONE, xError);

        /*close the file after writing one block*/
        TEST_ASSERT_EQUAL(FF_ERR_NONE, FF_Close(pxFile));
        /* clear file_path to update the next file's path */
        memset(file_path, 0, strlen(file_path));
    }

    /*directory path is reset before starting to read back from files*/
    memset(dir_path, 0, strlen(dir_path));

    /* open the file in the nested directory
     * read the file
     * verify the first and last byte of each file
     * close the file
     */
    for (f = 0; f < FF_TEST_FILE_COUNT; f++)
    {
        /*update the directory path*/
        sprintf(dir_name, "/d%x", f);
        strcat(dir_path, dir_name);
        /*update the file path, open the file in read mode and read from it*/
        memcpy(file_path, dir_path, strlen(dir_path));
        sprintf(file_name, "/f%x.txt", f);
        strcat(file_path, file_name);
        pxFile = FF_Open(DiskObject->pxIOManager, file_path,
                FF_MODE_READ, &xError);

        FF_Read(pxFile, 1, FF_TOTAL_FILE_SIZE, rbuf);
        TEST_ASSERT_EQUAL(FF_ERR_NONE, xError);
        /*compare first and last byte of each file with the fill_byte array*/
        TEST_ASSERT_EQUAL(fill_byte[ f ], rbuf[ 0 ]);
        TEST_ASSERT_EQUAL(fill_byte[ f ], rbuf[ 5119 ]);
        /*close the file after reading*/
        TEST_ASSERT_EQUAL(FF_ERR_NONE, FF_Close(pxFile));
        /*reset file_path to update the path to next file*/
        memset(file_path, 0, strlen(file_path));
    }

    /*delete directories and files after the test is completed*/
    for (f = 0; f < FF_TEST_FILE_COUNT; f++)
    {
        /*update the file path*/
        sprintf(file_name, "/f%x.txt", (FF_TEST_FILE_COUNT - 1) - f);
        memcpy(file_path, dir_path, strlen(dir_path));
        strcat(file_path, file_name);
        /*remove the file and then the directory*/
        TEST_ASSERT_EQUAL(FF_ERR_NONE,
                FF_RmFile(DiskObject->pxIOManager, file_path));
        TEST_ASSERT_EQUAL(FF_ERR_NONE,
                FF_RmDir(DiskObject->pxIOManager, dir_path));

        /*remove the deleted directory from the directory path*/
        size = strlen(dir_path);
        dir_path[ size - 3 ] = '\0';
        memset(file_path, 0, strlen(file_path));
    }
}

void fatfs_test( int disk_type )
{
    disk_no = disk_type;

    if( disk_no != -2 )
    {
        UNITY_BEGIN();
        TEST_SETUP(TEST_FAT);
        RUN_TEST_GROUP(TEST_FAT);
        UNITY_END();
    }
}
