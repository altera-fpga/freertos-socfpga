/*
 * SPDX-FileCopyrightText: Copyright (C) 2025 Altera Corporation
 *
 * SPDX-License-Identifier: MIT-0
 *
 * Header file partial reconfiguration sample 
 */


#ifndef __FGPA_PR_SAMPLE_H__
#define __FGPA_PR_SAMPLE_H__

#define SYSID_REG (0x20020000)

#define PERSONA0_SYSID (0x11111111)
#define PERSONA1_SYSID (0x22222222)
#define PERSONA0_RBF "/persona0.rbf"
#define PERSONA1_RBF "/persona1.rbf"

#define PR_FREEZE_BASE (0x20020400)
#define FREEZE_REG_VERSION_OFF (0x0000000C)
#define FREEZE_REG_VERSION (0xAD000003)

#endif /* __FGPA_PR_SAMPLE_H__ */
