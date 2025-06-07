/*
 * SPDX-FileCopyrightText: Copyright (C) 2024 Intel Corporation
 * SPDX-License-Identifier: MIT-0
 */

/*
 * SPDX-FileCopyrightText: Copyright (C) 2025 Altera Corporation
 *
 * SPDX-License-Identifier: MIT-0
 *
 * Implementation of LW HPS to FPGA bridge sample application
 */

#include "FreeRTOS.h"
#include "bridge_sample.h"
#include "osal_log.h"

int lwhps2fpga_bridge_sample(void)
{
    uint32_t sysid_id;
    uint32_t sysid_ts;
    uint64_t *first_word_ptr;
    uint64_t *last_word_ptr;
    uint64_t temp;

    /* validate the sysid signature */
    sysid_id = *((volatile uint32_t *)(LWH2F_SYSID_ID_BASE));

    if (sysid_id != LWH2_SIG)
    {
        ERROR("SYSID does not match");
        return 0;
    }

    /* capture the sysid timestamp */
    sysid_ts = *((volatile uint32_t *)(LWH2F_SYSID_TS_BASE));

    INFO("SYSID_ID : %x", sysid_id);
    INFO("SYSID_TS : %x", sysid_ts);

    /* write and read the memories in the FPGA fabric */
    first_word_ptr = (uint64_t *)(LWH2F_512M_BASE);
    last_word_ptr =
            (uint64_t *)(LWH2F_512M_BASE + LWH2F_512M_SPAN - sizeof(uint64_t));

    *first_word_ptr = 0x1111111111111111;

    *last_word_ptr = 0x2222222222222222;

    INFO("WRITE DATA via LWHPS2FPGA bridge");

    INFO("buffer1 : %lx", *first_word_ptr);
    INFO("buffer2 : %lx", *last_word_ptr);

    INFO("READ DATA via LWHPS2FPGA bridge");

    /* read from first word */
    temp = *first_word_ptr;
    INFO("buffer1 : %lx", temp);

    temp = *last_word_ptr;
    INFO("buffer2 : %lx", temp);

    return 1;
}

