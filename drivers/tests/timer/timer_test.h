#ifndef _TIMER_TEST_H_
#define _TIMER_TEST_H_

#include "socfpga_timer.h"

/*Api*/
#define iot_timer_open                            timer_open
#define iot_timer_set_callback                    timer_set_callback
#define iot_timer_start                           timer_start
#define iot_timer_stop                            timer_stop
#define iot_timer_get_value                       timer_get_value_us
#define iot_timer_delay                           timer_set_period_us
#define iot_timer_close                           timer_close
#define iot_timer_cancel                          timer_close

/*Structure and enums*/
#define IotTimerDescriptor                         TimerDescriptor
#define IotTimerCallback_t                         TimerCallback_t
#define IotTimerHandle_t                           Timer_Handle_t

#endif
