/*
 * SPDX-FileCopyrightText: Copyright (C) 2026 Altera Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 * SDMMC multi block task
 */

#include "FreeRTOS.h"
#include "task.h"
#include "osal.h"
#include "osal_log.h"
#include "ff_sddisk.h"
#include "ff_sys.h"
#include "portmacro.h"
#include <stdbool.h>

#define HET_STRESS_SDMMC_MOUNT_POINT  "/sdcard"
#define HET_STRESS_SDMMC_BUF_SIZE     (10 * 1024)

static uint8_t sdmmc_wr_buf[HET_STRESS_SDMMC_BUF_SIZE + 10] __attribute__((aligned(64)));
static uint8_t sdmmc_rd_buf[HET_STRESS_SDMMC_BUF_SIZE] __attribute__((aligned(64)));

extern uint32_t ulGetPhysicalCoreMpidr(void);
static const char *sdmmc_file_names[] =
{
    "/sample0.txt",
    "/sample1.txt",
    "/sample2.txt",
    "/sample3.txt",
    "/sample4.txt",
    "/sample5.txt"
};

static volatile uint32_t hetero_sd_error_count = 0;
static volatile uint32_t hetero_sd_success_count = 0;
static volatile uint32_t hetero_sd_current_core = 0;
static volatile bool hetero_sd_stop_requested = false;
static volatile bool hetero_sd_stopped = false;

static void init_sdmmc_test_data(void)
{
    for (uint32_t i = 0; i < HET_STRESS_SDMMC_BUF_SIZE + 10; i++)
    {
        sdmmc_wr_buf[i] = i % 256;
    }
}

void hetero_sd_task(void *arg)
{
    FF_FILE *sdmmc_file;
    FF_Error_t ff_error;
    FF_Disk_t *sdmmc_disk;
    size_t bytes_written, bytes_read;
    bool read_fail = false, write_fail = false, open_fail = false;
    int n_files = 5, n_xfer = 10;

    (void)arg;

    sdmmc_disk = FF_SDDiskInit(HET_STRESS_SDMMC_MOUNT_POINT, -1);
    if (sdmmc_disk == NULL)
    {
        ERROR("SDMMC init failed");
        hetero_sd_error_count++;
        vTaskDelete(NULL);
        return;
    }

    ff_error = FF_Mount(sdmmc_disk, 0);
    if (ff_error != FF_ERR_NONE)
    {
        ERROR("SDMMC mount failed");
        hetero_sd_error_count++;
        FF_SDDiskDelete(sdmmc_disk);
        vTaskDelete(NULL);
        return;
    }

    init_sdmmc_test_data();

    while (!hetero_sd_stop_requested)
    {
        hetero_sd_current_core = ulGetPhysicalCoreMpidr();
        int xfer = 0;
        for (int cycle = 0; cycle < (n_xfer / n_files); cycle++)
        {
            xfer = cycle * n_files;

            for (int i = 0; i < n_files; i++)
            {
                sdmmc_file = FF_Open(sdmmc_disk->pxIOManager,
                                     sdmmc_file_names[i],
                                     FF_MODE_WRITE | FF_MODE_CREATE, &ff_error);
                if ((sdmmc_file == NULL) || (ff_error != FF_ERR_NONE))
                {
                    ERROR("File open failed for write");
                    hetero_sd_error_count++;
                    open_fail = true;
                    break;
                }

                bytes_written = FF_Write(sdmmc_file, 1,
                                         HET_STRESS_SDMMC_BUF_SIZE,
                                         sdmmc_wr_buf + xfer);
                if (bytes_written != HET_STRESS_SDMMC_BUF_SIZE)
                {
                    ERROR("File %d write failed", i);
                    hetero_sd_error_count++;
                    write_fail = true;
                    break;
                }
                FF_Close(sdmmc_file);
                xfer = (xfer + 1) % 10;
            }
            if (open_fail || write_fail)
            {
                open_fail = false;
                write_fail = false;
                break;
            }

            xfer = cycle * n_files;
            for (int i = 0; i < n_files; i++)
            {
                sdmmc_file = FF_Open(sdmmc_disk->pxIOManager,
                                     sdmmc_file_names[i],
                                     FF_MODE_READ, &ff_error);
                if ((sdmmc_file == NULL) || (ff_error != FF_ERR_NONE))
                {
                    ERROR("File open failed for read");
                    hetero_sd_error_count++;
                    open_fail = true;
                    break;
                }

                bytes_read = FF_Read(sdmmc_file, 1,
                                     HET_STRESS_SDMMC_BUF_SIZE,
                                     sdmmc_rd_buf);
                if (bytes_read <= 0)
                {
                    ERROR("File %d read failed", i);
                    hetero_sd_error_count++;
                    read_fail = true;
                    break;
                }
                if (sdmmc_rd_buf[HET_STRESS_SDMMC_BUF_SIZE - 1] !=
                    sdmmc_wr_buf[(HET_STRESS_SDMMC_BUF_SIZE - 1) + xfer])
                {
                    ERROR("SDMMC data mismatch");
                    hetero_sd_error_count++;
                }
                else
                {
                    hetero_sd_success_count++;
                }
                FF_Close(sdmmc_file);
                xfer = (xfer + 1) % 10;
            }
            if (open_fail || read_fail)
            {
                open_fail = false;
                read_fail = false;
                break;
            }
        }
        if (open_fail || read_fail || write_fail)
        {
            open_fail = false;
            read_fail = false;
            write_fail = false;
            continue;
        }
    }

    ff_error = FF_Unmount(sdmmc_disk);
    if (ff_error != FF_ERR_NONE)
    {
        ERROR("SDMMC unmount failed");
        hetero_sd_error_count++;
    }
    FF_SDDiskDelete(sdmmc_disk);
    hetero_sd_stopped = true;
    vTaskDelete(NULL);
}

void hetero_sd_request_stop(void)
{
    hetero_sd_stop_requested = true;
}

void hetero_sd_print_status(void)
{
    PRINT("SDMMC: physical core 0x%lx ok %lu errors %lu",
          (unsigned long)hetero_sd_current_core,
          (unsigned long)hetero_sd_success_count,
          (unsigned long)hetero_sd_error_count);
}
