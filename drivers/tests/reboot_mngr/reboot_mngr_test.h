#ifndef _REBOOT_TEST_H_
#define _REBOOT_TEST_H_

#include "socfpga_reboot_mngr.h"

/* API */
#define iot_reboot_set_callback reboot_mngr_set_callback
#define iot_reboot_do_reboot reboot_mngr_do_reboot

/* return values */
#define IOT_REBOOT_SUCCESS REBOOT_SUCCESS
#define IOT_REBOOT_FAILURE REBOOT_FAILURE
#define IOT_REBOOT_WARM_REBOOT WARM_REBOOT
#define IOT_REBOOT_COLD_REBOOT COLD_REBOOT

/* structures and enums */
#define IotRebootCallback_t reboot_callback_t

#endif /* _REBOOT_TEST_H_ */
