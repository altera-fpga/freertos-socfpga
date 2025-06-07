/*
 * SPDX-FileCopyrightText: Copyright (C) 2024 Intel Corporation
 * SPDX-License-Identifier: MIT-0
 */

/*
 * SPDX-FileCopyrightText: Copyright (C) 2025 Altera Corporation
 *
 * SPDX-License-Identifier: MIT-0
 *
 * Implementation of HPS to FPGA bridge sample application
 */

#include "FreeRTOS.h"
#include "bridge_sample.h"
#include "osal_log.h"

int hps2fpga_bridge_sample(void)
{
    int i;
    int j;
    uint32_t sysid_id;
    uint32_t sysid_ts;
    long double *first_word_ptr[3];
    long double *last_word_ptr[3];
    long double first_word_write_value[3][2];
    long double last_word_write_value[3][2];
    long double first_word_read_value[3][2];
    long double last_word_read_value[3][2];

    /* validate the sysid signature */
    sysid_id = *((volatile uint32_t *)(H2F_SYSID_ID_BASE));

    if (sysid_id != H2F_SIG)
    {
        ERROR("H2F SYSID does not match");
        return 0;
    }

    /* capture the sysid timestamp */
    sysid_ts = *((volatile uint32_t *)(H2F_SYSID_TS_BASE));

    INFO("SYSID id : %x", sysid_id);
    INFO("SYSID ts : %x", sysid_ts);

    /* initialize pointers to first and last word of spans */
    first_word_ptr[0] = (long double *)(H2F_1G_BASE);
    last_word_ptr[0] =
            (long double *)(H2F_1G_BASE + H2F_1G_SPAN - sizeof(long double));
    first_word_ptr[1] = (long double *)(H2F_15G_BASE);
    last_word_ptr[1] =
            (long double *)(H2F_15G_BASE + H2F_15G_SPAN - sizeof(long double));
    first_word_ptr[2] = (long double *)(H2F_240G_BASE);
    last_word_ptr[2] =
            (long double *)(H2F_240G_BASE + H2F_240G_SPAN -
            sizeof(long double));

    /* initialize write data patterns */
    first_word_write_value[0][0] = 0x111111;
    last_word_write_value[0][0] = 0x22222;
    first_word_write_value[1][0] = 0x33333;
    last_word_write_value[1][0] = 0x44444;
    first_word_write_value[2][0] = 0x55555;
    last_word_write_value[2][0] = 0x666666;

    first_word_write_value[0][1] = ~0x11111;
    last_word_write_value[0][1] = ~0x2222;
    first_word_write_value[1][1] = ~0x33;
    last_word_write_value[1][1] = ~0x444;
    first_word_write_value[2][1] = ~0x555;
    last_word_write_value[2][1] = ~0x666;

    /* write values to first and last word of each span and read back */
    for (j = 0; j < 2; j++)
    {
        for (i = 0; i < 3; i++)
        {

            *first_word_ptr[i] = first_word_write_value[i][j];

            *last_word_ptr[i] = last_word_write_value[i][j];
        }

        for (i = 0; i < 3; i++)
        {
            /* read from first word */

            first_word_read_value[i][j] = *first_word_ptr[i];

            last_word_read_value[i][j] = *last_word_ptr[i];
        }
    }

    for (int i = 0; i < 2; i++)
    {
        for (int j = 0; j < 3; j++)
        {
            /* Compare read and write buffers */
            if ((first_word_read_value[j][i] != first_word_write_value[j][i]) ||
                    (last_word_read_value[j][i] != last_word_write_value[j][i]))
            {
                return 0;
            }
        }
    }

    return 1;
}

