/*
 * Copyright (c) 2024, Intel Corporation.
 *
 * SPDX-License-Identifier: MIT
 *
 * test implementation for SEU
 */

#include <stdbool.h>
#include "unity.h"
#include "unity_fixture.h"
#include "FreeRTOS.h"
#include "semphr.h"
#include "socfpga_seu.h"
#include "socfpga_mbox_client.h"

#define VALID_RAM_ID          0x1
#define VALID_ERR_TYPE        0x1
#define VALID_SEC_ADDR        0xFF
#define INVALID_RAM_ID        0xFF
#define INVALID_ERR_TYPE      0x3

#define VALID_CR0_SEL         0x0
#define VALID_CR1_SEL         0x0
#define INVALID_CR0_SEL       0x1F
#define INVALID_CR1_SEL       0x1F

#define VALID_INJ_CYCLE       0
#define INVALID_INJ_CYCLE     0x4

#define VALID_INJ_NUM         0x0
#define INVALID_INJ_NUM       0x1F

#define VALID_SEC_ADDR_SEU_1    0x5
#define VALID_SEC_ADDR_SEU_2    0xa


TEST_GROUP( TEST_SEU );

TEST_SETUP(TEST_SEU )
{
}

TEST_TEAR_DOWN(TEST_SEU )
{
}

TEST_GROUP_RUNNER( TEST_SEU )
{
    RUN_TEST_CASE(TEST_SEU, seu_insert_safe_err_fuzzing );
    RUN_TEST_CASE(TEST_SEU, seu_insert_safe_err_read );
    RUN_TEST_CASE(TEST_SEU, seu_insert_ecc_err );
}

TEST(TEST_SEU, seu_insert_ecc_err )
{
    uint8_t ret;

    TEST_ASSERT_EQUAL(seu_init(),0);

    ret=seu_insert_ecc_err(VALID_ERR_TYPE,VALID_RAM_ID,VALID_SEC_ADDR);
    TEST_ASSERT_EQUAL( ret, 0);
    ret=seu_insert_ecc_err(INVALID_ERR_TYPE,VALID_RAM_ID,VALID_SEC_ADDR);
    TEST_ASSERT_NOT_EQUAL( ret, 0);
    ret=seu_insert_ecc_err(VALID_ERR_TYPE,INVALID_RAM_ID,VALID_SEC_ADDR);
    TEST_ASSERT_NOT_EQUAL( ret, 0);
    TEST_ASSERT_EQUAL(seu_deinit(),0);
}


TEST(TEST_SEU, seu_insert_safe_err_read )
{
    uint8_t ret;
    seu_err_params_t err_params;
    read_err_data_t seu_err_data;

    TEST_ASSERT_EQUAL(seu_init(),0);

    err_params.sector_addr = VALID_SEC_ADDR_SEU_1;
    err_params.cram_sel0 = VALID_CR0_SEL;
    err_params.cram_sel1 = VALID_CR1_SEL;
    err_params.injection_cycle = VALID_INJ_CYCLE;
    err_params.no_of_injection =VALID_INJ_NUM;
    ret = seu_insert_safe_err(err_params);
    TEST_ASSERT_EQUAL( ret, 0);

    seu_err_data = seu_read_err();
    TEST_ASSERT_EQUAL(seu_err_data.err_cnt,VALID_INJ_NUM + 1);
    TEST_ASSERT_EQUAL(seu_err_data.sector_addr, VALID_SEC_ADDR_SEU_1);

    err_params.sector_addr = VALID_SEC_ADDR_SEU_2;
    err_params.cram_sel0 = 0x4;
    err_params.cram_sel1 = 0;
    err_params.injection_cycle = VALID_INJ_CYCLE;
    err_params.no_of_injection = 0x1;
    ret = seu_insert_safe_err(err_params);
    TEST_ASSERT_EQUAL( ret, 0);

    seu_err_data = seu_read_err();
    TEST_ASSERT_EQUAL(seu_err_data.err_cnt, 0x2 );
    TEST_ASSERT_EQUAL(seu_err_data.sector_addr,VALID_SEC_ADDR_SEU_2);
    TEST_ASSERT_EQUAL(seu_deinit(),0);
}

TEST( TEST_SEU, seu_insert_safe_err_fuzzing )
{
    uint8_t ret;
    seu_err_params_t err_params;

    err_params.sector_addr = VALID_SEC_ADDR_SEU_1;
    err_params.cram_sel0 = VALID_CR0_SEL;
    err_params.cram_sel1 = VALID_CR1_SEL;
    err_params.injection_cycle =  VALID_INJ_CYCLE;
    err_params.no_of_injection =  VALID_INJ_NUM;
    ret = seu_insert_safe_err(err_params);
    TEST_ASSERT_NOT_EQUAL( ret, 0);

    TEST_ASSERT_EQUAL(seu_init(),0);

    err_params.sector_addr = VALID_SEC_ADDR_SEU_1;
    err_params.cram_sel0 = VALID_CR0_SEL;
    err_params.cram_sel1 = VALID_CR1_SEL;
    err_params.injection_cycle =  VALID_INJ_CYCLE;
    err_params.no_of_injection =  VALID_INJ_NUM;
    ret = seu_insert_safe_err(err_params);
    TEST_ASSERT_EQUAL( ret, 0);

    err_params.sector_addr = VALID_SEC_ADDR_SEU_1;
    err_params.cram_sel0 =INVALID_CR0_SEL;
    err_params.cram_sel1 =VALID_CR1_SEL;
    err_params.injection_cycle =  VALID_INJ_CYCLE;
    err_params.no_of_injection =  VALID_INJ_NUM;
    ret = seu_insert_safe_err(err_params);
    TEST_ASSERT_NOT_EQUAL( ret, 0);

    err_params.sector_addr = VALID_SEC_ADDR_SEU_1;
    err_params.cram_sel0 = VALID_CR0_SEL;
    err_params.cram_sel1 = INVALID_CR1_SEL;
    err_params.injection_cycle =  VALID_INJ_CYCLE;
    err_params.no_of_injection = VALID_INJ_NUM;
    ret = seu_insert_safe_err(err_params);
    TEST_ASSERT_NOT_EQUAL( ret, 0);

    err_params.sector_addr = VALID_SEC_ADDR_SEU_1;
    err_params.cram_sel0 =  VALID_CR0_SEL;
    err_params.cram_sel1 = VALID_CR1_SEL;
    err_params.injection_cycle = INVALID_INJ_CYCLE;
    err_params.no_of_injection = VALID_INJ_NUM;
    ret = seu_insert_safe_err(err_params);
    TEST_ASSERT_NOT_EQUAL( ret, 0);

    err_params.sector_addr =  VALID_SEC_ADDR_SEU_1;
    err_params.cram_sel0 = VALID_CR0_SEL;
    err_params.cram_sel1 = VALID_CR1_SEL;
    err_params.injection_cycle = VALID_INJ_CYCLE;
    err_params.no_of_injection = INVALID_INJ_NUM;
    ret = seu_insert_safe_err(err_params);
    TEST_ASSERT_NOT_EQUAL( ret, 0);
    TEST_ASSERT_EQUAL(seu_deinit(),0);
}

void seu_test()
{
    UNITY_BEGIN();

    RUN_TEST_GROUP(TEST_SEU);

    UNITY_END();
}


