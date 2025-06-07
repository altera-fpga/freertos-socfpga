/* Standard includes */
#include <string.h>
#include <time.h>
#include <errno.h>

/* Test includes. */
#include "unity_fixture.h"
#include "unity.h"

/* Driver model includes */
#include "FreeRTOS.h"
#include "FreeRTOSConfig.h"
#include "semphr.h"
#include "qspi_test.h"
#include "socfpga_cache.h"

#define testIotQspi_MAX_TIMEOUT      pdMS_TO_TICKS( 10000 )
#define READ_START_ADDRESS           0x05000000
#define READ_START_ADDRESS_ASYNC     0x05030000U
#define BOUNDARY_START_ADDRESS       0x05FFFF00U
#define tetsIotQspi_DATASIZE         256
#define QSPI_LOWER_DATASIZE          64
#define MAX_QSPI_DEV                 4
#define tetsIotQspi_DATASIZE4k       4096
#define QSPI_LARGE_DATASIZE          1024

static SemaphoreHandle_t xtestIotQspiSemaphore = NULL;
uint32_t ucAssistedTestIotFlashSlaveAddr = 0;
uint32_t ucAssistedTestIotUnsupFlashSlaveAddr = 1;
uint8_t uctestIotQspiWrFlash[ tetsIotQspi_DATASIZE ] =
{ 0 };
uint8_t uctestIotQspiRdFlash[ tetsIotQspi_DATASIZE ] =
{ 0 };
uint8_t uctestIotQspiWrFlash4k[ tetsIotQspi_DATASIZE4k ] =
{ 0 };
uint8_t uctestIotQspiRdFlash4k[ tetsIotQspi_DATASIZE4k ] =
{ 0 };

uint8_t qspi_wr_buf_low[QSPI_LOWER_DATASIZE] = {0};
uint8_t qspi_rd_buf_low[QSPI_LOWER_DATASIZE] = {0};


IotFlashHandle_t flash_handle = NULL;
IotFlashHandle_t flash_handle_1 = NULL;

TEST_SETUP( TEST_IOT_QSPI )
{
    if ( xtestIotQspiSemaphore == NULL )
    {
        xtestIotQspiSemaphore = xSemaphoreCreateBinary();
        TEST_ASSERT_NOT_EQUAL(NULL, xtestIotQspiSemaphore);
    }
}

TEST_TEAR_DOWN( TEST_IOT_QSPI )
{
}

void prvQspiCallback( uint32_t xOpStatus, void *pvParam )
{
    /* Disable unused parameter warning. */
    (void) pvParam;

    if ( xOpStatus == 0 )
    {
        xSemaphoreGiveFromISR(xtestIotQspiSemaphore, NULL);
    }
}

void qspi_write_callback( uint32_t xOpStatus, void *pvUserContext )
{
    (void) xOpStatus;
    BaseType_t *pxIsWriteComplete = (BaseType_t*) pvUserContext;
    *pxIsWriteComplete = pdTRUE;
}

void qspi_read_callback( uint32_t xOpStatus, void *pvUserContext )
{
    (void) xOpStatus;
    BaseType_t *pxIsReadComplete = (BaseType_t*) pvUserContext;
    *pxIsReadComplete = pdTRUE;
}

void dummy_callback( uint32_t xOpStat, void *pvUserContext )
{
    /* Dummy callback for fuzzing tests */
    (void) pvUserContext;
    (void) xOpStat;
}

static void fill_buf( uint8_t *buf, uint32_t nbytes, uint32_t startNum,
        int pattern_no )
{
    switch (pattern_no)
    {
    case 1:
        for ( uint32_t i = 0; i < nbytes; i++ )
        {
            *(buf + i) = (startNum++ % 256);
        }
        break;

    case 2:
        for ( uint32_t i = 0; i < nbytes; i++ )
        {
            *(buf + i) = 0x00;
        }
        break;
    case 3:
        for ( uint32_t i = 0; i < nbytes; i++ )
        {
            *(buf + i) = 0xFF;
        }
        break;
    case 4:
        for ( uint32_t i = 0; i < nbytes; i++ )
        {
            *(buf + i) = 0xAA;
        }
        break;
    }
}

TEST_GROUP(TEST_IOT_QSPI);

void qspi_test( void )
{
    UNITY_BEGIN();

    RUN_TEST_GROUP(TEST_IOT_QSPI);

    UNITY_END();
}

