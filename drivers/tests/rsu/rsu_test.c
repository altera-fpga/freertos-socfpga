/*
 * FreeRTOS Common IO V0.1.3
 * Copyright (C) 2020 Amazon.com, Inc. or its affiliates.  All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * http://aws.amazon.com/freertos
 * http://www.FreeRTOS.org
 */

/*******************************************************************************
 * @file rsu_test.c
 * @brief Functional Unit Test - RSU
 *******************************************************************************/
/* Test includes */
#include <stdio.h>
#include <string.h>

/* Test includes */
#include "unity.h"
#include "unity_fixture.h"

/* FreeRTOS includes */
#include "FreeRTOS.h"
#include "FreeRTOSConfig.h"
#include "semphr.h"
#include "task.h"
#include "rsu_test.h"


/*This test is conducted in an 
 * image with  6 slots
 */
#define VALID_SLOT_COUNT 6
#define INVALID_SLOT_NUM 10

char *fileName           = "/app2.rpd";
char *invalidFileName    = "/nop.rpd";

/**
 * @brief Setup function called before each test in this group is executed.
 */
TEST_SETUP( TEST_IOT_RSU )
{

}

TEST_TEAR_DOWN( TEST_IOT_RSU )
{
}

TEST_GROUP(TEST_IOT_RSU);

void rsu_test( void )
{
    UNITY_BEGIN();

    RUN_TEST_GROUP(TEST_IOT_RSU);

    UNITY_END();
}

TEST_GROUP_RUNNER( TEST_IOT_RSU )
{
    /*Fuzzing Tests*/
    RUN_TEST_CASE(TEST_IOT_RSU, AFQP_IotRSU_InitExitSuccess);
    RUN_TEST_CASE(TEST_IOT_RSU, AFQP_IotRSUGetSlotInfoFail);
    RUN_TEST_CASE(TEST_IOT_RSU, AFQP_IotRSUSlotLoadFail);
    RUN_TEST_CASE(TEST_IOT_RSU, AFQP_IotRSUGetSlotPriorityFail);
    RUN_TEST_CASE(TEST_IOT_RSU, AFQP_IotRSUSlotEraseFail);
    RUN_TEST_CASE(TEST_IOT_RSU, AFQP_IotRSUDcmfVersionFail);
    RUN_TEST_CASE(TEST_IOT_RSU, AFQP_IotRSUDcmfStatusFail);
    RUN_TEST_CASE(TEST_IOT_RSU, AFQP_IotRSUMaxRetryCounterFail);

    /*Functional Tests*/
    RUN_TEST_CASE(TEST_IOT_RSU, AFQP_IotRSUGetSlotCountSuccess);
    RUN_TEST_CASE(TEST_IOT_RSU, AFQP_IotRSUGetSlotInfoSuccess);
    RUN_TEST_CASE(TEST_IOT_RSU, AFQP_IotRSUGetSlotStatusSuccess);
    RUN_TEST_CASE(TEST_IOT_RSU, AFQP_IotRSUGetSlotPrioritySuccess);
    RUN_TEST_CASE(TEST_IOT_RSU, AFQP_IotRSUSlotEraseSuccess);
    RUN_TEST_CASE(TEST_IOT_RSU, AFQP_IotRSUSlotEraseProgramSuccess);
    RUN_TEST_CASE(TEST_IOT_RSU, AFQP_IotRSUSlotEraseProgramVerifySuccess);
    RUN_TEST_CASE(TEST_IOT_RSU, AFQP_IotSlotEraseProgramVerifyFail);
    RUN_TEST_CASE(TEST_IOT_RSU, AFQP_IotRSUDcmfVersionSuccess);
    RUN_TEST_CASE(TEST_IOT_RSU, AFQP_IotRSUDcmfStatusSuccess);
    RUN_TEST_CASE(TEST_IOT_RSU, AFQP_IotRSUMaxRetryCounterSuccess);
    RUN_TEST_CASE(TEST_IOT_RSU, AFQP_IotRSUProgramWithoutEraseFail);

    /*This test case will cause the board to reboot*/
    //RUN_TEST_CASE(TEST_IOT_RSU, AFQP_IotRSUEraseProgramVerifyLoadSuccess);
}

TEST(TEST_IOT_RSU, AFQP_IotRSU_InitExitSuccess)
{
    int lRet;
    printf("RSU tests started. Estimated time of completion: 2 Min\r\n");

    lRet  = iot_rsu_init("");
    TEST_ASSERT_EQUAL(0, lRet);

    iot_rsu_exit();
}

TEST(TEST_IOT_RSU, AFQP_IotRSUGetSlotCountSuccess)
{
    int lRet;

    lRet = iot_rsu_init("");
    TEST_ASSERT_EQUAL(0, lRet);

    /*This test is conducted in an image with 6 slots*/
    int count = iot_rsu_slot_count();
    TEST_ASSERT_EQUAL(VALID_SLOT_COUNT, count);

    iot_rsu_exit();
}

