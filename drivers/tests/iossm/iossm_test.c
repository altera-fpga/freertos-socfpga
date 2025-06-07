/*
 * Copyright (c) 2024, Intel Corporation.
 *
 * SPDX-License-Identifier: MIT
 *
 * test implementation for iossm
 */

#include <stdbool.h>
#include "unity.h"
#include "unity_fixture.h"
#include "socfpga_iossm.h"
#include "FreeRTOS.h"
#include "semphr.h"
#include "iossm_test.h"

TEST_GROUP( TEST_IOSSM );

TEST_SETUP(TEST_IOSSM )
{
}

TEST_TEAR_DOWN(TEST_IOSSM )
{
}

TEST_GROUP_RUNNER( TEST_IOSSM )
{
    RUN_TEST_CASE(TEST_IOSSM, iossm_open_close );
    RUN_TEST_CASE( TEST_IOSSM, ecc_inject_error );
}

void iossm_test()
{
    UNITY_BEGIN();

    RUN_TEST_GROUP(TEST_IOSSM);

    UNITY_END();
}

TEST( TEST_IOSSM, iossm_open_close )
{
    TEST_ASSERT_EQUAL(NULL,iossm_open(3));
    TEST_ASSERT_NOT_EQUAL(NULL,iossm_open(IOSSM_INSTANCE_0));
    TEST_ASSERT_EQUAL(NULL,iossm_open(IOSSM_INSTANCE_0));
    TEST_ASSERT_NOT_EQUAL(0,iossm_close(3));
    TEST_ASSERT_EQUAL(0,iossm_close(IOSSM_INSTANCE_0));
    TEST_ASSERT_NOT_EQUAL(0,iossm_close(IOSSM_INSTANCE_0));
}

TEST( TEST_IOSSM, ecc_inject_error )
{
    xiossm_context *handle = iossm_open(IOSSM_INSTANCE_0);
    TEST_ASSERT_EQUAL(IOSSM_OP_SUCCESS,iossm_close(IOSSM_INSTANCE_0));
    TEST_ASSERT_NOT_EQUAL(0, iossm_inject_sbit_err(handle));
    TEST_ASSERT_NOT_EQUAL(0, iossm_inject_dbit_err(handle));
    TEST_ASSERT_NOT_EQUAL(0,
            iossm_ack_int(handle,IOSSM_ECC_CRCT_EVENT_DET));
    handle = iossm_open(IOSSM_INSTANCE_0);
    TEST_ASSERT_EQUAL(IOSSM_OP_SUCCESS, iossm_inject_sbit_err(handle));
    TEST_ASSERT_EQUAL(IOSSM_OP_SUCCESS,
            iossm_ack_int(handle,IOSSM_ECC_CRCT_EVENT_DET));
    TEST_ASSERT_EQUAL(IOSSM_OP_SUCCESS, iossm_inject_dbit_err(handle));
    TEST_ASSERT_EQUAL(IOSSM_OP_SUCCESS,
            iossm_ack_int(handle,IOSSM_ECC_UNCRCT_EVENT_DET));
    TEST_ASSERT_NOT_EQUAL(0, iossm_read_ecc_status(handle));
    TEST_ASSERT_NOT_EQUAL(0, iossm_inject_sbit_err(NULL));
    TEST_ASSERT_NOT_EQUAL(0, iossm_inject_dbit_err(NULL));
    TEST_ASSERT_NOT_EQUAL(0,
            iossm_ack_int(NULL,IOSSM_ECC_CRCT_EVENT_DET));
    TEST_ASSERT_EQUAL(0, iossm_read_ecc_status(NULL));
}