TEST_GROUP_RUNNER( TEST_IOT_QSPI )
{
    /* data transfer tests */
    RUN_TEST_CASE(TEST_IOT_QSPI, AFQP_IotQSPI_FlashWriteReadWriteReadSyncVerify);
    RUN_TEST_CASE(TEST_IOT_QSPI, AFQP_IotQSPI_FlashWriteReadSyncVerify);
    RUN_TEST_CASE(TEST_IOT_QSPI, AFQP_IotQSPI_FlashWriteReadAsyncVerify);
    RUN_TEST_CASE(TEST_IOT_QSPI, AFQP_IotQSPI_FlashWriteReadReadSyncVerify);
    RUN_TEST_CASE(TEST_IOT_QSPI, AFQP_IotQSPI_FlashWriteWriteReadSyncVerify);
    RUN_TEST_CASE(TEST_IOT_QSPI, AFQP_IotQSPI_FlashWriteReadBoundaryVerify);
    RUN_TEST_CASE(TEST_IOT_QSPI, AFQP_IotQSPI_FlashWriteReadPatternsVerify);
    RUN_TEST_CASE(TEST_IOT_QSPI, test_flash_write_read_verify);
    RUN_TEST_CASE(TEST_IOT_QSPI, AFQP_IotQSPIOpenCloseSuccess);
    RUN_TEST_CASE(TEST_IOT_QSPI, AFQP_IotQSPIOpenCloseFail);
    RUN_TEST_CASE(TEST_IOT_QSPI, AFQP_IotQSPIOpencloseFailUnsupportedInst);
    RUN_TEST_CASE(TEST_IOT_QSPI, AFQP_IotQSPI_FlashWriteReadWriteRead4kSyncVerify);
    RUN_TEST_CASE(TEST_IOT_QSPI, test_flash_transfer_size_less_than_water_lvl);
    RUN_TEST_CASE(TEST_IOT_QSPI, test_flash_bank_boundary);

    /* fuzzing tests */
    RUN_TEST_CASE(TEST_IOT_QSPI, test_flash_close_valid_handle);
    RUN_TEST_CASE(TEST_IOT_QSPI, test_flash_open_valid_device);
    RUN_TEST_CASE(TEST_IOT_QSPI, test_flash_open_invalid_device);
    RUN_TEST_CASE(TEST_IOT_QSPI, test_flash_open_fuzzing);
    RUN_TEST_CASE(TEST_IOT_QSPI, test_flash_erase_sectors_invalid_handle);
    RUN_TEST_CASE(TEST_IOT_QSPI, test_flash_erase_sectors_invalid_address_size);
    RUN_TEST_CASE(TEST_IOT_QSPI, test_flash_set_callback_valid_parameters);
    RUN_TEST_CASE(TEST_IOT_QSPI, test_flash_set_callback_invalid_handle);
    RUN_TEST_CASE(TEST_IOT_QSPI, test_flash_set_callback_invalid_callback);
    RUN_TEST_CASE(TEST_IOT_QSPI, test_flash_write_sync_invalid_data);
    RUN_TEST_CASE(TEST_IOT_QSPI, test_flash_write_async_invalid_data);
    RUN_TEST_CASE(TEST_IOT_QSPI, test_flash_read_sync_invalid_data);
    RUN_TEST_CASE(TEST_IOT_QSPI, test_flash_read_async_invalid_data);
    RUN_TEST_CASE(TEST_IOT_QSPI, test_flash_close_invalid_handle);
    RUN_TEST_CASE(TEST_IOT_QSPI, test_flash_read_sync_valid_parameters);
    RUN_TEST_CASE(TEST_IOT_QSPI, test_flash_write_sync_valid_parameters);

}

/*
 * @brief  Test function to test flash open and close case
 */

TEST(TEST_IOT_QSPI, AFQP_IotQSPIOpenCloseSuccess)
{
    
    int32_t lRetval;

    flash_handle = iot_flash_open(ucAssistedTestIotFlashSlaveAddr);
    TEST_ASSERT_NOT_EQUAL(NULL, flash_handle);

    lRetval = iot_flash_close(flash_handle);
    TEST_ASSERT_EQUAL(0, lRetval);
}

TEST(TEST_IOT_QSPI, AFQP_IotQSPIOpenCloseFail )
{
    int32_t lRetVal;
    
    flash_handle = iot_flash_open(ucAssistedTestIotFlashSlaveAddr);
    TEST_ASSERT_NOT_EQUAL(NULL, flash_handle);

    if ( TEST_PROTECT() )
    {
        /* Open i2c to initialize hardware again */
        flash_handle_1 = iot_flash_open(ucAssistedTestIotFlashSlaveAddr);
        TEST_ASSERT_EQUAL(NULL, flash_handle_1);
    }

    lRetVal = iot_flash_close(flash_handle);
    TEST_ASSERT_EQUAL(0, lRetVal);

    lRetVal = iot_flash_close(flash_handle);
    TEST_ASSERT_EQUAL((-EINVAL), lRetVal);

    lRetVal = iot_flash_close(flash_handle_1);
    TEST_ASSERT_NOT_EQUAL(0, lRetVal);
}

TEST(TEST_IOT_QSPI, AFQP_IotQSPIOpencloseFailUnsupportedInst )
{
    int32_t lRetVal;
    

    flash_handle = iot_flash_open(ucAssistedTestIotUnsupFlashSlaveAddr);
    TEST_ASSERT_EQUAL(NULL, flash_handle);

    lRetVal = iot_flash_close(flash_handle);
    TEST_ASSERT_NOT_EQUAL(0, lRetVal);
}

