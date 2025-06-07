#include <errno.h>

#include "FreeRTOS.h"
#include "FreeRTOSConfig.h"
#include "unity.h"
#include "unity_fixture.h"
#include "socfpga_defines.h"
#include "socfpga_i3c.h"
#include <stdbool.h>
#include "task.h"


// Helper function to fill a buffer with an incremental pattern starting from a given number
void fill_pattern( uint8_t *buffer, size_t length, uint8_t start )
{
    for ( size_t i = 0; i < length; i++ )
    {
        buffer[ i ] = start++;
    }
}


// Helper function to compare two buffers
bool compare_pattern( const uint8_t *buffer1, const uint8_t *buffer2,
        uint32_t length )
{
    for ( uint32_t i = 0; i < length; i++ )
    {
        if ( buffer1[ i ] != buffer2[ i ] )
        {
            return false;
        }
    }
    return true;
}

/* Define Test Group. */
TEST_GROUP(TEST_IOT_I3C);

/*-----------------------------------------------------------*/

/**
 * @brief Setup function called before each test in this group is executed.
 */
TEST_SETUP( TEST_IOT_I3C )
{
}

/*-----------------------------------------------------------*/

/**
 * @brief Tear down function called after each test in this group is executed.
 */
TEST_TEAR_DOWN( TEST_IOT_I3C )
{
}

/*-----------------------------------------------------------*/

TEST( TEST_IOT_I3C, test_SocfpgaI3c_open_valid_instance )
{
    int32_t result = i3c_open(I3C_INSTANCE2);
    TEST_ASSERT_EQUAL(0, result);
}

TEST( TEST_IOT_I3C, test_SocfpgaI3c_open_invalid_instance )
{
    int32_t result = i3c_open(I3C_NUM_INSTANCES);
    TEST_ASSERT_EQUAL((-EINVAL), result);
}

TEST( TEST_IOT_I3C, test_SocfpgaI3c_ioctl_target_attach_valid )
{
    struct i3c_I3cDevice device = { .staticAddress = 0, .deviceId = 0x0000020800B30, .preferredDynamicAddress = 0, };
    struct i3c_DevList devList = { .numDevices = 1, .list = &device, };
    // Initialize devList with valid data
    int32_t result = i3c_ioctl(I3C_INSTANCE2, I3C_IOCTL_TARGET_ATTACH, &devList);
    TEST_ASSERT_EQUAL(0, result);
}

TEST( TEST_IOT_I3C, test_SocfpgaI3c_ioctl_bus_init_valid )
{
    // Assume I3C_IOCTL_TARGET_ATTACH has been called
    int32_t result = i3c_ioctl(I3C_INSTANCE2, I3C_IOCTL_BUS_INIT, NULL);
    TEST_ASSERT_EQUAL(0, result);
}

TEST( TEST_IOT_I3C, test_SocfpgaI3c_ioctl_do_daa_valid )
{
    int32_t result = i3c_ioctl(I3C_INSTANCE2, I3C_IOCTL_DO_DAA, NULL);
    TEST_ASSERT_EQUAL(0, result);
}

TEST( TEST_IOT_I3C, test_SocfpgaI3c_ioctl_invalid_instance )
{
    int32_t result = i3c_ioctl(255, I3C_IOCTL_TARGET_ATTACH, NULL);
    TEST_ASSERT_EQUAL((-EINVAL), result);
}

TEST( TEST_IOT_I3C, test_SocfpgaI3c_TransferSync_transfer_unsuccessful )
{
    struct i3c_XferRequest xferRequest = {0};
    // Initialize xferRequest with data that causes an unsuccessful transfer
    int32_t result = i3c_TransferSync(I3C_INSTANCE2, 0x10, &xferRequest, 1, true);
    TEST_ASSERT_EQUAL(result, (-EIO));
}

TEST( TEST_IOT_I3C, test_SocfpgaI3c_open_boundary_values )
{
    int32_t result;

    // Test lower boundary
    result = i3c_open(0);
    TEST_ASSERT_EQUAL(0, result);

    // Test upper boundary
    result = i3c_open(I3C_NUM_INSTANCES - 1);
    TEST_ASSERT_EQUAL(0, result);

    // Test just beyond upper boundary
    result = i3c_open(I3C_NUM_INSTANCES);
    TEST_ASSERT_EQUAL((-EINVAL), result);
}

