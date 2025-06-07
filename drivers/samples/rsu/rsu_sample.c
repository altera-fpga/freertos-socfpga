/*
 * Copyright (c) 2025, Intel Corporation.
 *
 * SPDX-License-Identifier: MIT
 *
 * Sample application implementation for RSU
 */

#include <stdint.h>
#include "osal_log.h"

#include <libRSU.h>
#include <libRSU_OSAL.h>

/**
 * @defgroup rsu_sample RSU 
 * @ingroup samples
 *
 * Sample Application for RSU
 *
 * @details
 * @subsubsection description Description
 * This is a simple program to demonstrate the use of librsu. The sample application
 * erases the slot 2 and program an image from the sdcard to that slot and verifies it
 * and loads it after reboot
 * 
 * @subsubsection prerequisites Prerequisites
 * - Make sure to use a RSU an supported image with atleast 2 slots with ATF version 2.12.0
 * - Make sure that an rpd file is stored in the sdcard and its name is used in the variable(name below 5 characters) 'pcRsuFileName' <br>
 * - The sample pfg file to create the initial RSU jic image is available at @c drivers/samples/rsu/initial_image.pfg which create 6 slots(0 to 5). 
 * - Use the correct sof and bl2.bin and bl31.bin to build initial_image.jic
 * 
 * @subsubsection how_to_run How to Run
 * 1. Follow the common README for build and flashing instructions.
 * 2. Run the application on the board on top of the correct RSU image.
 * 3. After the successful execution of the sample application the board goes for an warm reboot. Restart the board once to load the new slot
 *
 * @subsubsection expected_results Expected Results
 * - The board goes for a warm reboot. After a power cycle the board boots up in the new slot.
 * @{
 */
/** @} */

char *pcRsuFileName    = "/app2.rpd";

static int rsu_client_get_slot_count(void)
{
    return rsu_slot_count();
}

static int rsu_client_erase_image(int slot_num)
{
    return rsu_slot_erase(slot_num);
}

static int rsu_client_copy_status_log(void)
{
    struct rsu_status_info info;
    int rtn = -1;

    if (!rsu_status_log(&info)) {
        rtn = 0;
        printf("      VERSION: 0x%08X\r\n", (int)info.version);
        printf("        STATE: 0x%08X\r\n", (int)info.state);
        printf("CURRENT IMAGE: 0x%016lX\r\n", info.current_image);
        printf("   FAIL IMAGE: 0x%016lX\r\n", info.fail_image);
        printf("    ERROR LOC: 0x%08X\r\n", (int)info.error_location);
        printf("ERROR DETAILS: 0x%08X\r\n", (int)info.error_details);
        if (RSU_VERSION_DCMF_VERSION(info.version) &&
                RSU_VERSION_ACMF_VERSION(info.version)) {
            printf("RETRY COUNTER: 0x%08X\r\n", (int)info.retry_counter);
        }
    }
    return rtn;
}

static int rsu_client_list_slot_attribute(int slot_num)
{
    struct rsu_slot_info info;
    int rtn = -1;

    if (!rsu_slot_get_info(slot_num, &info)) {
        rtn = 0;
        printf("      NAME: %s\r\n", info.name);
        printf("    OFFSET: 0x%016lX\r\n", info.offset);
        printf("      SIZE: 0x%08X\r\n", info.size);

        if (info.priority) {
            printf("  PRIORITY: %i\r\n", info.priority);
        } else {
            printf("  PRIORITY: [disabled]\r\n");
        }
    }
    return rtn;
}

static int rsu_client_add_app_image(char *image_name, int slot_num, int raw)
{
    if (raw) {
        return rsu_slot_program_file_raw(slot_num, image_name);
    }

    return rsu_slot_program_file(slot_num, image_name);
}

static int rsu_client_verify_data(char *file_name, int slot_num, int raw)
{
    if (raw) {
        return rsu_slot_verify_file_raw(slot_num, file_name);
    }

    return rsu_slot_verify_file(slot_num, file_name);
}

static int rsu_client_request_slot_be_loaded(int slot_num)
{
    return rsu_slot_load_after_reboot(slot_num);
}

void vRsuTask( void  )
{
    int lRet, slotCount;
    INFO("Starting  RSU sample");

    lRet = librsu_init("");
    if (lRet != 0)
    {
        ERROR("RSU initialization failed!!");
        return;
    }
    /*Get the slot count to print the details*/
    slotCount = rsu_client_get_slot_count();
    if (slotCount < 0)
    {
        ERROR("No available slots");
        return;
    }
    INFO("The number of slots avilable is :%d", slotCount);
    INFO("Available slot Info:");
    for (int idx = 0; idx < slotCount; idx++)
    {
        lRet = rsu_client_list_slot_attribute(idx);
        if (lRet != 0)
        {
            ERROR("Failed to get attributes for slot: %d", idx);
            return;
        }
    }
    INFO("Current slot status:");
    lRet = rsu_client_copy_status_log();
    if (lRet != 0)
    {
        ERROR("Failed to get the status log");
        return;
    }

    INFO("Erasing the slot 1..");
    lRet = rsu_client_erase_image(1);
    if (lRet != 0)
    {
        ERROR("Failed to erase the slot");
        return;
    }
    INFO("Slot 1  erase completed");

    INFO("Programming the image to slot 1..");
    lRet = rsu_client_add_app_image(pcRsuFileName, 1, 0);
    if (lRet != 0)
    {
        ERROR("Failed to program the image");
        return;
    }

    INFO("Completed programming image");

    INFO("Verifying the image..");
    lRet = rsu_client_verify_data(pcRsuFileName, 1, 0);
    if (lRet != 0)
    {
        ERROR("Slot verifiation failed");
        return;
    }
    INFO("Image verification completed");
    INFO("Loading 1st slot..");
    lRet = rsu_client_request_slot_be_loaded(1);
    if (lRet != 0)
    {
        ERROR("Failed to load the 1st slot");
        return;
    }
    INFO("Completed loading the slot");

    INFO("RSU sample exit\r\n");
}

