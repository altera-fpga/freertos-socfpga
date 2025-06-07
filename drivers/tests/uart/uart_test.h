#ifndef _UART_TEST_H_
#define _UART_TEST_H_

#include "socfpga_uart.h"
#include <errno.h>

void uart_run_test();
void test_uart();

/* Function names */
#define iot_uart_open uart_open
#define iot_uart_set_callback uart_set_callback
#define iot_uart_read_sync uart_read_sync
#define iot_uart_read_async uart_read_async
#define iot_uart_write_sync uart_write_sync
#define iot_uart_write_async uart_write_async
#define iot_uart_ioctl uart_ioctl
#define iot_uart_close uart_close
#define iot_uart_cancel uart_cancel

/* Macros */
#define IOT_UART_BAUD_RATE_DEFAULT UART_BAUD_RATE_DEFAULT

/* typedef */
#define IotUARTOperationStatus_t UartOperationStatus_t
#define IotUARTParity_t UartParity_t
#define IotUARTStopBits_t UartStopBits_t
#define IotUARTCallback_t UartCallback_t
#define IotUARTDescriptor UartDescriptor
#define IotUARTHandle_t UartHandle_t
#define IotUARTIoctlRequest_t UartIoctlRequest_t
#define IotUARTConfig_t UartConfig_t

#endif /* _UART_TEST_H_ */
