#ifndef GPIO_TEST_H_
#define GPIO_TEST_H_

/*GPIO HAL apis*/
#define iot_gpio_open                       gpio_open
#define iot_gpio_set_callback               gpio_set_callback
#define iot_gpio_read_sync                  gpio_read_sync
#define iot_gpio_write_sync                 gpio_write_sync
#define iot_gpio_read_async                 gpio_read_async
#define iot_gpio_write_async                gpio_write_async
#define iot_gpio_ioctl                      gpio_ioctl
#define iot_gpio_close                      gpio_close
#define iot_gpio_cancel                     gpio_cancel

/*Structure & handle*/
#define IotGpioDescriptor                   GpioDescriptor
#define IotGpioHandle_t                     GpioHandle_t
#define IotGpioCallback_t                   GpioCallback_t

#define IotGpioConfig_t                     int
#define IotGpioDirection_t                  GpioDirection_t
#define IotGpioInterrupt_t                  GpioInterrupt_t

void gpioTest();

#endif /* GPIO_TEST_H_ */
