/*
 * SPDX-FileCopyrightText: Copyright (C) 2026 Altera Corporation
 *
 * SPDX-License-Identifier: MIT-0
 *
 * Header file for core configuration
 */

/*
    File: FreeRTOS Config Smp

    Header file for core configuration.
*/

/*
    Constants: FreeRTOS Config Smp constants

    configMAX_NUM_CORES          - Value: 4.
    configSMP_BUILD              - Value: 0.
    configBOOT_CORE              - Value: 0.
    configSMP_CORE0_ENABLE       - Defined per compiler or configuration.
    configSMP_CORE1_ENABLE       - Defined per compiler or configuration.
    configSMP_CORE2_ENABLE       - Defined per compiler or configuration.
    configSMP_CORE3_ENABLE       - Defined per compiler or configuration.
    configSMP_CORE0_MPIDR        - Value: 0x0.
    configSMP_CORE1_MPIDR        - Value: 0x100.
    configSMP_CORE2_MPIDR        - Value: 0x200.
    configSMP_CORE3_MPIDR        - Value: 0x300.
    configSMP_ENABLED_CORE_COUNT - Expands to a multi-line definition.
    configSMP_EARLY_CORE_BRINGUP - Value: 0.
*/

#ifndef FREERTOS_CONFIG_SMP_H
#define FREERTOS_CONFIG_SMP_H

#define configMAX_NUM_CORES     4

/* Provided by the build system:
 *   SMP=On             -> configSMP_BUILD = 1
 *   CORE=A55 (default) -> configBOOT_CORE = 0  (A55 cluster: cores 0,1)
 *   CORE=A76           -> configBOOT_CORE = 2  (A76 cluster: cores 2,3)
 *
 *   HSMP=On            -> configHSMP_BUILD = 1
 *
 */
#ifndef configSMP_BUILD
    #define configSMP_BUILD 0
#endif
#ifndef configHSMP_BUILD
    #define configHSMP_BUILD 0
#endif
#ifndef configBOOT_CORE
    #define configBOOT_CORE 0
#endif

/* SMP core enable controls.
 * SMP on : enable both cores of the boot core's cluster.
 * SMP off: enable only the boot core.
 */
#if ( configSMP_BUILD != 0 )
    #if ( configBOOT_CORE >= 2 )
        #define configSMP_CORE0_ENABLE  0
        #define configSMP_CORE1_ENABLE  0
        #define configSMP_CORE2_ENABLE  1
        #define configSMP_CORE3_ENABLE  1
    #else
        #define configSMP_CORE0_ENABLE  1
        #define configSMP_CORE1_ENABLE  1
        #define configSMP_CORE2_ENABLE  0
        #define configSMP_CORE3_ENABLE  0
    #endif
#elif ( configHSMP_BUILD != 0 )
    /* HSMP core enable controls.
     * HSMP on : user must manually enable cores 0/1(A55) and 2/3(A76) from here for HSMP testing.
     * HSMP off: enables only the default boot core.
     */
    #define configSMP_CORE0_ENABLE  1
    #define configSMP_CORE1_ENABLE  1
    #define configSMP_CORE2_ENABLE  1
    #define configSMP_CORE3_ENABLE  1
#else
    /*
    * Default Boot Core
    */
    #define configSMP_CORE0_ENABLE  ( configBOOT_CORE == 0 )
    #define configSMP_CORE1_ENABLE  ( configBOOT_CORE == 1 )
    #define configSMP_CORE2_ENABLE  ( configBOOT_CORE == 2 )
    #define configSMP_CORE3_ENABLE  ( configBOOT_CORE == 3 )
#endif

/* Agilex3 has no A76 cluster; never enable cores 2/3. */
#if defined( AGILEX3 )
    #undef  configSMP_CORE2_ENABLE
    #undef  configSMP_CORE3_ENABLE
    #define configSMP_CORE2_ENABLE 0
    #define configSMP_CORE3_ENABLE 0
#endif

#if ( ( configSMP_CORE0_ENABLE != 0 ) && ( configSMP_CORE0_ENABLE != 1 ) )
    #error configSMP_CORE0_ENABLE must be 0 or 1
#endif
#if ( ( configSMP_CORE1_ENABLE != 0 ) && ( configSMP_CORE1_ENABLE != 1 ) )
    #error configSMP_CORE1_ENABLE must be 0 or 1
#endif
#if ( ( configSMP_CORE2_ENABLE != 0 ) && ( configSMP_CORE2_ENABLE != 1 ) )
    #error configSMP_CORE2_ENABLE must be 0 or 1
#endif
#if ( ( configSMP_CORE3_ENABLE != 0 ) && ( configSMP_CORE3_ENABLE != 1 ) )
    #error configSMP_CORE3_ENABLE must be 0 or 1
#endif

/* SMP core MPIDR values. */
#define configSMP_CORE0_MPIDR   0x0
#define configSMP_CORE1_MPIDR   0x100
#define configSMP_CORE2_MPIDR   0x200
#define configSMP_CORE3_MPIDR   0x300

#define configSMP_ENABLED_CORE_COUNT ( \
    ( configSMP_CORE0_ENABLE ) + \
    ( configSMP_CORE1_ENABLE ) + \
    ( configSMP_CORE2_ENABLE ) + \
    ( configSMP_CORE3_ENABLE ) )

/* Bring up the secondary cores before main. Auto-enabled by 
 * Debug builds (BUILD_TYPE=Debug); can also be forced with 
 * -DconfigSMP_EARLY_CORE_BRINGUP=1. */
#ifndef configSMP_EARLY_CORE_BRINGUP
    #define configSMP_EARLY_CORE_BRINGUP 0
#endif

#endif /* FREERTOS_CONFIG_SMP_H */
