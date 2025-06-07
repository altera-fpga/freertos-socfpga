/*
 * Copyright (c) 2024, Intel Corporation.
 *
 * SPDX-License-Identifier: MIT
 *
 * test for agilex5 usb3
 */
#include <assert.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <socfpga_timer.h>
#include "unity.h"
#include "unity_fixture.h"
#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"
#include "osal_log.h"
#include "msc_app.h"
#include "usb_main.h"
#include "ff_sddisk.h"
#include "ff_stdio.h"

#define ITERATION      10

#define LEN (512)
#define LBA (100)
#define BLOCK_COUNT (1)

uint8_t write_buffer[LEN];
uint8_t read_buffer[LEN];

TEST_GROUP(TEST_USB3);

TEST_SETUP( TEST_USB3 )
{
}

TEST_TEAR_DOWN( TEST_USB3 )
{
}

TEST_GROUP_RUNNER( TEST_USB3 )
{
    RUN_TEST_CASE(TEST_USB3, test_usb3_perfomance);
    RUN_TEST_CASE(TEST_USB3, test_usb3_xfer);
    RUN_TEST_CASE(TEST_USB3, test_usb3_block_read);
    RUN_TEST_CASE(TEST_USB3, test_usb3_block_write);
}

TEST( TEST_USB3,test_usb3_block_read )
{
    uint64_t read_buffer[ 128 ];
    uint64_t invalidAddr = 0xFFFFFFFFFFFFFFFF;
    uint32_t numberOfBlocks = 1;
    bool eRetVal;

    /*Null buffer*/
    eRetVal = usb_disk_read(NULL, 0, numberOfBlocks);
    TEST_ASSERT_NOT_EQUAL(true, eRetVal);

    /*Valid read*/
    eRetVal = usb_disk_read(read_buffer, 0, numberOfBlocks);
    TEST_ASSERT_EQUAL(true, eRetVal);

    /*Invalid address*/
    eRetVal = usb_disk_read(read_buffer, invalidAddr, numberOfBlocks);
    TEST_ASSERT_NOT_EQUAL(true, eRetVal);

    /*Zero length*/
    eRetVal = usb_disk_read(read_buffer, 0, 0);
    TEST_ASSERT_NOT_EQUAL(true, eRetVal);

}

TEST( TEST_USB3,test_usb3_block_write )
{
    uint64_t write_buffer[ 128 ];
    uint32_t invalidAddr = 0xFFFFFFFF;
    uint32_t numberOfBlocks = 1;
    bool eRetVal;

    /*Null buffer*/
    eRetVal = usb_disk_write(NULL, 0, numberOfBlocks);
    TEST_ASSERT_NOT_EQUAL(true, eRetVal);

    /*Valid write*/
    eRetVal = usb_disk_write(write_buffer, 0, numberOfBlocks);
    TEST_ASSERT_EQUAL(true, eRetVal);

    /*Invalid address*/
    eRetVal = usb_disk_write(write_buffer, invalidAddr, numberOfBlocks);
    TEST_ASSERT_NOT_EQUAL(true, eRetVal);

    /*Zero length*/
    eRetVal = usb_disk_write(write_buffer, 0, 0);
    TEST_ASSERT_NOT_EQUAL(true, eRetVal);
}

TEST( TEST_USB3,test_usb3_xfer )
{
    char character = '?';
    bool RespCode = 0;

    for (int i = 0; i < ITERATION; i++)
    {
        character += 1;
        memset(write_buffer, character, LEN);

        RespCode = usb_disk_write(write_buffer, LBA, BLOCK_COUNT);
        TEST_ASSERT_EQUAL(true, RespCode);

        RespCode = usb_disk_read(read_buffer, LBA, BLOCK_COUNT);
        TEST_ASSERT_EQUAL(true, RespCode);

        TEST_ASSERT_EQUAL(0, memcmp(write_buffer, read_buffer, LEN));
    }
}

TEST( TEST_USB3, test_usb3_perfomance)
{
    uint32_t time_start = 0;
    uint32_t time_stop = 0;
    long int xfer_speed = 0;
    int count = 10;
    int i = 0;
    uint32_t file_size = 10240000;

    uint8_t *buffer;

    buffer = (uint8_t *)pvPortMalloc(file_size);
    TEST_ASSERT_NOT_EQUAL(buffer, NULL);

    Timer_Handle_t pxTimerHandle;
    pxTimerHandle = timer_open(0);
    timer_set_period_us( pxTimerHandle, 0xFFFFFFFF );
    timer_start( pxTimerHandle );

    timer_get_value_us( pxTimerHandle, &time_start );

    for(i = 0; i < count; i++)
    {
        usb_disk_write(buffer, 100, (file_size/512));
    }

    timer_get_value_us( pxTimerHandle, &time_stop );

    INFO("----------WRITE OPERATION------------");
    INFO("\tData Transferred  : %d MB", file_size*count/1000000);
    INFO("\tTime elapsed      : %d ms", (time_start-time_stop)/1000);
    xfer_speed = ((file_size*count) / (time_start - time_stop)) * (1e6 / 1048576);
    INFO("\tWrite performance : %ld MBps ", xfer_speed);

    timer_get_value_us( pxTimerHandle, &time_start );

    for(i = 0; i < count; i++)
    {
        usb_disk_read(buffer, 100, (file_size/512));
    }

    timer_get_value_us( pxTimerHandle, &time_stop );

    INFO("----------READ OPERATION------------");
    INFO("\tData Transferred  : %d MB", file_size*count/1000000);
    INFO("\tTime elapsed      : %d ms", (time_start-time_stop)/1000);
    xfer_speed = ((file_size*count) / (time_start - time_stop)) * (1e6 / 1048576);
    INFO("\tRead performance : %ld MBps ", xfer_speed);

    vPortFree(buffer);
}

int usb3_test(void)
{
    UNITY_BEGIN();
    TEST_SETUP(TEST_USB3);
    RUN_TEST_GROUP(TEST_USB3);
    return UNITY_END();
}