TEST( TEST_IOT_I3C, test_SocfpgaI3c_open_random_values )
{
    for ( int i = 0; i < 100; i++ )
    {
        srand(1);
        uint8_t random_instance = rand() % 256; // Generate random value between 0 and 255
        int32_t result = i3c_open(random_instance);
        if ( random_instance < I3C_NUM_INSTANCES )
        {
            TEST_ASSERT_EQUAL(0, result);
        }
        else
        {
            TEST_ASSERT_EQUAL((-EINVAL), result);
        }
    }
}

TEST( TEST_IOT_I3C, test_SocfpgaI3c_ioctl_boundary_values )
{
    int32_t result;

    // Test lower boundary
    struct i3c_DevList devList;
    result = i3c_ioctl(0, I3C_IOCTL_TARGET_ATTACH, &devList);
    TEST_ASSERT_EQUAL(0, result);

    // Test upper boundary
    result = i3c_ioctl(I3C_NUM_INSTANCES - 1, I3C_IOCTL_TARGET_ATTACH,
            &devList);
    TEST_ASSERT_EQUAL(0, result);

    // Test just beyond upper boundary
    result = i3c_ioctl(I3C_NUM_INSTANCES, I3C_IOCTL_TARGET_ATTACH,
            &devList);
    TEST_ASSERT_EQUAL((-EINVAL), result);
}

TEST( TEST_IOT_I3C, test_SocfpgaI3c_open_repeated_calls )
{
    int32_t result;

    // Call the function multiple times with the same valid instance
    for ( int i = 0; i < 10; i++ )
    {
        result = i3c_open(I3C_INSTANCE2);
        TEST_ASSERT_EQUAL(0, result);
    }
}

TEST( TEST_IOT_I3C, test_SocfpgaI3c_TransferSync_zero_transfers )
{
    struct i3c_XferRequest xferRequest;
    uint8_t buffer[ 10 ] =
    { 0 };
    xferRequest.buffer = buffer;
    xferRequest.length = sizeof(buffer);
    xferRequest.read = true;

    int32_t result = i3c_TransferSync(I3C_INSTANCE2, 0x10, &xferRequest, 0,
    true);
    TEST_ASSERT_EQUAL((-EIO), result);
}

TEST( TEST_IOT_I3C, test_SocfpgaI3c_TransferSync_invalid_dynamic_address )
{
    struct i3c_XferRequest xferRequest;
    uint8_t buffer[ 10 ] =
    { 0 };
    xferRequest.buffer = buffer;
    xferRequest.length = sizeof(buffer);
    xferRequest.read = true;

    int32_t result = i3c_TransferSync(I3C_INSTANCE2, 0xFF, &xferRequest, 1,
    true);
    TEST_ASSERT_EQUAL((-EIO), result);
}

TEST( TEST_IOT_I3C, test_SocfpgaI3c_TransferSync_large_number_of_transfers )
{
    struct i3c_XferRequest xferRequest;
    uint8_t buffer[ 10 ] =
    { 0 };
    xferRequest.buffer = buffer;
    xferRequest.length = sizeof(buffer);
    xferRequest.read = true;

    int32_t result = i3c_TransferSync(I3C_INSTANCE2, 0x10, &xferRequest,
            255,
            true);
    TEST_ASSERT_EQUAL(0, result);
}

