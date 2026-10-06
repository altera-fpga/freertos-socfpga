/*
 * Common IO - basic V1.0.0
 * Copyright (C) 2020 Amazon.com, Inc. or its affiliates. All Rights Reserved.
 * Copyright (C) 2025-2026 Altera Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * http://aws.amazon.com/freertos
 * http://www.FreeRTOS.org
 *
 * Modified for SoC FPGA
 */

#ifndef __SOCFPGA_WATCHDOG_H__
#define __SOCFPGA_WATCHDOG_H__

/*
    File: Watchdog Timer Driver

    File for the HAL APIs of the WatchDog timer called by application layer.

    APIs for SoC FPGA Watchdog Timer.

    This is the Watchdog Timer block implementation for SoC FPGA. It provides
    APIs for configuring the Watchdog Timer, setting the bark and bite time,
    and starting or stopping the timer. For example usage, refer to Watchdog
    Timer sample application.
*/

#include "socfpga_defines.h"


/*
    Group: Macros

    Watchdog Timer specific macros
*/

/*
    Enum: wdt_status_t

    WatchDog timer status values
*/
typedef enum
{
    WDT_STOPPED,     /* WatchDog is stopped */
    WDT_RUNNING,     /* WatchDog is running */
    WDT_EXPIRED  /* WatchDog timeout timer expired */
} wdt_status_t;

/*
    Enum: wdt_timeout_config_t

    WatchDog timer timeout behavior setting
*/
typedef enum
{
    WDT_TIMEOUT_RST,    /* Reset the device when WatchDog timeout timer expires */
    WDT_TIMEOUT_INTR /* Generate Interrupt when WatchDog timeout timer expires */
} wdt_timeout_config_t;

/*
    Variable: wdt_descriptor

    WatchDog descriptor type defined in the source file.
*/
struct wdt_descriptor;

/*
    Type: wdt_handle_t

    wdt_handle_t type is the WatchDog handle returned by calling wdt_open()
    this is initialized in open and returned to caller. Caller must pass this
    pointer to the rest of the APIs.
*/
typedef struct wdt_descriptor *wdt_handle_t;

/*
    Enum: wdt_ioctl_t

    Ioctl request types.

    Notes:

        InitTimeoutTime is the initial timeout duration for the watchdog timer
        immediately
        after it is enabled.
        TimeoutTime specifies the duration for the watchdog timer.
        The timer begins counting after the initial timeout period is
        triggered.
        Warning: the InitTimeoutTime must be greater than or equal to the
        TimeoutTime
*/
typedef enum
{
    WDT_SET_INIT_TIMEOUT,     /* Set the WatchDog initial timeout value.
                               * The time shall be in milliseconds */
    WDT_GET_INIT_TIMEOUT,     /* Get the WatchDog initial timeout value.
                              * The returned values are in milliseconds */
    WDT_SET_TIMEOUT,     /* Set the WatchDog expire time (timeout value).
                          * The time shall be in milliseconds */
    WDT_GET_TIMEOUT,     /* Get the WatchDog expire time (timeout value).
                          * The returned values are in milliseconds */
    WDT_GET_STATUS,       /* Returns the WatchDog timer status of type <wdt_status_t> */

    WDT_SET_TIMEOUT_BEHAVIOUR /* Set the WatchDog timeout behavior. Takes <wdt_timeout_config_t> type */
    /* Warning: Not all platforms may support interrupt generation. */
} wdt_ioctl_t;

/*
    Callback: wdt_callback_t

    WatchDog notification callback type. This callback is passed to the driver
    by using wdt_set_callback API. This callback is used for warning
    notification when watchdog timer expires. This callback will be called
    only if WatchdogTimeout behavior is set to interrupt.

    Parameters:

        param - User Context passed when setting the callback. This is not
            used by the driver, but just passed back to the user in the
            callback.
*/
typedef void (*wdt_callback_t)(void *param);

/*
    Group: Functions

    Entry points for the Watchdog Timer driver.
*/

/*
    Function: wdt_open

    wdt_open is used to initialize the WatchDog, This function will stop the
    timer if it was started and resets the timer if any was configured
    earlier.

    Parameters:

        instance - The instance of the WatchDog to initialize.

    Returns:

        - Handle to wdt_handle_t on success
        - NULL if
            - invalid instance
            - instance is already open
*/
wdt_handle_t wdt_open(uint32_t instance);

