/*
 * SPDX-FileCopyrightText: Copyright (C) 2025 Altera Corporation
 *
 * SPDX-License-Identifier: MIT-0
 *
 * Header file for SMC calls
 */

#ifndef _SOCFPGA_SIP_HANDLER_H_
#define _SOCFPGA_SIP_HANDLER_H_

/**
 * \defgroup smc_call SMC Call
 * \ingroup drivers
 *
 *
 * @{
 */

 /**
 * @defgroup smc_call_fns Functions
 * @ingroup smc_call
 * SDM Mailbox HAL APIs
 */

#include <stdint.h>
#include <stddef.h>

/**
 * @addtogroup smc_call_fns
 * @{
 */

/**
 * @brief	Perform SMC call
 * smc_call is used to load values into CPU registers and perform a SMC call.
 * The value in the x0 register after the SMC call is used as the return value
 *
 * @param[in] function_id			ulFunctionID is the function ID corresponding
                                    to a specific SIP call.
 * @param[in, out] register_val	    The pullRegValues is an array with each
                                    element corresponding to a register value.
                                    Once the SMC call is performed the new values
                                    in the registers are stored back into the
                                    array
 * @return
 *   -The value of the x0 register after the SMC call
 */
int smc_call( uint64_t function_id, uint64_t *register_val );

/**
 * @}
 */
/* end of group smc_call_fns */

#endif /*_SOCFPGA_SIP_HANDLER_H_*/
/**
 * @}
 */

