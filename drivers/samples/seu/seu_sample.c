/*
 * SPDX-FileCopyrightText: Copyright (C) 2025 Altera Corporation
 *
 * SPDX-License-Identifier: MIT-0
 *
 * sample implementation for SEU
 */

/**
 * @file seu_sample.c
 * @brief Sample Application for seu operations.
 */

/**
 * @defgroup seu_sample SEU Sample
 * @ingroup samples
 *
 * Sample Application for SEU (Single Event Upset) Injection and Validation
 *
 * @details
 * @subsubsection description Description
 * This sample demonstrates the ability of the system to handle SEU (Single Event Upset)
 * scenarios. It performs operations such as injecting SEU-safe errors, reading back
 * error-injected data, and retrieving SEU-related error statistics for system validation.
 *
 * @subsubsection prerequisites Prerequisites
 * - A system or FPGA image (SOF) with SEU feature enabled.
 * - Properly initialized SEU-related hardware or memory regions.
 *
 * @subsubsection how_to_run How to Run
 * 1. Follow the common README for building and flashing instructions.
 * 2. Ensure that the board is running a SEU-enabled SOF image.
 * 3. Run the sample binary. The application will:
 *    - Inject known SEU-safe errors into memory.
 *    - Read back and verify that the error injection is as expected.
 *    - Fetch and display SEU error detection statistics.
 *
 * @subsubsection expected_results Expected Results
 * - Successful injection and detection of SEU events.
 * - Accurate reporting of SEU statistics and error types.
 * - Demonstrates the system's capability to detect, report, and tolerate SEU faults.
 *
 * @{
 */
/** @} */



#include <stdio.h>
#include <stdint.h>
#include "socfpga_seu.h"
#include "socfpga_mbox_client.h"
#include "osal.h"

#define ECC_ERROR_TYPE     0x1
#define RAM_ID             0x1
#define ECC_SECTOR_ADDR    0xFF
#define SEU_SECTOR_ADDR    0x5

osal_semaphore_def_t seu_semphr_def_inject;
osal_semaphore_t seu_semphr_inject;

void seu_injection_done()
{
    osal_semaphore_post(seu_semphr_inject);
}

void seu_task()
{
    uint8_t ret;
    read_err_data_t seu_err_data;
    seu_err_params_t err_params;
    seu_stat_t seu_err_stats;
    seu_semphr_inject = osal_semaphore_create(&seu_semphr_def_inject );

    printf("\r Sample applicaton for seu and ecc error injection starts \n");
    ret = seu_init();
    if (ret != 0)
    {
        printf("\r SEU  Error Injection Failed \n");
        return;
    }
    seu_set_call_back(seu_injection_done);
    err_params.sector_addr = SEU_SECTOR_ADDR;
    err_params.cram_sel0 = 0;
    err_params.cram_sel1 = 0;
    err_params.injection_cycle =0;
    err_params.no_of_injection =0;
    ret = seu_insert_safe_err(err_params);

    osal_semaphore_wait(seu_semphr_inject,OSAL_TIMEOUT_WAIT_FOREVER);
    if (ret != 0)
    {
        printf("\r SEU  Error Injection Failed \n");
        return;
    }
    seu_err_data = seu_read_err();
    if (seu_err_data.op_state != 0)
    {
        printf("\r Error Read Failed \n");
        return;
    }
    printf("\r Error Count %u \n",seu_err_data.err_cnt);
    printf("\r Injected Sector Address %u \n",seu_err_data.sector_addr);
    printf("\r Error Type %u \n",seu_err_data.err_type);
    printf("\r Node Specific Status %u \n",seu_err_data.node_specific_status);
    printf("\r Correction Status %u \n",seu_err_data.correction_status);

    seu_err_stats = seu_read_stat(SEU_SECTOR_ADDR);
    if (seu_err_stats.op_state != 0)
    {
        printf("\r Error Stat Failed \n");
        return;
    }
    printf("\r SEU Cycle %u \n",seu_err_stats.t_seu_cycle);
    printf("\r SEU Detect %u \n",seu_err_stats.t_seu_detect);
    printf("\r SEU correct  %u \n",seu_err_stats.t_seu_correct);
    printf("\r SEU Inject Detect %u \n",seu_err_stats.t_seu_inject_detect);
    printf("\r SDM SEU Poll Interval %u \n",
            seu_err_stats.t_sdm_seu_poll_interval);

    ret = seu_insert_ecc_err( ECC_ERROR_TYPE, RAM_ID, ECC_SECTOR_ADDR );
    if (seu_err_stats.op_state != 0)
    {
        printf("\r ECC Error Insertion Failed \n");
        return;
    }
    ret = seu_deinit();
    if (ret != 0)
    {
        printf("\r Failed to close SEU\n");
    }

    printf("\r Sample applicaton for seu and ecc error injection completed \n");
}