/*
    Function: wdt_start

    wdt_start is used to start the WatchDog timer counter. WatchDog expiry
    (timeout) time must be set before starting the WatchDog counter. Use the
    eSetWatchdogInitTimeout IOCTL to set the initial timeout period and the
    eSetWatchdogTimeout IOCTL to define the regular timeout duration.

    Parameters:

        hwdt - handle to WatchDog interface returned in wdt_open.

    Returns:

        - 0: on success
        - -EINVAL:  if hwdt is NULL
        - -ENODATA: if init_timeout or timeout time has not been set.
*/
int32_t wdt_start(const wdt_handle_t hwdt);

/*
    Function: wdt_stop

    wdt_stop is used to stop and resets the WatchDog timer counter. After
    stopping the timer and before starting the timer again, expire time must
    be set.

    Parameters:

        hwdt - handle to WatchDog interface returned in wdt_open.

    Returns:

        - 0: on success
        - -EINVAL: if hwdt is NULL
*/
int32_t wdt_stop(const wdt_handle_t hwdt);

/*
    Function: wdt_restart

    wdt_restart is used to restart the WatchDog timer to the originally
    programmed values.This function should be called periodically from a
    thread or the main loop to refresh the watchdog timer and prevent it from
    expiring. Failure to call this function within the configured timeout
    window may result in a system reset.

    Parameters:

        hwdt - handle to WatchDog interface returned in wdt_open.

    Returns:

        - 0: on success
        - -EINVAL:  if hwdt is NULL
        - -ENODATA: if watchdog init_timeout or timeout time have not been set.

    Notes:

        Ensure that the interval between calls is shorter than the configured
        watchdog timeout.
        The main difference b/w wdt_start and wdt_restart
        APIs are, the former requires the time values are set using the IOCTLs
        and the latter
        re-uses the already programmed values and re-programs them. If
        restart_timer is used
        without first setting the timers, it will return an error.
*/
int32_t wdt_restart(const wdt_handle_t hwdt);

/*
    Function: wdt_set_callback

    wdt_set_callback is used to set the callback to be called when
    init_timeout time reaches the WatchDog counter or if the timeout time is
    configured to generate interrupt. The caller must set the timers using
    IOCTL and start the timer for the callback to be called back.

    Parameters:

        hwdt - handle to WatchDog interface returned in wdt_open.
        callback - The callback function to be called.
        param - The user context to be passed when callback is called.

    Notes:

        A single callback is assigned per instance when eWdtTimeoutInterrupt
        is configured either
        for the initial timeout or upon expiry of the WatchDog counter's
        timeout period.
        Newly set callback overrides the one previously set
        Warning: If input handle or if callback function is NULL, this
        function silently takes no action.
*/
void wdt_set_callback(const wdt_handle_t hwdt, wdt_callback_t callback,
        void *param);

/*
    Function: wdt_ioctl

    wdt_ioctl is used to configure the WatchDog timer properties like the
    WatchDog timeout value, WatchDog timeout behaviour etc.

    Parameters:

        hwdt - handle to WatchDog interface returned in wdt_open.
        cmd - configuration request of type <wdt_ioctl_t>
        buf - the configuration buffer to hold the request or response of
            IOCTL.

    Returns:

        - 0: on success
        - -EINVAL: if
            - hwdt is NULL
            - xRequest is invalid
            - pvBuffer is NULL
            - error in timeout behavior set
*/
int32_t wdt_ioctl(const wdt_handle_t hwdt, wdt_ioctl_t cmd, void *const buf);

/*
    Function: wdt_close

    wdt_close is used to de-initializes the WatchDog, stops the timer if it
    was started and resets the timer value.

    Parameters:

        hwdt - handle to WatchDog interface returned in wdt_open.

    Returns:

        - 0: on success
        - -EINVAL: if
            - hwdt == NULL
            - hwdt is not open (previously closed).
            - failed to stop wdt
*/
int32_t wdt_close(const wdt_handle_t hwdt);
/* end of group wdt_fns */
/* end of group wdt */

#endif /* __SOCFPGA_WATCHDOG_H__ */
