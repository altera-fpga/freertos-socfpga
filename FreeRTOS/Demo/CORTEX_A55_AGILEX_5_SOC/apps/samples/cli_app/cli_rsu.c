/*
 * SPDX-FileCopyrightText: Copyright (C) 2025 Altera Corporation
 *
 * SPDX-License-Identifier: MIT-0
 *
 * Implementation of CLI commands for RSU
 */

 /**
 * @defgroup cli_rsu RSU
 * @ingroup cli
 *
 * Perform operations on RSU
 *
 * @details
 * It supports the following commands:
 * -rsu count
 * -rsu log
 * -rsu info    <slot>
 * -rsu erase   <slot>
 * -rsu program <slot> <file>
 * -rsu verify  <slot> <file>
 * -rsu load    <slot>
 * -rsu help 
 *
 * Typical usage:
 * - Use  'rsu count' command to get the number of slots..
 * - Use  'rsu log'   command to get current slot details.
 * - Use  'rsu erase' command to erase a slot.
 * - Use  'rsu program' command to program a slot with sdcard file..
 * - Use  'rsu verify'  command to verify a slot with a sdcard file.
 * - Use  'rsu load'    command to load a slot after reboot..
 * @subsection commands Commands
 * @subsubsection rsu_count rsu count
 * Count the number of rsu slots  <br>
 *
 * Usage:  <br>
 *   rsu count  <br>
 *
 * No arguments.  <br>
 *
 * @subsubsection rsu_log rsu log
 * List the details of current slot  <br>
 *
 * Usage:  <br>
 *   rsu log <br>
 *
 * No arguments
 *
 * @subsubsection rsu_info rsu info
 * Get the information about the slot number  <br>
 *
 * Usage:  <br>
 *   rsu info <slotNum>  <br>
 * It requires the following arguments:
 * - slotNum -      The slot number  <br>
 *
 * @subsubsection rsu_erase rsu erase
 * Erase the content of the slot  <br>
 *
 * Usage:  <br>
 *   rsu erase <slotNum>  <br>
 *
 * It requires the following arguments:
 * - slotNum -      The slot number  <br>
 *
 * @subsubsection rsu_program rsu program
 * Write image to a given slot  <br>
 *
 * Usage:  <br>
 *   rsu program <slotNum> <file> <br>
 * It requires the following arguments:
 * - slotNum -      The slot number  <br>
 * - file    -      The name of the sdcard image(use absolute path).  <br>
 *
 * @subsubsection rsu_verify rsu verify
 * Verify image in the slot to the file  <br>
 *
 * Usage:  <br>
 *   rsu verify <slotNum> <file> <br>
 * It requires the following arguments:
 * - slotNum -      The slot number  <br>
 * - file    -      The name of the sdcard image(use absolute path).  <br>
 *
 * @subsubsection rsu_load rsu load
 * Load the image in the given slot after reboot  <br>
 *
 * Usage:  <br>
 *   rsu load <slotNum> <br>
 * It requires the following arguments:
 * - slotNum -      The slot number  <br>
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <FreeRTOS.h>
#include <socfpga_uart.h>
#include <FreeRTOS_CLI.h>
#include "cli_app.h"
#include <libRSU.h>

#define SLOT_COUNT_CMD    ("count")
#define SLOT_INFO_CMD     ("info")
#define SLOT_ERASE_CMD    ("erase")
#define SLOT_PRGM_CMD     ("program")
#define SLOT_LOAD_CMD     ("load")
#define STATUS_LOG_CMD    ("log")
#define SLOT_VRFY_CMD     ("verify")


BaseType_t cmd_rsu( char *pcWriteBuffer, size_t xWriteBufferLen,
        const char *pcCommandString )
{
    (void) xWriteBufferLen;
    (void) pcWriteBuffer;

    const char *pcParam1;
    const char *pcParam2;
    const char *pcParam3;

    BaseType_t Param1StrLen;
    BaseType_t Param2StrLen;
    BaseType_t Param3StrLen;

    int slotNum;
    char tempStr[20] = { 0 };

    pcParam1 = FreeRTOS_CLIGetParameter(pcCommandString, 1, &Param1StrLen);
    strncpy(tempStr, pcParam1, Param1StrLen);

    if (strncmp( pcParam1, "help", 4) == 0)
    {
        printf("\r\nPerform rsu operations in a rsu suppoted image"
                "\r\n\nIt supports the following sub commands:"
                "\r\n  rsu count"
                "\r\n  rsu log"
                "\r\n  rsu info    <slot>"
                "\r\n  rsu erase   <slot>"
                "\r\n  rsu program <slot> <file>"
                "\r\n  rsu verify  <slot> <file>"
                "\r\n  rsu load    <slot>"
                "\r\n  rsu help"
                "\r\n\nTypical usage:"
                "\r\n- Use count command to get the number of slots."
                "\r\n- Use log command to get current slot details."
                "\r\n- Use info command to get details of a slot."
                "\r\n- Use erase command to erase a slot."
                "\r\n- Use program command to program a slot with sdcard file."
                "\r\n- Use verify command to verify a slot with a sdcard file."
                "\r\n- Use load command to load a slot after reboot."
                "\r\n\nFor help on the specific commands please do:"
                "\r\n  rsu <command> help\r\n"
                );

        return pdFALSE;
    }

    pcParam2 = FreeRTOS_CLIGetParameter(pcCommandString, 2, &Param2StrLen);
    if (strcmp(tempStr, STATUS_LOG_CMD) == 0)
    {
        struct rsu_status_info info;
        if (librsu_init("") != 0)
        {
            printf("\r\n Failed to initialize libRSU");
            return pdFAIL;
        }
        if (rsu_status_log(&info) != 0)
        {
            printf("\r\nFailed to fetch status log");
            librsu_exit();
            return pdFAIL;
        }
        printf("\r\n      VERSION: 0x%08X", (int)info.version);
        printf("\r\n        STATE: 0x%08X", (int)info.state);
        printf("\r\nCURRENT IMAGE: 0x%016lX", info.current_image);
        printf("\r\n   FAIL IMAGE: 0x%016lX", info.fail_image);
        printf("\r\n    ERROR LOC: 0x%08X", (int)info.error_location);
        printf("\r\nERROR DETAILS: 0x%08X\r\n", (int)info.error_details);
        if (RSU_VERSION_DCMF_VERSION(info.version) &&
                RSU_VERSION_ACMF_VERSION(info.version))
        {
            printf("\r\nRETRY COUNTER: 0x%08X", (int)info.retry_counter);
        }
        librsu_exit();
        return pdFALSE;
    }
    else if (strcmp(tempStr, SLOT_INFO_CMD) == 0)
    {
        if (strncmp( pcParam2, "help", 4 ) == 0)
        {
            printf("\r\nGet the slot info"
                    "\r\n\nUsage:"
                    "\r\n  rsu info <slot>"
                    "\r\n\nIt requires the following arguments:"
                    "\r\n  slot   slot number of the slot."
                    );
            return pdFALSE;
        }
        struct rsu_slot_info info;
        pcParam2 = FreeRTOS_CLIGetParameter(pcCommandString, 2, &Param2StrLen);
        if (pcParam2 == NULL)
        {
            printf("\r\nIncorrect number of arguments for rsu info command");
            printf("\r\nEnter 'help' to view the list of available commands");
            return pdFAIL;
        }
        slotNum = strtol(pcParam2, NULL, 10);
        if (librsu_init("") != 0)
        {
            printf("\r\nFailed to initialize libRSU");
            return pdFAIL;
        }
        int Totalslots = rsu_slot_count();
        if ((slotNum > (Totalslots - 1)) || (slotNum < 0))
        {
            printf("Invalid slot number enter slot number for 0 to %d",
                    Totalslots - 1);
            librsu_exit();
            return pdFAIL;
        }
        if (rsu_slot_get_info(slotNum, &info) != 0)
        {
            printf("\r\nFailed to get slot info");
            librsu_exit();
            return pdFAIL;
        }
        printf("\r\n      NAME: %s", info.name);
        printf("\r\n    OFFSET: 0x%016lX", info.offset);
        printf("\r\n      SIZE: 0x%08X", info.size);
        if (info.priority)
        {
            printf("\r\n  PRIORITY: %i", info.priority);
        } else
        {
            printf("\r\n  PRIORITY: [disabled]");
        }
        librsu_exit();
        return pdFALSE;
    }
    else if (strcmp(tempStr, SLOT_COUNT_CMD) == 0)
    {
        if (librsu_init("") != 0)
        {
            printf("\r\nFailed to initialize libRSU");
            return pdFAIL;
        }
        int slotCount = rsu_slot_count();
        if (slotCount < 0)
        {
            printf("Failed to get the slot count");
            librsu_exit();
            return pdFAIL;
        }
        printf("\r\nTHE SLOT COUNT IS :%d", slotCount);
        librsu_exit();
        return pdFALSE;
    }
    else if (strcmp(tempStr, SLOT_ERASE_CMD) == 0)
    {
        if (strncmp( pcParam2, "help", 4 ) == 0)
        {
            printf("\r\nErase rsu slot"
                    "\r\n\nUsage:"
                    "\r\n  rsu erase <slot>"
                    "\r\n\nIt requires the following arguments:"
                    "\r\n  slot      slot in the rsu image to be erased."
                    );

            return pdFALSE;
        }
        pcParam2 = FreeRTOS_CLIGetParameter(pcCommandString, 2, &Param2StrLen);
        if (pcParam2 == NULL)
        {
            printf(
                    "\r\nIncorrect number of arguments for rsu slot_erase command");
            printf("\r\nEnter 'help' to view the list of available commands");
            return pdFAIL;
        }
        slotNum = strtol(pcParam2, NULL, 10);
        if (librsu_init("") != 0)
        {
            printf("\r\nFailed to initialize libRSU");
            return pdFAIL;
        }
        int Totalslots = rsu_slot_count();
        if ((slotNum > (Totalslots - 1)) || (slotNum < 0))
        {
            printf("Invalid slot number enter slot number for 0 to %d",
                    Totalslots - 1);
            librsu_exit();
            return pdFAIL;
        }
        if (rsu_slot_erase(slotNum) != 0)
        {
            printf("\r\nFailed to ersase the slot : %d", slotNum);
            librsu_exit();
            return pdFAIL;
        }
        printf("\r\nSLOT %d ERASED SUCCESSFULLY", slotNum);
        librsu_exit();
        return pdFALSE;
    }
    else if (strcmp(tempStr, SLOT_PRGM_CMD) == 0)
    {
        if (strncmp( pcParam2, "help", 4 ) == 0)
        {
            printf("\r\nProgram rsu slot with sdcard file"
                    "\r\n\nUsage:"
                    "\r\n  rsu program <slot> <file>"
                    "\r\n\nIt requires the following arguments:"
                    "\r\n  slot   rsu slot that has to be programmed"
                    "\r\n  file   file to be programmed from sdcard(Ex: /app.rpd)"
                    );
            return pdFALSE;
        }
        pcParam2 = FreeRTOS_CLIGetParameter(pcCommandString, 2, &Param2StrLen);
        if (pcParam2 == NULL)
        {
            printf(
                    "\r\nIncorrect number of arguments for rsu slot_program command");
            printf("\r\nEnter 'help' to view the list of available commands");
            return pdFAIL;
        }
        pcParam3 = FreeRTOS_CLIGetParameter(pcCommandString, 3, &Param3StrLen);
        if (pcParam3 == NULL)
        {
            printf(
                    "\r\nIncorrect number of arguments for rsu slot_program command");
            printf("\r\nEnter 'help' to view the list of available commands");
            return pdFAIL;
        }
        slotNum = strtol(pcParam2, NULL, 10);
        if (librsu_init("") != 0)
        {
            printf("\r\nFailed to initialize libRSU");
            return pdFAIL;
        }
        int Totalslots = rsu_slot_count();
        if ((slotNum > (Totalslots - 1)) || (slotNum < 0))
        {
            printf("Invalid slot number enter slot number for 0 to %d",
                    Totalslots - 1);
            librsu_exit();
            return pdFAIL;
        }
        if (rsu_slot_program_file(slotNum, (char *)pcParam3) != 0)
        {
            printf("\r\nFailed to program slot %d", slotNum);
            librsu_exit();
            return pdFAIL;
        }
        printf("\r\nSLOT %d PROGRAMMING SUCCESSFUL", slotNum);
        librsu_exit();
        return pdFALSE;
    }
    else if (strcmp(tempStr, SLOT_VRFY_CMD) == 0)
    {
        if (strncmp( pcParam2, "help", 4 ) == 0)
        {
            printf("\r\nVerify rsu slot with sdcard file"
                    "\r\n\nUsage:"
                    "\r\n  rsu vefify <slot> <file>"
                    "\r\n\nIt requires the following arguments:"
                    "\r\n  slot   rsu slot that has to be programmed"
                    "\r\n  file   file to be verified  from sdcard(Ex: /app.rpd)"
                    );
            return pdFALSE;
        }
        pcParam2 = FreeRTOS_CLIGetParameter(pcCommandString, 2, &Param2StrLen);
        if (pcParam2 == NULL)
        {
            printf(
                    "\r\nIncorrect number of arguments for rsu slot_verify command");
            printf("\r\nEnter 'help' to view the list of available commands");
            return pdFAIL;
        }
        pcParam3 = FreeRTOS_CLIGetParameter(pcCommandString, 3, &Param3StrLen);
        if (pcParam3 == NULL)
        {
            printf(
                    "\r\nIncorrect number of arguments for rsu slot_verify command");
            printf("\r\nEnter 'help' to view the list of available commands");
            return pdFAIL;
        }
        slotNum = strtol(pcParam2, NULL, 0);
        if (librsu_init("") != 0)
        {
            printf("\r\nFailed to initialize libRSU");
            return pdFAIL;
        }
        int Totalslots = rsu_slot_count();
        if ((slotNum > (Totalslots - 1)) || (slotNum < 0))
        {
            printf("Invalid slot number enter slot number for 0 to %d",
                    Totalslots - 1);
            librsu_exit();
            return pdFAIL;
        }
        if (rsu_slot_verify_file(slotNum, (char *)pcParam3) != 0)
        {
            printf("\r\nFailed to verify slot %d", slotNum);
            librsu_exit();
            return pdFAIL;
        }
        printf("\r\nSLOT %d VERIFICATION SUCCESSFUL", slotNum);
        librsu_exit();
        return pdFALSE;
    }
    else if ((strcmp(tempStr, SLOT_LOAD_CMD) == 0))
    {
        if (strncmp( pcParam2, "help", 4 ) == 0)
        {
            printf("\r\nLoad image from the slot"
                    "\r\n\nUsage:"
                    "\r\n  rsu load <slot>"
                    "\r\n\nIt requires the following arguments:"
                    "\r\n  slot   rsu slot that has to be loaded"
                    );
            return pdFALSE;
        }
        pcParam2 = FreeRTOS_CLIGetParameter(pcCommandString, 2, &Param2StrLen);
        if (pcParam2 == NULL)
        {
            printf("\r\nIncorrect number of arguments for rsu slot_load command");
            printf("\r\nEnter 'help' to view the list of available commands");
            return pdFAIL;
        }
        slotNum = strtol(pcParam2, NULL, 0);
        if (librsu_init("") != 0)
        {
            printf("\r\nFailed to initialize libRSU");
            return pdFAIL;
        }
        int Totalslots = rsu_slot_count();
        if ((slotNum > (Totalslots - 1)) || (slotNum < 0))
        {
            printf("Invalid slot number enter slot number for 0 to %d",
                    Totalslots - 1);
            librsu_exit();
            return pdFAIL;
        }
        printf("\r\nREBOOTING SYSTEM FOR SLOT LOAD");
        if (rsu_slot_load_after_reboot(slotNum) != 0)
        {
            printf("Failed to load slot %d", slotNum);
            librsu_exit();
            return pdFAIL;
        }
        librsu_exit();
        return pdFALSE;
    }
    else
    {
        printf("\r\nInvalid command for rsu");
        printf("\r\nEnter 'help' to view the list of available commands.");
        return pdFAIL;
    }
    return pdFALSE;
}