TEST(TEST_IOT_QSPI, AFQP_IotQSPI_FlashWriteReadWriteReadSyncVerify)
{

    uint32_t address = 0x05000000;

    int32_t lRetVal;
    int32_t read_result;
    flash_handle = iot_flash_open(QSPI_DEV0);

    memset(uctestIotQspiWrFlash, 0, sizeof(uctestIotQspiWrFlash));
    memset(uctestIotQspiRdFlash, 0, sizeof(uctestIotQspiRdFlash));
    fill_buf(uctestIotQspiWrFlash, tetsIotQspi_DATASIZE, 0, 1);
    cache_force_write_back((void*) uctestIotQspiWrFlash, sizeof(uctestIotQspiWrFlash));
    

    int32_t erase_result = iot_flash_erase(flash_handle, address,
    FLASH_SECTOR_SIZE);
    TEST_ASSERT_EQUAL(1, erase_result);
    

    int32_t write_result = iot_flash_write_sync(flash_handle, address, uctestIotQspiWrFlash,
    sizeof(uctestIotQspiWrFlash));
    TEST_ASSERT_EQUAL(0, write_result);
    

    read_result = iot_flash_read_sync(flash_handle, address, uctestIotQspiRdFlash,
    sizeof(uctestIotQspiRdFlash));
    TEST_ASSERT_EQUAL(0, read_result);
    cache_force_invalidate((void*) uctestIotQspiRdFlash, (sizeof(uctestIotQspiRdFlash)));

    lRetVal = memcmp(uctestIotQspiWrFlash, uctestIotQspiRdFlash,
    sizeof(uctestIotQspiRdFlash));
    TEST_ASSERT_EQUAL(0, lRetVal);

    erase_result = iot_flash_erase(flash_handle, address,
    FLASH_SECTOR_SIZE);

    lRetVal = iot_flash_close(flash_handle);
    TEST_ASSERT_EQUAL(0, lRetVal);

    
    address = 0x05200000;
    flash_handle = iot_flash_open(QSPI_DEV0);
    fill_buf(uctestIotQspiWrFlash, tetsIotQspi_DATASIZE, 0, 1);
    cache_force_write_back((void*) uctestIotQspiWrFlash, sizeof(uctestIotQspiWrFlash));
    

    erase_result = iot_flash_erase(flash_handle, address,
    FLASH_SECTOR_SIZE);
    TEST_ASSERT_EQUAL(1, erase_result);

    

    write_result = iot_flash_write_sync(flash_handle, address, uctestIotQspiWrFlash,
    sizeof(uctestIotQspiWrFlash));
    TEST_ASSERT_EQUAL(0, write_result);
    

    read_result = iot_flash_read_sync(flash_handle, address, uctestIotQspiRdFlash,
    sizeof(uctestIotQspiRdFlash));
    TEST_ASSERT_EQUAL(0, read_result);
    cache_force_invalidate((void*) uctestIotQspiRdFlash, (sizeof(uctestIotQspiRdFlash)));

    lRetVal = memcmp(uctestIotQspiWrFlash, uctestIotQspiRdFlash,
    sizeof(uctestIotQspiRdFlash));
    TEST_ASSERT_EQUAL(0, lRetVal);
    lRetVal = iot_flash_close(flash_handle);
    TEST_ASSERT_EQUAL(0, lRetVal);
    
}

TEST(TEST_IOT_QSPI, AFQP_IotQSPI_FlashWriteReadWriteRead4kSyncVerify)
{

    uint32_t address = 0x05000000;

    int32_t lRetVal;
    int32_t read_result;
    flash_handle = iot_flash_open(QSPI_DEV0);

    memset(uctestIotQspiWrFlash4k, 0, sizeof(uctestIotQspiWrFlash4k));
    memset(uctestIotQspiRdFlash4k, 0, sizeof(uctestIotQspiRdFlash4k));
    fill_buf(uctestIotQspiWrFlash4k, tetsIotQspi_DATASIZE4k, 0, 1);
    cache_force_write_back((void*) uctestIotQspiWrFlash4k, sizeof(uctestIotQspiWrFlash4k));
    

    int32_t erase_result = iot_flash_erase(flash_handle, address,
    tetsIotQspi_DATASIZE4k);
    TEST_ASSERT_EQUAL(1, erase_result);
    

    int32_t write_result = iot_flash_write_sync(flash_handle, address, uctestIotQspiWrFlash4k,
    sizeof(uctestIotQspiWrFlash4k));
    TEST_ASSERT_EQUAL(0, write_result);
    

    read_result = iot_flash_read_sync(flash_handle, address, uctestIotQspiRdFlash4k,
    sizeof(uctestIotQspiRdFlash4k));
    TEST_ASSERT_EQUAL(0, read_result);
    cache_force_invalidate((void*) uctestIotQspiRdFlash4k, (sizeof(uctestIotQspiRdFlash4k)));

    lRetVal = memcmp(uctestIotQspiWrFlash4k, uctestIotQspiRdFlash4k,
    sizeof(uctestIotQspiRdFlash4k));
    TEST_ASSERT_EQUAL(0, lRetVal);

    erase_result = iot_flash_erase(flash_handle, address,
    tetsIotQspi_DATASIZE4k);

    lRetVal = iot_flash_close(flash_handle);
    TEST_ASSERT_EQUAL(0, lRetVal);

    
    address = 0x05200000;
    flash_handle = iot_flash_open(QSPI_DEV0);
    fill_buf(uctestIotQspiWrFlash, tetsIotQspi_DATASIZE4k, 0, 1);
    cache_force_write_back((void*) uctestIotQspiWrFlash4k, sizeof(uctestIotQspiWrFlash4k));
    

    erase_result = iot_flash_erase(flash_handle, address,
    tetsIotQspi_DATASIZE4k);
    TEST_ASSERT_EQUAL(1, erase_result);

    write_result = iot_flash_write_sync(flash_handle, address, uctestIotQspiWrFlash4k,
    sizeof(uctestIotQspiWrFlash4k));
    TEST_ASSERT_EQUAL(0, write_result);

    read_result = iot_flash_read_sync(flash_handle, address, uctestIotQspiRdFlash4k,
    sizeof(uctestIotQspiRdFlash4k));
    TEST_ASSERT_EQUAL(0, read_result);
    cache_force_invalidate((void*) uctestIotQspiRdFlash4k, (sizeof(uctestIotQspiRdFlash4k)));

    lRetVal = memcmp(uctestIotQspiWrFlash4k, uctestIotQspiRdFlash4k,
    sizeof(uctestIotQspiRdFlash4k));
    TEST_ASSERT_EQUAL(0, lRetVal);
    lRetVal = iot_flash_close(flash_handle);
    TEST_ASSERT_EQUAL(0, lRetVal);
    
}

