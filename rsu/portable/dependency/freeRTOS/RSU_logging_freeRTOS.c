/*
 * SPDX-FileCopyrightText: Copyright (C) 2025 Altera Corporation
 *
 * SPDX-License-Identifier: MIT-0
 */

/*
    File: RSU Logging FreeRTOS Library

    Library routines for the RSU Logging FreeRTOS: 3 functions.

    They are RSU_set_logging, RSU_logging_init and RSU_logging_exit.
*/

#include <string.h>
#include <stdarg.h>

#include <libRSU_OSAL.h>
#include <utils/RSU_logging.h>
#include <utils/RSU_utils.h>

RSU_OSAL_INT RSU_set_logging(rsu_loglevel_t level)
{
    (void)level;
    return 0;
}

RSU_OSAL_INT RSU_logging_init(RSU_OSAL_CHAR *cfg_file)
{
    (void)cfg_file;
    return 0;
}

RSU_OSAL_VOID RSU_logging_exit(RSU_OSAL_VOID)
{
    return;
}

