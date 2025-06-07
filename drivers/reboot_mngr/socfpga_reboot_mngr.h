/*
 * SPDX-FileCopyrightText: Copyright (C) 2025 Altera Corporation
 *
 * SPDX-License-Identifier: MIT-0
 *
 * HAL driver implementation for i2c
 */


#ifndef __SOCFPGA_REBOOT_MNGR_H__
#define __SOCFPGA_REBOOT_MNGR_H__

/**
 * @file socfpga_reboot_mngr.h
 * @brief This file contains all the Reboot Manager HAL API definitions
 *
 */

/**
 * @defgroup rbt Reboot Manager
 * @ingroup drivers
 * @{
 */

/**
 * @defgroup rbt_fns Functions
 * @ingroup rbt
 * Reboot Manager HAL APIs
 */

/**
 * @defgroup rbt_macros Macros
 * @ingroup rbt
 * Reboot Manager Specific Macros
 */

/**
 * @addtogroup rbt_macros
 * @{
 */
#define WARM_REBOOT           1U
#define COLD_REBOOT           0U
#define REBOOT_SUCCESS        0U
#define REBOOT_FAILURE        1U
#define RET_NOT_SUPPORTED     -134
#define REBOOT_CMD_INVALID    -22
#define REBOOT_DENIED         -1
/**
 * @}
 */

/**
 * @addtogroup rbt_fns
 * @{
 */
/**
 * Function pointer for user callback
 */
typedef void (*reboot_callback_t)( void *data );



/**
 * @brief   Set the callback function
 *
 * Sets the callback function that is invoked before initiating cold
 * /warm reboot.
 *
 * @param[in] reboot_callback_t - Call back function pointer
 * @param[in] event - Callback for warm/cold reboot.
 *
 * @return
 * - SUCCESS on success
 * - FAILURE if any arg is invalid
 */
uint32_t reboot_mngr_set_callback( reboot_callback_t call_back,
        uint32_t event );

/**
 * @brief   Reboot function
 *
 * Sets the callback function that is invoked before initiating cold
 * /warm reboot.
 *
 * @param[in] reboot_callback_t - Call back function pointer
 * @param[in] event - Callback for warm/cold reboot.
 *
 * @return
 * - SUCCESS on success
 * - FAILURE if arg is invalid
 */
uint32_t reboot_mngr_do_reboot( uint32_t event );
/**
 * @}
 */


/**
 * @}
 */
#endif

