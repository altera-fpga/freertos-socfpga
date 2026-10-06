/*
 * FreeRTOS V202212.00
 * Copyright (C) 2020 Amazon.com, Inc. or its affiliates. All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * https://www.FreeRTOS.org
 * https://github.com/FreeRTOS
 *
 */

/*
    File: FreeRTOS Config

    Compile-time settings: 111 definitions and 3 declarations.
*/

/*
    Constants: FreeRTOS constants

    configUSE_PREEMPTION                            - Value: 1.
    configUSE_TICKLESS_IDLE                         - Value: 0.
    configCPU_CLOCK_HZ                              - Expands to (SystemCoreClock).
    configTICK_RATE_HZ                              - Expands to ((TickType_t)500).
    configMAX_PRIORITIES                            - Value: 5.
    configMINIMAL_STACK_SIZE                        - Expands to ((unsigned short)1024).
    configMAX_TASK_NAME_LEN                         - Value: 20.
    configUSE_16_BIT_TICKS                          - Value: 0.
    configIDLE_SHOULD_YIELD                         - Value: 1.
    configUSE_TASK_NOTIFICATIONS                    - Value: 1.
    configUSE_MUTEXES                               - Value: 1.
    configUSE_RECURSIVE_MUTEXES                     - Value: 1.
    configUSE_COUNTING_SEMAPHORES                   - Value: 1.
    configUSE_ALTERNATIVE_API                       - Value: 0.
    configQUEUE_REGISTRY_SIZE                       - Value: 8.
    configUSE_QUEUE_SETS                            - Value: 1.
    configUSE_TIME_SLICING                          - Value: 1.
    configUSE_NEWLIB_REENTRANT                      - Value: 0.
    configENABLE_BACKWARD_COMPATIBILITY             - Value: 0.
    configNUM_THREAD_LOCAL_STORAGE_POINTERS         - Value: 5.
    configUSE_SB_COMPLETED_CALLBACK                 - Value: 1.
    configFRTOS_MEMORY_SCHEME                       - Used memory allocation (heap_x.c).
    configINCLUDE_FREERTOS_TASK_C_ADDITIONS_H       - Tasks.c additions (e.g. Thread Aware Debug capability).
    configSUPPORT_STATIC_ALLOCATION                 - Value: 0.
    configSUPPORT_DYNAMIC_ALLOCATION                - Value: 1.
    configTOTAL_HEAP_SIZE                           - Expands to ((size_t)(4 * 1024 * 1024)).
    configAPPLICATION_ALLOCATED_HEAP                - Value: 0.
    configGENERATE_RUN_TIME_STATS                   - Value: 0.
    configUSE_TRACE_FACILITY                        - Value: 1.
    configUSE_STATS_FORMATTING_FUNCTIONS            - Value: 0.
    configRECORD_STACK_HIGH_ADDRESS                 - Task aware debugging.
    configUSE_CO_ROUTINES                           - Value: 0.
    configMAX_CO_ROUTINE_PRIORITIES                 - Value: 2.
    configUSE_TIMERS                                - Value: 1.
    configTIMER_TASK_PRIORITY                       - Expands to (configMAX_PRIORITIES - 1).
    configTIMER_QUEUE_LENGTH                        - Value: 10.
    configTIMER_TASK_STACK_DEPTH                    - Expands to (configMINIMAL_STACK_SIZE * 2).
    configASSERT                                    - Define to trap errors during development.
    INCLUDE_vTaskPrioritySet                        - Value: 1.
    INCLUDE_uxTaskPriorityGet                       - Value: 1.
    INCLUDE_vTaskDelete                             - Value: 1.
    INCLUDE_vTaskSuspend                            - Value: 1.
    INCLUDE_vTaskDelayUntil                         - Value: 1.
    INCLUDE_vTaskDelay                              - Value: 1.
    INCLUDE_xTaskGetSchedulerState                  - Value: 1.
    INCLUDE_xTaskGetCurrentTaskHandle               - Value: 1.
    INCLUDE_uxTaskGetStackHighWaterMark             - Value: 0.
    INCLUDE_xTaskGetIdleTaskHandle                  - Value: 0.
    INCLUDE_eTaskGetState                           - For Full Demo.
    INCLUDE_xTimerPendFunctionCall                  - Value: 1.
    INCLUDE_xTaskAbortDelay                         - Value: 1.
    INCLUDE_xTaskGetHandle                          - Value: 1.
    INCLUDE_xTaskResumeFromISR                      - Value: 1.
    INCLUDE_xQueueGetMutexHolder                    - For Full Demo.
    configPRIO_BITS                                 - __BVIC_PRIO_BITS will be specified when CMSIS is being used.
    configLIBRARY_LOWEST_INTERRUPT_PRIORITY         - Expands to ((1U << (configPRIO_BITS)) - 1).
    configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY    - Value: 2.
    configKERNEL_INTERRUPT_PRIORITY                 - Expands to a multi-line definition.
    configMAX_SYSCALL_INTERRUPT_PRIORITY            - Expands to a multi-line definition.
    vPortSVCHandler                                 - Expands to SVC_Handler.
    xPortPendSVHandler                              - Expands to PendSV_Handler.
    xPortSysTickHandler                             - Expands to SysTick_Handler.
    configINTERRUPT_CONTROLLER_BASE_ADDRESS         - Value: 0X1D000000U.
    configINTERRUPT_CONTROLLER_CPU_INTERFACE_OFFSET - Value: 0x1000.
    configUNIQUE_INTERRUPT_PRIORITIES               - Refer GIC600 TRM - Implementation-defined features reference - Interrupt prioritization.
    configMAX_API_CALL_INTERRUPT_PRIORITY           - In non secure mode we only get half that.
    configENABLE_CONSOLE_UART                       - Value: 1.
    configCONSOLE_MAKE_FLUSH_TASK                   - Value: 1.
    configCONSOLE_FLUSH_TASK_PRIORITY               - Expands to tskIDLE_PRIORITY.
    configCONSOLE_UART_ID                           - Value: 0.
    configSTART_TASK_NOTIFY_TESTS                   - Value: 1.
    configSTART_TASK_NOTIFY_ARRAY_TESTS             - Value: 1.
    configSTART_BLOCKING_QUEUE_TESTS                - Value: 1.
    configSTART_SEMAPHORE_TESTS                     - Value: 1.
    configSTART_POLLED_QUEUE_TESTS                  - Value: 1.
    configSTART_INTEGER_MATH_TESTS                  - Value: 1.
    configSTART_GENERIC_QUEUE_TESTS                 - Value: 1.
    configSTART_PEEK_QUEUE_TESTS                    - Value: 1.
    configSTART_MATH_TESTS                          - Value: 1.
    configSTART_RECURSIVE_MUTEX_TESTS               - Value: 1.
    configSTART_COUNTING_SEMAPHORE_TESTS            - Value: 1.
    configSTART_QUEUE_SET_TESTS                     - Value: 1.
    configSTART_QUEUE_OVERWRITE_TESTS               - Value: 1.
    configSTART_EVENT_GROUP_TESTS                   - Value: 1.
    configSTART_INTERRUPT_SEMAPHORE_TESTS           - Value: 1.
    configSTART_QUEUE_SET_POLLING_TESTS             - Value: 1.
    configSTART_BLOCK_TIME_TESTS                    - Value: 1.
    configSTART_ABORT_DELAY_TESTS                   - Value: 1.
    configSTART_MESSAGE_BUFFER_TESTS                - Value: 1.
    configSTART_STREAM_BUFFER_TESTS                 - Value: 1.
    configSTART_STREAM_BUFFER_INTERRUPT_TESTS       - Value: 1.
    configSTART_TIMER_TESTS                         - Value: 1.
    configSTART_INTERRUPT_QUEUE_TESTS               - Value: 1.
    configSTART_REGISTER_TESTS                      - Value: 1.
    configSTART_DELETE_SELF_TESTS                   - Value: 1.
    configNUMBER_OF_CORES                           - Expands to configSMP_ENABLED_CORE_COUNT.
    configUSE_CORE_AFFINITY                         - Value: 1.
    configRUN_MULTIPLE_PRIORITIES                   - Value: 1.
    configUSE_PASSIVE_IDLE_HOOK                     - Value: 0.
    configUSE_PORT_OPTIMISED_TASK_SELECTION         - Value: 0.
    configUSE_IDLE_HOOK                             - Value: 1.
    configUSE_TICK_HOOK                             - Value: 1.
    configCHECK_FOR_STACK_OVERFLOW                  - Value: 0.
    configUSE_MALLOC_FAILED_HOOK                    - Value: 1.
    configUSE_DAEMON_TASK_STARTUP_HOOK              - Value: 0.
    configTASK_NOTIFICATION_ARRAY_ENTRIES           - Value: 3.
    configPRINTF                                    - Expands to ( printf X ).
    configCOMMAND_INT_MAX_OUTPUT_SIZE               - Value: ( 200 ).
    ipTRUE_BOOL                                     - Expands to ( 1 == 1 ).
    ipFALSE_BOOL                                    - Expands to ( 1 == 2 ).
*/

