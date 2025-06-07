#ifndef _QSPI_TEST_H_
#define _QSPI_TEST_H_

#include "socfpga_flash.h"

#define iot_flash_erase                      flash_erase_sectors
#define iot_flash_open                       flash_open
#define iot_flash_set_callback               flash_set_callback
#define iot_flash_read_sync                  flash_read_sync
#define iot_flash_write_sync                 flash_write_sync
#define iot_flash_read_async                 flash_read_async
#define iot_flash_write_async                flash_write_async
#define iot_flash_close                      flash_close

#define IotFlashDescriptor                   FlashDescriptor
#define IotFlashHandle_t                     FlashHandle_t
#define IotFlashCallback_t                   FlashCallback_t


#endif
