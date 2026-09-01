/*
 * SPDX-FileCopyrightText: Copyright (C) 2026 Altera Corporation
 *
 * SPDX-License-Identifier: MIT-0
 *
 * Main file for heterogeneous multi block test
 */

#include "FreeRTOS.h"
#include "FreeRTOSConfigSmp.h"
#include "task.h"
#include "timers.h"
#include "socfpga_interrupt.h"
#include "socfpga_console.h"
#include "socfpga_smmu.h"
#include "heterogeneous_multi_block_test_config.h"
#include "osal_log.h"
#include <stdbool.h>

#define HET_STRESS_TASK_PRIORITY  (configMAX_PRIORITIES - 3)

void hetero_mbox_task(void *arg);
void hetero_ethernet_task(void *arg);
void hetero_sd_task(void *arg);
void hetero_qspi_task(void *arg);
void hetero_rtos_task(void *arg);
void heterogeneous_multi_block_test(void);

void hetero_mbox_print_status(void);
void hetero_mbox_request_stop(void);
void hetero_sd_print_status(void);
void hetero_sd_request_stop(void);
void hetero_qspi_print_status(void);
void hetero_qspi_request_stop(void);
void hetero_ethernet_print_status(void);
void hetero_ethernet_request_stop(void);
void hetero_rtos_print_status(void);
void hetero_rtos_request_stop(void);

uint32_t hetero_pin_index = 0;
static TimerHandle_t hetero_status_timer;
static TimerHandle_t hetero_stop_timer;

/* Defined by the AArch64 FreeRTOS SMP port, maps logical index to MPIDR ID. */
extern const uint32_t ulCoreMpidrList[ configNUMBER_OF_CORES ];
uint32_t ulGetPhysicalCoreMpidr( void )
{
    uint32_t logical_core = ulGetCoreId();
    if (logical_core < configNUMBER_OF_CORES) {
        return ulCoreMpidrList[logical_core];
    } 
    return 0U;
}

#if HET_STRESS_ENABLE_PINNING
static void set_task_affinity(TaskHandle_t task_handle)
{
    uint32_t core_index = hetero_pin_index % configSMP_ENABLED_CORE_COUNT;

    vTaskCoreAffinitySet(task_handle, (1U << core_index));
    hetero_pin_index++;
}
#endif

static void hetero_status_timer_callback(TimerHandle_t timer_handle)
{
    (void)timer_handle;

    PRINT("\r\nHeterogeneous multi block status");
#if HET_STRESS_ENABLE_SDM_MAILBOX
    hetero_mbox_print_status();
#endif
#if HET_STRESS_ENABLE_SD
    hetero_sd_print_status();
#endif
#if HET_STRESS_ENABLE_QSPI
    hetero_qspi_print_status();
#endif
#if HET_STRESS_ENABLE_ETHERNET
    hetero_ethernet_print_status();
#endif
#if HET_STRESS_ENABLE_RTOS_TESTS
    hetero_rtos_print_status();
#endif
}

static void hetero_print_final_stats(void)
{
    PRINT("Heterogeneous multi block final stats");

#if HET_STRESS_ENABLE_SDM_MAILBOX
    hetero_mbox_print_status();
#endif
#if HET_STRESS_ENABLE_SD
    hetero_sd_print_status();
#endif
#if HET_STRESS_ENABLE_QSPI
    hetero_qspi_print_status();
#endif
#if HET_STRESS_ENABLE_ETHERNET
    hetero_ethernet_print_status();
#endif
#if HET_STRESS_ENABLE_RTOS_TESTS
    hetero_rtos_print_status();
#endif
}

static void hetero_stop_timer_callback(TimerHandle_t timer_handle)
{
    (void)timer_handle;

#if HET_STRESS_ENABLE_SDM_MAILBOX
    hetero_mbox_request_stop();
#endif
#if HET_STRESS_ENABLE_SD
    hetero_sd_request_stop();
#endif
#if HET_STRESS_ENABLE_QSPI
    hetero_qspi_request_stop();
#endif
#if HET_STRESS_ENABLE_ETHERNET
    hetero_ethernet_request_stop();
#endif
#if HET_STRESS_ENABLE_RTOS_TESTS
    hetero_rtos_request_stop();
#endif

    if (hetero_status_timer != NULL)
    {
        (void)xTimerStop(hetero_status_timer, 0);
    }

    hetero_print_final_stats();
}

void vApplicationTickHook(void)
{
}

static void prvSetupHardware(void)
{
    /* Initialize the GIC. */
    interrupt_init_gic();

    /* Enable SMMU */
    (void)smmu_enable();

    /* Initialize the console uart */
#if configENABLE_CONSOLE_UART
    console_init(configCONSOLE_UART_ID, "115200-8N1");
#endif
}

