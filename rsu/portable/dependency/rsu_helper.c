/*
 * SPDX-FileCopyrightText: Copyright (C) 2026 Altera Corporation
 *
 * SPDX-License-Identifier: MIT-0
 *
 * Common RSU helper routines shared across the RSU samples.
 */

/*
    File: RSU Helpers Implementation

    Common RSU helper routines shared across the RSU samples.
*/

#include "rsu_helper.h"

/*
    Function: rsu_find_top_slot

    Find the slot that is currently highest priority (priority == 1, i.e. the
    slot the decision firmware boots first).

    On success returns 0 and writes the slot number into *top_slot, or -1 into
    *top_slot if no slot is currently in the CPB. On failure returns a negative
    libRSU error code.
*/

static RSU_OSAL_INT rsu_find_top_slot(RSU_OSAL_INT *top_slot)
{
	RSU_OSAL_INT s, count;

	*top_slot = -1;

	count = rsu_slot_count();
	if (count < 0) {
		return count;
	}

	for (s = 0; s < count; s++) {
		if (rsu_slot_priority(s) == 1) {
			*top_slot = s;
			break;
		}
	}

	return 0;
}

/*
    Function: rsu_restore_top_slot

    After a slot has been programmed (which makes it the new top priority),
    restore the previous top slot back to the top - this demotes the freshly
    programmed slot to priority 2, giving the "not top" behaviour.
*/

static RSU_OSAL_INT rsu_restore_top_slot(RSU_OSAL_INT keep_slot, RSU_OSAL_INT programmed_slot)
{
	if (keep_slot >= 0 && keep_slot != programmed_slot) {
		return rsu_slot_enable(keep_slot);
	}

	return 0;
}

RSU_OSAL_INT rsu_slot_program_buf_priority(RSU_OSAL_INT slot, RSU_OSAL_VOID *buf,
					   RSU_OSAL_INT size, RSU_OSAL_INT top)
{
	RSU_OSAL_INT ret, keep_slot;

	/* Default behaviour: the new slot becomes the highest priority. */
	if (top) {
		return rsu_slot_program_buf(slot, buf, size);
	}

	/* top == 0: remember the current top slot, program, then restore it. */
	ret = rsu_find_top_slot(&keep_slot);
	if (ret) {
		return ret;
	}

	ret = rsu_slot_program_buf(slot, buf, size);
	if (ret) {
		return ret;
	}

	return rsu_restore_top_slot(keep_slot, slot);
}

RSU_OSAL_INT rsu_slot_program_file_priority(RSU_OSAL_INT slot, RSU_OSAL_CHAR *filename,
					    RSU_OSAL_INT top)
{
	RSU_OSAL_INT ret, keep_slot;

	/* Default behaviour: the new slot becomes the highest priority. */
	if (top) {
		return rsu_slot_program_file(slot, filename);
	}

	/* top == 0: remember the current top slot, program, then restore it. */
	ret = rsu_find_top_slot(&keep_slot);
	if (ret) {
		return ret;
	}

	ret = rsu_slot_program_file(slot, filename);
	if (ret) {
		return ret;
	}

	return rsu_restore_top_slot(keep_slot, slot);
}
