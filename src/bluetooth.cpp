#define BTSTACK_FILE__ "bluetooth.cpp"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "pico/multicore.h"
#include "hardware/i2c.h"
#include "hardware/uart.h"
#include "hardware/gpio.h"

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

#include "btstack.h"

#include "ADXL375/ADXL375.h"
#include "UBLOX/UBLOX.h"

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
void gps_task(void *pvParameters);

// Global GPS objects
UBLOX::Pico_I2C* gps_i2c = nullptr;
UBLOX::UBLOX* gps_module = nullptr;

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

    return true;
}

int main(void) {
    // Initialize the hardware
    if (!setup_hardware()) {
        return -1;
    }

    ADXL375::ADXL375 sensor;
    sensor.hello();  // Call the ADXL375 module's hello function
    
    // Initialize GPS module with FreeRTOS abstractions
    gps_i2c = new UBLOX::Pico_I2C(i2c0, 4, 5);  // SDA=4, SCL=5
    gps_module = new UBLOX::UBLOX(gps_i2c);
    
    UBLOX::Configuration gps_config;
    gps_config.output_ubx = true;
    gps_config.output_nmea = false;
    gps_config.task_priority = 3;
    gps_config.stack_size = 2048;
    gps_config.measurement_rate_ms = 1000;  // 1Hz
    
    if (gps_module->initialize(gps_config)) {
        printf("GPS: Module initialized successfully\n");
    } else {
        printf("GPS: Failed to initialize module\n");
    }
    
    printf("FreeRTOS SMP starting on Raspberry Pi Pico\n");
    
    // Create tasks for sensors
    xTaskCreate(simple_task, "SimpleTask", configMINIMAL_STACK_SIZE * 3, NULL, 2, NULL);
    xTaskCreate(gps_task, "GPSTask", configMINIMAL_STACK_SIZE * 4, NULL, 2, NULL);
    
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

void gps_task(void *pvParameters) {
    printf("GPS Task: Started\n");
    
    // Wait for GPS module to be ready
    vTaskDelay(pdMS_TO_TICKS(1000));
    
    if (!gps_module) {
        printf("GPS Task: Module not initialized\n");
        vTaskDelete(NULL);
        return;
    }
    
    UBLOX::GPSData gps_data;
    uint32_t message_count = 0;
    
    while (true) {
        // Try to get GPS data (non-blocking with 100ms timeout)
        if (gps_module->get_data(gps_data, 100)) {
            message_count++;
            
            printf("GPS [%lu]: Received %zu bytes at tick %lu\n", 
                   message_count, gps_data.length, gps_data.timestamp);
            
            // Print first few bytes for debugging (UBX messages start with 0xB5 0x62)
            if (gps_data.length >= 6) {
                printf("GPS Data: %02X %02X %02X %02X %02X %02X...\n",
                       gps_data.data[0], gps_data.data[1], gps_data.data[2],
                       gps_data.data[3], gps_data.data[4], gps_data.data[5]);
            }
            
            // Print GPS module status every 10 messages
            if (message_count % 10 == 0) {
                gps_module->print_status();
                
                // Print task statistics
                TaskStatus_t task_status;
                gps_module->get_task_stats(task_status);
                printf("GPS Task Stats: State=%d, Stack HWM=%lu\n",
                       task_status.eCurrentState, task_status.usStackHighWaterMark);
            }
        } else {
            // No data available, just continue
            printf("GPS: No data available\n");
        }
        
        // Task runs every 500ms
        vTaskDelay(pdMS_TO_TICKS(500));
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