TEST(TEST_IOT_RSU, AFQP_IotRSUGetSlotStatusSuccess)
{
    int lRet;
    struct rsu_status_info info;
    memset(&info, 0, sizeof(struct rsu_status_info));

    lRet = iot_rsu_init("");
    TEST_ASSERT_EQUAL(0, lRet);

    /*This test is conducted in an image with 6 slots*/
    lRet = iot_rsu_status_log(&info);
    TEST_ASSERT_EQUAL(0, lRet);
    
    TEST_ASSERT_NOT_EQUAL(0, info.version);

    iot_rsu_exit();
}

TEST(TEST_IOT_RSU, AFQP_IotRSUGetSlotInfoSuccess)
{
    int lRet;
    struct rsu_slot_info info;

    lRet = iot_rsu_init("");
    TEST_ASSERT_EQUAL(0, lRet);

    for(int idx = 0; idx < VALID_SLOT_COUNT; idx++)
    {
	lRet = iot_rsu_slot_get_info(idx, &info);
	TEST_ASSERT_EQUAL(0, lRet);
	
    }
    iot_rsu_exit();
}

TEST(TEST_IOT_RSU, AFQP_IotRSUGetSlotInfoFail)
{
    int lRet;
    struct rsu_slot_info info;

    lRet = iot_rsu_init("");
    TEST_ASSERT_EQUAL(0, lRet);

    lRet = iot_rsu_slot_get_info(INVALID_SLOT_NUM, &info);
    TEST_ASSERT_NOT_EQUAL(0, lRet);

    iot_rsu_exit();
}

TEST(TEST_IOT_RSU, AFQP_IotRSUSlotEraseSuccess)
{
    int lRet;

    lRet = iot_rsu_init("");
    TEST_ASSERT_EQUAL(0, lRet);

    lRet = iot_rsu_slot_erase(1);
    TEST_ASSERT_EQUAL(0, lRet);

    iot_rsu_exit();
}

TEST(TEST_IOT_RSU, AFQP_IotRSUSlotEraseFail)
{
    int lRet;

    lRet = iot_rsu_init("");
    TEST_ASSERT_EQUAL(0, lRet);

    lRet = iot_rsu_slot_erase(INVALID_SLOT_NUM);
    TEST_ASSERT_NOT_EQUAL(0, lRet);

    iot_rsu_exit();
}

TEST(TEST_IOT_RSU, AFQP_IotRSUSlotProgramFail)
{
    int lRet;

    lRet = iot_rsu_init("");
    TEST_ASSERT_EQUAL(0, lRet);

    /*Test with invalid slot number*/
    lRet = iot_rsu_slot_program_file(10, fileName);
    TEST_ASSERT_NOT_EQUAL(0, lRet);

    /*Test with invalid file*/
    lRet = iot_rsu_slot_program_file(2, NULL);
    TEST_ASSERT_NOT_EQUAL(0, lRet);

    iot_rsu_exit();
}

TEST(TEST_IOT_RSU, AFQP_IotRSUSlotEraseProgramSuccess)
{
    int lRet;

    lRet = iot_rsu_init("");
    TEST_ASSERT_EQUAL(0, lRet);

    lRet = iot_rsu_slot_erase(1);
    TEST_ASSERT_EQUAL(0, lRet);

    lRet = iot_rsu_slot_program_file(1, fileName);
    TEST_ASSERT_EQUAL(0, lRet);

    iot_rsu_exit();
}

TEST(TEST_IOT_RSU, AFQP_IotRSUProgramWithoutEraseFail)
{
    int lRet;

    lRet = iot_rsu_init("");
    TEST_ASSERT_EQUAL(0, lRet);


    lRet = iot_rsu_slot_program_file(1, fileName);
    TEST_ASSERT_NOT_EQUAL(0, lRet);

    iot_rsu_exit();
}

TEST(TEST_IOT_RSU, AFQP_IotRSUSlotEraseProgramVerifySuccess)
{
    int lRet;

    lRet = iot_rsu_init("");
    TEST_ASSERT_EQUAL(0, lRet);

    lRet = iot_rsu_slot_erase(2);
    TEST_ASSERT_EQUAL(0, lRet);

    lRet = iot_rsu_slot_program_file(2, fileName);
    TEST_ASSERT_EQUAL(0, lRet);

    lRet = iot_rsu_slot_verify_file(2, fileName);
    TEST_ASSERT_EQUAL(0, lRet);

    iot_rsu_exit();
}

TEST(TEST_IOT_RSU, AFQP_IotSlotEraseProgramVerifyFail)
{
    int lRet;

    lRet = iot_rsu_init("");
    TEST_ASSERT_EQUAL(0, lRet);

    lRet = iot_rsu_slot_erase(2);
    TEST_ASSERT_EQUAL(0, lRet);

    lRet = iot_rsu_slot_program_file(2, fileName);
    TEST_ASSERT_EQUAL(0, lRet);

    lRet = iot_rsu_slot_verify_file(2, invalidFileName);
    TEST_ASSERT_NOT_EQUAL(0, lRet);

    iot_rsu_exit();
}

