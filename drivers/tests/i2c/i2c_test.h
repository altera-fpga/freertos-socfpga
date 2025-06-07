#ifndef _I2C_TEST_H_
#define _I2C_TEST_H_

#include "socfpga_i2c.h"

/*GPIO HAL apis*/
#define iot_i2c_open                       i2c_open
#define iot_i2c_set_callback               i2c_set_callback
#define iot_i2c_read_sync                  i2c_read_sync
#define iot_i2c_write_sync                 i2c_write_sync
#define iot_i2c_read_async                 i2c_read_async
#define iot_i2c_write_async                i2c_write_async
#define iot_i2c_ioctl                      i2c_ioctl
#define iot_i2c_close                      i2c_close
#define iot_i2c_cancel                     i2c_cancel

/*Structure & handle*/
#define IotI2CDescriptor                   I2cDescriptor
#define IotI2CHandle_t                     I2cHandle_t
#define IotI2CCallback_t                   I2cCallback_t
#define IotI2CConfig_t                     I2cConfig_t
#define IotI2COperationStatus_t            I2cOperationStatus_t

#define IOT_I2C_STANDARD_MODE_BPS          I2C_STANDARD_MODE_BPS
#define IOT_I2C_FAST_MODE_BPS              I2C_FAST_MODE_BPS
#define IOT_I2C_FAST_PLUS_MODE_BPS         I2C_FAST_MODE_PLUS_BPS

#endif

