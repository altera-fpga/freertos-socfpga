/*
 * SPDX-FileCopyrightText: Copyright (C) 2025 Altera Corporation
 *
 * SPDX-License-Identifier: MIT-0
 *
 * Header file for SDM Mailbox HAL driver
 */

#ifndef _SOCFPGA_MBOX_CLIENT_H_
#define _SOCFPGA_MBOX_CLIENT_H_

/**
 * @file socfpga_mbox_client.h
 * @brief Header file for SDM Mailbox HAL driver
 */

/**
 * @defgroup sdm_mbox SDM Mailbox
 * @ingroup drivers
 * @{
 */

/**
 * @defgroup sdm_mbox_fns Functions
 * @ingroup sdm_mbox
 * SDM Mailbox HAL APIs
 */

/**
 * @defgroup sdm_mbox_structs Structures
 * @ingroup sdm_mbox
 * SDM Mailbox Specific Structures
 */

#include <stdint.h>

/**
 * @addtogroup sdm_mbox_macros
 * @{
 */

#define MBOX_WORD_SIZE    (4U)        /*!< Mailbox Word Size. Used to format data for mailbox*/

/**
 * @}
 */

/**
 * @addtogroup sdm_mbox_structs
 * @{
 *
 */

/**
 * @brief	sdm_client_descriptor is defined in source file
 */
struct sdm_client_descriptor;

/**
 * @brief	sdm_client_handle type is the Mailbox Client handle returned by calling open_client()
 *          this is initialized in open and returned to caller. Caller must pass this pointer
 *          to the rest of the APIs.
 */
typedef struct sdm_client_descriptor *sdm_client_handle;

/**
 * @}
 */

/**
 * @addtogroup sdm_mbox_fns
 * @{
 */

typedef void (*mbox_call_back_t)( uint64_t *resp_data );

/**
 * @brief   mbox_init is used to create the mailbox task, allocate memory for the descriptor
 *			    and initialize the semaphores and mutexes
 */
int mbox_init(void);

/**
 * @brief   mbox_deinit is used to delete the mailbox task and free up mailbox related resources
 *
 * @return
 * - 0 on success.
 * - -EIO if some internal errors occur.
 */
int mbox_deinit(void);

/**
 * @brief   open_client is used to open the Mailbox Client handle.
 *          The application must call this function to use other functions.
 *
 * @return
 * - 0 on success.
 * - -EIO if some internal errors occur.
 */
sdm_client_handle mbox_open_client(void);
/**
 * @brief   close_client is used to close the Mailbox Client handle.
 *
 * @param[in] mbox_handle Handle of mailbox client to be closed
 *
 * @return
 * - 0 on success.
 * - -EIO if some internal errors occur.
 */
int32_t mbox_close_client( sdm_client_handle mbox_handle );


/**
 * @brief   mbox_send_command is used to send a generic mailbox command using
 *          the generic mailbox SIP SVC call. The commmand accepts buffers for
 *          the arguments and response of the mailbox command and an additional
 *          buffer to store the SMC call status and return size
 *
 *
 * @param[in] mbox_handle         The Client handle returned in the open() call.
 * @param[in] command             Function Id specifying what command to perform.
 * @param[in] command_args        Arguments required for the command.
 * @param[in] arg_size            Length of the arguments provided in bytes.
 * @param[out] resp               Pointer to store the response data.
 * @param[in] resp_size           Expected response length.
 * @param[out] smc_resp           Pointer to store the response data.
 * @param[in] smc_resp_len        Expected response length.
 *
 * @return
 * - 0 on success.
 * - Negative value if SIP SVC call failed.
 * - -EIO If some internal errors occur.
 */
int32_t mbox_send_command(sdm_client_handle mbox_handle,
        uint32_t command, uint32_t *command_args,
        uint32_t arg_size, uint32_t *resp, uint32_t resp_size,
        uint64_t *smc_resp, uint32_t smc_resp_len);
/**
 * @brief   sip_svc_send is used to send a SIP SVC request to the ATF
 *          The arguments for the command are stored in args and the
 *          buffer to store the response is provided by resp_data.
 *
 *
 * @param[in] mbox_handle         The Client handle returned in the open() call.
 * @param[in] smc_fid             Function Id specifying what command to perform.
 * @param[in] mbox_args           Arguments required for the command.
 * @param[in] arg_len             Length of the arguments provided in bytes.
 * @param[out] resp_data          Pointer to store the response data.
 * @param[in] resp_len            Expected response length.
 *
 * @return
 * - 0 on success.
 * - Negative value if SIP SVC call failed.
 * - -EIO If some internal errors occur.
 */
int32_t sip_svc_send( sdm_client_handle mbox_handle,
        uint64_t smc_func_id, uint64_t *mbox_args, uint32_t arg_len,
        uint64_t *resp_data, uint32_t resp_len);

/**
 * @brief  mbox_set_callback is used to set the callback for a client.
 *         The application must call this function to use the mbox_write_async function.
 *
 * @param[in] mbox_handle   The Client handle returned in the open() call.
 * @param[in] callback_fn   Function pointer to callback function
 *
 * @return
 * - 0 on success.
 * - -EIO if some internal errors occur.
 */
int32_t mbox_set_callback( sdm_client_handle mbox_handle,
        mbox_call_back_t pxCallback );

/**
 * @}
 */
/* end of group sdm_mbox_fns */

/**
 * @}
 */
/* end of group sdm_mbox */

#endif /*_SOCFPGA_MBOX_CLIENT_H_*/
/**
 * @}
 */

