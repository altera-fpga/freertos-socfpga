/*
 * SPDX-FileCopyrightText: Copyright (C) 2025 Altera Corporation
 *
 * SPDX-License-Identifier: MIT-0
 *
 * Common entry function for all sample apps
 */

/*
    File: IOSSM Sample Entry Point

    Common entry function for all sample apps.
*/

#include "FreeRTOS.h"
#include "FreeRTOSConfig.h"
#include "task.h"
#include "socfpga_interrupt.h"
#include "socfpga_console.h"
#include "socfpga_smmu.h"

#define TASK_PRIORITY    (configMAX_PRIORITIES - 2)
void run_samples( void *arg );
void iossm_task();

/*
    Function: vApplicationTickHook

    Tick hook, called by the kernel from the RTOS tick interrupt.

    It runs in an interrupt context on every tick, so it must return quickly
    and may only use the interrupt-safe API. FreeRTOS calls it only when
    configUSE_TICK_HOOK is 1.
*/
void vApplicationTickHook( void )
{
    /*
     * This is called from RTOS tick handler
     * Not used in this demo, But defined to keep the configuration sharing
     * simple
     * */
}

/*
    Function: vApplicationMallocFailedHook

    Called when the kernel cannot allocate memory.

    pvPortMalloc() calls this when it has no block large enough, which happens
    while creating a task, queue, timer or semaphore once the heap set by
    configTOTAL_HEAP_SIZE is exhausted. FreeRTOS calls it only when
    configUSE_MALLOC_FAILED_HOOK is 1.
*/
void vApplicationMallocFailedHook( void )
{
    /* vApplicationMallocFailedHook() will only be called if
       configUSE_MALLOC_FAILED_HOOK is set to 1 in FreeRTOSConfig.h.  It is a hook
       function that will get called if a call to pvPortMalloc() fails.
       pvPortMalloc() is called internally by the kernel whenever a task, queue,
       timer or semaphore is created.  It is also called by various parts of the
       demo application.  If heap_1.c or heap_2.c are used, then the size of the
       heap available to pvPortMalloc() is defined by configTOTAL_HEAP_SIZE in
       FreeRTOSConfig.h, and the xPortGetFreeHeapSize() API function can be used
       to query the size of free heap space that remains (although it does not
       provide information on how the remaining heap might be fragmented). */
    taskDISABLE_INTERRUPTS();
    for ( ;; )
        ;
}

/*
    Function: samples_main

    Sets the IOSSM sample running: creates the "Run_Samples" task, which runs
    <run_samples>; then starts the FreeRTOS scheduler.

    The scheduler is only started if the task was created, so a sample that
    runs out of heap stops here instead of running with nothing to do.
*/
void samples_main()
{
    BaseType_t xReturn;

    xReturn = xTaskCreate(run_samples, "Run_Samples", configMINIMAL_STACK_SIZE,
            NULL, TASK_PRIORITY, NULL);
    if (xReturn == 1)
    {
        vTaskStartScheduler();
    }

}

/*
    Function: run_samples

    Task that carries out the IOSSM sample: runs iossm_task.

    It then suspends itself, leaving the scheduler to run the remaining tasks.

    Parameters:
        arg - Not used.
*/
void run_samples( void *arg )
{
    (void) arg;

    iossm_task();

    vTaskSuspend(NULL);
}

/*
    Function: prvSetupHardware

    Brings the platform up before any task runs.

    Calls <interrupt_init_gic>, <smmu_enable> and <console_init>, in that
    order.
*/
static void prvSetupHardware( void )
{
    /* Initialize the GIC. */
    interrupt_init_gic();

    /* Enable SMMU */
    (void)smmu_enable();

    /* Initialize the console uart*/
#if configENABLE_CONSOLE_UART
    console_init(configCONSOLE_UART_ID, "115200-8N1");
#endif
}

/*
    Function: vApplicationIdleHook

    Idle hook, called repeatedly from the idle task.

    It must never attempt to block or suspend, because the idle task has to
    stay ready to run. FreeRTOS calls it only when configUSE_IDLE_HOOK is 1.
*/
void vApplicationIdleHook( void )
{
}


/*
    Function: main

    Entry point for the IOSSM sample.

    Everything the sample does starts here: it calls <prvSetupHardware> and
    <samples_main>. The loop at the end never falls through, so this is the
    last frame on the stack for the life of the system.

    Returns:
        Never returns. The scheduler owns the processor from here on, and the
        sample runs until the device is reset.
*/
int main( void )
{
    prvSetupHardware();

    samples_main();

    /*Block here indefinitely; Should never reach here*/
    while ( 1 )
    {
    }
}
