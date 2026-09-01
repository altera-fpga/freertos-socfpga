/*
 * SPDX-FileCopyrightText: Copyright (C) 2025 Altera Corporation
 *
 * SPDX-License-Identifier: MIT-0
 *
 * Configuration for heterogeneous multi block test
 */

#ifndef __HETEROGENEOUS_MULTI_BLOCK_TEST_CONFIG_H__
#define __HETEROGENEOUS_MULTI_BLOCK_TEST_CONFIG_H__

#define HET_STRESS_ENABLE_SDM_MAILBOX  1
#define HET_STRESS_ENABLE_ETHERNET     1
#define HET_STRESS_ENABLE_SD           1
#define HET_STRESS_ENABLE_QSPI         1
#define HET_STRESS_ENABLE_RTOS_TESTS   1

#define HET_STRESS_ENABLE_PINNING      1

#define HET_STRESS_RUN_TIME_SEC        300U

#endif