TEST( TEST_IOT_I3C, test_SocfpgaI3c_write_read_verify_64_bytes )
{
    int32_t result;

    // Step 1: Open the I3C controller
    result = i3c_open(I3C_INSTANCE2);
    TEST_ASSERT_EQUAL(0, result);

    // Step 2: Attach devices to the I3C controller
    struct i3c_I3cDevice device[ ] =
    {
    { .staticAddress = 0, // TODO :device NAKED for SETDASA (0x5C and 0x5D), currently set to 0 to trigger ENTDAA
            .deviceId = 0x0000020800B30, // LPS27HHTW MEMS pressure sensor PID(TRM)
            .preferredDynamicAddress = 0 },
    { .deviceId = 0x00, .staticAddress = 0x50, // Static address for ATMEL178 24C256 EEPROM
            .preferredDynamicAddress = 0x00, .dynamicAddress = 0x00 } };
    struct i3c_DevList devList =
    { .numDevices = 2, .list = device };
    result = i3c_ioctl(I3C_INSTANCE2, I3C_IOCTL_TARGET_ATTACH, &devList);
    TEST_ASSERT_EQUAL(0, result);

    // Step 3: Initialize the I3C bus
    result = i3c_ioctl(I3C_INSTANCE2, I3C_IOCTL_BUS_INIT, &device[ 0 ]);
    TEST_ASSERT_EQUAL(0, result);

    result = i3c_ioctl(I3C_INSTANCE2, I2C_IOCTL_ADDRESS_VALID,
            &device[ 1 ]);
    TEST_ASSERT_EQUAL(0, result);

    // Step 4: Fill a buffer with an incremental pattern
    uint8_t writeBuffer[ 64 ];
    uint8_t XferData[ 66 ], i;
    uint32_t addr = 0x00;
    fill_pattern(writeBuffer, sizeof(writeBuffer), 0x00);

    // Step 5: Write the buffer to the EEPROM page by page
    XferData[ 0 ] = (addr >> 8) & 0xFF;
    XferData[ 1 ] = addr & 0xFF;
    for ( i = 0; i < sizeof(writeBuffer); i++ )
    {
        XferData[ 2 + i ] = writeBuffer[ i ];
    }
    struct i3c_XferRequest xferRequest;
    xferRequest.buffer = XferData;
    xferRequest.length = sizeof(writeBuffer) + 2;
    xferRequest.read = false;

    result = i3c_TransferSync(I3C_INSTANCE2, device[ 1 ].staticAddress,
            &xferRequest, 1, true);
    TEST_ASSERT_EQUAL(0, result);
    vTaskDelay(100);

    // Step 6: Read back the data from the EEPROM page by page
    struct i3c_XferRequest xferReq[ 2 ];
    uint8_t xferCmd[ 2 ];
    uint8_t readBuffer[ 64 ] =
    { 0 };

    xferCmd[ 0 ] = (addr >> 8) & 0xFF;
    xferCmd[ 1 ] = addr & 0xFF;

    xferReq[ 0 ].buffer = xferCmd;
    xferReq[ 0 ].length = 2;
    xferReq[ 0 ].read = false;

    xferReq[ 1 ].buffer = readBuffer;
    xferReq[ 1 ].length = sizeof(readBuffer);
    xferReq[ 1 ].read = true;

    result = i3c_TransferSync(I3C_INSTANCE2, device[ 1 ].staticAddress,
            xferReq, 2, true);
    TEST_ASSERT_EQUAL(0, result);

    // Step 7: Verify the read data against the original pattern
    bool verify_result = compare_pattern(writeBuffer, readBuffer,
            sizeof(writeBuffer));
    TEST_ASSERT_TRUE(verify_result);
}

volatile uint16_t offset = 0;
uint8_t readBuffer1[ 4096 ] =
{ 0 };
uint8_t readBuffer2[ 4096 ] = { 0 };
uint8_t cmpBuff1[4096] = { 0 };
uint8_t cmpBuff2[4096] = { 0 };
struct i3c_XferRequest xferReq[ 2 ];
uint8_t xferCmd[ 2 ];
uint8_t writeBuffer1[ 64 ];
uint8_t writeBuffer2[ 64 ];
uint8_t xferData[ sizeof(writeBuffer1) + 2 ] =
{ 0 };

TEST( TEST_IOT_I3C, test_SocfpgaI3c_write_read_verify_4KB )
{
    int32_t result;
    uint32_t i;

    // Step 1: Open the I3C controller
    result = i3c_open(I3C_INSTANCE2);
    TEST_ASSERT_EQUAL(0, result);

    // Step 2: Attach devices to the I3C controller
    struct i3c_I3cDevice device =
    { .deviceId = 0x00, .staticAddress = 0x50, // Static address for ATMEL178 24C256 EEPROM
            .preferredDynamicAddress = 0x00, .dynamicAddress = 0x00 };
    struct i3c_DevList devList =
    { .numDevices = 1, .list = &device };
    result = i3c_ioctl(I3C_INSTANCE2, I3C_IOCTL_TARGET_ATTACH, &devList);
    TEST_ASSERT_EQUAL(0, result);

    // Step 3: Initialize the I3C bus
    result = i3c_ioctl(I3C_INSTANCE2, I3C_IOCTL_BUS_INIT, NULL);
    TEST_ASSERT_EQUAL(0, result);

    // Step 4: Fill a buffer with an incremental pattern
    fill_pattern(writeBuffer1, sizeof(writeBuffer1), 0x00);
    for ( i = 0; i < 64; i++ )
    {
        xferData[ 2 + i ] = writeBuffer1[ i ];
    }

    for (i = 0; i < 4096; i+=64)
    {
        memcpy(&cmpBuff1[i], writeBuffer1, 64);
    }
    // Step 5: Write the buffer to the EEPROM page by page
    struct i3c_XferRequest xferRequest;
    xferRequest.read = false;
    for ( offset = 0; offset < 4096; offset += 64 )
    {
        xferData[ 0 ] = (offset >> 8) & 0xFF;
        xferData[ 1 ] = offset & 0xFF;
        xferRequest.buffer = xferData;
        xferRequest.length = 66;
        result = i3c_TransferSync(I3C_INSTANCE2, device.staticAddress,
                &xferRequest, 1, true);
        TEST_ASSERT_EQUAL(0, result);
        vTaskDelay(pdMS_TO_TICKS(5));
    }

    // Step 6: Read back the data from the EEPROM page by page
    xferCmd[ 0 ] = (0x00 >> 8) & 0xFF;
    xferCmd[ 1 ] = 0x00 & 0xFF;

    xferReq[ 0 ].buffer = xferCmd;
    xferReq[ 0 ].length = 2;
    xferReq[ 0 ].read = false;

    xferReq[ 1 ].buffer = readBuffer1;
    xferReq[ 1 ].length = sizeof(readBuffer1);
    xferReq[ 1 ].read = true;

    result = i3c_TransferSync(I3C_INSTANCE2, device.staticAddress, xferReq,
            2, true);
    TEST_ASSERT_EQUAL(0, result);

    // Step 7: Verify the read data against the original pattern
    bool verify_result = compare_pattern(cmpBuff1, readBuffer1,
            4096);
    TEST_ASSERT_TRUE(verify_result);
}

