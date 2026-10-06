/*
 * SPDX-FileCopyrightText: Copyright (C) 2025 Altera Corporation
 *
 * SPDX-License-Identifier: MIT-0
 *
 * Header file partial reconfiguration sample
 */

/*
    File: FPGA Manager PR Sample

    Header file partial reconfiguration sample.
*/

/*
    Constants: FPGA Manager PR constants

    SYSID_REG              - Value: (0x20020000).
    PERSONA0_SYSID         - Value: (0x11111111U).
    PERSONA1_SYSID         - Value: (0x22222222U).
    PERSONA0_RBF           - Value: "/p0.rbf".
    PERSONA1_RBF           - Value: "/p1.rbf".
    PR_FREEZE_BASE         - Value: (0x20020400U).
    FREEZE_REG_VERSION_OFF - Value: (0x0000000CU).
    FREEZE_REG_VERSION     - Value: (0xAD000003U).
*/

#ifndef __FPGA_PR_SAMPLE_H__
#define __FPGA_PR_SAMPLE_H__

#define SYSID_REG    (0x20020000)

#define PERSONA0_SYSID    (0x11111111U)
#define PERSONA1_SYSID    (0x22222222U)
#define PERSONA0_RBF      "/p0.rbf"
#define PERSONA1_RBF      "/p1.rbf"

#define PR_FREEZE_BASE            (0x20020400U)
#define FREEZE_REG_VERSION_OFF    (0x0000000CU)
#define FREEZE_REG_VERSION        (0xAD000003U)

#endif /* __FPGA_PR_SAMPLE_H__ */
