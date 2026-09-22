/* SPDX-License-Identifier: MIT-0 */
/*
 * Copyright (C) 2025 Altera
 */

/*
    File: Portable Layer FCS Library Logging

    logging API used inside LibFCS
*/

#ifndef LIBFCS_LOGGING_H
#define LIBFCS_LOGGING_H

#include <libfcs_osal.h>
#include "osal_log.h"
#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/*
    Enum: fcs_loglevel

    FCS log levels
*/
enum fcs_loglevel {
	L_LOG_NONE = 0,
	L_LOG_ERROR = 1,
	L_LOG_WARN = 2,
	L_LOG_INFO = 3,
	L_LOG_DEBUG = 4,
};


#undef FCS_LOG_DBG
#define FCS_LOG_DBG(...) DEBUG(__VA_ARGS__)

#undef FCS_LOG_INF
#define FCS_LOG_INF(...) INFO(__VA_ARGS__)

#undef FCS_LOG_WRN
#define FCS_LOG_WRN(...) WARN(__VA_ARGS__)

#undef FCS_LOG_ERR
#define FCS_LOG_ERR(...) ERROR(__VA_ARGS__)

/*
    Function: fcs_set_logging

    set logging level of logger system

    Parameters:

        level - logging level

    Returns:

        0 on success, negative number on error.
*/
FCS_OSAL_INT fcs_set_logging(enum fcs_loglevel level);

/*
    Function: fcs_logging_init

    initialize logging

    Parameters:

        log_file - file to log the messages.

    Returns:

        0 on success, negative number on error.
*/
FCS_OSAL_INT fcs_logging_init(FCS_OSAL_CHAR *log_file);

/*
    Function: fcs_logger

    logging function each platform needs to define

    Parameters:

        level - logging level
        format - string format

    Returns:

        Nil
*/
FCS_OSAL_VOID fcs_logger(enum fcs_loglevel level, const FCS_OSAL_CHAR *format, ...);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* LIBFCS_LOGGING_H */
