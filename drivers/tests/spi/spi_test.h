
#ifndef DRIVERS_TESTS_SPI_SPI_TEST_H_
#define DRIVERS_TESTS_SPI_SPI_TEST_H_

#include "socfpga_spi.h"

/*SPI HAL apis*/
#define iot_spi_open                       spi_open
#define iot_spi_set_callback               spi_set_callback
#define iot_spi_select_slave               spi_select_slave
#define iot_spi_read_sync                  spi_read_sync
#define iot_spi_write_sync                 spi_write_sync
#define iot_spi_read_async                 spi_read_async
#define iot_spi_write_async                spi_write_async
#define iot_spi_transfer_sync              spi_transfer_sync
#define iot_spi_transfer_async             spi_transfer_async
#define iot_spi_ioctl                      spi_ioctl
#define iot_spi_close                      spi_close
#define iot_spi_cancel                     spi_cancel

/*Structure & handle*/
#define IotSPIDescriptor                   SpiDescriptor
#define IotSPIHandle_t                     SpiHandle_t
#define IotSPICallback_t                   SpiCallback_t
#define IotSPIMasterConfig_t	           SpiMasterConfig_t
#define IotSPIBitOrder_t                   SpiBitOrder_t
#define IotSPIMode_t                       SpiMode_t
#define IotSPITransactionStatus_t          SpiTransactionStatus_t
#define IotSPIIoctlRequest_t               SpiIoctlRequest_t



#endif /* DRIVERS_TESTS_SPI_SPI_TEST_H_ */
