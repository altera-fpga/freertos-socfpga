#ifndef WDT_TEST_H
#define WDT_TEST_H

#include "socfpga_watchdog.h"

/*Functions*/
#define iot_watchdog_ioctl            wdt_ioctl
#define iot_watchdog_restart          wdt_restart
#define iot_watchdog_open             wdt_open
#define iot_watchdog_close            wdt_close
#define iot_watchdog_start            wdt_start
#define iot_watchdog_stop             wdt_stop
#define iot_watchdog_set_callback     wdt_set_callback

/*Enums*/
#define IotWatchdogHandle_t        WdtHandle_t
#define IotWatchdogStatus_t        WdtStatus_t
#define IotWatchdogBiteConfig_t    WdtBiteConfig_t
#define IotWatchdogDescriptor      WdtDescriptor
#define IotWatchdogIoctlRequest_t  WdtIoctlRequest_t

#define IotWatchdogCallback_t      WdtCallback_t

#endif
