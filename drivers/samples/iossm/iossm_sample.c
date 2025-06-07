/*
 * SPDX-FileCopyrightText: Copyright (C) 2025 Altera Corporation
 *
 * SPDX-License-Identifier: MIT-0
 *
 * sample implementation for iossm
 */
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "FreeRTOS.h"
#include "task.h"
#include "socfpga_iossm.h"
#include "iossm_sample.h"
#include "osal_log.h"

/**
 * The Sample application demonstrates the use of the IOSSM driver.
 *
 * This application serves as a basic demonstration of the IOSSM driver functionalities.
 * It performs the following operations:
 * - Initializes the IOSSM module.
 * - Injects single-bit and double-bit ECC errors to simulate fault conditions.
 * - Demonstrates the handling and reporting of ECC error events.
 *
 * This sample can be used as a reference for understanding how to:
 * - Simulate ECC error scenarios for testing and validation.
 * - Integrate IOSSM callbacks for error reporting in embedded systems.
 */


static xiossm_context* handle;

void error_handler(void);
void iossm_task()
{
    uint32_t resp_state;
    uint32_t ecc_status;

    printf("The IOSSM sample application for error injection starts \r\n");
    handle = iossm_open(IOSSM_INSTANCE_0);

    ecc_status = iossm_read_ecc_status(handle);
    printf("Reading number of errors (%d) \r\n", ecc_status);
    
    iossm_clear_ecc_buffer(handle);
    
    ecc_status = iossm_read_ecc_status(handle);
    printf("Clearing errors (%d) \r\n", ecc_status);

    iossm_set_callback(handle,error_handler);

    /* Inject double bit error*/

    resp_state = iossm_inject_dbit_err(handle);
    if (resp_state == 0)
    {
        printf("double bit error injected succesfully \r\n");
    }
    else
    {
        printf("double bit error injection failed \r\n");
        return;
    }

    resp_state = iossm_ack_int(handle, IOSSM_ECC_UNCRCT_EVENT_DET );
    if (resp_state == 0)
    {
        printf("interrupt acknowledged succesfully \r\n");
    }
    else
    {
        printf("failed to acknowledge interrupt \r\n");
        return;
    }

    /* Inject single bit error*/

    resp_state = iossm_inject_sbit_err(handle);
    if (resp_state == 0)
    {
        printf("single bit error injected succesfully \r\n");
    }
    else
    {
        printf("single bit error injection failed \r\n");
        return;
    }

    resp_state = iossm_ack_int(handle, IOSSM_ECC_UNCRCT_EVENT_DET );
    if (resp_state == 0)
    {
        printf("interrupt acknowledged succesfully \r\n");
    }
    else
    {
        printf("failed to acknowledge interrupt \r\n");
        return;
    }

    printf("The IOSSM sample application for error injection completed \r\n");

    vTaskDelete(NULL);
}

void error_handler()
{
    uint32_t ecc_status;

    ecc_status = iossm_read_ecc_status(handle);
    INFO("iossm error interrupt triggered (%d)\r\n", ecc_status);
}

