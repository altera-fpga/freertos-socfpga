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
 *
 */

#ifndef __CLI_UTILS_H__
#define __CLI_UTILS_H__

#include <stdint.h>

/*
    File: CLI Utilities

    This file contains definitions for cli argument validation functions
*/

/*
    Function: cli_get_decimal

    Validate the decimal function argument for CLI.

    Parameters:

        cli - command.
        Input - parameter name.
        Input - parameter.
        Lower - boundary for argument.
        Upper - boundary for argument.
        Pointer - to value to be returned.

    Returns:

        -  0         if operation succeeded.
        - -1         if operation failed.
*/
int cli_get_decimal(const char *cmd, const char *param, const char *input,
        long lo_bound,long hi_bound, uint32_t *val);

/*
    Function: cli_get_hex

    Validate the hexadecimal function argument for CLI.

    Parameters:

        cli - command.
        Input - parameter name.
        Input - parameter.
        Lower - boundary for argument.
        Upper - boundary for argument.
        Pointer - to value to be returned.

    Returns:

        -  0         if operation succeeded.
        - -1         if operation failed.
*/
int cli_get_hex(const char *cmd, const char *param, const char *input,
        long lo_bound,long hi_bound, uint32_t *val);

/*
    Function: cli_get_file_name

    Validate file name length for CLI.

    Parameters:

        string - to be validated.
        Command - which uses the string.

    Returns:

        -  0         if operation succeeded.
        - -1         if operation failed.
*/
int cli_get_file_name(const char *name, const char *command);

/*
    Function: cli_get_dir_name

    Validate directory name length for CLI.

    Parameters:

        string - to be validated.
        Command - which uses the string.

    Returns:

        -  0         if operation succeeded.
        - -1         if operation failed.
*/
int cli_get_dir_name(const char *name, const char *command);

/*
    Function: cli_is_pinmux_i2c

    To check if I2C instance is supported.

    Parameters:

        I2C - instance to be validated

    Returns:

        -  0         if instance is supported.
        - -1         if instance is not supported.
*/
int cli_is_pinmux_i2c(uint8_t instance);

/*
    Function: cli_is_valid_ip

    To check if IP is valid.

    Parameters:

        IP - to be validated.

    Returns:

        -  0         if IP is valid.
        - -1         if IP is not valid.
*/
int cli_is_valid_ip(const char *ip);

#endif