TEST(TEST_IOT_QSPI, AFQP_IotQSPI_FlashWriteReadSyncVerify)
{
    
    int32_t lRetVal;
    memset(uctestIotQspiWrFlash, 0, sizeof(uctestIotQspiWrFlash));
    memset(uctestIotQspiRdFlash, 0, sizeof(uctestIotQspiRdFlash));

    flash_handle = iot_flash_open(ucAssistedTestIotFlashSlaveAddr);
    TEST_ASSERT_NOT_EQUAL(NULL, flash_handle);

    if ( TEST_PROTECT() )
    {
        lRetVal = iot_flash_erase(flash_handle, READ_START_ADDRESS,
        tetsIotQspi_DATASIZE);

        TEST_ASSERT_EQUAL(1, lRetVal);

        fill_buf(uctestIotQspiWrFlash, tetsIotQspi_DATASIZE, 0, 1);
        cache_force_write_back((void*) uctestIotQspiWrFlash,
                (tetsIotQspi_DATASIZE));

        lRetVal = iot_flash_write_sync(flash_handle, READ_START_ADDRESS,
                uctestIotQspiWrFlash, tetsIotQspi_DATASIZE);
        TEST_ASSERT_EQUAL(0, lRetVal);

        cache_force_write_back((void*) uctestIotQspiRdFlash,
                (tetsIotQspi_DATASIZE));
        
        lRetVal = iot_flash_read_sync(flash_handle, READ_START_ADDRESS,
                uctestIotQspiRdFlash, tetsIotQspi_DATASIZE);
        cache_force_invalidate((void*) uctestIotQspiRdFlash,
                (tetsIotQspi_DATASIZE));

        TEST_ASSERT_EQUAL(0, lRetVal);

        lRetVal = memcmp(uctestIotQspiWrFlash, uctestIotQspiRdFlash,
        tetsIotQspi_DATASIZE);
        TEST_ASSERT_EQUAL(0, lRetVal);
    }
    lRetVal = iot_flash_close(flash_handle);
    TEST_ASSERT_EQUAL(0, lRetVal);
}

TEST(TEST_IOT_QSPI, AFQP_IotQSPI_FlashWriteReadAsyncVerify)
{
    int32_t lRetVal;
    memset(uctestIotQspiWrFlash, 0, sizeof(uctestIotQspiWrFlash));
    memset(uctestIotQspiRdFlash, 0, sizeof(uctestIotQspiRdFlash));

    xtestIotQspiSemaphore = xSemaphoreCreateBinary();
    
    flash_handle = iot_flash_open(ucAssistedTestIotFlashSlaveAddr);
    TEST_ASSERT_NOT_EQUAL(NULL, flash_handle);

    if ( TEST_PROTECT() )
    {
        lRetVal = iot_flash_erase(flash_handle, READ_START_ADDRESS_ASYNC,
        tetsIotQspi_DATASIZE);

        TEST_ASSERT_EQUAL(1, lRetVal);

        iot_flash_set_callback(flash_handle, prvQspiCallback, NULL);

        fill_buf(uctestIotQspiWrFlash, tetsIotQspi_DATASIZE, 0, 1);
        cache_force_write_back((void*) uctestIotQspiWrFlash,
                (tetsIotQspi_DATASIZE));
        lRetVal = iot_flash_write_async(flash_handle, READ_START_ADDRESS_ASYNC,
                uctestIotQspiWrFlash, tetsIotQspi_DATASIZE);
        TEST_ASSERT_EQUAL(0, lRetVal);
        xSemaphoreTake(xtestIotQspiSemaphore, testIotQspi_MAX_TIMEOUT);
        cache_force_write_back((void*) uctestIotQspiRdFlash,
                (tetsIotQspi_DATASIZE));
        
        lRetVal = iot_flash_read_async(flash_handle, READ_START_ADDRESS_ASYNC,
                uctestIotQspiRdFlash, tetsIotQspi_DATASIZE);
        xSemaphoreTake(xtestIotQspiSemaphore, testIotQspi_MAX_TIMEOUT);
        cache_force_invalidate((void*) uctestIotQspiRdFlash,
                (tetsIotQspi_DATASIZE));
        TEST_ASSERT_EQUAL(0, lRetVal);

        lRetVal = memcmp(uctestIotQspiWrFlash, uctestIotQspiRdFlash,
        tetsIotQspi_DATASIZE);
        TEST_ASSERT_EQUAL(0, lRetVal);
    }
    lRetVal = iot_flash_close(flash_handle);
    TEST_ASSERT_EQUAL(0, lRetVal);
}

TEST(TEST_IOT_QSPI, AFQP_IotQSPI_FlashWriteWriteReadSyncVerify)
{
    
    int32_t lRetVal;
    memset(uctestIotQspiWrFlash, 0, sizeof(uctestIotQspiWrFlash));
    memset(uctestIotQspiRdFlash, 0, sizeof(uctestIotQspiRdFlash));

    flash_handle = iot_flash_open(ucAssistedTestIotFlashSlaveAddr);
    TEST_ASSERT_NOT_EQUAL(NULL, flash_handle);

    if ( TEST_PROTECT() )
    {
        lRetVal = iot_flash_erase(flash_handle, READ_START_ADDRESS,
        tetsIotQspi_DATASIZE);

        TEST_ASSERT_EQUAL(1, lRetVal);

        fill_buf(uctestIotQspiWrFlash, tetsIotQspi_DATASIZE, 0, 1);
        cache_force_write_back((void*) uctestIotQspiWrFlash,
                (tetsIotQspi_DATASIZE));

        lRetVal = iot_flash_write_sync(flash_handle, READ_START_ADDRESS,
                uctestIotQspiWrFlash, tetsIotQspi_DATASIZE);
        TEST_ASSERT_EQUAL(0, lRetVal);

        lRetVal = iot_flash_erase(flash_handle, READ_START_ADDRESS,
        tetsIotQspi_DATASIZE);

        TEST_ASSERT_EQUAL(1, lRetVal);

        fill_buf(uctestIotQspiWrFlash, tetsIotQspi_DATASIZE, 10, 1);
        cache_force_write_back((void*) uctestIotQspiWrFlash,
                (tetsIotQspi_DATASIZE));

        lRetVal = iot_flash_write_sync(flash_handle, READ_START_ADDRESS,
                uctestIotQspiWrFlash, tetsIotQspi_DATASIZE);
        TEST_ASSERT_EQUAL(0, lRetVal);

        cache_force_write_back((void*) uctestIotQspiRdFlash,
                (tetsIotQspi_DATASIZE));
        lRetVal = iot_flash_read_sync(flash_handle, READ_START_ADDRESS,
                uctestIotQspiRdFlash, tetsIotQspi_DATASIZE);
        cache_force_invalidate((void*) uctestIotQspiRdFlash,
                (tetsIotQspi_DATASIZE));

        TEST_ASSERT_EQUAL(0, lRetVal);

        lRetVal = memcmp(uctestIotQspiWrFlash, uctestIotQspiRdFlash,
        tetsIotQspi_DATASIZE);
        TEST_ASSERT_EQUAL(0, lRetVal);
    }
    lRetVal = iot_flash_close(flash_handle);
    TEST_ASSERT_EQUAL(0, lRetVal);
    
}