#ifndef __ASSEMBLER__
#include <stddef.h>
#endif

#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H

#include "FreeRTOSConfigSmp.h"

/*-----------------------------------------------------------
 * Application specific definitions.
 *
 * These definitions should be adjusted for your particular hardware and
 * application requirements.
 *
 * THESE PARAMETERS ARE DESCRIBED WITHIN THE 'CONFIGURATION' SECTION OF THE
 * FreeRTOS API DOCUMENTATION AVAILABLE ON THE FreeRTOS.org WEB SITE.
 *
 * See http://www.freertos.org/a00110.html.
 *----------------------------------------------------------*/

#define configUSE_PREEMPTION                    1
#define configUSE_TICKLESS_IDLE                 0
#define configCPU_CLOCK_HZ                      (SystemCoreClock)
#define configTICK_RATE_HZ                      ((TickType_t)500)
#define configMAX_PRIORITIES                    5
#define configMINIMAL_STACK_SIZE                ((unsigned short)1024)
#define configMAX_TASK_NAME_LEN                 20
#define configUSE_16_BIT_TICKS                  0
#define configIDLE_SHOULD_YIELD                 1
#define configUSE_TASK_NOTIFICATIONS            1
#define configUSE_MUTEXES                       1
#define configUSE_RECURSIVE_MUTEXES             1
#define configUSE_COUNTING_SEMAPHORES           1
#define configUSE_ALTERNATIVE_API               0 /* Deprecated! */
#define configQUEUE_REGISTRY_SIZE               8
#define configUSE_QUEUE_SETS                    1
#define configUSE_TIME_SLICING                  1
#define configUSE_NEWLIB_REENTRANT              0
#define configENABLE_BACKWARD_COMPATIBILITY     0
#define configNUM_THREAD_LOCAL_STORAGE_POINTERS 5
#define configUSE_SB_COMPLETED_CALLBACK         1

