#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "pico/multicore.h"
#include "hardware/i2c.h"
#include "hardware/uart.h"
#include "hardware/gpio.h"

#include "FreeRTOS.h"
#include "FreeRTOS-Kernel/include/task.h"
#include "FreeRTOS-Kernel/include/queue.h"
#include "FreeRTOS-Kernel/include/semphr.h"

#include "ADXL375.h"

// Constants
#define PICO_DEFAULT_LED_PIN 25

// Pin definitions
#define I2C0_SDA_PIN 4
#define I2C0_SCL_PIN 5
#define I2C1_SDA_PIN 18
#define I2C1_SCL_PIN 19

#define UART0_TX_PIN 12
#define UART0_RX_PIN 13

#define I2C_FREQUENCY 400000
#define UART_BAUD_RATE 115200

static bool setup_hardware(void);

// Task function prototypes
void simple_task(void *pvParameters);

// void vApplicationMallocFailedHook(TaskHandle_t xTask, signed portCHAR* pcTaskName);
void vApplicationStackOverflowHook(TaskHandle_t pxTask, char *pcTaskName);
void vApplicationIdleHook(void);
void vApplicationTickHook(void);

// Function to initialize hardware
static bool setup_hardware(void) {
    stdio_init_all();

    if (cyw43_arch_init()) {
        printf("Wi-Fi init failed");
        return false;
    }

    // Initialize LED
    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);
    gpio_put(PICO_DEFAULT_LED_PIN, 1);

    // Initialize I2C0
    i2c_init(i2c0, I2C_FREQUENCY);
    gpio_set_function(I2C0_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(I2C0_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(I2C0_SDA_PIN);
    gpio_pull_up(I2C0_SCL_PIN);
    printf("I2C0 initialized on pins %d (SDA) and %d (SCL)\n", I2C0_SDA_PIN, I2C0_SCL_PIN);

    // Initialize I2C1
    i2c_init(i2c1, I2C_FREQUENCY);
    gpio_set_function(I2C1_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(I2C1_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(I2C1_SDA_PIN);
    gpio_pull_up(I2C1_SCL_PIN);
    printf("I2C1 initialized on pins %d (SDA) and %d (SCL)\n", I2C1_SDA_PIN, I2C1_SCL_PIN);

    return true;
}

static bool setup_sensors(void) {
    // Initialize the ADXL375 sensor
    ADXL375::ADXL375<I2C> adxl375_sensor(i2c0, 0x53); // Example I2C address
    adxl375_sensor.hello();

    // Additional sensors can be initialized here

    return true;
}

int main(void) {
    // Initialize the hardware
    if (!setup_hardware()) {
        return -1;
    }

    if (!setup_sensors()) {
        printf("Failed to setup sensors\n");
        return -1;
    }
    
    printf("FreeRTOS SMP starting on Raspberry Pi Pico\n");
    // Create tasks for sensors
    xTaskCreate(simple_task, "SimpleTask", configMINIMAL_STACK_SIZE * 3, NULL, 2, NULL);
    
    // Start the FreeRTOS scheduler
    vTaskStartScheduler();
    
    // We should never get here
    while (true) {
        printf("Error: FreeRTOS scheduler failed to start\n");
        vTaskDelay(pdMS_TO_TICKS(1000));
    }

    return -1;
}

void simple_task(void *pvParameters) {
    // This is a simple task that just toggles the LED
    while (true) {
        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
        vTaskDelay(pdMS_TO_TICKS(250));    // Delay for 500 ms
        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);
        vTaskDelay(pdMS_TO_TICKS(250));    // Delay for 500 ms
    }
}


// Standard FreeRTOS hook functions already implemented in the existing main.c
/*-----------------------------------------------------------*/

// void vApplicationMallocFailedHook(TaskHandle_t xTask, signed portCHAR* pcTaskName)
// {
//     /* Called if a call to pvPortMalloc() fails because there is insufficient
//     free memory available in the FreeRTOS heap.  pvPortMalloc() is called
//     internally by FreeRTOS API functions that create tasks, queues, software
//     timers, and semaphores.  The size of the FreeRTOS heap is set by the
//     configTOTAL_HEAP_SIZE configuration constant in FreeRTOSConfig.h. */

//     /* Force an assert. */
//     configASSERT( ( volatile void * ) NULL );
// }
/*-----------------------------------------------------------*/

void vApplicationStackOverflowHook( TaskHandle_t pxTask, char *pcTaskName )
{
    ( void ) pcTaskName;
    ( void ) pxTask;

    /* Run time stack overflow checking is performed if
    configCHECK_FOR_STACK_OVERFLOW is defined to 1 or 2.  This hook
    function is called if a stack overflow is detected. */

    /* Force an assert. */
    configASSERT( ( volatile void * ) NULL );
}
/*-----------------------------------------------------------*/

void vApplicationIdleHook( void )
{
    volatile size_t xFreeHeapSpace;

    /* This is just a trivial example of an idle hook.  It is called on each
    cycle of the idle task.  It must *NOT* attempt to block.  In this case the
    idle task just queries the amount of FreeRTOS heap that remains.  See the
    memory management section on the http://www.FreeRTOS.org web site for memory
    management options.  If there is a lot of heap memory free then the
    configTOTAL_HEAP_SIZE value in FreeRTOSConfig.h can be reduced to free up
    RAM. */
    xFreeHeapSpace = xPortGetFreeHeapSize();

    /* Remove compiler warning about xFreeHeapSpace being set but never used. */
    ( void ) xFreeHeapSpace;
}
/*-----------------------------------------------------------*/

void vApplicationTickHook( void )
{
#if mainCREATE_SIMPLE_BLINKY_DEMO_ONLY == 0
    {
        /* The full demo includes a software timer demo/test that requires
        prodding periodically from the tick interrupt. */
        #if (mainENABLE_TIMER_DEMO == 1)
        vTimerPeriodicISRTests();
        #endif

        /* Call the periodic queue overwrite from ISR demo. */
        #if (mainENABLE_QUEUE_OVERWRITE == 1)
        vQueueOverwritePeriodicISRDemo();
        #endif

        /* Call the periodic event group from ISR demo. */
        #if (mainENABLE_EVENT_GROUP == 1)
        vPeriodicEventGroupsProcessing();
        #endif

        /* Call the code that uses a mutex from an ISR. */
        #if (mainENABLE_INTERRUPT_SEMAPHORE == 1)
        vInterruptSemaphorePeriodicTest();
        #endif

        /* Call the code that 'gives' a task notification from an ISR. */
        #if (mainENABLE_TASK_NOTIFY == 1)
        xNotifyTaskFromISR();
        #endif
    }
#endif
}