TEST( TEST_IOT_I3C, test_SocfpgaI3c_write_write_read_2KB )
{
    int32_t result;
    uint32_t i;

    // Step 1: Open the I3C controller
    result = i3c_open(I3C_INSTANCE2);
    TEST_ASSERT_EQUAL(0, result);

    // Step 2: Attach devices to the I3C controller
    struct i3c_I3cDevice device =
    { .deviceId = 0x00, .staticAddress = 0x50, // Static address for ATMEL178 24C256 EEPROM
            .preferredDynamicAddress = 0x00, .dynamicAddress = 0x00 };
    struct i3c_DevList devList =
    { .numDevices = 1, .list = &device };
    result = i3c_ioctl(I3C_INSTANCE2, I3C_IOCTL_TARGET_ATTACH, &devList);
    TEST_ASSERT_EQUAL(0, result);

    // Step 3: Initialize the I3C bus
    result = i3c_ioctl(I3C_INSTANCE2, I3C_IOCTL_BUS_INIT, NULL);
    TEST_ASSERT_EQUAL(0, result);

    // Step 4: Fill a buffer with an incremental pattern
    fill_pattern(writeBuffer1, sizeof(writeBuffer1), 0x00);
    for ( i = 0; i < 64; i++ )
    {
        xferData[ 2 + i ] = writeBuffer1[ i ];
    }

    // Step 5: Write the buffer to the EEPROM page by page
    struct i3c_XferRequest xferRequest;
    xferRequest.read = false;
    for ( offset = 0; offset < 2048; offset += 64 )
    {
        xferData[ 0 ] = (offset >> 8) & 0xFF;
        xferData[ 1 ] = offset & 0xFF;
        xferRequest.buffer = xferData;
        xferRequest.length = 66;
        result = i3c_TransferSync(I3C_INSTANCE2, device.staticAddress,
                &xferRequest, 1, true);
        TEST_ASSERT_EQUAL(0, result);
        vTaskDelay(pdMS_TO_TICKS(5));
    }

    // Step 6: Fill the buffer with a different incremental pattern
    fill_pattern(writeBuffer2, sizeof(writeBuffer2), 0x80);
    for(i = 0; i < 64; i++)
    {
        xferData[2 + i] = writeBuffer2[i];
    }

    for(i = 0; i < 2048; i += 64)
    {
        memcpy(&cmpBuff1[i], writeBuffer2, 64);
    }

    // Step 7: Write the buffer to the EEPROM again page by page
    xferRequest.read = false;
    for ( offset = 0; offset < 2048; offset += 64 )
    {
        xferData[ 0 ] = (offset >> 8) & 0xFF;
        xferData[ 1 ] = offset & 0xFF;
        xferRequest.buffer = xferData;
        xferRequest.length = 66;
        result = i3c_TransferSync(I3C_INSTANCE2, device.staticAddress,
                &xferRequest, 1, true);
        TEST_ASSERT_EQUAL(0, result);
        vTaskDelay(pdMS_TO_TICKS(5));
    }

    // Step 8: Read back the data from the EEPROM page by page
    xferCmd[ 0 ] = (0x00 >> 8) & 0xFF;
    xferCmd[ 1 ] = 0x00 & 0xFF;

    xferReq[ 0 ].buffer = xferCmd;
    xferReq[ 0 ].length = 2;
    xferReq[ 0 ].read = false;

    xferReq[ 1 ].buffer = readBuffer1;
    xferReq[ 1 ].length = 2048;
    xferReq[ 1 ].read = true;

    result = i3c_TransferSync(I3C_INSTANCE2, device.staticAddress, xferReq,
            2, true);
    TEST_ASSERT_EQUAL(0, result);

    // Step 9: Verify the read data
    bool verify_result = compare_pattern(cmpBuff1, readBuffer1,
            2048);
    TEST_ASSERT_TRUE(verify_result);
}