/* Used memory allocation (heap_x.c) */
#define configFRTOS_MEMORY_SCHEME               4
/* Tasks.c additions (e.g. Thread Aware Debug capability) */
#define configINCLUDE_FREERTOS_TASK_C_ADDITIONS_H 0

/* Memory allocation related definitions. */
#define configSUPPORT_STATIC_ALLOCATION         0
#define configSUPPORT_DYNAMIC_ALLOCATION        1
#define configTOTAL_HEAP_SIZE                   ((size_t)(4 * 1024 * 1024))
#define configAPPLICATION_ALLOCATED_HEAP        0

/* Run time and task stats gathering related definitions. */
#define configGENERATE_RUN_TIME_STATS           0
#define configUSE_TRACE_FACILITY                1
#define configUSE_STATS_FORMATTING_FUNCTIONS    0

/* Task aware debugging. */
#define configRECORD_STACK_HIGH_ADDRESS         1

/* Co-routine related definitions. */
#define configUSE_CO_ROUTINES                   0
#define configMAX_CO_ROUTINE_PRIORITIES         2

/* Software timer related definitions. */
#define configUSE_TIMERS                        1
#define configTIMER_TASK_PRIORITY               (configMAX_PRIORITIES - 1)
#define configTIMER_QUEUE_LENGTH                10
#define configTIMER_TASK_STACK_DEPTH            (configMINIMAL_STACK_SIZE * 2)