TEST(TEST_IOT_QSPI, AFQP_IotQSPI_FlashWriteReadReadSyncVerify)
{
    
    int32_t lRetVal;
    memset(uctestIotQspiWrFlash, 0, sizeof(uctestIotQspiWrFlash));
    memset(uctestIotQspiRdFlash, 0, sizeof(uctestIotQspiRdFlash));

    flash_handle = iot_flash_open(ucAssistedTestIotFlashSlaveAddr);
    TEST_ASSERT_NOT_EQUAL(NULL, flash_handle);

    if ( TEST_PROTECT() )
    {
        lRetVal = iot_flash_erase(flash_handle, READ_START_ADDRESS,
        tetsIotQspi_DATASIZE);
        TEST_ASSERT_EQUAL(1, lRetVal);
        
        fill_buf(uctestIotQspiWrFlash, tetsIotQspi_DATASIZE, 0, 1);
        cache_force_write_back((void*) uctestIotQspiWrFlash,
                (tetsIotQspi_DATASIZE));
        lRetVal = iot_flash_write_sync(flash_handle, READ_START_ADDRESS,
                uctestIotQspiWrFlash, tetsIotQspi_DATASIZE);
        TEST_ASSERT_EQUAL(0, lRetVal);

        lRetVal = iot_flash_read_sync(flash_handle, READ_START_ADDRESS,
                uctestIotQspiRdFlash, tetsIotQspi_DATASIZE);
        cache_force_invalidate((void*) uctestIotQspiRdFlash,
                (tetsIotQspi_DATASIZE));
        TEST_ASSERT_EQUAL(0, lRetVal);

        /*Read the same data again*/
        cache_force_write_back((void*) uctestIotQspiRdFlash,
                (tetsIotQspi_DATASIZE));
        lRetVal = iot_flash_read_sync(flash_handle, READ_START_ADDRESS,
                uctestIotQspiRdFlash, tetsIotQspi_DATASIZE);
        cache_force_invalidate((void*) uctestIotQspiRdFlash,
                (tetsIotQspi_DATASIZE));
        TEST_ASSERT_EQUAL(0, lRetVal);
        lRetVal = memcmp(uctestIotQspiWrFlash, uctestIotQspiRdFlash,
        tetsIotQspi_DATASIZE);
        TEST_ASSERT_EQUAL(0, lRetVal);
    }
    lRetVal = iot_flash_close(flash_handle);
    TEST_ASSERT_EQUAL(0, lRetVal);
    
}

TEST(TEST_IOT_QSPI, AFQP_IotQSPI_FlashWriteReadBoundaryVerify)
{
    
    int32_t lRetVal;
    memset(uctestIotQspiWrFlash, 0, sizeof(uctestIotQspiWrFlash));
    memset(uctestIotQspiRdFlash, 0, sizeof(uctestIotQspiRdFlash));
    uint32_t addresses[ ] =
    { 0x00000000, (FLASH_SECTOR_SIZE - tetsIotQspi_DATASIZE), FLASH_SECTOR_SIZE,
            (FLASH_SECTOR_SIZE + tetsIotQspi_DATASIZE) };

    if ( TEST_PROTECT() )
    {
        for ( int i = 0; i < (int) (sizeof(addresses) / sizeof(addresses[ 0 ]));
                i++ )
        {
    	    flash_handle = iot_flash_open(ucAssistedTestIotFlashSlaveAddr);
    	    TEST_ASSERT_NOT_NULL(flash_handle);
            uint32_t address = addresses[ i ];
            lRetVal = iot_flash_erase(flash_handle, address,
            tetsIotQspi_DATASIZE );
            TEST_ASSERT_NOT_EQUAL(-1, lRetVal);

            
            memset(uctestIotQspiWrFlash, 0, sizeof(uctestIotQspiWrFlash));

            fill_buf(uctestIotQspiWrFlash, tetsIotQspi_DATASIZE, 0, 1);
            cache_force_write_back((void*) uctestIotQspiWrFlash,
                    sizeof(uctestIotQspiWrFlash));
            lRetVal = iot_flash_write_sync(flash_handle, address,
                    uctestIotQspiWrFlash, tetsIotQspi_DATASIZE);
            TEST_ASSERT_EQUAL(0, lRetVal);
            memset(uctestIotQspiRdFlash, 0, sizeof(uctestIotQspiRdFlash));
            cache_force_write_back((void*) uctestIotQspiRdFlash,
                    sizeof(uctestIotQspiRdFlash));
            lRetVal = iot_flash_read_sync(flash_handle, address,
                    uctestIotQspiRdFlash, tetsIotQspi_DATASIZE);
            cache_force_invalidate((void*) uctestIotQspiRdFlash,
                    sizeof(uctestIotQspiRdFlash));
            TEST_ASSERT_EQUAL(0, lRetVal);
            lRetVal = memcmp(uctestIotQspiWrFlash, uctestIotQspiRdFlash,
            tetsIotQspi_DATASIZE);
            TEST_ASSERT_EQUAL(0, lRetVal);
    	    lRetVal = iot_flash_close(flash_handle);
            TEST_ASSERT_EQUAL(0, lRetVal);
        }
    }
}


