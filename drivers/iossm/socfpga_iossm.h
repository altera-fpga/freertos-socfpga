/*
 * SPDX-FileCopyrightText: Copyright (C) 2025-2026 Altera Corporation
 *
 * SPDX-License-Identifier: MIT-0
 *
 * Header file for iossm HAL driver
 */

#ifndef __SOCFPGA_IOSSM_H__
#define __SOCFPGA_IOSSM_H__

/*
    File: IOSSM Driver

    Header file for IOSSM HAL driver

    APIs for SoC FPGA IOSSM driver.

    IOSSM is an ECC engine for DDR error detection and correction. It provides
    APIs to read ECC status, inject single and double bit errors, acknowledge
    interrupts, and set callbacks for error handling. For example usage, see
    IOSSM sample application.
*/

/*
    Group: Macros

    Callback signatures for the IOSSM driver.
*/

#include <stdint.h>
#include <stdbool.h>

#define IOSSM_ECC_CRCT_EVENT_DET          (1U << 0U)   /* Single ECC correctable event detected. */
#define IOSSM_MUL_ECC_CRCT_EVENT_DET      (1U << 1U)   /* Multiple ECC correctable events detected. */
#define IOSSM_ECC_UNCRCT_EVENT_DET        (1U << 2U)   /* Single ECC uncorrectable event detected. */
#define IOSSM_MUL_ECC_UNCRCT_EVENT_DET    (1U << 3U)   /* Multiple ECC uncorrectable events detected. */
#define IOSSM_ECC_WRT_BACK_ERROR          (1U << 6U)   /* ECC write-back error occurred. */
#define IOSSM_SCRUB_OP_OK                 (1U << 7U)   /* Scrub operation completed successfully. */
#define IOSSM_ECC_CRCT_SCRB_EVENT_DET     (1U << 8U)   /* ECC correctable error detected during scrub operation. */
#define IOSSM_WR_ECC_SIN_BIT              (1U << 9U)   /* Single-bit ECC error during write. */
#define IOSSM_WR_ECC_DBL_BIT              (1U << 10U)  /* Double-bit ECC error during write. */
#define IOSSM_RD_ECC_SIN_BIT              (1U << 11U)  /* Single-bit ECC error during read. */
#define IOSSM_RD_ECC_DBL_BIT              (1U << 12U)  /* Double-bit ECC error during read. */
#define IOSSM_RMW_ECC_DBL_BIT             (1U << 13U)  /* Double-bit ECC error during read-modify-write. */

#define IOSSM_INSTANCE_OFFSET    (0x400000U)           /* Offset between IOSSM hardware instances. */
#define IOSSM_SINGLE_BIT_ERR     (0U)                  /* Indicator for single-bit ECC error. */
#define IOSSM_DOUBLE_BIT_ERR     (1U)                  /* Indicator for double-bit ECC error. */

#define IOSSM_OP_SUCCESS    0U               /* IOSSM operation completed successfully */
#define IOSSM_OP_FAILED     1U               /* IOSSM operation failed */

#define IOSSM_INSTANCE_0      0U                /* Identifier for IOSSM instance 0. */
#define IOSSM_INSTANCE_1      1U                /* Identifier for IOSSM instance 1. */
#define IOSSM_MAX_INSTANCE    2U                /* Maximum number of supported IOSSM instances. */

/*
    Callback: iossm_cb_handler_t

    IOSSM callback function type
*/
typedef void (*iossm_cb_handler_t)(void);

/*
    Group: Enumerations

    Enumerated values for the IOSSM driver.
*/

/*
    Enum: iossm_ecc_mode_t

    IOSSM ECC modes
*/
typedef enum
{
    IOSSM_ECC_DI,              /* ECC disabled. */
    IOSSM_ECC_NO_TRIG,         /* ECC enabled but no trigger on error. */
    IOSSM_ECC_DETECT,          /* ECC enabled with error detection only. */
    IOSSM_ECC_DETECT_CORRECT   /* ECC enabled with error detection and correction. */
} iossm_ecc_mode_t;


/*
    Enum: iossm_ecc_type_t

    IOSSM ECC types
*/
typedef enum
{
    IOSSM_OUT_OF_BAND, /* ECC is stored out-of-band (separately from the main data). */
    IOSSM_IN_LINE      /* ECC is stored in-line (alongside the main data). */
} iossm_ecc_type_t;

/*
    Group: Structures

    Structures for the IOSSM driver.
*/

/*
    Struct: iossm_context_t

    IOSSM driver context information.
*/
typedef struct
{
    uint32_t iossm_base_addr;           /* Base address of the IOSSM instance. */
    uint8_t iossm_ip_type;              /* IP type identifier for the IOSSM instance. */
    uint8_t iossm_instance_id;          /* Unique instance ID of the IOSSM. */
    iossm_cb_handler_t iossm_cb_fun;    /* Callback function for IOSSM event handling. */
    bool iossm_instance_is_open;        /* Indicates whether the IOSSM instance is open (true) or closed (false). */
} iossm_context_t;

