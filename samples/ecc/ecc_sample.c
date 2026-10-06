/*
 * SPDX-FileCopyrightText: Copyright (C) 2025 Altera Corporation
 *
 * SPDX-License-Identifier: MIT-0
 *
 * Sample application for SoC FPGA ECC
 */

#include "osal.h"
#include "osal_log.h"
#include "socfpga_ecc.h"
#include "socfpga_defines.h"

/*
    File: ECC Sample

    Sample Application for ECC driver.
    Group: Description

    This is a sample application to demonstrate the usage of the ECC driver.
    It performs a single bit and double bit error injection to a specified ECC
    module.

    Group: Prerequisites

    None

    Group: Configurable Parameters

    - The module to perform error injection can be modified in the ECC_MODULE macro

    Group: How to Run

    - Follow the common README for build and flashing instructions.
    - Run the sample.

    Group: Expected Results

    - The success/failure logs are displayed in the console.
*/

#define ECC_MODULE     ECC_QSPI
/* Wait time for callback in ms */
#define ECC_TIMEOUT    1000U

osal_semaphore_t callback_sem;

/*
    Function: ecc_callback

    Calls osal_semaphore_post.
*/
void ecc_callback(uint32_t error_type)
{
    if (error_type == ECC_SINGLE_BIT_ERROR)
    {
        /* Handle the single bit error */
        PRINT("Single bit error processed");
    }
    else if (error_type == ECC_DOUBLE_BIT_ERROR)
    {
        /* Handle the double bit error */
        PRINT("Double bit error processed");
    }
    osal_semaphore_post(callback_sem);
}

/*
    Function: ecc_task
*/
void ecc_task(void)
{
    int ret;
    uint32_t module_list = 0U;

    callback_sem = osal_semaphore_create(NULL);
    if (callback_sem == NULL)
    {
        ERROR("Failed to create semaphore");
        return;
    }

    PRINT("\nECC sample application");
    PRINT("Initializing ECC module");
    ret = ecc_init();
    if (ret != 0)
    {
        ERROR("ECC initialization failed");
        return;
    }
    PRINT("ECC initialized");

    PRINT("Setting ECC callback");
    ret = ecc_set_callback(ecc_callback);
    if (ret != 0)
    {
        ERROR("Failed to set ECC callback");
        return;
    }
    PRINT("ECC callback set");

    module_list = (1U << ECC_MODULE);
    PRINT("Enabling ECC module");
    ret = ecc_enable_modules(module_list);
    if (ret != 0)
    {
        ERROR("Failed to enable ECC module");
        return;
    }
    PRINT("ECC module %d enabled", ECC_MODULE);

    PRINT("Injecting single bit error");
    ret = ecc_inject_error(ECC_MODULE, ECC_SINGLE_BIT_ERROR);
    if (ret == 0)
    {
        if (osal_semaphore_wait(callback_sem, ECC_TIMEOUT) == pdTRUE)
        {
            PRINT("Single bit error injected successfully");
        }
        else
        {
            ERROR("Failed to receive callback");
        }
    }
    else
    {
        ERROR("Failed to inject single bit error");
    }

    PRINT("Injecting double bit error");
    ret = ecc_inject_error(ECC_MODULE, ECC_DOUBLE_BIT_ERROR);
    if (ret == 0)
    {
        if (osal_semaphore_wait(callback_sem, ECC_TIMEOUT) == pdTRUE)
        {
            PRINT("Double bit error injected successfully");
        }
        else
        {
            ERROR("Failed to receive callback");
        }
    }
    else
    {
        ERROR("Failed to inject double bit error");
    }
    PRINT("ECC sample application completed");
}