TEST(TEST_IOT_QSPI, AFQP_IotQSPI_FlashWriteReadPatternsVerify)
{

    uint32_t address = 0x05000000;
    int32_t lRetVal;
    int32_t read_result;
    memset(uctestIotQspiWrFlash, 0, sizeof(uctestIotQspiWrFlash));
    memset(uctestIotQspiRdFlash, 0, sizeof(uctestIotQspiRdFlash));
    for ( int i = 1; i <= 4; i++ )
    {
        flash_handle = iot_flash_open(QSPI_DEV0);

        memset(uctestIotQspiWrFlash, 0, sizeof(uctestIotQspiWrFlash));
        memset(uctestIotQspiRdFlash, 0, sizeof(uctestIotQspiRdFlash));
        fill_buf(uctestIotQspiWrFlash, tetsIotQspi_DATASIZE, 0, i);
        cache_force_write_back((void*) uctestIotQspiWrFlash, sizeof(uctestIotQspiWrFlash));
        

        int32_t erase_result = iot_flash_erase(flash_handle, address,
        FLASH_SECTOR_SIZE );
        TEST_ASSERT_EQUAL(1, erase_result);
        

        int32_t write_result = iot_flash_write_sync(flash_handle, address, uctestIotQspiWrFlash,
        sizeof(uctestIotQspiWrFlash));
        TEST_ASSERT_EQUAL(0, write_result);
        

        read_result = iot_flash_read_sync(flash_handle, address, uctestIotQspiRdFlash,
        sizeof(uctestIotQspiRdFlash));
        TEST_ASSERT_EQUAL(0, read_result);
        cache_force_invalidate((void*) uctestIotQspiRdFlash, (sizeof(uctestIotQspiRdFlash)));

        lRetVal = memcmp(uctestIotQspiWrFlash, uctestIotQspiRdFlash,
        sizeof(uctestIotQspiRdFlash));
        TEST_ASSERT_EQUAL(0, lRetVal);

        erase_result = iot_flash_erase(flash_handle, address,
        FLASH_SECTOR_SIZE);

        lRetVal = iot_flash_close(flash_handle);
        TEST_ASSERT_EQUAL(0, lRetVal);

        
    }
}

TEST(TEST_IOT_QSPI, test_flash_write_read_verify)
{
    int32_t lRetVal;
    memset(uctestIotQspiWrFlash, 0, sizeof(uctestIotQspiWrFlash));
    memset(uctestIotQspiRdFlash, 0, sizeof(uctestIotQspiRdFlash));

    uint32_t address = 0x05000000;
    flash_handle = iot_flash_open(QSPI_DEV0);

    // Initialize test data with the string
    memset(uctestIotQspiWrFlash, 0, sizeof(uctestIotQspiWrFlash)); // Clear the uctestIotQspiWrFlash uctestIotQspiRdFlash
    memset(uctestIotQspiRdFlash, 0, sizeof(uctestIotQspiRdFlash));
    fill_buf(uctestIotQspiWrFlash, tetsIotQspi_DATASIZE, 0, 1);
    cache_force_write_back((void*) uctestIotQspiWrFlash, sizeof(uctestIotQspiWrFlash));
    
    // Erase the sector before writing
    int32_t erase_result = iot_flash_erase(flash_handle, address,
    FLASH_SECTOR_SIZE);
    TEST_ASSERT_EQUAL(1, erase_result); // Expecting 1 sector to be erased
    int32_t read_result;
    
    // Write uctestIotQspiWrFlash to flash memory
    int32_t write_result = iot_flash_write_sync(flash_handle, address, uctestIotQspiWrFlash,
    sizeof(uctestIotQspiWrFlash));
    TEST_ASSERT_EQUAL(0, write_result);
    
    // Read data back from flash memory
    read_result = iot_flash_read_sync(flash_handle, address, uctestIotQspiRdFlash,
    sizeof(uctestIotQspiRdFlash));
    TEST_ASSERT_EQUAL(0, read_result);
    cache_force_invalidate((void*) uctestIotQspiRdFlash, (sizeof(uctestIotQspiRdFlash)));
    // Verify the read data matches the written data
    lRetVal = memcmp(uctestIotQspiWrFlash, uctestIotQspiRdFlash,
    sizeof(uctestIotQspiRdFlash));
    TEST_ASSERT_EQUAL(0, lRetVal);

    // Erase the sector before writing
    erase_result = iot_flash_erase(flash_handle, address,
    FLASH_SECTOR_SIZE);

    lRetVal = iot_flash_close(flash_handle);
    TEST_ASSERT_EQUAL(0, lRetVal);

#if 1

    address = 0x05200000;
    flash_handle = iot_flash_open(QSPI_DEV0);
    fill_buf(uctestIotQspiWrFlash, tetsIotQspi_DATASIZE, 0, 5);
    cache_force_write_back((void*) uctestIotQspiWrFlash, sizeof(uctestIotQspiWrFlash));
    

    // Erase the sector before writing
    erase_result = iot_flash_erase(flash_handle, address,
    FLASH_SECTOR_SIZE);
    TEST_ASSERT_EQUAL(1, erase_result); // Expecting 1 sector to be erased

    
    // Write data to flash memory
    write_result = iot_flash_write_sync(flash_handle, address, uctestIotQspiWrFlash,
    sizeof(uctestIotQspiWrFlash));
    TEST_ASSERT_EQUAL(0, write_result);
    
    // Read data back from flash memory
    read_result = iot_flash_read_sync(flash_handle, address, uctestIotQspiRdFlash,
    sizeof(uctestIotQspiRdFlash));
    TEST_ASSERT_EQUAL(0, read_result);
    cache_force_invalidate((void*) uctestIotQspiRdFlash, (sizeof(uctestIotQspiRdFlash)));
    // Verify the read data matches the written data
    lRetVal = memcmp(uctestIotQspiWrFlash, uctestIotQspiRdFlash,
    sizeof(uctestIotQspiRdFlash));
    TEST_ASSERT_EQUAL(0, lRetVal);
    lRetVal = iot_flash_close(flash_handle);
    TEST_ASSERT_EQUAL(0, lRetVal);

    
#endif 
}