TEST(TEST_IOT_RSU, AFQP_IotRSUSlotLoadFail)
{
    int lRet;

    lRet = iot_rsu_init("");
    TEST_ASSERT_EQUAL(0, lRet);

    lRet = iot_rsu_slot_load_after_reboot(INVALID_SLOT_NUM);
    TEST_ASSERT_NOT_EQUAL(0, lRet);

    iot_rsu_exit();
}

TEST(TEST_IOT_RSU, AFQP_IotRSUGetSlotPrioritySuccess)
{
    int lRet;

    lRet = iot_rsu_init("");
    TEST_ASSERT_EQUAL(0, lRet);

    int priority = 0;
    priority = iot_rsu_slot_priority(0);
    TEST_ASSERT_NOT_EQUAL(0, priority);

    iot_rsu_exit();
}

TEST(TEST_IOT_RSU, AFQP_IotRSUGetSlotPriorityFail)
{
    int lRet;

    lRet = iot_rsu_init("");
    TEST_ASSERT_EQUAL(0, lRet);

    lRet = iot_rsu_slot_priority(INVALID_SLOT_NUM);
    TEST_ASSERT_NOT_EQUAL(0, lRet);

    iot_rsu_exit();
}

TEST(TEST_IOT_RSU, AFQP_IotRSUDcmfVersionSuccess)
{
    int lRet;
    uint32_t dcmf_ver[4] = { 0 };

    lRet = iot_rsu_init("");
    TEST_ASSERT_EQUAL(0, lRet);

    lRet = iot_rsu_dcmf_version(dcmf_ver);
    TEST_ASSERT_EQUAL(0, lRet);
    
    for(int i = 0; i < 4; i++)
    {
    	TEST_ASSERT_NOT_EQUAL(0, dcmf_ver[i]);
    }

    iot_rsu_exit();
}

TEST(TEST_IOT_RSU, AFQP_IotRSUDcmfVersionFail)
{
    int lRet;

    lRet = iot_rsu_init("");
    TEST_ASSERT_EQUAL(0, lRet);

    lRet = iot_rsu_dcmf_version(NULL);
    TEST_ASSERT_NOT_EQUAL(0, lRet);
    
    iot_rsu_exit();
}

TEST(TEST_IOT_RSU, AFQP_IotRSUDcmfStatusSuccess)
{
    int lRet;
    int dcmf_status[4] = { 0 };

    lRet = iot_rsu_init("");
    TEST_ASSERT_EQUAL(0, lRet);

    lRet = iot_rsu_dcmf_status(dcmf_status);
    TEST_ASSERT_EQUAL(0, lRet);

    for(int i = 0; i < 4; i++)
    {
    	TEST_ASSERT_NOT_EQUAL(1, dcmf_status[i]);
    }
    
    iot_rsu_exit();
}

TEST(TEST_IOT_RSU, AFQP_IotRSUDcmfStatusFail)
{
    int lRet;

    lRet = iot_rsu_init("");
    TEST_ASSERT_EQUAL(0, lRet);

    lRet = iot_rsu_dcmf_status(NULL);
    TEST_ASSERT_NOT_EQUAL(0, lRet);
    
    iot_rsu_exit();
}

TEST(TEST_IOT_RSU, AFQP_IotRSUMaxRetryCounterSuccess)
{
    int lRet;
    uint8_t count = -1;

    lRet = iot_rsu_init("");
    TEST_ASSERT_EQUAL(0, lRet);

    lRet = iot_rsu_max_retry(&count);
    TEST_ASSERT_EQUAL(0, lRet);
    
    iot_rsu_exit();
}

TEST(TEST_IOT_RSU, AFQP_IotRSUMaxRetryCounterFail)
{
    int lRet;

    lRet = iot_rsu_init("");
    TEST_ASSERT_EQUAL(0, lRet);

    lRet = iot_rsu_max_retry(NULL);
    TEST_ASSERT_NOT_EQUAL(0, lRet);
    
    iot_rsu_exit();
}

TEST(TEST_IOT_RSU, AFQP_IotRSUEraseProgramVerifyLoadSuccess)
{
    int lRet;

    lRet = iot_rsu_init("");
    TEST_ASSERT_EQUAL(0, lRet);

    lRet = iot_rsu_slot_erase(2);
    TEST_ASSERT_EQUAL(0, lRet);

    lRet = iot_rsu_slot_program_file(2, fileName);
    TEST_ASSERT_EQUAL(0, lRet);

    lRet = iot_rsu_slot_verify_file(2, fileName);
    TEST_ASSERT_EQUAL(0, lRet);

    lRet = iot_rsu_slot_load_after_reboot(2);
    TEST_ASSERT_EQUAL(0, lRet);
    
    iot_rsu_exit();
}