/* Define to trap errors during development. */
#define configASSERT(x) \
    do { \
        if ((x) == 0) { \
            taskDISABLE_INTERRUPTS(); \
            for (;;) { \
            } \
        } \
    } while (0)

/* Optional functions - most linkers will remove unused functions anyway. */
#define INCLUDE_vTaskPrioritySet                1
#define INCLUDE_uxTaskPriorityGet               1
#define INCLUDE_vTaskDelete                     1
#define INCLUDE_vTaskSuspend                    1
#define INCLUDE_vTaskDelayUntil                 1
#define INCLUDE_vTaskDelay                      1
#define INCLUDE_xTaskGetSchedulerState          1
#define INCLUDE_xTaskGetCurrentTaskHandle       1
#define INCLUDE_uxTaskGetStackHighWaterMark     0
#define INCLUDE_xTaskGetIdleTaskHandle          0
#define INCLUDE_eTaskGetState                   1 // For Full Demo.
#define INCLUDE_xTimerPendFunctionCall          1
#define INCLUDE_xTaskAbortDelay                 1
#define INCLUDE_xTaskGetHandle                  1
#define INCLUDE_xTaskResumeFromISR              1
#define INCLUDE_xQueueGetMutexHolder            1 // For Full Demo.


/* Interrupt nesting behaviour configuration. Cortex-M specific. */
#ifdef __NVIC_PRIO_BITS
/* __BVIC_PRIO_BITS will be specified when CMSIS is being used. */
#define configPRIO_BITS __NVIC_PRIO_BITS
#else
#define configPRIO_BITS 4 /* 15 priority levels */
#endif

/* The lowest interrupt priority that can be used in a call to a "set priority"
function. */
#define configLIBRARY_LOWEST_INTERRUPT_PRIORITY ((1U << (configPRIO_BITS)) - 1)

/* The highest interrupt priority that can be used by any interrupt service
routine that makes calls to interrupt safe FreeRTOS API functions.  DO NOT CALL
INTERRUPT SAFE FREERTOS API FUNCTIONS FROM ANY INTERRUPT THAT HAS A HIGHER
PRIORITY THAN THIS! (higher priorities are lower numeric values. */
#define configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY 2

/* Interrupt priorities used by the kernel port layer itself.  These are generic
to all Cortex-M ports, and do not rely on any particular library functions. */
#define configKERNEL_INTERRUPT_PRIORITY (configLIBRARY_LOWEST_INTERRUPT_PRIORITY << (8 - configPRIO_BITS))
/* !!!! configMAX_SYSCALL_INTERRUPT_PRIORITY must not be set to zero !!!!
See http://www.FreeRTOS.org/RTOS-Cortex-M3-M4.html. */
#define configMAX_SYSCALL_INTERRUPT_PRIORITY (configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY << (8 - configPRIO_BITS))

/* Definitions that map the FreeRTOS port interrupt handlers to their CMSIS
standard names. */
#define vPortSVCHandler SVC_Handler
#define xPortPendSVHandler PendSV_Handler
#define xPortSysTickHandler SysTick_Handler

