/*
 * Common IO - basic V1.0.0
 * Copyright (C) 2020 Amazon.com, Inc. or its affiliates.  All Rights Reserved.
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
 */

/*
 * SPDX-FileCopyrightText: Copyright (C) 2025 Altera Corporation
 *
 * SPDX-License-Identifier: MIT-0
 */

#ifndef __SOCFPGA_WATCHDOG_H__
#define __SOCFPGA_WATCHDOG_H__

/**
 * @file    socfpga_watchdog.h
 * @brief   File for the HAL APIs of the WatchDog timer called by application layer.
 */

#include "socfpga_defines.h"

/**
 * @defgroup wdt Watchdog Timer
 * @ingroup drivers
 * @{
 */

/**
 * @defgroup wdt_fns Functions
 * @ingroup wdt
 * Watchdog Timer HAL APIs
 */

/**
 * @defgroup wdt_structs Structures 
 * @ingroup wdt
 * Watchdog Timer Specific Structures
 */

/**
 * @defgroup wdt_enums Enumerations
 * @ingroup wdt
 * Watchdog Timer Specific Enumerations
 */

/**
 * @defgroup wdt_macros Macros
 * @ingroup wdt
 * Watchdog Timer specific macros 
 */

/**
 * @brief WatchDog timer status values
 * @ingroup wdt_enums
 */
typedef enum
{
    eWdtTimerStopped,     /*!< WatchDog is stopped */
    eWdtTimerRunning,     /*!< WatchDog is running */
    eWdtTimerBarkExpired, /*!< WatchDog bark timer expired */
    eWdtTimerBiteExpired  /*!< WatchDog bite timer expired */
} WdtStatus_t;

/**
 * @brief WatchDog timer bite behavior setting
 * @ingroup wdt_enums
 */
typedef enum
{
    eWdtBiteTimerReset,    /*!< Reset the device when WatchDog bite timer expires */
    eWdtBiteTimerInterrupt /*!< Generate Interrupt when WatchDog bite timer expires */
} WdtBiteConfig_t;

/**
 * @brief   WatchDog descriptor type defined in the source file.
 * @ingroup wdt_structs
 */
struct WdtDescriptor;

/**
 * @brief   WdtHandle_t type is the WatchDog handle returned by calling wdt_open()
 *          this is initialized in open and returned to caller. Caller must pass this pointer
 *          to the rest of the APIs.
 * @ingroup wdt_structs
 */
typedef struct WdtDescriptor *WdtHandle_t;

/**
 * @brief Ioctl request types.
 * @ingroup wdt_enums
 *
 * @note BarkTime is the time before a warning signaled in the form of an interrupt call to the
 * set callback function.
 * @note BiteTime is the time before a critical condition is signaled in the form of an interrupt
 * (If supported and configured), and usually ends with the system being reset.
 *
 * @warning the BarkTime must be less than or equal to the BiteTime
 */
typedef enum
{
    eSetWatchdogBarkTime,     /*!< Set the WatchDog warning time (bark value).
                               * The time shall be selected from the enumeration constants defined
                               * by #WdtTimeout_t */
    eGetWatchdogBarkTime,     /*!< Get the WatchDog warning time (bark value).
                               * The returned values are from the enumeration constants defined
                               * by #WdtTimeout_t */
    eSetWatchdogBiteTime,     /*!< Set the WatchDog expire time (bite value).
                               * The time shall be selected from the enumeration constants defined
                               * by #WdtTimeout_t */
    eGetWatchdogBiteTime,     /*!< Get the WatchDog expire time (bite value).
                               * The returned values are from the enumeration constants defined
                               * by #WdtTimeout_t */
    eGetWatchdogStatus,       /*!< Returns the WatchDog timer status of type #WdtStatus_t */
    eSetWatchdogBiteBehaviour /*!< Set the WatchDog bite behavior. Takes #WdtBiteConfig_t type */
    /*!< @warning  Not all platforms may support interrupt generation. */
} WdtIoctlRequest_t;

/**
 * @addtogroup wdt_macros
 * @{
 */
/**
 * @brief WatchDog timer timeout values
 */
#define WDT_TIMEOUT_0P6MSEC      0U  /*!< 0.6 ms timeout */
#define WDT_TIMEOUT_1P3MSEC      1U /*!< 1.3 ms timeout */
#define WDT_TIMEOUT_2P6MSEC      2U /*!< 2.6 ms timeout */
#define WDT_TIMEOUT_5P1MSEC      3U /*!< 5.1 ms timeout */
#define WDT_TIMEOUT_10MSEC       4U /*!< 10 ms timeout */
#define WDT_TIMEOUT_20MSEC       5U /*!< 20 ms timeout */
#define WDT_TIMEOUT_40MSEC       6U /*!< 40 ms timeout */
#define WDT_TIMEOUT_80MSEC       7U /*!< 80 ms timeout */
#define WDT_TIMEOUT_160MSEC      8U /*!< 160 ms timeout */
#define WDT_TIMEOUT_320MSEC      9U /*!< 320 ms timeout */
#define WDT_TIMEOUT_640MSEC      10U /*!< 640 ms timeout */
#define WDT_TIMEOUT_1280MSEC     11U /*!< 1280 ms timeout */
#define WDT_TIMEOUT_2560MSEC     12U /*!< 2560 ms timeout */
#define WDT_TIMEOUT_5120MSEC     13U /*!< 5120 ms timeout*/
#define WDT_TIMEOUT_10000MSEC    14U /*!< 10000 ms timeout */
#define WDT_TIMEOUT_20000MSEC    15U /*!< 20000 ms timeout */
/**
 * @}
 */

