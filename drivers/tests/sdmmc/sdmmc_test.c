/*
 * Copyright (c) 2024, Intel Corporation.
 *
 * SPDX-License-Identifier: MIT
 *
 * test for SoC FPGA sdmmc
 */
#include <assert.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include "socfpga_sdmmc.h"
#include "unity.h"
#include "unity_fixture.h"
#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"
#include "osal_log.h"
#include "socfpga_cache.h"
#include <socfpga_timer.h>

#define BLK_SIZE       512
#define BLK_NUM        8192
#define BUFF_ADD       0x8e900000
#define BLK_CNT        2000
#define BLK_CNT_PER    20000
#define SYMBOL         0x6
#define ITERATION      10

uint8_t writebuff[ BLK_SIZE * BLK_CNT_PER ];
uint8_t readbuff[ BLK_SIZE * BLK_CNT_PER ];

static void sdmmc_cb( int32_t state );
static void wait_xfer_done();
static int32_t xfer_resp;
static osal_semaphore_t xfer_lock;
static osal_semaphore_def_t osal_def_xfer;


static int32_t SD_READ_TEST( uint32_t ulBlockNum, uint32_t ulBlockCnt,
        uint8_t *buff )
{
    return sdmmc_read_block_sync((uint64_t*) buff, ulBlockNum * BLK_SIZE, BLK_SIZE,
            ulBlockCnt);
}

static int32_t SD_WRITE_TEST( uint32_t ulBlockNum, uint32_t ulBlockCnt,
        uint8_t *buff )
{
    return sdmmc_write_block_sync((uint64_t*) buff, ulBlockNum * BLK_SIZE, BLK_SIZE,
            ulBlockCnt);
}

static int32_t SD_READ_TEST_ASYNC( uint32_t ulBlockNum,
        uint32_t ulBlockCnt,
        uint8_t *buff )
{
    return sdmmc_read_block_async((uint64_t*) buff, ulBlockNum * BLK_SIZE,
            BLK_SIZE,
            ulBlockCnt,sdmmc_cb);
}

static int32_t SD_WRITE_TEST_ASYNC( uint32_t ulBlockNum,
        uint32_t ulBlockCnt,
        uint8_t *buff )
{
    return sdmmc_write_block_async((uint64_t*) buff, ulBlockNum * BLK_SIZE,
            BLK_SIZE,
            ulBlockCnt,sdmmc_cb);
}

TEST_GROUP(TEST_SDMMC);

TEST_SETUP( TEST_SDMMC )
{
}

TEST_TEAR_DOWN( TEST_SDMMC )
{
}

TEST_GROUP_RUNNER( TEST_SDMMC )
{
    RUN_TEST_CASE(TEST_SDMMC, test_sdmmc_init);
    RUN_TEST_CASE( TEST_SDMMC, test_sdmmc_write_perfomance)
    RUN_TEST_CASE( TEST_SDMMC, test_sdmmc_read_perfomance)
    RUN_TEST_CASE(TEST_SDMMC, test_socfpga_xfer);
    RUN_TEST_CASE(TEST_SDMMC, test_socfpga_block_read);
    RUN_TEST_CASE(TEST_SDMMC, test_socfpga_block_write);
    RUN_TEST_CASE(TEST_SDMMC, test_socfpga_block_read_async);
    RUN_TEST_CASE(TEST_SDMMC, test_socfpga_block_write_async);
    RUN_TEST_CASE( TEST_SDMMC,test_socfpga_xfer_async );
}

TEST( TEST_SDMMC,test_sdmmc_init )
{
    uint64_t secNum = 0;
    int32_t eRetVal;

    /*Null pointer*/
    eRetVal = sdmmc_init_card(NULL);
    TEST_ASSERT_NOT_EQUAL(0, eRetVal);

    /*Valid initialization*/
    eRetVal = sdmmc_init_card(&secNum);
    TEST_ASSERT_EQUAL(0, eRetVal);
}

TEST( TEST_SDMMC,test_socfpga_block_read )
{
    uint64_t readBuffer[ 128 ];
    uint32_t blockSize = 512;
    uint32_t numberOfBlocks = 1;
    int32_t eRetVal;

    /*Null buffer*/
    eRetVal = sdmmc_read_block_sync(NULL, 0, blockSize, numberOfBlocks);
    TEST_ASSERT_NOT_EQUAL(0, eRetVal);

    /*Valid read*/
    eRetVal = sdmmc_read_block_sync(readBuffer, 0, blockSize, numberOfBlocks);
    TEST_ASSERT_EQUAL(0, eRetVal);

    /*Zero length*/
    eRetVal = sdmmc_read_block_sync(readBuffer, 0, 0, numberOfBlocks);
    TEST_ASSERT_NOT_EQUAL(0, eRetVal);

}