#define configINTERRUPT_CONTROLLER_BASE_ADDRESS               0X1D000000U
#define configINTERRUPT_CONTROLLER_CPU_INTERFACE_OFFSET       0x1000
#define configUNIQUE_INTERRUPT_PRIORITIES                     16// Refer GIC600 TRM - Implementation-defined features reference - Interrupt prioritization
                                                                // The system supports 32 levels of interrupt priority, but since we are running
                                                                // in non secure mode we only get half that
#define configMAX_API_CALL_INTERRUPT_PRIORITY                 9

#define configENABLE_CONSOLE_UART 1
#define configCONSOLE_MAKE_FLUSH_TASK 1
#define configCONSOLE_FLUSH_TASK_PRIORITY tskIDLE_PRIORITY

#if configENABLE_CONSOLE_UART == 1
#define configCONSOLE_UART_ID               0
#endif

/* Definition for FreeRTOS tests. */
#define configSTART_TASK_NOTIFY_TESTS             1
#define configSTART_TASK_NOTIFY_ARRAY_TESTS       1
#define configSTART_BLOCKING_QUEUE_TESTS          1
#define configSTART_SEMAPHORE_TESTS               1
#define configSTART_POLLED_QUEUE_TESTS            1
#define configSTART_INTEGER_MATH_TESTS            1
#define configSTART_GENERIC_QUEUE_TESTS           1
#define configSTART_PEEK_QUEUE_TESTS              1
#define configSTART_MATH_TESTS                    1
#define configSTART_RECURSIVE_MUTEX_TESTS         1
#define configSTART_COUNTING_SEMAPHORE_TESTS      1
#define configSTART_QUEUE_SET_TESTS               1
#define configSTART_QUEUE_OVERWRITE_TESTS         1
#define configSTART_EVENT_GROUP_TESTS             1
#define configSTART_INTERRUPT_SEMAPHORE_TESTS     1
#define configSTART_QUEUE_SET_POLLING_TESTS       1
#define configSTART_BLOCK_TIME_TESTS              1
#define configSTART_ABORT_DELAY_TESTS             1
#define configSTART_MESSAGE_BUFFER_TESTS          1
#define configSTART_STREAM_BUFFER_TESTS           1
#define configSTART_STREAM_BUFFER_INTERRUPT_TESTS 1
#define configSTART_TIMER_TESTS                   1
#define configSTART_INTERRUPT_QUEUE_TESTS         1
#define configSTART_REGISTER_TESTS                1
#define configSTART_DELETE_SELF_TESTS             1

#ifndef configNUMBER_OF_CORES
    #define configNUMBER_OF_CORES               configSMP_ENABLED_CORE_COUNT
#endif

#if ( configNUMBER_OF_CORES > 1 )
    #define configUSE_CORE_AFFINITY             1
    #define configRUN_MULTIPLE_PRIORITIES       1
#endif

#define configUSE_PASSIVE_IDLE_HOOK             0
#define configUSE_PORT_OPTIMISED_TASK_SELECTION 0

/* Hook function related definitions. */
#define configUSE_IDLE_HOOK                     1
#define configUSE_TICK_HOOK                     1
#define configCHECK_FOR_STACK_OVERFLOW          0
#define configUSE_MALLOC_FAILED_HOOK            1
#define configUSE_DAEMON_TASK_STARTUP_HOOK      0

#define configTASK_NOTIFICATION_ARRAY_ENTRIES     3

#ifndef __ASSEMBLER__
void vLoggingPrintf( const char * format,
                     ... );
#endif

#define configPRINTF( X )                         vLoggingPrintf X
#define configCOMMAND_INT_MAX_OUTPUT_SIZE         ( 200 )

/* Required for TCP stack TODO : Need to find right place */
#define ipTRUE_BOOL         ( 1 == 1 )
#define ipFALSE_BOOL        ( 1 == 2 )

#ifndef __ASSEMBLER__
extern void * pvPortMallocCoherent( size_t xWantedSize );
extern void * pvPortAlignedAlloc( size_t xAlignemnt, size_t xWantedSize );
#endif

#endif /* FREERTOS_CONFIG_H */