/**
 * @brief   WatchDog notification callback type. This callback is passed
 *          to the driver by using wdt_set_callback API. This callback is used for
 *          warning notification when the bark timer or bite timer expires based on the configuration.
 *          Caller can check the status of the WatchDog timer by using #eGetWatchdogStatus IOCTL
 *@ingroup  wdt_fns
 * 
 * @warning The callback will only be called if WatchdogBark is supported, or if WatchdogBite behavior
 *          is set to interrupt and is supported.
 *
 * @param[in] pvUserContext User Context passed when setting the callback.
 *                          This is not used by the driver, but just passed back to the user
 *                          in the callback.
 */
typedef void (*WdtCallback_t)( void *pvUserContext );

/**
 * @addtogroup wdt_fns
 * @{
 */

/**
 * @brief   wdt_open is used to initialize the WatchDog,
 *          This function will stop the timer if it was started and resets the timer
 *          if any was configured earlier.
 *
 * @param[in]   lWatchdogInstance   The instance of the WatchDog to initialize.
 *
 * @return
 *   - Handle to WdtHandle_t on success
 *   - NULL if
 *      - invalid lWatchdogInstance
 *      - instance is already open
 */
WdtHandle_t wdt_open( uint32_t lWatchdogInstance );

/**
 * @brief   wdt_start is used to start the WatchDog timer counter.
 *          WatchDog expiry (bite) time must be set before starting the WatchDog counter.
 *          Set the bite time using eSetBiteTime IOCTL.
 *
 * @param[in]   pxWdtHandle handle to WatchDog interface returned in
 *                              wdt_open.
 *
 * @return
 *   - 0 on success
 *   - -EINVAL if pxWdtHandle is NULL
 *   - -ENODATA if bark or bite time has not been set.
 */
int32_t wdt_start( const WdtHandle_t pxWdtHandle );

/**
 * @brief   wdt_stop is used to stop and resets the WatchDog timer counter.
 *          After stopping the timer and before starting the timer again,
 *          expire time must be set.
 *
 * @param[in]   pxWdtHandle handle to WatchDog interface returned in
 *                              wdt_open.
 *
 * @return
 *   - 0 on success
 *   - -EINVAL if pxWdtHandle is NULL
 */
int32_t wdt_stop( const WdtHandle_t pxWdtHandle );

/**
 * @brief   wdt_restart is used to restart the WatchDog timer to the
 *          originally programmed values. This function is usually to service the watchdog
 *          periodically to prevent the watchdog from expiring. It may be called periodically
 *          from a thread or may be called from the bark time expiry callback function.
 *          The main difference b/w wdt_start and wdt_restart
 *          APIs are, the former requires the time values are set using the IOCTLs and the latter
 *          re-uses the already programmed values and re-programs them. If restart_timer is used
 *          without first setting the timers, it will return an error.
 *
 * @param[in]   pxWdtHandle handle to WatchDog interface returned in
 *                              wdt_open.
 *
 * @return
 *   - 0 on success
 *   - -EINVAL if pxWdtHandle is NULL
 *   - -ENODATA if watchdog bark or bite time have not been set.
 */
int32_t wdt_restart( const WdtHandle_t pxWdtHandle );

/*!
 * @brief   wdt_set_callback is used to set the callback to be called when
 *          bark time reaches the WatchDog counter or if the bite time is configured to
 *          generate interrupt. The caller must set the timers using
 *          IOCTL and start the timer for the callback to be called back.
 *          Caller must restart the timer when bark timer expires and bite time is configured
 *          to reset the device to avoid the target reset.
 *
 * @note Single callback is used, per instance, if eWdtBiteTimerInterrupt has been configured or
 *       if the bark time reaches the WatchDog counter.
 * @note Newly set callback overrides the one previously set
 *
 * @warning If input handle or if callback function is NULL, this function silently takes no action.
 *
 * @param[in]   pxWdtHandle handle to WatchDog interface returned in
 *                              wdt_open.
 * @param[in]   xCallback       The callback function to be called.
 * @param[in]   pvUserContext   The user context to be passed when callback is called.
 */
void wdt_set_callback( const WdtHandle_t pxWdtHandle,
        WdtCallback_t xCallback,
        void *pvUserContext );

/**
 * @brief   wdt_ioctl is used to configure the WatchDog timer properties
 *          like the WatchDog timeout value, WatchDog bite behaviour etc.
 *
 * @param[in]   pxWdtHandle handle to WatchDog interface returned in
 *                              wdt_open.
 * @param[in]   xRequest    configuration request of type #SocfpgaFlashIoctlRequest_t
 * @param[in,out] pvBuffer   the configuration buffer to hold the request or response of IOCTL.
 *
 * @return
 *   - returns 0 on success
 *   - -EINVAL if
 *      - pxWdtHandle is NULL
 *      - xRequest is invalid
 *      - pvBuffer is NULL
 *      - error in bite behavior set
 */
int32_t wdt_ioctl( const WdtHandle_t pxWdtHandle,
        WdtIoctlRequest_t xRequest,
        void *const pvBuffer );

/**
 * @brief   wdt_close is used to de-initializes the WatchDog, stops the timer
 *          if it was started and resets the timer value.
 *
 * @param[in]   pxWdtHandle handle to WatchDog interface returned in
 *                              wdt_open.
 * @return
 *   - 0 on success
 *   - -EINVAL if
 *      - pxWdtHandle == NULL
 *      - pxWdtHandle is not open (previously closed).
 *      - failed to stop wdt
 */
int32_t wdt_close( const WdtHandle_t pxWdtHandle );

/**
 * @}
 */
/* end of group wdt_fns */

/**
 * @}
 */
/* end of group wdt */

#endif /* __SOCFPGA_WATCHDOG_H__ */
