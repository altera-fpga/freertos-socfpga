#include <errno.h>
/*
 * SPDX-FileCopyrightText: Copyright (C) 2025-2026 Altera Corporation
 *
 * SPDX-License-Identifier: MIT-0
 *
 * Header file for SDM Mailbox HAL driver
 */

#ifndef __SOCFPGA_MBOX_CLIENT_H__
#define __SOCFPGA_MBOX_CLIENT_H__

/*
    File: SDM Mailbox Client Driver

    Header file for SDM Mailbox HAL driver

    APIs for SoC FPGA SDM Mailbox driver.

    The sip_svc subsystem provides a set of APIs to interact with the Secure
    Monitor Call (SMC) interface in the Arm Trusted Firmware (ATF). The APIs
    allow the application to send commands to the ATF which in turn performs a
    variety of mailbox commands such as reading hardware monitor values,
    cryptographic operations etc. To see example usage, refer
    sdm_mbox_sample "SDM Mailbox sample application".
*/

#include <stdint.h>

#define MBOX_WORD_SIZE    (4U)        /* Mailbox Word Size. Used to format data for mailbox */

/*
    Group: Structures

    Types for the SDM Mailbox Client driver.
*/

struct sdm_client_descriptor;

/*
    Type: sdm_client_handle_t

    sdm_client_handle_t type is the Mailbox Client handle returned by calling
    open_client() this is initialized in open and returned to caller. Caller
    must pass this pointer to the rest of the APIs.
*/
typedef struct sdm_client_descriptor *sdm_client_handle_t;

/*
    Group: Functions

    Entry points and callback signatures for the SDM Mailbox Client driver.
*/

/*
    Callback: mbox_call_back_t

    Mailbox callback function type

    Parameters:

        resp_data - Pointer to the response data
*/
typedef void (*mbox_call_back_t)(uint64_t *resp_data);

/*
    Function: mbox_init

    mbox_init is used to create the mailbox task, allocate memory for the
    descriptor and initialize the semaphores and mutexes
*/
int mbox_init(void);

/*
    Function: mbox_deinit

    mbox_deinit is used to delete the mailbox task and free up mailbox related
    resources

    Returns:

        - 0: on success.
        - -EIO: if some internal errors occur.
*/
int mbox_deinit(void);

/*
    Function: mbox_open_client

    Open a mailbox client

    The application must call this function before calling other functions

    Returns:

        - The handle if the client is opened successfully
        - NULL if there is an error
*/
sdm_client_handle_t mbox_open_client(void);

/*
    Function: mbox_close_client

    Close the mbox client

    Parameters:

        mbox_handle - Handle of mailbox client to be closed

    Returns:

        - 0: on success.
        - -EIO: if some internal errors occur.
*/
int32_t mbox_close_client(sdm_client_handle_t mbox_handle);

/*
    Function: mbox_send_command

    Send a generic mailbox command

    Used to send a generic mailbox command using the generic mailbox SIP SVC
    call.

    Parameters:

        mbox_handle - The Client handle returned in the open() call.
        command - Function Id specifying what command to perform.
        command_args - Arguments required for the command.
        arg_size - Length of the arguments provided in bytes.
        resp - Pointer to store the response data.
        resp_size - Expected response length.
        smc_resp - Pointer to store the response data.
        smc_resp_len - Expected response length.

    Returns:

        - 0: on success.
        - -EIO: If some internal errors occur.
*/

int32_t mbox_send_command(sdm_client_handle_t mbox_handle, uint32_t command,
        uint32_t *command_args, uint32_t arg_size,
        uint32_t *resp, uint32_t resp_size,
        uint64_t *smc_resp, uint32_t smc_resp_len);
/*
    Function: sip_svc_send

    Send an SIP SVC request

    Used to send a SIP SVC request to the ATF.

    Parameters:

        mbox_handle - The Client handle returned in the open() call.
        smc_func_id - Function Id specifying what command to perform.
        mbox_args - Arguments required for the command.
        arg_len - Length of the arguments provided in bytes.
        resp_data - Pointer to store the response data.
        resp_len - Expected response length.

    Returns:

        - 0: on success.
        - -EIO: If some internal errors occur.
*/
int32_t sip_svc_send(sdm_client_handle_t mbox_handle, uint64_t smc_func_id,
        uint64_t *mbox_args, uint32_t arg_len, uint64_t *resp_data, uint32_t
        resp_len);

/*
    Function: mbox_set_callback

    Set callback function for a client

    Set the callback function to notify the completion of an async operation.
    The application must call this function to use the mbox_write_async
    function.

    Parameters:

        mbox_handle - The Client handle returned in the open() call.
        callback - Function pointer to callback function

    Returns:

        - 0: on success.
        - -EIO: if some internal errors occur.
*/
int32_t mbox_set_callback(sdm_client_handle_t mbox_handle, mbox_call_back_t
        callback);
/* end of group sdm_mbox_fns */
/* end of group sdm_mbox */

#endif /*_SOCFPGA_MBOX_CLIENT_H_*/
