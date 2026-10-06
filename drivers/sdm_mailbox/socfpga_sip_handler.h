/*
 * SPDX-FileCopyrightText: Copyright (C) 2025-2026 Altera Corporation
 *
 * SPDX-License-Identifier: MIT-0
 *
 * Header file for SMC calls
 */

#ifndef __SOCFPGA_SIP_HANDLER_H__
#define __SOCFPGA_SIP_HANDLER_H__

/*
    File: SDM Mailbox SIP Handler Driver

    API to perform SMC calls

    This is the implementation of SMC call API for SoC FPGA. It loads values
    into CPU registers and performs a SMC call.
*/

#include <stdint.h>
#include <stddef.h>

/*
    Group: Functions

    Entry points for the SDM Mailbox SIP Handler driver.
*/

/*
    Function: smc_call

    Perform SMC call

    smc_call is used to load values into CPU registers and perform a SMC call.
    The value in the x0 register after the SMC call is used as the return
    value

    Parameters:

        function_id - Id of the SIP call
        register_val - An array with each element corresponding to a
            register value. Once the SMC call is performed the new values in
            the registers are stored back into the array

    Returns:

        -The value of the x0 register after the SMC call
*/
int smc_call(uint64_t function_id, uint64_t *register_val);
/* end of group smc_call_fns */
#endif /*_SOCFPGA_SIP_HANDLER_H_*/