/* fuzzing tests */
TEST(TEST_IOT_QSPI, test_flash_open_valid_device)
{
    int32_t lRetVal;

    flash_handle = flash_open(QSPI_DEV0);
    TEST_ASSERT_NOT_NULL(flash_handle);
    lRetVal = flash_close(flash_handle);
    TEST_ASSERT_EQUAL(0, lRetVal);
}

TEST(TEST_IOT_QSPI, test_flash_open_invalid_device)
{

    flash_handle = flash_open(MAX_QSPI_DEV);
    TEST_ASSERT_NULL(flash_handle);

    flash_handle = flash_open(-1);
    TEST_ASSERT_NULL(flash_handle);
}

TEST(TEST_IOT_QSPI, test_flash_open_fuzzing)
{
    int32_t lRetVal;


    srand(5);
    for ( int i = 0; i < 100; i++ )
    {
        uint32_t random_dev = rand();
        flash_handle = flash_open(random_dev);
        if ( random_dev < MAX_QSPI_DEV )
        {
            TEST_ASSERT_NOT_NULL(flash_handle);
            lRetVal = flash_close(flash_handle);
            TEST_ASSERT_EQUAL(0, lRetVal);
        }
        else
        {
            TEST_ASSERT_NULL(flash_handle);
        }
    }
}

TEST(TEST_IOT_QSPI, test_flash_erase_sectors_invalid_handle)
{
    int32_t lRetVal;
    lRetVal = flash_erase_sectors(NULL, 0x1000, 0x1000);
    TEST_ASSERT_EQUAL((-EINVAL), lRetVal);
}

TEST(TEST_IOT_QSPI, test_flash_erase_sectors_invalid_address_size)
{
    int32_t lRetVal;


    flash_handle = flash_open(QSPI_DEV0);
    TEST_ASSERT_NOT_NULL(flash_handle);

    lRetVal = flash_erase_sectors(flash_handle, 0x1000, 0);
    TEST_ASSERT_EQUAL(0, lRetVal);

    lRetVal = flash_close(flash_handle);
    TEST_ASSERT_EQUAL(0, lRetVal);
}

TEST(TEST_IOT_QSPI, test_flash_set_callback_valid_parameters)
{
    int32_t lRetVal;


    flash_handle = flash_open(QSPI_DEV0);
    TEST_ASSERT_NOT_NULL(flash_handle);

    int result = flash_set_callback(flash_handle, dummy_callback, NULL);
    TEST_ASSERT_EQUAL(0, result);

    lRetVal = flash_close(flash_handle);
    TEST_ASSERT_EQUAL(0, lRetVal);
}

TEST(TEST_IOT_QSPI, test_flash_set_callback_invalid_handle)
{
    int32_t lRetVal;
    lRetVal = flash_set_callback(NULL, dummy_callback, NULL);
    TEST_ASSERT_EQUAL((-EINVAL), lRetVal);
}

TEST(TEST_IOT_QSPI, test_flash_set_callback_invalid_callback)
{
    int32_t lRetVal;


    flash_handle = flash_open(QSPI_DEV0);
    TEST_ASSERT_NOT_NULL(flash_handle);

    lRetVal = flash_set_callback(flash_handle, NULL, NULL);
    TEST_ASSERT_EQUAL(0, lRetVal);

    lRetVal = flash_close(flash_handle);
    TEST_ASSERT_EQUAL(0, lRetVal);
}

TEST(TEST_IOT_QSPI, test_flash_write_sync_valid_parameters)
{
    int32_t lRetVal;


    flash_handle = flash_open(QSPI_DEV0);
    TEST_ASSERT_NOT_NULL(flash_handle);

    lRetVal = flash_erase_sectors(flash_handle, 0x1000, 256);
    TEST_ASSERT_EQUAL(1, lRetVal);

    lRetVal = flash_write_sync(flash_handle, 0x1000, uctestIotQspiWrFlash,
            256);
    TEST_ASSERT_EQUAL(0, lRetVal);

    osal_task_delay(5);
    lRetVal = flash_close(flash_handle);
    TEST_ASSERT_EQUAL(0, lRetVal);
}

TEST(TEST_IOT_QSPI, test_flash_write_sync_invalid_data)
{
    int32_t lRetVal;
    flash_handle = flash_open(QSPI_DEV0);
    TEST_ASSERT_NOT_NULL(flash_handle);
    lRetVal = flash_write_sync(flash_handle, 0x1000, NULL, 256);
    TEST_ASSERT_EQUAL((-EINVAL), lRetVal);

    lRetVal = flash_close(flash_handle);
    TEST_ASSERT_EQUAL(0, lRetVal);
    
}

TEST(TEST_IOT_QSPI, test_flash_write_async_invalid_data)
{
    int32_t lRetVal;


    flash_handle = flash_open(QSPI_DEV0);
    TEST_ASSERT_NOT_NULL(flash_handle);

    int result = flash_write_async(flash_handle, 0x1000, NULL, 256);
    TEST_ASSERT_EQUAL((-EINVAL), result);

    lRetVal = flash_close(flash_handle);
    TEST_ASSERT_EQUAL(0, lRetVal);
    
}