TEST( TEST_IOT_I3C, test_SocfpgaI3c_write_read_read_read_read_2KB )
{
    int32_t result;
    uint32_t i;

    // Step 1: Open the I3C controller
    result = i3c_open(I3C_INSTANCE2);
    TEST_ASSERT_EQUAL(0, result);

    // Step 2: Attach devices to the I3C controller
    struct i3c_I3cDevice device =
    { .deviceId = 0x00, .staticAddress = 0x50, // Static address for ATMEL178 24C256 EEPROM
            .preferredDynamicAddress = 0x00, .dynamicAddress = 0x00 };
    struct i3c_DevList devList =
    { .numDevices = 1, .list = &device };
    result = i3c_ioctl(I3C_INSTANCE2, I3C_IOCTL_TARGET_ATTACH, &devList);
    TEST_ASSERT_EQUAL(0, result);

    // Step 3: Initialize the I3C bus
    result = i3c_ioctl(I3C_INSTANCE2, I3C_IOCTL_BUS_INIT, NULL);
    TEST_ASSERT_EQUAL(0, result);

    // Step 4: Fill a buffer with an incremental pattern
    fill_pattern(writeBuffer1, sizeof(writeBuffer1), 0x00);
    for ( i = 0; i < 64; i++ )
    {
        xferData[ 2 + i ] = writeBuffer1[ i ];
    }
    for (i = 0; i < 2048; i += 64)
    {
        memcpy(&cmpBuff1[i], writeBuffer1, 64);
    }

    // Step 5: Write the buffer to the EEPROM page by page
    struct i3c_XferRequest xferRequest;
    xferRequest.read = false;
    for ( offset = 0; offset < 2048; offset += 64 )
    {
        xferData[ 0 ] = (offset >> 8) & 0xFF;
        xferData[ 1 ] = offset & 0xFF;
        xferRequest.buffer = xferData;
        xferRequest.length = 66;
        result = i3c_TransferSync(I3C_INSTANCE2, device.staticAddress,
                &xferRequest, 1, true);
        TEST_ASSERT_EQUAL(0, result);
        vTaskDelay(pdMS_TO_TICKS(5));
    }

    // Step 6: Read back the data from the EEPROM page by page
    xferCmd[ 0 ] = (0x00 >> 8) & 0xFF;
    xferCmd[ 1 ] = 0x00 & 0xFF;

    xferReq[ 0 ].buffer = xferCmd;
    xferReq[ 0 ].length = 2;
    xferReq[ 0 ].read = false;

    xferReq[ 1 ].buffer = readBuffer1;
    xferReq[ 1 ].length = 2048;
    xferReq[ 1 ].read = true;

    result = i3c_TransferSync(I3C_INSTANCE2, device.staticAddress, xferReq,
            2, true);
    TEST_ASSERT_EQUAL(0, result);

    // Step 7: Verify the read data
    bool verify_result = compare_pattern(cmpBuff1, readBuffer1,
            2048);
    TEST_ASSERT_TRUE(verify_result);

    // Step 8: Perform two more read operations and verify the data
    for ( int i = 0; i < 2; i++ )
    {
        memset(readBuffer1, 0, sizeof(readBuffer1));
        xferCmd[ 0 ] = (0x00 >> 8) & 0xFF;
        xferCmd[ 1 ] = 0x00 & 0xFF;
    
        xferReq[ 0 ].buffer = xferCmd;
        xferReq[ 0 ].length = 2;
        xferReq[ 0 ].read = false;
    
        xferReq[ 1 ].buffer = readBuffer1;
        xferReq[ 1 ].length = 2048;
        xferReq[ 1 ].read = true;
    
        result = i3c_TransferSync(I3C_INSTANCE2, device.staticAddress, xferReq,
                2, true);
        TEST_ASSERT_EQUAL(0, result);
    
        // Step 9: Verify the read data
        bool verify_result = compare_pattern(cmpBuff1, readBuffer1,
                2048);
        TEST_ASSERT_TRUE(verify_result);
    }
}

