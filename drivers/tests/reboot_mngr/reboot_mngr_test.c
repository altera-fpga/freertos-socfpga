#include <stdio.h>

/* FreeRTOS includes */
#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"

/* Test includes */
#include "reboot_mngr_test.h"
#include "unity.h"
#include "unity_fixture.h"

#define EVENT_SELECT 1

#if EVENT_SELECT
#define REBOOT_EVENT IOT_REBOOT_WARM_REBOOT
#else
#define REBOOT_EVENT IOT_REBOOT_COLD_REBOOT
#endif

#define INVALID_REBOOT_EVENT 10

TEST_GROUP(TEST_IOT_REBOOT_MANAGER);

TEST_SETUP( TEST_IOT_REBOOT_MANAGER )
{
}

TEST_TEAR_DOWN( TEST_IOT_REBOOT_MANAGER )
{
}

static void test_reboot_callback( void *param )
{
    (void) param;
    UNITY_END();
#if EVENT_SELECT
    printf("\n\rInitiating WARM REBOOT");
#else
    printf("\n\rInitiating COLD REBOOT");
#endif
}

static

TEST_GROUP_RUNNER( TEST_IOT_REBOOT_MANAGER )
{
    RUN_TEST_CASE(TEST_IOT_REBOOT_MANAGER, SetCallbackFuzzing);
    RUN_TEST_CASE(TEST_IOT_REBOOT_MANAGER, RebootFuzzing);
    RUN_TEST_CASE(TEST_IOT_REBOOT_MANAGER, CallbackAssistedReboot);
}

void reboot_test()
{
    UNITY_BEGIN();
    RUN_TEST_GROUP(TEST_IOT_REBOOT_MANAGER);
    UNITY_END();
}

TEST(TEST_IOT_REBOOT_MANAGER, SetCallbackFuzzing)
{
    int32_t result = 0;

    /*set valid callback */
    result = iot_reboot_set_callback(test_reboot_callback, REBOOT_EVENT);
    TEST_ASSERT_EQUAL(IOT_REBOOT_SUCCESS, result);

    /*set null callback */
    result = iot_reboot_set_callback(NULL, REBOOT_EVENT);
    TEST_ASSERT_EQUAL(IOT_REBOOT_SUCCESS, result);

    /*set invalid event for callback*/
    result = iot_reboot_set_callback(test_reboot_callback,
    INVALID_REBOOT_EVENT);
    TEST_ASSERT_EQUAL(IOT_REBOOT_FAILURE, result);
}

TEST( TEST_IOT_REBOOT_MANAGER, RebootFuzzing )
{
    int32_t result = 0;

    /* set invalid reboot event */
    result = iot_reboot_do_reboot(INVALID_REBOOT_EVENT);
    TEST_ASSERT_EQUAL(IOT_REBOOT_FAILURE, result);
}

TEST( TEST_IOT_REBOOT_MANAGER, CallbackAssistedReboot )
{
    int32_t result = 0;

    result = iot_reboot_set_callback(test_reboot_callback, REBOOT_EVENT);
    TEST_ASSERT_EQUAL(IOT_REBOOT_SUCCESS, result);

    result = iot_reboot_do_reboot(REBOOT_EVENT);
    TEST_ASSERT_EQUAL(IOT_REBOOT_SUCCESS, result);
}
