/*
 * SPDX-FileCopyrightText: Copyright (C) 2025 Altera Corporation
 *
 * SPDX-License-Identifier: MIT-0
 *
 * Test implementation for SDM mailbox
 */

/* Test includes */
#include "socfpga_mbox_client.h"
#include "unity.h"
#include "unity_fixture.h"

/* Driver includes */
#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"
#include "osal.h"
#include "osal_log.h"

#define MBOX_HWMON_READTEMP 0x420000E8U
#define MBOX_TIMEOUT        1000U
/*-----------------------------------------------------------*/

/* Globals values which can be overwritten by the test
 * framework invoking these tests */
/*-----------------------------------------------------------*/
int count;
sdm_client_handle mbox_handle;

/*-----------------------------------------------------------*/
/** Static globals */
/*-----------------------------------------------------------*/
static SemaphoreHandle_t test_sem = NULL;

/*-----------------------------------------------------------*/

/* Define Test Group. */
TEST_GROUP(TEST_MBOX);

/*-----------------------------------------------------------*/

/**
 * @brief Setup function called before each test in this group is executed.
 */
TEST_SETUP( TEST_MBOX )
{
}

/*-----------------------------------------------------------*/

/**
 * @brief Tear down function called after each test in this group is executed.
 */
TEST_TEAR_DOWN( TEST_MBOX )
{
}

/*-----------------------------------------------------------*/
static void mbox_callback( uint64_t *resp_values )
{
    (void)resp_values;
    osal_semaphore_post(test_sem);
}
static void multiple_req_callback( uint64_t *resp_values )
{
    if( resp_values[0] == 0 )
    {
        count++;
    }
}

/*-----------------------------------------------------------*/

/**
 * @brief Function to define which tests to execute as part of this group.
 */
TEST_GROUP_RUNNER( TEST_MBOX )
{
    test_sem = osal_semaphore_create(NULL);
    TEST_ASSERT_NOT_EQUAL(NULL, test_sem);

    RUN_TEST_CASE(TEST_MBOX, AFQP_Mbox_OpenCloseClient);

    RUN_TEST_CASE(TEST_MBOX, AFQP_Mbox_SetCallBack);
    RUN_TEST_CASE( TEST_MBOX, AFQP_SipSvcSendCommand);
    RUN_TEST_CASE(TEST_MBOX, AFQP_MboxSendCommand);
    RUN_TEST_CASE(TEST_MBOX, AFQP_MboxMultipleReqs);

    RUN_TEST_CASE(TEST_MBOX, AFQP_Mbox_SipSvcSendCommandFuzzing);
    RUN_TEST_CASE(TEST_MBOX, AFQP_Mbox_MboxSendCommandFuzzing);
    RUN_TEST_CASE(TEST_MBOX, AFQP_Mbox_SetCallbackFuzzing);

    RUN_TEST_CASE(TEST_MBOX, AFQP_Mbox_OpenFuzzing);
    RUN_TEST_CASE(TEST_MBOX, AFQP_Mbox_CloseFuzzing);
}

/*-----------------------------------------------------------*/

/*Main function to invoke the tests*/
void mbox_test( void )
{
    mbox_init();
    UNITY_BEGIN();
    TEST_SETUP(TEST_MBOX);
    RUN_TEST_GROUP(TEST_MBOX);
    UNITY_END();
    mbox_deinit();
}

/*-----------------------------------------------------------*/

/**
 * @brief Test Function to test mbox_open_client and mbox_close_client.
 */
TEST( TEST_MBOX, AFQP_Mbox_OpenCloseClient )
{
    sdm_client_handle client_handle;
    int ret;

    client_handle = mbox_open_client();
    TEST_ASSERT_NOT_EQUAL(NULL, client_handle);

    ret = mbox_close_client(client_handle);
    TEST_ASSERT_EQUAL(0, ret);

}

/*-----------------------------------------------------------*/
/**
 * @brief Test Function to test mbox_set_callback.
 */
TEST( TEST_MBOX, AFQP_Mbox_SetCallBack )
{
    sdm_client_handle client_handle;
    int ret;

    client_handle = mbox_open_client();
    TEST_ASSERT_NOT_EQUAL(NULL, client_handle);

    ret = mbox_set_callback(client_handle, mbox_callback);
    TEST_ASSERT_EQUAL(0, ret);

    ret = mbox_close_client(client_handle);
    TEST_ASSERT_EQUAL(0, ret);

}