TEST( TEST_SDMMC,test_socfpga_block_write )
{
    uint64_t writeBuffer[ 128 ];
    uint32_t blockSize = 512;
    uint32_t numberOfBlocks = 1;
    uint64_t secNum = 0;
    int32_t eRetVal;

    /*INFO: reinit the card after tx time out error*/
    sdmmc_init_card(&secNum);

    /*Null buffer*/
    eRetVal = sdmmc_write_block_sync(NULL, 0, blockSize, numberOfBlocks);
    TEST_ASSERT_NOT_EQUAL(0, eRetVal);

    /*Valid write*/
    eRetVal = sdmmc_write_block_sync(writeBuffer, 0, blockSize, numberOfBlocks);
    TEST_ASSERT_EQUAL(0, eRetVal);

    /*Zero length*/
    eRetVal = sdmmc_write_block_sync(writeBuffer, 0, 0, numberOfBlocks);
    TEST_ASSERT_NOT_EQUAL(0, eRetVal);

}

TEST( TEST_SDMMC,test_socfpga_xfer )
{
    uint8_t character = SYMBOL;
    int32_t RespCode = 0;

    for (int i = 0; i < ITERATION; i++)
    {
        character += 1;
        memset(writebuff, character, BLK_CNT * BLK_SIZE);

        RespCode = SD_WRITE_TEST( BLK_NUM, BLK_CNT, writebuff);
        TEST_ASSERT_EQUAL(0, RespCode);
        RespCode = SD_READ_TEST( BLK_NUM, BLK_CNT, readbuff);
        TEST_ASSERT_EQUAL(0, RespCode);
        TEST_ASSERT_EQUAL(0, memcmp(writebuff, readbuff, BLK_CNT * BLK_SIZE));

    }
}

TEST( TEST_SDMMC, test_sdmmc_write_perfomance)
{
    uint32_t time_start = 0;
    uint32_t time_stop = 0;
    int32_t RespCode = 0;
    uint64_t secNum = 0;
    int mbps =0;
    int size;
    Timer_Handle_t pxTimerHandle;
    memset(writebuff,0x9, BLK_CNT_PER * BLK_SIZE);
    /*INFO: reinit the card after tx time out error*/
    sdmmc_init_card(&secNum);

    pxTimerHandle = timer_open(0);
    timer_set_period_us( pxTimerHandle, 0xFFFFFFFF );
    timer_start( pxTimerHandle );
    timer_get_value_us( pxTimerHandle, &time_start );

    RespCode = SD_WRITE_TEST( BLK_NUM, BLK_CNT_PER, writebuff);
    timer_get_value_us( pxTimerHandle, &time_stop );
    timer_stop( pxTimerHandle );
    timer_close( pxTimerHandle );
    size = BLK_SIZE * BLK_CNT_PER;
    mbps = ((size) / (time_start - time_stop)) * (1e6 / 1048576);
    printf("\r Write Perfomance Measured -> %d MBps\n",mbps );
    TEST_ASSERT_EQUAL(0, RespCode);
}
TEST( TEST_SDMMC, test_sdmmc_read_perfomance)
{
    uint32_t time_start = 0;
    uint32_t time_stop = 0;
    int32_t RespCode = 0;
    uint64_t secNum = 0;
    int mbps = 0;
    int size;
    Timer_Handle_t pxTimerHandle;
    /*INFO: reinit the card after tx time out error*/
    sdmmc_init_card(&secNum);

    pxTimerHandle = timer_open(0);
    timer_set_period_us( pxTimerHandle, 0xFFFFFFFF );
    timer_start( pxTimerHandle );
    timer_get_value_us( pxTimerHandle, &time_start );

    RespCode = SD_READ_TEST( BLK_NUM, BLK_CNT_PER, readbuff);
    timer_get_value_us( pxTimerHandle, &time_stop );
    timer_stop( pxTimerHandle );
    timer_close( pxTimerHandle );
    size = BLK_SIZE * BLK_CNT_PER;
    mbps = ((size) / (time_start - time_stop)) * (1e6 / 1048576);
    printf("\r Read Perfomance Measured -> %d MBps\n",mbps );
    TEST_ASSERT_EQUAL(0, RespCode);
}


