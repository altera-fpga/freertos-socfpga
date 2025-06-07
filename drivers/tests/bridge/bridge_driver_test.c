/*
 * Copyright (c) 2024, Intel Corporation.
 *
 * SPDX-License-Identifier: MIT
 *
 * test for agilex5 bridge driver
 */
#include <assert.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include "unity.h"
#include "unity_fixture.h"
#include "FreeRTOS.h"
#include "task.h"
#include "osal_log.h"
#include "socfpga_bridge.h"
#include "socfpga_defines.h"

TEST_GROUP(TEST_BRIDGE);

TEST_SETUP( TEST_BRIDGE )
{
}

TEST_TEAR_DOWN( TEST_BRIDGE )
{
}

TEST_GROUP_RUNNER( TEST_BRIDGE )
{
    RUN_TEST_CASE( TEST_BRIDGE, test_hps2fpga_bridge)
    RUN_TEST_CASE( TEST_BRIDGE, test_lwhsp2fpga_bridge)
    RUN_TEST_CASE(TEST_BRIDGE, test_fpga2hps_bridge);
    RUN_TEST_CASE(TEST_BRIDGE, test_fpga2sdram_bridge);
}

TEST( TEST_BRIDGE, test_hps2fpga_bridge )
{
    int ret;

    /* Enable the bridge consecutively */
    ret = enable_hps2fpga_bridge();
    TEST_ASSERT_EQUAL(RET_SUCCESS, ret);

    ret = enable_hps2fpga_bridge();
    TEST_ASSERT_EQUAL(RET_SUCCESS, ret);

    ret = disable_hps2fpga_bridge();
    TEST_ASSERT_EQUAL(RET_SUCCESS, ret);

    /* Enable the bridge just after the disable command */
    ret = enable_hps2fpga_bridge();
    TEST_ASSERT_EQUAL(RET_SUCCESS, ret);
}

TEST( TEST_BRIDGE, test_lwhsp2fpga_bridge)
{
    int ret;

    /* Enable the bridge consecutively */
    ret = enable_lwhps2fpga_bridge();
    TEST_ASSERT_EQUAL(RET_SUCCESS, ret);

    ret = enable_lwhps2fpga_bridge();
    TEST_ASSERT_EQUAL(RET_SUCCESS, ret);

    ret = disable_lwhps2fpga_bridge();
    TEST_ASSERT_EQUAL(RET_SUCCESS, ret);

    /* Enable the bridge just after the disable command */
    ret = enable_lwhps2fpga_bridge();
    TEST_ASSERT_EQUAL(RET_SUCCESS, ret);

}

TEST( TEST_BRIDGE, test_fpga2hps_bridge)
{
    int ret;

    /* Enable the bridge consecutively */
    ret = enable_fpga2hps_bridge();
    TEST_ASSERT_EQUAL(RET_SUCCESS, ret);

    ret = enable_fpga2hps_bridge();
    TEST_ASSERT_EQUAL(RET_SUCCESS, ret);

    ret = disable_fpga2hps_bridge();
    TEST_ASSERT_EQUAL(RET_SUCCESS, ret);

    /* Enable the bridge just after the disable command */
    ret = enable_fpga2hps_bridge();
    TEST_ASSERT_EQUAL(RET_SUCCESS, ret);

}

TEST( TEST_BRIDGE, test_fpga2sdram_bridge)
{
    int ret;

    /* Enable the bridge consecutively */
    ret = enable_fpga2sdram_bridge();
    TEST_ASSERT_EQUAL(RET_SUCCESS, ret);

    ret = enable_fpga2sdram_bridge();
    TEST_ASSERT_EQUAL(RET_SUCCESS, ret);

    ret = disable_fpga2sdram_bridge();
    TEST_ASSERT_EQUAL(RET_SUCCESS, ret);

    /* Enable the bridge just after the disable command */
    ret = enable_fpga2sdram_bridge();
    TEST_ASSERT_EQUAL(RET_SUCCESS, ret);

}

int bridge_test(void)
{
    UNITY_BEGIN();
    TEST_SETUP(TEST_BRIDGE);
    RUN_TEST_GROUP(TEST_BRIDGE);
    return UNITY_END();
}