/*-----------------------------------------------------------*/

/**
 * @brief Test Function to test sip_svc_send
 */
TEST( TEST_MBOX, AFQP_SipSvcSendCommand )
{
    int ret;
    uint64_t mbox_args, mbox_resp[2];
    mbox_handle = mbox_open_client();
    
    TEST_ASSERT_NOT_EQUAL(NULL, mbox_handle);

    if (TEST_PROTECT())
    {
        ret = mbox_set_callback(mbox_handle, mbox_callback);
        TEST_ASSERT_EQUAL(0, ret);

        mbox_args = 0x1;
        ret = sip_svc_send(mbox_handle, MBOX_HWMON_READTEMP, &mbox_args, 
                sizeof(mbox_args), mbox_resp, sizeof(mbox_resp));
        TEST_ASSERT_EQUAL(0, ret);
        if(TEST_PROTECT())
        {
            ret = osal_semaphore_wait(test_sem, MBOX_TIMEOUT);
            if ((ret == pdTRUE))
            {
                /* mailbox error in resp[0] and mailbox response in resp[1] */
                TEST_ASSERT_EQUAL(0, mbox_resp[0]);
                TEST_ASSERT_NOT_EQUAL(0, mbox_resp[1]);
            }
        }
    }
    ret = mbox_close_client(mbox_handle);
    TEST_ASSERT_EQUAL(0, ret);
}

/*-----------------------------------------------------------*/

/**
 * @brief Test Function to test mbox_send_command
 */
TEST( TEST_MBOX, AFQP_MboxSendCommand )
{
    int ret;
    uint64_t smc_resp[2];
    uint32_t  *mbox_args, *mbox_resp;
    mbox_handle = mbox_open_client();
    
    TEST_ASSERT_NOT_EQUAL(NULL, mbox_handle);

    if (TEST_PROTECT())
    {
        ret = mbox_set_callback(mbox_handle, mbox_callback);
        TEST_ASSERT_EQUAL(0, ret);

        mbox_args = pvPortMalloc(sizeof(uint32_t));
        TEST_ASSERT_NOT_EQUAL(NULL, mbox_args);
        mbox_resp = pvPortMalloc(sizeof(uint32_t));
        TEST_ASSERT_NOT_EQUAL(NULL, mbox_resp);
        if(TEST_PROTECT())
        {
            *mbox_args = 0x1;
            *mbox_resp = 0;
            ret = mbox_send_command(mbox_handle, 0x19, mbox_args, 
                    sizeof(uint32_t), mbox_resp, sizeof(mbox_resp),
                    smc_resp, sizeof(smc_resp));
            TEST_ASSERT_EQUAL(0, ret);
            if(TEST_PROTECT())
            {
                ret = osal_semaphore_wait(test_sem, MBOX_TIMEOUT);
                if ((ret == pdTRUE))
                {
                    /* mailbox error in resp[0] and mailbox response in resp[1] */
                    TEST_ASSERT_EQUAL(0, smc_resp[0]);
                    TEST_ASSERT_EQUAL(4, smc_resp[1]);
                    TEST_ASSERT_NOT_EQUAL(0, *mbox_resp);
                }
            }
        }
    }
    ret = mbox_close_client(mbox_handle);
    TEST_ASSERT_EQUAL(0, ret);
}

/*-----------------------------------------------------------*/

/**
 * @brief Test Function to test multiple mailbox requests
 */
TEST( TEST_MBOX, AFQP_MboxMultipleReqs )
{
    int i, ret;
    uint64_t mbox_args, mbox_resp[2];
    /* Open timer to initialize hardware. */
    mbox_handle = mbox_open_client();
    TEST_ASSERT_NOT_EQUAL(NULL, mbox_handle);
    
    count = 0;
    mbox_args = 0x1;
    if (TEST_PROTECT())
    {
        count = 0;
        /* Set up the callback */
        mbox_set_callback(mbox_handle, multiple_req_callback);

        for (i = 0; i < 6; i++)
        {
            ret = sip_svc_send(mbox_handle, MBOX_HWMON_READTEMP, &mbox_args, 
                sizeof(mbox_args), mbox_resp, sizeof(mbox_resp));
            TEST_ASSERT_EQUAL(0, ret);
            osal_delay_ms(10);
        }
        
        TEST_ASSERT_EQUAL(6, count);

        ret = mbox_close_client(mbox_handle);
        TEST_ASSERT_EQUAL(0, ret);
    }
}