TEST( TEST_IOT_I3C, test_SocfpgaI3c_write_read_write_read_2KB )
{
    int32_t result;
    uint32_t i;

    // Step 1: Open the I3C controller
    result = i3c_open(I3C_INSTANCE2);
    TEST_ASSERT_EQUAL(0, result);

    // Step 2: Attach devices to the I3C controller
    struct i3c_I3cDevice device =
    { .deviceId = 0x00, .staticAddress = 0x50, // Static address for ATMEL178 24C256 EEPROM
            .preferredDynamicAddress = 0x00, .dynamicAddress = 0x00 };
    struct i3c_DevList devList =
    { .numDevices = 1, .list = &device };
    result = i3c_ioctl(I3C_INSTANCE2, I3C_IOCTL_TARGET_ATTACH, &devList);
    TEST_ASSERT_EQUAL(0, result);

    // Step 3: Initialize the I3C bus
    result = i3c_ioctl(I3C_INSTANCE2, I3C_IOCTL_BUS_INIT, NULL);
    TEST_ASSERT_EQUAL(0, result);

    // Step 4: Fill a buffer with a random incremental pattern
    fill_pattern(writeBuffer1, sizeof(writeBuffer1), 6);
    for ( i = 0; i < 64; i++ )
    {
        xferData[ 2 + i ] = writeBuffer1[ i ];
    }
    for(i = 0; i < 2048; i += 64)
    {
        memcpy(&cmpBuff1[i], writeBuffer1, 64);
    }

    // Step 5: Write the buffer to the EEPROM page by page
    struct i3c_XferRequest xferRequest;
    xferRequest.read = false;
    for ( offset = 0; offset < 2048; offset += 64 )
    {
        xferData[ 0 ] = (offset >> 8) & 0xFF;
        xferData[ 1 ] = offset & 0xFF;
        xferRequest.buffer = xferData;
        xferRequest.length = 66;
        result = i3c_TransferSync(I3C_INSTANCE2, device.staticAddress,
                &xferRequest, 1, true);
        TEST_ASSERT_EQUAL(0, result);
        vTaskDelay(pdMS_TO_TICKS(5));
    }

    // Step 6: Read back the data from the EEPROM page by page
    xferCmd[ 0 ] = (0x00 >> 8) & 0xFF;
    xferCmd[ 1 ] = 0x00 & 0xFF;

    xferReq[ 0 ].buffer = xferCmd;
    xferReq[ 0 ].length = 2;
    xferReq[ 0 ].read = false;

    xferReq[ 1 ].buffer = readBuffer1;
    xferReq[ 1 ].length = 2048;
    xferReq[ 1 ].read = true;

    result = i3c_TransferSync(I3C_INSTANCE2, device.staticAddress, xferReq,
            2, true);
    TEST_ASSERT_EQUAL(0, result);

    // Step 7: Verify the read data
    bool verify_result = compare_pattern(cmpBuff1, readBuffer1,
            2048);
    TEST_ASSERT_TRUE(verify_result);

    // Step 8: Fill the buffer with a different random incremental pattern
    fill_pattern(writeBuffer2, sizeof(writeBuffer2), 7);
    for(i = 0; i < 64; i++)
    {
        xferData[2 + i] = writeBuffer2[i];
    }

    for(i = 0; i < 2048; i += 64)
    {
        memcpy(&cmpBuff1[i], writeBuffer2, 64);
    }

    // Step 9: Write the buffer to the EEPROM again page by page
    xferRequest.read = false;
    for ( offset = 0; offset < 2048; offset += 64 )
    {
        xferData[ 0 ] = (offset >> 8) & 0xFF;
        xferData[ 1 ] = offset & 0xFF;
        xferRequest.buffer = xferData;
        xferRequest.length = 66;
        result = i3c_TransferSync(I3C_INSTANCE2, device.staticAddress,
                &xferRequest, 1, true);
        TEST_ASSERT_EQUAL(0, result);
        vTaskDelay(pdMS_TO_TICKS(5));
    }

    // Step 10: Read back the data from the EEPROM page by page
    memset(readBuffer1, 0, sizeof(readBuffer1));
    xferCmd[ 0 ] = (0x00 >> 8) & 0xFF;
    xferCmd[ 1 ] = 0x00 & 0xFF;

    xferReq[ 0 ].buffer = xferCmd;
    xferReq[ 0 ].length = 2;
    xferReq[ 0 ].read = false;

    xferReq[ 1 ].buffer = readBuffer1;
    xferReq[ 1 ].length = 2048;
    xferReq[ 1 ].read = true;

    result = i3c_TransferSync(I3C_INSTANCE2, device.staticAddress, xferReq,
            2, true);
    TEST_ASSERT_EQUAL(0, result);

    // Step 11: Verify the read data
    verify_result = compare_pattern(cmpBuff1, readBuffer1,
            sizeof(writeBuffer2));
    TEST_ASSERT_TRUE(verify_result);
}

