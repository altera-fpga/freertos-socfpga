#include "uart_test_print.h"

#include "socfpga_uart.h"

#define COMPLETE_STR ("TESTS COMPLETED\n")

uint8_t unity_buffer[ 1000 ] =
{ 0 };
uint32_t buf_index = 0;

int uart_putchar( uint8_t ch )
{
    unity_buffer[ buf_index++ ] = ch;
    return ch;
}

void dump_buffer()
{
    UartHandle_t xHandle;
    UartConfig_t xConfig;

    xHandle = uart_open(0);

    xConfig.ucWordlength = 8;
    xConfig.ulBaudrate = UART_BAUD_RATE_DEFAULT;
    xConfig.xParity = eUartParityNone;
    xConfig.xStopbits = eUartStopBitsOne;

    uart_ioctl(xHandle, eUartSetConfig, &xConfig);

    uart_write_sync(xHandle, unity_buffer, buf_index);
    uart_write_sync(xHandle, (uint8_t*)COMPLETE_STR, sizeof(COMPLETE_STR));

    uart_close(xHandle);
}