/*-----------------------------------------------------------*/
/**
 * @brief Test Function to test fuzzing mbox_set_callback.
 */
TEST( TEST_MBOX, AFQP_Mbox_SetCallbackFuzzing )
{
    int ret;

    ret = mbox_set_callback(NULL, mbox_callback);
    TEST_ASSERT_NOT_EQUAL(0, ret);
}

/*-----------------------------------------------------------*/

/**
 * @brief Test Function to test fuzzing of sip_svc_send
 */
TEST( TEST_MBOX, AFQP_Mbox_SipSvcSendCommandFuzzing )
{
    int ret;
    uint64_t mbox_args, mbox_resp[2];
    sdm_client_handle mbox_handle;

    /* Passing handle as NULL */
    ret = sip_svc_send(NULL, MBOX_HWMON_READTEMP, &mbox_args, 
            sizeof(mbox_args), mbox_resp, sizeof(mbox_resp));
    TEST_ASSERT_NOT_EQUAL(0, ret);

    mbox_handle = mbox_open_client();
    ret = sip_svc_send(mbox_handle, MBOX_HWMON_READTEMP, NULL, 
        4, mbox_resp, sizeof(mbox_resp));
    TEST_ASSERT_NOT_EQUAL(0, ret); 

    ret = sip_svc_send(mbox_handle, MBOX_HWMON_READTEMP, &mbox_args, 
        0, mbox_resp, sizeof(mbox_resp));
    TEST_ASSERT_NOT_EQUAL(0, ret); 

    mbox_close_client(mbox_handle);
    /*Both args and resp can be NULL, so fuzzing test is concluded */
}

/*-----------------------------------------------------------*/

/**
 * @brief Test Function to test fuzzing of mbox_send_command
 */
TEST( TEST_MBOX, AFQP_Mbox_MboxSendCommandFuzzing )
{
    int ret;
    uint32_t mbox_arg = 0x1, mbox_resp;
    uint64_t smc_resp[2];
    sdm_client_handle mbox_handle;
    
    /* Passing handle as NULL */
    ret = mbox_send_command(NULL, 0x19, &mbox_arg, 
            sizeof(mbox_arg), &mbox_resp, sizeof(mbox_resp),
            smc_resp, sizeof(smc_resp));
    TEST_ASSERT_NOT_EQUAL(0, ret);

    /* invalid argument size */
    mbox_handle = mbox_open_client();
    ret = mbox_send_command(mbox_handle, 0x19, &mbox_arg, 
            2, &mbox_resp, sizeof(mbox_resp),
            smc_resp, sizeof(smc_resp));
    TEST_ASSERT_NOT_EQUAL(0, ret);
    mbox_close_client(mbox_handle);
}
/*-----------------------------------------------------------*/

/**
 * @brief Test Function to test fuzzing of mbox_open_client
 */
TEST( TEST_MBOX, AFQP_Mbox_OpenFuzzing )
{
    sdm_client_handle test_handles[ 17 ];
    int ret, i;
    for (i = 2; i < 16; i++)
    {
        test_handles[ i ] = mbox_open_client();
        TEST_ASSERT_NOT_EQUAL(NULL, test_handles[ i ]);
    }
    test_handles[ i ] = mbox_open_client();
    TEST_ASSERT_EQUAL(NULL, test_handles[ i ]);
    for (i = 2; i < 16; i++)
    {
        ret = mbox_close_client(test_handles[ i ]);
        TEST_ASSERT_EQUAL(0, ret);
    }
}

/*-----------------------------------------------------------*/

/**
 * @brief Test Function to test fuzzing of mbox_close_client
 */
TEST( TEST_MBOX, AFQP_Mbox_CloseFuzzing )
{
    sdm_client_handle test_handle = NULL;

    int ret = mbox_close_client(test_handle);
    TEST_ASSERT_NOT_EQUAL(0, ret);
}

/*-----------------------------------------------------------*/