TEST( TEST_IOT_I3C, test_SocfpgaI3c_write_write_read_read_2KB )
{
    int32_t result;
    uint32_t i;

    // Step 1: Open the I3C controller
    result = i3c_open(I3C_INSTANCE2);
    TEST_ASSERT_EQUAL(0, result);

    // Step 2: Attach devices to the I3C controller
    struct i3c_I3cDevice device =
    { .deviceId = 0x00, .staticAddress = 0x50, // Static address for ATMEL178 24C256 EEPROM
            .preferredDynamicAddress = 0x00, .dynamicAddress = 0x00 };
    struct i3c_DevList devList =
    { .numDevices = 1, .list = &device };
    result = i3c_ioctl(I3C_INSTANCE2, I3C_IOCTL_TARGET_ATTACH, &devList);
    TEST_ASSERT_EQUAL(0, result);

    // Step 3: Initialize the I3C bus
    result = i3c_ioctl(I3C_INSTANCE2, I3C_IOCTL_BUS_INIT, NULL);
    TEST_ASSERT_EQUAL(0, result);

    // Step 4: Fill a buffer with an incremental pattern
    fill_pattern(writeBuffer1, sizeof(writeBuffer1), 8);
    for ( i = 0; i < 64; i++ )
    {
        xferData[ 2 + i ] = writeBuffer1[ i ];
    }
    for(i = 0; i < 2048; i += 64)
    {
        memcpy(&cmpBuff1[i], writeBuffer1, 64);
    }

    // Step 5: Write the buffer to the EEPROM page by page
    struct i3c_XferRequest xferRequest;
    xferRequest.read = false;
    for ( offset = 0; offset < 2048; offset += 64 )
    {
        xferData[ 0 ] = (offset >> 8) & 0xFF;
        xferData[ 1 ] = offset & 0xFF;
        xferRequest.buffer = xferData;
        xferRequest.length = 66;
        result = i3c_TransferSync(I3C_INSTANCE2, device.staticAddress,
                &xferRequest, 1, true);
        TEST_ASSERT_EQUAL(0, result);
        vTaskDelay(pdMS_TO_TICKS(5));
    }

    // Step 6: Fill the buffer with a different random incremental pattern
    fill_pattern(writeBuffer2, sizeof(writeBuffer2), 9);
    for ( i = 0; i < 64; i++ )
    {
        xferData[ 2 + i ] = writeBuffer2[ i ];
    }
    for(i = 0; i < 2048; i += 64)
    {
        memcpy(&cmpBuff2[i], writeBuffer2, 64);
    }

    // Step 7: Write the buffer to the EEPROM again page by page
    xferRequest.read = false;
    for ( offset = 2048; offset < 4096; offset += 64 )
    {
        xferData[ 0 ] = (offset >> 8) & 0xFF;
        xferData[ 1 ] = offset & 0xFF;
        xferRequest.buffer = xferData;
        xferRequest.length = 66;
        result = i3c_TransferSync(I3C_INSTANCE2, device.staticAddress,
                &xferRequest, 1, true);
        TEST_ASSERT_EQUAL(0, result);
        vTaskDelay(pdMS_TO_TICKS(5));
    }

    // Step 8: Read back the data written into 0x0000
    xferCmd[ 0 ] = (0x00 >> 8) & 0xFF;
    xferCmd[ 1 ] = 0x00 & 0xFF;

    xferReq[ 0 ].buffer = xferCmd;
    xferReq[ 0 ].length = 2;
    xferReq[ 0 ].read = false;

    xferReq[ 1 ].buffer = readBuffer1;
    xferReq[ 1 ].length = 2048;
    xferReq[ 1 ].read = true;

    result = i3c_TransferSync(I3C_INSTANCE2, device.staticAddress, xferReq,
            2, true);
    TEST_ASSERT_EQUAL(0, result);

    // Step 9: Verify the read data
    bool verify_result = compare_pattern(cmpBuff1, readBuffer1,
            2048);
    TEST_ASSERT_TRUE(verify_result);

    // Step 10: Read back the data written into 0x0800
    xferCmd[ 0 ] = (0x0800 >> 8) & 0xFF;
    xferCmd[ 1 ] = 0x0800 & 0xFF;

    xferReq[ 0 ].buffer = xferCmd;
    xferReq[ 0 ].length = 2;
    xferReq[ 0 ].read = false;

    xferReq[ 1 ].buffer = readBuffer2;
    xferReq[ 1 ].length = 2048;
    xferReq[ 1 ].read = true;

    result = i3c_TransferSync(I3C_INSTANCE2, device.staticAddress, xferReq,
            2, true);
    TEST_ASSERT_EQUAL(0, result);

    // Step 11: Verify the read data
    verify_result = compare_pattern(cmpBuff2, readBuffer2,
            2048);
    TEST_ASSERT_TRUE(verify_result);
}

