#include "tusb.h"
#include "tusb_config.h"
#include "usb_main.h"
#include "msc_app.h"
#include "osal_log.h"

void wait_for_usb_mount_complete()
{
    while(1)
    {
        if( is_msc_mount_complete() == 1 )
        {
            break;
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }

    //printf("\r\n MSC mount completed\r\n");
}

void usb3_task(void *arg)
{
    (void)arg;

    tusb_rhport_init_t host_init = {
        .role = TUSB_ROLE_HOST,
        .speed = TUSB_SPEED_AUTO
    };

    if( !tusb_init(USB3_SS_PORT, &host_init))   /*initialize host stack for usb3 port*/
    {
        ERROR("Error in initialising usb3 port");
        vTaskSuspend(NULL);
        /*suspend the task*/
    }

    if( !tusb_init(USB3_HS_PORT, &host_init))   /*initialize host stack for usb3 port*/
    {
        ERROR("Error in initialising usb3 port");
        vTaskSuspend(NULL);
        /*suspend the task*/
    }

    INFO("USB3.1 initialized successfully");

    while(1)
    {
        tuh_task();
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