static void run_stress(void *arg)
{
    (void)arg;

    heterogeneous_multi_block_test();
    vTaskSuspend(NULL);
}

void vApplicationIdleHook(void)
{
}

void vApplicationMallocFailedHook(void)
{
    taskDISABLE_INTERRUPTS();
    for (;;)
        ;
}

void heterogeneous_multi_block_test(void)
{
    BaseType_t xReturn;
#if HET_STRESS_ENABLE_PINNING
    TaskHandle_t task_handle = NULL;
#endif

    hetero_status_timer = xTimerCreate("Het_Multi_Block_Status",
                                       pdMS_TO_TICKS(5000),
                                       pdTRUE, NULL,
                                       hetero_status_timer_callback);
    if (hetero_status_timer != NULL)
    {
        (void)xTimerStart(hetero_status_timer, 0);
    }
    else
    {
        ERROR("Failed to create heterogeneous multi block status timer");
    }

    hetero_stop_timer = xTimerCreate("Het_Multi_Block_Stop",
                                     pdMS_TO_TICKS(HET_STRESS_RUN_TIME_SEC * 1000U),
                                     pdFALSE, NULL,
                                     hetero_stop_timer_callback);
    if (hetero_stop_timer != NULL)
    {
        (void)xTimerStart(hetero_stop_timer, 0);
    }
    else
    {
        ERROR("Failed to create heterogeneous multi block stop timer");
    }

#if HET_STRESS_ENABLE_SDM_MAILBOX
    xReturn = xTaskCreate(hetero_mbox_task, "Het_Mbox_Stress",
                          configMINIMAL_STACK_SIZE, NULL,
                          HET_STRESS_TASK_PRIORITY,
#if HET_STRESS_ENABLE_PINNING
                          &task_handle
#else
                          NULL
#endif
                          );
    if (xReturn == pdPASS)
    {
#if HET_STRESS_ENABLE_PINNING
        set_task_affinity(task_handle);
#endif
    }
    else
    {
        ERROR("Failed to create mailbox stress task");
    }
#endif

#if HET_STRESS_ENABLE_SD
    xReturn = xTaskCreate(hetero_sd_task, "Het_SD_Stress",
                          configMINIMAL_STACK_SIZE * 4, NULL,
                          HET_STRESS_TASK_PRIORITY,
#if HET_STRESS_ENABLE_PINNING
                          &task_handle
#else
                          NULL
#endif
                          );
    if (xReturn == pdPASS)
    {
#if HET_STRESS_ENABLE_PINNING
        set_task_affinity(task_handle);
#endif
    }
    else
    {
        ERROR("Failed to create SD stress task");
    }
#endif

#if HET_STRESS_ENABLE_QSPI
    xReturn = xTaskCreate(hetero_qspi_task, "Het_QSPI_Stress",
                          configMINIMAL_STACK_SIZE * 5, NULL,
                          HET_STRESS_TASK_PRIORITY,
#if HET_STRESS_ENABLE_PINNING
                          &task_handle
#else
                          NULL
#endif
                          );
    if (xReturn == pdPASS)
    {
#if HET_STRESS_ENABLE_PINNING
        set_task_affinity(task_handle);
#endif
    }
    else
    {
        ERROR("Failed to create QSPI stress task");
    }
#endif

#if HET_STRESS_ENABLE_RTOS_TESTS
    xReturn = xTaskCreate(hetero_rtos_task, "Het_RTOS_Test",
                          configMINIMAL_STACK_SIZE * 2, NULL,
                          HET_STRESS_TASK_PRIORITY, NULL);
    if (xReturn != pdPASS)
    {
        ERROR("Failed to create RTOS test task");
    }
#endif

#if HET_STRESS_ENABLE_ETHERNET
    xReturn = xTaskCreate(hetero_ethernet_task, "Het_Eth_Stress",
                          configMINIMAL_STACK_SIZE * 2, NULL,
                          HET_STRESS_TASK_PRIORITY,
#if HET_STRESS_ENABLE_PINNING
                          &task_handle
#else
                          NULL
#endif
                          );
    if (xReturn == pdPASS)
    {
#if HET_STRESS_ENABLE_PINNING
        set_task_affinity(task_handle);
#endif
    }
    else
    {
        ERROR("Failed to create Ethernet stress task");
    }
#endif
}

int main(void)
{
    BaseType_t xReturn;

    prvSetupHardware();

    xReturn = xTaskCreate(run_stress, "Run_Het_Stress",
                          configMINIMAL_STACK_SIZE, NULL,
                          HET_STRESS_TASK_PRIORITY, NULL);
    if (xReturn == pdPASS)
    {
        vTaskStartScheduler();
    }

    while (1)
    {
    }
}
