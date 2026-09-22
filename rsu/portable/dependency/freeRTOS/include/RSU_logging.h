/*
 * SPDX-FileCopyrightText: Copyright (C) 2025 Altera Corporation
 *
 * SPDX-License-Identifier: MIT-0
 */

/*
    File: FreeRTOS RSU Logging Library

    logging API used inside LibRSU
*/

#ifndef RSU_LOGGING_H
#define RSU_LOGGING_H

#include "osal_log_config.h"
#include <logging_stack.h>
#include <libRSU_OSAL.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/*
    Enum: rsu_loglevel_t

    RSU log levels
*/
typedef enum {
	L_LOG_OFF = 0,
	L_LOG_ERR = 1,
	L_LOG_WRN = 2,
	L_LOG_INF = 3,
	L_LOG_DBG = 4,
	L_LOG_MAX
} rsu_loglevel_t;

/*
    default log level
*/
#ifndef L_LOG_LVL
#define L_LOG_LVL L_LOG_INF
#endif

/*
    Macro: RSU_LOG_DBG

    debug log API
*/
#define RSU_LOG_DBG(...)  LogDebug((__VA_ARGS__))
/*
    Macro: RSU_LOG_INF

    info log API
*/
#define RSU_LOG_INF(...)  LogInfo((__VA_ARGS__))
/*
    Macro: RSU_LOG_WRN

    warn log API
*/
#define RSU_LOG_WRN(...)  LogWarn((__VA_ARGS__))
/*
    Macro: RSU_LOG_ERR

    error log API
*/
#define RSU_LOG_ERR(...)  LogError((__VA_ARGS__))

/*
    Function: RSU_set_logging

    set logging level of logger system

    Parameters:

        level - logging level

    Returns:

        0 on success, negative number on error.

    Notes:

        for zephyr, the logging system is integrated with zephyr, so this
        function is redundant.
*/
RSU_OSAL_INT RSU_set_logging(rsu_loglevel_t level);

/*
    Function: RSU_logging_init

    initialize logging

    Parameters:

        cfg_file - file to configuration file which provides parameters
            during initialization.

    Returns:

        0 on success, negative number on error.

    Notes:

        for zephyr, the logging system is integrated with zephyr, so this
        function is redundant.
*/
RSU_OSAL_INT RSU_logging_init(RSU_OSAL_CHAR *cfg_file);

/*
    Function: RSU_logging_exit

    exit logging

    Returns:

        Nil

    Notes:

        for zephyr, the logging system is integrated with zephyr, so this
        function is redundant.
*/
RSU_OSAL_VOID RSU_logging_exit(RSU_OSAL_VOID);

/*
    Function: RSU_logger

    logging function each platform needs to define

    Parameters:

        level - logging level
        format - format string

    Returns:

        Nil
*/
RSU_OSAL_VOID RSU_logger(rsu_loglevel_t level, const RSU_OSAL_CHAR *format, ...);

#if PLATFORM_ZEPHYR
#include <zephyr/logging/log.h>

/*
    Function: OG_MODULE_DECLARE

    integrate RSU logging with zephyr logging system.
*/

LOG_MODULE_DECLARE(uniLibRSU, CONFIG_UNILIBRSU_LOG_LEVEL);

#undef RSU_LOG_DBG
#define RSU_LOG_DBG(format, ...) LOG_DBG(format, ##__VA_ARGS__)

#undef RSU_LOG_INF
#define RSU_LOG_INF(format, ...) LOG_INF(format, ##__VA_ARGS__)

#undef RSU_LOG_WRN
#define RSU_LOG_WRN(format, ...) LOG_WRN(format, ##__VA_ARGS__)

#undef RSU_LOG_ERR
#define RSU_LOG_ERR(format, ...) LOG_ERR(format, ##__VA_ARGS__)
#endif

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