TEST(TEST_SDMMC, test_socfpga_block_read_async)
{
    uint64_t readBuffer[ 128 ];
    uint32_t blockSize = 512;
    uint32_t numberOfBlocks = 1;
    int32_t eRetVal;
    uint64_t secNum = 0;

    /*INFO: reinit the card after tx time out error*/
    sdmmc_init_card(&secNum);

    /*Null buffer*/
    eRetVal =
            sdmmc_read_block_async(NULL, 0, blockSize, numberOfBlocks,sdmmc_cb);
    TEST_ASSERT_NOT_EQUAL(0, eRetVal);

    /*Valid read*/
    eRetVal = sdmmc_read_block_async(readBuffer, 0, blockSize, numberOfBlocks,
            sdmmc_cb);
    TEST_ASSERT_EQUAL(0, eRetVal);
    wait_xfer_done();
    TEST_ASSERT_EQUAL(0, xfer_resp);

    /*Zero length*/
    eRetVal = sdmmc_read_block_async(readBuffer, 0, 0, numberOfBlocks,sdmmc_cb);
    TEST_ASSERT_NOT_EQUAL(0, eRetVal);



}
TEST(TEST_SDMMC, test_socfpga_block_write_async)
{
    uint64_t writeBuffer[ 128 ];
    uint32_t blockSize = 512;
    uint32_t numberOfBlocks = 1;
    uint64_t secNum = 0;
    int32_t eRetVal;

    /*INFO: reinit the card after tx time out error*/
    sdmmc_init_card(&secNum);

    /*Null buffer*/
    eRetVal = sdmmc_write_block_async(NULL, 0, blockSize, numberOfBlocks,
            sdmmc_cb);
    TEST_ASSERT_NOT_EQUAL(0, eRetVal);

    /*Valid write*/
    eRetVal = sdmmc_write_block_async(writeBuffer, 0, blockSize, numberOfBlocks,
            sdmmc_cb);
    TEST_ASSERT_EQUAL(0, eRetVal);
    wait_xfer_done();
    TEST_ASSERT_EQUAL(0, xfer_resp);

    /*Zero length*/
    eRetVal =
            sdmmc_write_block_async(writeBuffer, 0, 0, numberOfBlocks,sdmmc_cb);
    TEST_ASSERT_NOT_EQUAL(0, eRetVal);

}

TEST( TEST_SDMMC,test_socfpga_xfer_async )
{
    uint8_t character = SYMBOL;
    int32_t RespCode = 0;
    uint64_t secNum = 0;

    /*INFO: reinit the card after tx time out error*/
    sdmmc_init_card(&secNum);

    for (int i = 0; i < ITERATION; i++)
    {
        character += 1;
        memset(writebuff, character, BLK_CNT * BLK_SIZE);

        RespCode = SD_WRITE_TEST_ASYNC( BLK_NUM, BLK_CNT, writebuff);
        TEST_ASSERT_EQUAL(0, RespCode);
        wait_xfer_done();
        TEST_ASSERT_EQUAL(0, xfer_resp);
        RespCode = SD_READ_TEST_ASYNC( BLK_NUM, BLK_CNT, readbuff);
        TEST_ASSERT_EQUAL(0, RespCode);
        wait_xfer_done();
        cache_force_invalidate((uint64_t*) readbuff, BLK_CNT * BLK_SIZE);
        TEST_ASSERT_EQUAL(0, xfer_resp);
        TEST_ASSERT_EQUAL(0, memcmp(writebuff, readbuff, BLK_CNT * BLK_SIZE));
    }
}
int sdmmc_test()
{
    xfer_lock = osal_semaphore_create(&osal_def_xfer);
    UNITY_BEGIN();
    TEST_SETUP(TEST_SDMMC);
    RUN_TEST_GROUP(TEST_SDMMC);
    return UNITY_END();
}


static void sdmmc_cb( int32_t state )
{
    xfer_resp = state;
    osal_semaphore_post(xfer_lock);
}


static void wait_xfer_done()
{
    osal_semaphore_wait(xfer_lock, OSAL_TIMEOUT_WAIT_FOREVER);
}
