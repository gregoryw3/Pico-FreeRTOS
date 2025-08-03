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

// Include BTstack and GATT counter
#include "btstack.h"
#include "gatt_counter.h"

#define ENABLE_GATT_OVER_CLASSIC
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
void bluetooth_task(void *pvParameters);

static btstack_packet_callback_registration_t hci_event_callback_registration;

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

    printf("FreeRTOS SMP starting on Raspberry Pi Pico\n");
    
    xTaskCreate(simple_task, "SimpleTask", configMINIMAL_STACK_SIZE * 3, NULL, 2, NULL);
    xTaskCreate(bluetooth_task, "BluetoothTask", configMINIMAL_STACK_SIZE * 8, NULL, 3, NULL);
    // Start the FreeRTOS scheduler
    vTaskStartScheduler();    // We should never get here
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

static void packet_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size){
    UNUSED(size);
    UNUSED(channel);
    bd_addr_t local_addr;
    if (packet_type != HCI_EVENT_PACKET) return;
    switch(hci_event_packet_get_type(packet)){
        case BTSTACK_EVENT_STATE:
            if (btstack_event_state_get_state(packet) != HCI_STATE_WORKING) return;
            gap_local_bd_addr(local_addr);
            printf("BTstack up and running on %s.\n", bd_addr_to_str(local_addr));
            break;
        default:
            break;
    }
}

void bluetooth_task(void *pvParameters) {
    hci_event_callback_registration.callback = &packet_handler;
    hci_add_event_handler(&hci_event_callback_registration);

    // If we're using lwip but not via cyw43 (e.g. pan) we have to call this
    #if HAVE_LWIP && !CYW43_LWIP
        lwip_freertos_init(cyw43_arch_async_context());
    #endif

        btstack_main();

    #ifdef TEST_BLINK_TASK
        xTaskCreate(blink_task, "BlinkThread", configMINIMAL_STACK_SIZE, NULL, BLINK_TASK_PRIORITY, NULL);
    #endif

        while(true) {
            vTaskDelay(1000);
        }

    #if HAVE_LWIP && !CYW43_LWIP
        lwip_freertos_deinit(cyw43_arch_async_context());
    #endif
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
