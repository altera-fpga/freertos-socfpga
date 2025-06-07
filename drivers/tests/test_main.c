#include "FreeRTOS.h"
#include "FreeRTOSConfig.h"
#include "task.h"
#include "semphr.h"
#include "test_main.h"
#include "usb_main.h"
/* Platform specific files.*/
#include "socfpga_interrupt.h"
#include "socfpga_console.h"
#include "socfpga_smmu.h"

#define TASK_PRIORITY (configMAX_PRIORITIES - 2)
#define USBH_STACK_SIZE    (5*configMINIMAL_STACK_SIZE)

void run_tests();
void wdt_test( void );
void timer_test( void );
void gpio_test( void );
void uart_test( void );
void i2c_test( void );
void iossm_test(void);
int sdmmc_test( void );
void mbox_test( void );
int dma_test( void);
int fatfs_test(int disk_type);
void reboot_test(void);
void qspi_test(void);
void spi_test( void );
extern int dma_test( void);
void seu_test();
void i3c_test(void);
int usb3_test(void);
int fpga_mngr_test(void);
int bridge_test(void);
void fcs_test(void);
void rsu_test();
void iossm_test();

void vApplicationTickHook( void )
{
    /*
     * This is called from RTOS tick handler
     * Not used in this demo, But defined to keep the configuration sharing
     * simple
     * */
}

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

void test_main()
{
    BaseType_t xReturn;

    xReturn = xTaskCreate(run_tests, "RunTests", configMINIMAL_STACK_SIZE, NULL, TASK_PRIORITY, NULL);
    if ( xReturn == pdPASS )
    {
        vTaskStartScheduler();
    }
}

void run_tests()
{
#if SOCFPGA_TEST_DRIVER_RSU
    rsu_test();
#endif
#if SOCFPGA_TEST_DRIVER_BRIDGE
    bridge_test();
#endif
#if SOCFPGA_TEST_DRIVER_FPGA_MANAGER
    fpga_mngr_test();
#endif
#if SOCFPGA_TEST_DRIVER_SEU
    seu_test();
#endif

#if SOCFPGA_TEST_DRIVER_DMA
    dma_test();
#endif
#if SOCFPGA_TEST_DRIVER_WDT
    wdt_test();
#endif
#if SOCFPGA_TEST_DRIVER_TIMER
    timer_test();
#endif
#if SOCFPGA_TEST_DRIVER_GPIO
    gpio_test();
#endif
#if SOCFPGA_TEST_DRIVER_I2C
    i2c_test();
#endif
#if SOCFPGA_TEST_DRIVER_UART
    uart_test();
#endif
#if SOCFPGA_TEST_DRIVER_IOSSM
    iossm_test();
#endif
#if SOCFPGA_TEST_DRIVER_I3C
    i3c_test();
#endif
#if SOCFPGA_TEST_DRIVER_SDMMC
    sdmmc_test();
#endif
#if SOCFPGA_TEST_DRIVER_FATFS
    fatfs_test(-1);
#endif
#if SOCFPGA_TEST_DRIVER_SDM_MAILBOX
    mbox_test();
#endif
#if SOCFPGA_TEST_DRIVER_FCS
    fcs_test();
#endif
#if SOCFPGA_TEST_DRIVER_REBOOT
    reboot_test();
#endif
#if SOCFPGA_TEST_DRIVER_QSPI
    qspi_test();
#endif
#if SOCFPGA_TEST_DRIVER_SPI
    spi_test();
#endif
#if SOCFPGA_TEST_DRIVER_USB3
    /* Usb3 task priority should be less than other task priority */
    BaseType_t xReturn;
    xReturn = xTaskCreate(usb3_task, "usbtask", configMINIMAL_STACK_SIZE*20, NULL, TASK_PRIORITY-1, NULL);
    if ( xReturn != pdPASS )
    {
        //task creation failed
        while(1);
    }

    wait_for_usb_mount_complete();
    fatfs_test(0);  //0 for usb3 
    usb3_test();
#endif

    vTaskSuspend(NULL);
}

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

void vApplicationIdleHook( void )
{
#if configENABLE_CONSOLE_UART
    /*Clear any buffered prints to console*/
    console_clear_pending();
#endif
}

/*-----------------------------------------------------------*/

int main( void )
{
    prvSetupHardware();

    test_main();

    /*Block here indefinitely; Should never reach here*/
    while ( 1 )
    {
    }
}
/*-----------------------------------------------------------*/