TEST(TEST_IOT_QSPI, test_flash_read_sync_valid_parameters)
{
    int32_t lRetVal;


    flash_handle = flash_open(QSPI_DEV0);
    TEST_ASSERT_NOT_NULL(flash_handle);


    int result = flash_read_sync(flash_handle, 0x1000, uctestIotQspiRdFlash,
            256);
    TEST_ASSERT_EQUAL(0, result);

    lRetVal = flash_close(flash_handle);
    TEST_ASSERT_EQUAL(0, lRetVal);
    
}

TEST(TEST_IOT_QSPI, test_flash_read_sync_invalid_data)
{
    int32_t lRetVal;


    flash_handle = flash_open(QSPI_DEV0);
    TEST_ASSERT_NOT_NULL(flash_handle);

    lRetVal = flash_read_sync(flash_handle, 0x1000, NULL, 256);
    TEST_ASSERT_EQUAL((-EINVAL), lRetVal);

    lRetVal = flash_close(flash_handle);
    TEST_ASSERT_EQUAL(0, lRetVal);
    
}

TEST(TEST_IOT_QSPI, test_flash_read_async_invalid_data)
{
    int32_t lRetVal;


    flash_handle = flash_open(QSPI_DEV0);
    TEST_ASSERT_NOT_NULL(flash_handle);

    lRetVal = flash_read_async(flash_handle, 0x1000, NULL, 256); // NULL data uctestIotQspiRdFlash
    TEST_ASSERT_EQUAL((-EINVAL), lRetVal);

    lRetVal = flash_close(flash_handle);
    TEST_ASSERT_EQUAL(0, lRetVal);
    
}

TEST(TEST_IOT_QSPI, test_flash_close_valid_handle)
{
    int32_t lRetVal;


    flash_handle = flash_open(QSPI_DEV0);
    TEST_ASSERT_NOT_NULL(flash_handle);

    lRetVal = flash_close(flash_handle);
    TEST_ASSERT_EQUAL(0, lRetVal);
}

TEST(TEST_IOT_QSPI, test_flash_close_invalid_handle)
{
    int result = flash_close(NULL);
    TEST_ASSERT_EQUAL((-EINVAL), result);
}

TEST(TEST_IOT_QSPI, test_flash_transfer_size_less_than_water_lvl)
{
    int32_t lRetVal;

    flash_handle = flash_open(QSPI_DEV0);
    TEST_ASSERT_NOT_NULL(flash_handle);

    lRetVal = flash_erase_sectors(flash_handle, READ_START_ADDRESS_ASYNC, QSPI_LOWER_DATASIZE);
    TEST_ASSERT_NOT_EQUAL(-1, lRetVal);

    lRetVal = flash_set_callback(flash_handle, prvQspiCallback, NULL);
    TEST_ASSERT_EQUAL(0, lRetVal);

    fill_buf(qspi_wr_buf_low, QSPI_LOWER_DATASIZE, 9, 1);
    cache_force_write_back((void*) qspi_wr_buf_low, QSPI_LOWER_DATASIZE);

    lRetVal = flash_write_async(flash_handle, READ_START_ADDRESS_ASYNC,
            qspi_wr_buf_low, QSPI_LOWER_DATASIZE);
    TEST_ASSERT_EQUAL(0, lRetVal);

    xSemaphoreTake(xtestIotQspiSemaphore, testIotQspi_MAX_TIMEOUT);
    cache_force_write_back((void*) qspi_rd_buf_low, QSPI_LOWER_DATASIZE);

    lRetVal = flash_read_async(flash_handle, READ_START_ADDRESS_ASYNC,
            qspi_rd_buf_low, QSPI_LOWER_DATASIZE);
    xSemaphoreTake(xtestIotQspiSemaphore, testIotQspi_MAX_TIMEOUT);
    cache_force_invalidate((void*) qspi_rd_buf_low, QSPI_LOWER_DATASIZE);
    TEST_ASSERT_EQUAL(0, lRetVal);

    lRetVal = memcmp(qspi_wr_buf_low, qspi_rd_buf_low,
            QSPI_LOWER_DATASIZE);
    TEST_ASSERT_EQUAL(0, lRetVal);

    lRetVal = flash_close(flash_handle);
    TEST_ASSERT_EQUAL(0, lRetVal);
    
}

TEST(TEST_IOT_QSPI, test_flash_bank_boundary)
{
    fill_buf(uctestIotQspiWrFlash4k, tetsIotQspi_DATASIZE4k, 0, 1);
    cache_force_write_back((void*) uctestIotQspiWrFlash4k, sizeof(uctestIotQspiWrFlash4k));

    flash_handle = flash_open(QSPI_DEV0);
    TEST_ASSERT_NOT_NULL(flash_handle);

    int32_t retval = flash_erase_sectors(flash_handle, BOUNDARY_START_ADDRESS, tetsIotQspi_DATASIZE4k);
    TEST_ASSERT_NOT_EQUAL(-1, retval);

    retval = flash_write_sync(flash_handle, BOUNDARY_START_ADDRESS, uctestIotQspiWrFlash4k, tetsIotQspi_DATASIZE4k);
    TEST_ASSERT_EQUAL(0, retval);

    cache_force_write_back((void*) uctestIotQspiRdFlash4k, sizeof(uctestIotQspiRdFlash4k));

    retval = flash_read_sync(flash_handle, BOUNDARY_START_ADDRESS, uctestIotQspiRdFlash4k, tetsIotQspi_DATASIZE4k);
    TEST_ASSERT_EQUAL(0, retval);

    cache_force_invalidate((void*) uctestIotQspiRdFlash4k, sizeof(uctestIotQspiRdFlash4k));

    retval = memcmp(uctestIotQspiWrFlash4k, uctestIotQspiRdFlash4k, sizeof(uctestIotQspiRdFlash4k));
    TEST_ASSERT_EQUAL(0, retval);

    retval = flash_close(flash_handle);
    TEST_ASSERT_EQUAL(0, retval);
    
}