TEST(TEST_IOT_I3C, test_ioctl_get_dynamic_address)
{
    int32_t result;
    struct i3c_I3cDevice connected_device =
    {
        .staticAddress = 0,
        .deviceId = 0x0000020800B30,
        .preferredDynamicAddress = 0
    };
    struct i3c_DevList devList = { .numDevices = 1, .list = &connected_device, };

    result = i3c_open(I3C_INSTANCE2);
    TEST_ASSERT_EQUAL(0, result);

    result = i3c_ioctl(I3C_INSTANCE2, I3C_IOCTL_TARGET_ATTACH, &devList);
    TEST_ASSERT_EQUAL(0, result);

    result = i3c_ioctl(I3C_INSTANCE2, I3C_IOCTL_BUS_INIT, &connected_device);
    TEST_ASSERT_EQUAL(0, result);

    result = i3c_ioctl(I3C_INSTANCE2, I3C_IOCTL_GET_DYNADDRESS, &connected_device);
    TEST_ASSERT_EQUAL(0, result);
}

/**
 * @brief Function to execute for test.
 */
void i3c_test()
{
    UNITY_BEGIN();

    RUN_TEST_GROUP(TEST_IOT_I3C);

    UNITY_END();
}

/*-----------------------------------------------------------*/

/**
 * @brief Function to define which tests to execute as part of this group.
 */
TEST_GROUP_RUNNER( TEST_IOT_I3C )
{
    /* data transfer tests */
    RUN_TEST_CASE(TEST_IOT_I3C, test_SocfpgaI3c_write_read_verify_64_bytes);
    RUN_TEST_CASE(TEST_IOT_I3C, test_SocfpgaI3c_write_read_verify_4KB);
    RUN_TEST_CASE(TEST_IOT_I3C, test_SocfpgaI3c_write_write_read_2KB);
    RUN_TEST_CASE(TEST_IOT_I3C, test_SocfpgaI3c_write_read_read_read_read_2KB);
    RUN_TEST_CASE(TEST_IOT_I3C, test_SocfpgaI3c_write_read_write_read_2KB);
    RUN_TEST_CASE(TEST_IOT_I3C, test_SocfpgaI3c_write_write_read_read_2KB);

    /* fuzzing tests */
    RUN_TEST_CASE(TEST_IOT_I3C, test_SocfpgaI3c_open_valid_instance);
    RUN_TEST_CASE(TEST_IOT_I3C, test_SocfpgaI3c_open_invalid_instance);
    RUN_TEST_CASE(TEST_IOT_I3C, test_SocfpgaI3c_ioctl_target_attach_valid);
    RUN_TEST_CASE(TEST_IOT_I3C, test_SocfpgaI3c_ioctl_bus_init_valid);
    RUN_TEST_CASE(TEST_IOT_I3C, test_SocfpgaI3c_ioctl_do_daa_valid);
    RUN_TEST_CASE(TEST_IOT_I3C, test_ioctl_get_dynamic_address);
    RUN_TEST_CASE(TEST_IOT_I3C, test_SocfpgaI3c_ioctl_invalid_instance);
    RUN_TEST_CASE(TEST_IOT_I3C, test_SocfpgaI3c_TransferSync_transfer_unsuccessful);
    RUN_TEST_CASE(TEST_IOT_I3C, test_SocfpgaI3c_open_boundary_values);
    RUN_TEST_CASE(TEST_IOT_I3C, test_SocfpgaI3c_open_random_values);
    RUN_TEST_CASE(TEST_IOT_I3C, test_SocfpgaI3c_open_repeated_calls);
}

/*-----------------------------------------------------------*/
