/*
 * SPDX-FileCopyrightText: Copyright (C) 2024 Intel Corporation
 * SPDX-License-Identifier: MIT-0
 */

/*
 * SPDX-FileCopyrightText: Copyright (C) 2025 Altera Corporation
 *
 * SPDX-License-Identifier: MIT-0
 *
 * Implementation of FPGA to HPS bridge sample application
 */

#include <string.h>
#include "FreeRTOS.h"
#include "bridge_sample.h"
#include "socfpga_cache.h"
#include "osal_log.h"

int fpga2hps_bridge_sample(void)
{
    uint64_t i;
    int status = 0;
    uint32_t sysid_id;
    uint32_t sysid_ts;
    uint64_t cntfrq_el0;
    uint64_t timeout;
    uint64_t temp;
    uint32_t temp_32;
    uint8_t *dma_buffer_0 = (void*)(F2H_BUF_0_BASE);
    uint8_t *dma_buffer_1 = (void*)(F2H_BUF_1_BASE);
    uint64_t *dma_buffer_0_word_ptr = (uint64_t *)dma_buffer_0;
    uint32_t descriptor[8];
    uint64_t dma_wr_ptr;
    uint64_t dma_rd_ptr;

    /* validate the sysid signature */
    sysid_id = *((volatile uint32_t *)(F2H_SYSID_ID_BASE));

    if (sysid_id != F2H_SIG)
    {
        ERROR("F2H sysid_id does not match");
        return 0;
    }

    INFO("SYSID_ID : %x", sysid_id);

    /* capture the sysid timestamp */
    sysid_ts = *((volatile uint32_t *)(F2H_SYSID_TS_BASE));
    INFO("SYSID_TS : %x", sysid_ts);

    /*
     * we will now DMA data from dma_buffer_0 to the FPGA OCRAM buffer
     * and then DMA from the FPGA OCRAM buffer to dma_buffer_1
     *
     * the data cache WILL be flushed prior to triggering the dma so the
     * coherent transactions WILL NOT hit the cache
     */

    /* get the system counter frequency to calculate timeout delays */
    asm volatile (
        "mrs %[cntfrq_el0], cntfrq_el0\n"
        : [cntfrq_el0] "=r" (cntfrq_el0)
        );

    /* clear the three buffers that we'll use for DMA transfers */
    /* this isn't necessary but for demonstration purposes we'll do it */
    memset(dma_buffer_0, 0, F2H_OCRAM_32K_SPAN);
    memset(dma_buffer_1, 0, F2H_OCRAM_32K_SPAN);

    cache_force_write_back(dma_buffer_0, F2H_OCRAM_32K_SPAN);
    cache_force_write_back(dma_buffer_1, F2H_OCRAM_32K_SPAN);

    /* fill the initial buffer with a randomized data pattern */
    asm volatile (
        "mrs %[temp], cntpct_el0\n"
        : [temp] "=r" (temp)
        );
    temp ^= 0xFbFdF1F3F5F7FbFd;
    temp += 0x0b0d010305070b0d;
    for (i = 0; i < (F2H_OCRAM_32K_SPAN / sizeof(uint64_t)); i++)
    {
        temp += 0x0b0d010305070b0d;
        dma_buffer_0_word_ptr[i] = temp++;
    }

    /* flush the data cache */
    cache_flush(dma_buffer_0, F2H_OCRAM_32K_SPAN);
    cache_flush(dma_buffer_1, F2H_OCRAM_32K_SPAN);

    /* make sure the MSGDMA is not currently busy */
    temp_32 = ((volatile uint32_t *)(F2H_MSGDMA_CSR_BASE))[0];
    if (temp_32 != 0x00000002)
    {
        ERROR("DMA busy error!!!");
        return 0;
    }

    /* build our descriptor for dma_buffer_0 to FPGA OCRAM */
    descriptor[0] = (uint64_t)(dma_buffer_0) & 0xFFFFFFFF;
    descriptor[1] = MSGDMA_OCRAM_32K_BASE & 0xFFFFFFFF;
    descriptor[2] = F2H_OCRAM_32K_SPAN;
    descriptor[3] = 0x00000000;
    descriptor[4] = 0x00000000;
    descriptor[5] = (uint64_t)(dma_buffer_0) >> 32;
    descriptor[6] = MSGDMA_OCRAM_32K_BASE >> 32;
    descriptor[7] = 0x80000000;

    /* push the descriptor into the MSGDMA */
    for (i = 0; i < 8; i++)
    {
        ((volatile uint32_t *)(F2H_MSGDMA_DES_BASE))[i] = descriptor[i];
    }

    /* wait for the DMA to idle, timeout after 1ms */
    asm volatile (
        "dsb sy\n"
        "isb\n"
        "mrs %[temp], cntpct_el0\n"
        : [temp] "=r" (temp)
        );
    timeout = temp + (cntfrq_el0 / 1000);

    temp_32 = ((volatile uint32_t *)(F2H_MSGDMA_CSR_BASE))[0];
    while ((temp_32 & 0x00000001) == 0x00000001) {
        asm volatile (
            "dsb sy\n"
            "isb\n"
            "mrs %[temp], cntpct_el0\n"
            : [temp] "=r" (temp)
            );
        if (temp >= timeout) {
            ERROR("DMA timeout occured");
            return 0;
        }
        temp_32 = ((volatile uint32_t *)(F2H_MSGDMA_CSR_BASE))[0];
    }

    /* initialize intermediate DMA pointers for F2H transfer */
    dma_rd_ptr = MSGDMA_OCRAM_32K_BASE;
    dma_wr_ptr = (uint64_t)(dma_buffer_1);

    /* make sure the MSGDMA is not currently busy */
    temp_32 = ((volatile uint32_t *)(F2H_MSGDMA_CSR_BASE))[0];
    if (temp_32 != 0x00000002) {
        ERROR("DMA busy error!!!");
        return -1;
    }

    /* build our descriptor for FPGA OCRAM to dma_buffer_0 */
    descriptor[0] = dma_rd_ptr & 0xFFFFFFFF;
    descriptor[1] = dma_wr_ptr & 0xFFFFFFFF;
    descriptor[2] = F2H_OCRAM_32K_SPAN;
    descriptor[3] = 0x00000000;
    descriptor[4] = 0x00000000;
    descriptor[5] = dma_rd_ptr >> 32;
    descriptor[6] = dma_wr_ptr >> 32;
    descriptor[7] = 0x80000000;

    /* push the descriptor into the MSGDMA */
    for (i = 0; i < 4; i++)
    {
        ((volatile uint64_t *)(F2H_MSGDMA_DES_BASE))[i] =
                ((uint64_t)(descriptor[(i * 2) + 1]) << 32) | descriptor[i * 2];
    }

    /* wait for the DMA to idle, timeout after 1ms */
    asm volatile (
        "dsb sy\n"
        "isb\n"
        "mrs %[temp], cntpct_el0\n"
        : [temp] "=r" (temp)
        );
    timeout = temp + (cntfrq_el0 / 1000);

    temp_32 = ((volatile uint32_t *)(F2H_MSGDMA_CSR_BASE))[0];
    while ((temp_32 & 0x00000001) == 0x00000001) {
        asm volatile (
            "dsb sy\n"
            "isb\n"
            "mrs %[temp], cntpct_el0\n"
            : [temp] "=r" (temp)
            );
        if (temp >= timeout) {
            ERROR("DMA timeout occured");
            return 0;
        }
        temp_32 = ((volatile uint32_t *)(F2H_MSGDMA_CSR_BASE))[0];
    }

    /* dsb so that a76 waits for outstanding io before comparing buffers */
    asm volatile (
        "dsb sy\n"
        );


    /* verify the initial buffer matches the final buffer */
    cache_force_invalidate(dma_buffer_0, F2H_OCRAM_32K_SPAN);
    cache_force_invalidate(dma_buffer_1, F2H_OCRAM_32K_SPAN);

    if (memcmp(dma_buffer_0, dma_buffer_1, F2H_OCRAM_32K_SPAN) == 0)
    {
        status = 1;
    }
    else
    {
        return 0;
    }

    /*
     * we will now DMA data from dma_buffer_0 to the FPGA OCRAM buffer
     * and then DMA from the FPGA OCRAM buffer to dma_buffer_1
     *
     * the data cache WILL NOT be flushed prior to triggering the dma so the
     * coherent transactions WILL hit the cache
     */

    /* get the system counter frequency to calculate timeout delays */
    asm volatile (
        "mrs %[cntfrq_el0], cntfrq_el0\n"
        : [cntfrq_el0] "=r" (cntfrq_el0)
        );

    /* clear the three buffers that we'll use for DMA transfers */
    /* this isn't necessary but for demonstration purposes we'll do it */
    memset(dma_buffer_0, 0, F2H_OCRAM_32K_SPAN);
    memset(dma_buffer_1, 0, F2H_OCRAM_32K_SPAN);

    cache_force_write_back(dma_buffer_0, F2H_OCRAM_32K_SPAN);
    cache_force_write_back(dma_buffer_1, F2H_OCRAM_32K_SPAN);


    /* fill the initial buffer with a randomized data pattern */
    asm volatile (
        "mrs %[temp], cntpct_el0\n"
        : [temp] "=r" (temp)
        );
    temp ^= 0xFbFdF1F3F5F7FbFd;
    temp += 0x0b0d010305070b0d;
    for (i = 0; i < (F2H_OCRAM_32K_SPAN / sizeof(uint64_t)); i++)
    {
        temp += 0x0b0d010305070b0d;
        dma_buffer_0_word_ptr[i] = temp++;
    }

    /* need to check the need for cache write back */
    cache_force_write_back(dma_buffer_0, F2H_OCRAM_32K_SPAN);
    cache_force_write_back(dma_buffer_1, F2H_OCRAM_32K_SPAN);


    /* make sure the MSGDMA is not currently busy */
    temp_32 = ((volatile uint32_t *)(F2H_MSGDMA_CSR_BASE))[0];
    if (temp_32 != 0x00000002) {
        ERROR("DMA busy error!!!");
        return 0;
    }

    /* build our descriptor for dma_buffer_0 to FPGA OCRAM */
    descriptor[0] = (uint64_t)(dma_buffer_0) & 0xFFFFFFFF;
    descriptor[1] = MSGDMA_OCRAM_32K_BASE & 0xFFFFFFFF;
    descriptor[2] = F2H_OCRAM_32K_SPAN;
    descriptor[3] = 0x00000000;
    descriptor[4] = 0x00000000;
    descriptor[5] = (uint64_t)(dma_buffer_0) >> 32;
    descriptor[6] = MSGDMA_OCRAM_32K_BASE >> 32;
    descriptor[7] = 0x80000000;

    /* push the descriptor into the MSGDMA */
    for (i = 0; i < 8; i++)
    {
        ((volatile uint32_t *)(F2H_MSGDMA_DES_BASE))[i] =
                descriptor[i];
    }

    /* wait for the DMA to idle, timeout after 1ms */
    asm volatile (
        "dsb sy\n"
        "isb\n"
        "mrs %[temp], cntpct_el0\n"
        : [temp] "=r" (temp)
        );
    timeout = temp + (cntfrq_el0 / 1000);

    temp_32 = ((volatile uint32_t *)(F2H_MSGDMA_CSR_BASE))[0];
    while ((temp_32 & 0x00000001) == 0x00000001) {
        asm volatile (
            "dsb sy\n"
            "isb\n"
            "mrs %[temp], cntpct_el0\n"
            : [temp] "=r" (temp)
            );
        if (temp >= timeout) {
            ERROR("DMA timeout occuerd!!!");
            return 0;
        }
        temp_32 = ((volatile uint32_t *)(F2H_MSGDMA_CSR_BASE))[0];
    }

    /* initialize intermediate DMA pointers for F2H transfer */
    dma_rd_ptr = MSGDMA_OCRAM_32K_BASE;
    dma_wr_ptr = (uint64_t)(dma_buffer_1);

    /* make sure the MSGDMA is not currently busy */
    temp_32 = ((volatile uint32_t *)(F2H_MSGDMA_CSR_BASE))[0];
    if (temp_32 != 0x00000002) {
        ERROR("DMA busy error!!!");
        return 0;
    }

    /* build our descriptor for FPGA OCRAM to dma_buffer_0 */
    descriptor[0] = dma_rd_ptr & 0xFFFFFFFF;
    descriptor[1] = dma_wr_ptr & 0xFFFFFFFF;
    descriptor[2] = F2H_OCRAM_32K_SPAN;
    descriptor[3] = 0x00000000;
    descriptor[4] = 0x00000000;
    descriptor[5] = dma_rd_ptr >> 32;
    descriptor[6] = dma_wr_ptr >> 32;
    descriptor[7] = 0x80000000;

    /* push the descriptor into the MSGDMA */
    for (i = 0; i < 4; i++)
    {
        ((volatile uint64_t *)(F2H_MSGDMA_DES_BASE))[i] =
                ((uint64_t)(descriptor[(i * 2) + 1]) << 32) | descriptor[i * 2];
    }

    /* wait for the DMA to idle, timeout after 1ms */
    asm volatile (
        "dsb sy\n"
        "isb\n"
        "mrs %[temp], cntpct_el0\n"
        : [temp] "=r" (temp)
        );
    timeout = temp + (cntfrq_el0 / 1000);

    temp_32 = ((volatile uint32_t *)(F2H_MSGDMA_CSR_BASE))[0];
    while ((temp_32 & 0x00000001) == 0x00000001) {
        asm volatile (
            "dsb sy\n"
            "isb\n"
            "mrs %[temp], cntpct_el0\n"
            : [temp] "=r" (temp)
            );
        if (temp >= timeout) {
            ERROR("DMA timeout occured");
            return 0;
        }
        temp_32 =
                ((volatile uint32_t *)(F2H_MSGDMA_CSR_BASE))[0];
    }

    /* dsb so that a76 waits for outstanding io before comparing buffers */
    asm volatile (
        "dsb sy\n"
        );

    /* verify the initial buffer matches the final buffer */
    cache_force_invalidate(dma_buffer_0, F2H_OCRAM_32K_SPAN);
    cache_force_invalidate(dma_buffer_1, F2H_OCRAM_32K_SPAN);

    if (memcmp(dma_buffer_0, dma_buffer_1, F2H_OCRAM_32K_SPAN) == 0)
    {
        status = 1;
    }
    else
    {
        return 0;
    }

    return status;
}

