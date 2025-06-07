/*
 * Copyright (c) 2024, Intel Corporation.
 *
 * SPDX-License-Identifier: MIT
 *
 */

#ifndef __TEST_MAIN__
#define __TEST_MAIN__

/* Enable the macro to enable the tests
 * */

#define SOCFPGA_TEST_DRIVER_DMA     	0
#define SOCFPGA_TEST_DRIVER_WDT     	0
#define SOCFPGA_TEST_DRIVER_TIMER   	0
#define SOCFPGA_TEST_DRIVER_SDMMC   0
#define SOCFPGA_TEST_DRIVER_FATFS   0
#define SOCFPGA_TEST_DRIVER_FCS   0
#define SOCFPGA_TEST_DRIVER_REBOOT  0
#define SOCFPGA_TEST_DRIVER_QSPI  0
#define SOCFPGA_TEST_DRIVER_SDM_MAILBOX 0
#define SOCFPGA_TEST_DRIVER_USB3  0

/* For GPIO tests,
 * Ensure the test is conducted with HPS expansion board(HPS EB) connected
 * Test failure will occur for other boards as the pin used for test might
 * not be enabled for gpio.
 * */
#define SOCFPGA_TEST_DRIVER_GPIO    	0

/* For UART tests,
 * Add cflag -DUNITY_OUTPUT_CHAR=uart_putchar
 * Disable configENABLE_CONSOLE_UART (set to zero, FreeRTOSConfig.h)
 * */
#define SOCFPGA_TEST_DRIVER_UART    	0

/*Enable the RSU tests with RSU enabled jic images*/
#define SOCFPGA_TEST_DRIVER_RSU      0

/* For I2C test,
 * Make sure the right daughter card is inserted
 * */
#define SOCFPGA_TEST_DRIVER_I2C     	0

/* For SPI test,
 * Make sure the right daughter card is inserted
 * */
#define SOCFPGA_TEST_DRIVER_SPI     0

/*Requires IOSSM enabled SOF*/
#define SOCFPGA_TEST_DRIVER_IOSSM       1

/*Enable the SEU tests with seu enabled SOF*/
#define SOCFPGA_TEST_DRIVER_SEU         0

/* For I3C test,
 * Make sure the right daughter card is inserted
 * */
#define SOCFPGA_TEST_DRIVER_I3C         0

#define SOCFPGA_TEST_DRIVER_FPGA_MANAGER 0
#define SOCFPGA_TEST_DRIVER_BRIDGE 0

#endif
