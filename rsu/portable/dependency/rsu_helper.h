/*
 * SPDX-FileCopyrightText: Copyright (C) 2026 Altera Corporation
 *
 * SPDX-License-Identifier: MIT-0
 *
 * Common RSU helper routines shared across the RSU samples. These are built
 * purely on top of the public libRSU API so that librsu itself does not need
 * to be modified.
 */

#ifndef __RSU_HELPER_H__
#define __RSU_HELPER_H__

#include <libRSU.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * @brief Program a slot from a buffer with control over the resulting priority.
 *
 * This provides the same behaviour as a priority-aware program call, but is
 * implemented entirely on top of the public libRSU API (rsu_slot_count,
 * rsu_slot_priority, rsu_slot_program_buf and rsu_slot_enable), so librsu is
 * left untouched.
 *
 * @param[in] slot slot number to program (must already be erased)
 * @param[in] buf  pointer to the image data buffer
 * @param[in] size number of bytes to read from @p buf
 * @param[in] top  1: the freshly programmed slot becomes the highest priority
 *                    (identical to rsu_slot_program_buf());
 *                 0: the slot that is currently highest priority stays on top
 *                    and the newly programmed slot is placed just below it.
 * @return 0 on success, or a negative libRSU error code.
 */
RSU_OSAL_INT rsu_slot_program_buf_priority(RSU_OSAL_INT slot, RSU_OSAL_VOID *buf,
					   RSU_OSAL_INT size, RSU_OSAL_INT top);

/**
 * @brief Program a slot from a file with control over the resulting priority.
 *
 * File-based counterpart of rsu_slot_program_buf_priority(), implemented on top
 * of the public libRSU API (rsu_slot_count, rsu_slot_priority,
 * rsu_slot_program_file and rsu_slot_enable), so librsu is left untouched.
 *
 * @param[in] slot     slot number to program (must already be erased)
 * @param[in] filename input image file
 * @param[in] top      1: the freshly programmed slot becomes the highest
 *                        priority (identical to rsu_slot_program_file());
 *                     0: the slot that is currently highest priority stays on
 *                        top and the newly programmed slot is placed just below.
 * @return 0 on success, or a negative libRSU error code.
 */
RSU_OSAL_INT rsu_slot_program_file_priority(RSU_OSAL_INT slot, RSU_OSAL_CHAR *filename,
					    RSU_OSAL_INT top);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __RSU_HELPER_H__ */
