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
#include "unity.h"
#include "unity_fixture.h"
#include "FreeRTOS.h"
#include "socfpga_fpga_manager.h"
#include "socfpga_mmc.h"
#include "socfpga_freeze_ip.h"
#include "osal_log.h"

#define CMD_RET_SUCCESS    (1)
#define CMD_RET_FAIL       (-1)

TEST_GROUP(TEST_FPGA_MNGR);

TEST_SETUP( TEST_FPGA_MNGR )
{
}

TEST_TEAR_DOWN( TEST_FPGA_MNGR )
{
}

TEST_GROUP_RUNNER( TEST_FPGA_MNGR )
{
    RUN_TEST_CASE(TEST_FPGA_MNGR, test_fpga_mngr_configure)
    RUN_TEST_CASE(TEST_FPGA_MNGR, test_fpga_mngr_partial_reconfiguration)
}

TEST( TEST_FPGA_MNGR, test_fpga_mngr_configure )
{
    const char *rbf_file = "/ghrd.core.rbf";
    const char *dummy_file = "/ghrd.core1.rbf";          /* invalid rbf file name, rbf should be of different version from the jic used */
    uint32_t file_size;
    uint8_t *rbf_ptr;
    int ret;

    /*Valid bitstream file */
    rbf_ptr = mmc_read_file(SOURCE_SDMMC, rbf_file, &file_size);
    if (rbf_ptr != NULL)
    {
        ret = load_fpga_bitstream(rbf_ptr, file_size);
        TEST_ASSERT_EQUAL(CMD_RET_SUCCESS, ret);
        vPortFree(rbf_ptr);
    }

    /*Invalid bitstream file */
    rbf_ptr = mmc_read_file(SOURCE_SDMMC, dummy_file, &file_size);
    if (rbf_ptr != NULL)
    {
        ret = load_fpga_bitstream(rbf_ptr, file_size);
        TEST_ASSERT_EQUAL(CMD_RET_FAIL, ret);
        vPortFree(rbf_ptr);
    }
}

TEST( TEST_FPGA_MNGR, test_fpga_mngr_partial_reconfiguration)
{
    const char *rbf_base = "/ghrd.core.rbf";
    const char *persona1 = "/persona1.rbf";
    uint8_t *rbf_ptr;
    uint32_t file_size;
    int ret;

    /*Valid bitstream file */
    rbf_ptr = mmc_read_file(SOURCE_SDMMC, rbf_base, &file_size);

    if (rbf_ptr != NULL)
    {
        ret = load_fpga_bitstream(rbf_ptr, file_size);
        TEST_ASSERT_EQUAL(CMD_RET_SUCCESS, ret);
        vPortFree(rbf_ptr);

        ret = do_freeze_pr_region();
        TEST_ASSERT_EQUAL(ret, 0);

        rbf_ptr = mmc_read_file(SOURCE_SDMMC, persona1, &file_size);
        if (rbf_ptr != NULL)
        {
            ret = do_unfreeze_pr_region();
            TEST_ASSERT_EQUAL(ret, 0);
            vPortFree(rbf_ptr);
        }

    }
}

int fpga_mngr_test(void)
{
    UNITY_BEGIN();
    TEST_SETUP(TEST_FPGA_MNGR);
    RUN_TEST_GROUP(TEST_FPGA_MNGR);
    return UNITY_END();
}
