/*
 * SPDX-FileCopyrightText: Copyright (C) 2025 Altera Corporation
 *
 * SPDX-License-Identifier: MIT-0
 *
 * Implementation of CLI commands for I3C bus
 */

/**
 * @defgroup cli_ros ROS
 * @ingroup cli
 *
 * Perform operations on ROS
 *
 * @details
 * It supports the following commands:
 * -ros erase   <slot>
 * -ros program <slot> <file>
 * -ros verify  <slot> <file>
 * -ros help
 *
 * Typical usage:
 * - Use  'ros erase' command to erase a slot.
 * - Use  'ros program' command to program a slot with sdcard file..
 * - Use  'ros verify' command to verify a slot with a sdcard file.
 * @subsection commands Commands
 * @subsubsection ros_erase ros erase
 * Erase the content of the slot  <br>
 *
 * Usage:  <br>
 *   ros erase <slotNum>  <br>
 *
 * It requires the following arguments:
 * - slotNum -      The slot number  <br>
 *
 * @subsubsection ros_program ros program
 * Write image to a given slot  <br>
 *
 * Usage:  <br>
 *   ros program <slotNum> <file> <br>
 * It requires the following arguments:
 * - slotNum -      The slot number  <br>
 * - file    -      The name of the sdcard image(use absolute path).  <br>
 *
 * @subsubsection ros_verify ros verify
 * Verify image in the slot to the file  <br>
 *
 * Usage:  <br>
 *   ros verify <slotNum> <file> <br>
 * It requires the following arguments:
 * - slotNum -      The slot number  <br>
 * - file    -      The name of the sdcard image(use absolute path).  <br>
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <FreeRTOS.h>
#include <socfpga_uart.h>
#include <FreeRTOS_CLI.h>
#include "cli_app.h"
#include <libRSU.h>

#define SLOT_ERASE_CMD    ("erase")
#define SLOT_PRGM_CMD     ("program")
#define SLOT_VRFY_CMD     ("verify")


BaseType_t cmd_ros( char *pcWriteBuffer, size_t xWriteBufferLen,
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
        printf("\r\nPerform ros operations in a rsu suppoted image"
                "\r\n\nIt supports the following sub commands:"
                "\r\n  ros erase   <slot>"
                "\r\n  ros program <slot> <file>"
                "\r\n  ros verify  <slot> <file>"
                "\r\n  ros help"
                "\r\n\nTypical usage:"
                "\r\n- Use erase command to erase a slot."
                "\r\n- Use program command to program a slot with sdcard file."
                "\r\n- Use verify command to verify a slot with a sdcard file."
                "\r\n  ros <command> help\r\n"
                );

        return pdFALSE;
    }

    pcParam2 = FreeRTOS_CLIGetParameter(pcCommandString, 2, &Param2StrLen);
    if (strcmp(tempStr, SLOT_ERASE_CMD) == 0)
    {
        if (strncmp( pcParam2, "help", 4 ) == 0)
        {
            printf("\r\nErase rsu slot"
                    "\r\n\nUsage:"
                    "\r\n  ros erase <slot>"
                    "\r\n\nIt requires the following arguments:"
                    "\r\n  slot      slot in the rsu image to be erased."
                    );
            return pdFALSE;
        }
        pcParam2 = FreeRTOS_CLIGetParameter(pcCommandString, 2, &Param2StrLen);
        if (pcParam2 == NULL)
        {
            printf(
                    "\r\nIncorrect number of arguments for ros slot_erase command");
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
            printf("\r\nProgram  slot with sdcard file"
                    "\r\n\nUsage:"
                    "\r\n  ros program <slot> <file>"
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
                    "\r\nIncorrect number of arguments for ros slot_program command");
            printf("\r\nEnter 'help' to view the list of available commands");
            return pdFAIL;
        }
        pcParam3 = FreeRTOS_CLIGetParameter(pcCommandString, 3, &Param3StrLen);
        if (pcParam3 == NULL)
        {
            printf(
                    "\r\nIncorrect number of arguments for ros slot_program command");
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
        if (rsu_slot_program_file_raw(slotNum,(char *) pcParam3) != 0)
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
                    "\r\n  ros verify <slot> <file>"
                    "\r\n\nIt requires the following arguments:"
                    "\r\n  slot   rsu slot that has to be programmed"
                    "\r\n  file   file to be verified  from sdcard(Ex: /fip.rpd)"
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
                    "\r\nIncorrect number of arguments for ros slot_verify command");
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
        if (rsu_slot_verify_file_raw(slotNum,(char *) pcParam3) != 0)
        {
            printf("\r\nFailed to verify slot %d", slotNum);
            librsu_exit();
            return pdFAIL;
        }
        printf("\r\nSLOT %d VERIFICATION SUCCESSFUL", slotNum);
        librsu_exit();
        return pdFALSE;
    }
    else
    {
        printf("\r\nInvalid command for ros");
        printf("\r\nEnter 'help' to view the list of available commands.");
        return pdFAIL;
    }
    return pdFALSE;
}