/*
    Struct: err_info_t

    Hold error information reported by the IOSSM ECC handler.
*/
typedef struct
{
    uintptr_t iossm_err_info_ptr;   /* Pointer to additional error information or metadata (platform-dependent). */
    uint64_t iossm_err_addr;        /* Address at which the ECC error occurred. */
    uint64_t iossm_err_data;        /* Data value that caused or triggered the ECC error. */
    uint32_t iossm_err_dat_size;    /* Size of the erroneous data in bytes. */
    uint32_t iossm_err_id;          /* Identifier indicating the source or origin of the error. */
    uint32_t iossm_err_syndrome;    /* ECC syndrome value used for error detection and correction. */
    uint8_t iossm_err_type;         /* Type of ECC error (e.g., correctable, uncorrectable, single/double-bit). */
} err_info_t;

/*
    Group: Functions

    Entry points for the IOSSM driver.
*/

/*
    Function: iossm_open

    Used to initialize the iossm instance Once a instance is opened, it needs
    to be closed before invoking open again.

    Parameters:

        instance - The iossm instance to open.

    Returns:

        - iossm handle on success.
        - NULL if Invalid iossm instance.
        - NULL if same instance already opened.
*/
iossm_context_t *iossm_open(uint32_t instance);

/*
    Function: iossm_close

    Close the iossm instance.

    Parameters:

        instance - Iossm instance to close.

    Returns:

        - 0 on success.
        - -EINVAL: if invalid instance id
        - -EIO:    if the instance is not opened.
*/
int32_t iossm_close(uint32_t instance);

/*
    Function: iossm_read_ecc_status

    Read the ecc status.

    Parameters:

        xhandle - iossm handle returned by open API.

    Returns:

        - ecc status on success (number of errors).
        - -EINVAL: if invalid instance id
        - -EIO:    if the instance is not opened.
*/
int32_t iossm_read_ecc_status(const iossm_context_t *xhandle);

/*
    Function: iossm_mask_int

    Mask the ecc interrupt on IO96B.

    Parameters:

        xhandle - iossm handle returned by open API.
        interrupts - interrupts to be masked.

    Returns:

        - 0 on success
        - -EINVAL: if iossm handle is NULL.
        - -EIO:    if,
            - the instance is not opened.
            - if the response has failed.
*/
int32_t iossm_mask_int(const iossm_context_t *xhandle, uint32_t interrupts);

/*
    Function: iossm_inject_sbit_err

    Inject single bit error.

    Parameters:

        xhandle - iossm handle returned by open API.

    Returns:

        - 0 on success
        - -EINVAL: if iossm handle is NULL.
        - -EIO:    if,
            - the instance is not opened.
            - if the response has failed.
*/
int32_t iossm_inject_sbit_err(const iossm_context_t *xhandle);

/*
    Function: iossm_inject_dbit_err

    Inject double bit error.

    Parameters:

        xhandle - iossm handle returned by open API.

    Returns:

        - 0 on success
        - -EINVAL: if iossm handle is NULL.
        - -EIO:    if,
            - the instance is not opened.
            - if the response has failed.
*/
int32_t iossm_inject_dbit_err(const iossm_context_t *xhandle);

/*
    Function: iossm_ack_int

    Acknowledge ecc interrupt status.

    Parameters:

        xhandle - iossm handle returned by open API.
        interrupts - interrupts to be acknowledged.

    Returns:

        - 0 on success
        - -EINVAL: if iossm handle is NULL.
        - -EIO:    if,
            - the instance is not opened.
            - if the response has failed.
*/
int32_t iossm_ack_int(const iossm_context_t *xhandle, uint32_t interrupts);

/*
    Function: iossm_set_callback

    set iossm interrupt call back function.

    Parameters:

        xhandle - iossm handle returned by open API.
        call_back_fun - Call back function pointer.
*/
void iossm_set_callback(iossm_context_t *xhandle, iossm_cb_handler_t
        call_back_fun);


/*
    Function: iossm_clear_ecc_buffer

    clear iossm error buffers

    Parameters:

        xhandle - iossm handle returned by open API.

    Returns:

        -  0 on success
        - -EINVAL: if iossm handle is NULL.
        - -EIO:    if,
            - the instance is not opened.
            - if the response has failed.
*/
int32_t iossm_clear_ecc_buffer(const iossm_context_t *xhandle);

/*
    Function: iossm_get_err_type

    get the error type from error index.

    Parameters:

        xhandle - iossm handle returned by open API.
        index - index of the error (usually got from status register).

    Returns:

        -  Error type on success
        - -EINVAL: if iossm handle is NULL.
        - -EIO:    if the instance is not opened.
*/
int32_t iossm_get_err_type(const iossm_context_t *xhandle, uint32_t index);

/*
    Function: iossm_get_err_addr_offset

    get the error address offset from error index.

    Parameters:

        xhandle - iossm handle returned by open API.
        index - index of the error (usually got from status register)

    Returns:

        -  Error address offset on success
        - -EINVAL: if iossm handle is NULL.
        - -EIO:    if the instance is not opened.
*/
int32_t iossm_get_err_addr_offset(const iossm_context_t *xhandle, uint32_t
        index);
#endif /* __SOCFPGA_IOSSM_H__ */
