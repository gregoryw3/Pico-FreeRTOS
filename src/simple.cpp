#include <stdio.h>
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

// Constants
#define PICO_DEFAULT_LED_PIN 25

// Pin definitions
#define I2C0_SDA_PIN 4
#define I2C0_SCL_PIN 5
#define I2C1_SDA_PIN 18
#define I2C1_SCL_PIN 19

#define UART1_TX_PIN 8
#define UART1_RX_PIN 9

#define I2C_FREQUENCY 100000
#define UART_BAUD_RATE 38400

static bool setup_hardware(void);

// Task function prototypes
void simple_task(void *pvParameters);
void uart_usb_bridge_task(void *pvParameters);
void i2c_scan_task(void *pvParameters);

// I2C helper function
bool reserved_addr(uint8_t addr);

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

    // Initialize UART1 for GPS
    uart_init(uart1, UART_BAUD_RATE);
    gpio_set_function(UART1_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(UART1_RX_PIN, GPIO_FUNC_UART);
    uart_set_format(uart1, 8, 1, UART_PARITY_NONE);
    uart_set_hw_flow(uart1, false, false);
    uart_set_fifo_enabled(uart1, true);

    // Initialize I2C0 for device scanning
    i2c_init(i2c0, I2C_FREQUENCY);
    gpio_set_function(I2C0_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(I2C0_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(I2C0_SDA_PIN);
    gpio_pull_up(I2C0_SCL_PIN);

    // Initialize I2C1 for device scanning
    i2c_init(i2c1, I2C_FREQUENCY);
    gpio_set_function(I2C1_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(I2C1_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(I2C1_SDA_PIN);
    gpio_pull_up(I2C1_SCL_PIN);

    return true;
}

void uart_baudrate_sweep_task(void *pvParameters) {
    uint32_t baudrates[] = {4800, 9600, 19200, 38400, 57600, 115200};
    const int num_rates = sizeof(baudrates) / sizeof(baudrates[0]);
    uint8_t buf[64];

    vTaskDelay(pdMS_TO_TICKS(5000)); // Allow time for GPS to stabilize

    for (int i = 0; i < num_rates; ++i) {
        uart_init(uart1, baudrates[i]);
        printf("\n--- Testing baudrate: %u ---\n", baudrates[i]);
        absolute_time_t until = make_timeout_time_ms(2000);
        int count = 0;
        while (absolute_time_diff_us(get_absolute_time(), until) > 0 && count < sizeof(buf)) {
            if (uart_is_readable(uart1)) {
                uart_read_blocking(uart1, buf + count, 1);
                putchar(buf[count]);
                count++;
            }
        }
        printf("\n--- End of baudrate %u ---\n", baudrates[i]);
        sleep_ms(500); // Short pause between rates
    }

    // Optionally delete the task when done
    vTaskDelete(NULL);
}

int main(void) {
    // Initialize the hardware
    if (!setup_hardware()) {
        return -1;
    }

    // xTaskCreate(uart_baudrate_sweep_task, "BaudSweep", configMINIMAL_STACK_SIZE * 4, NULL, 2, NULL);

    
    printf("FreeRTOS SMP starting on Raspberry Pi Pico\n");
    // Create tasks for sensors
    // xTaskCreate(simple_task, "SimpleTask", configMINIMAL_STACK_SIZE * 3, NULL, 2, NULL);
    // Create UART-USB bridge task
    // xTaskCreate(uart_usb_bridge_task, "UARTUSBBridge", configMINIMAL_STACK_SIZE * 4, NULL, 2, NULL);
    // Create I2C scan task (runs once every 10 seconds)
    xTaskCreate(i2c_scan_task, "I2CScan", configMINIMAL_STACK_SIZE * 4, NULL, 1, NULL);
    
    // Give hardware time to stabilize before starting scheduler
    printf("Waiting for hardware to stabilize...\n");
    sleep_ms(2000);
    
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




// UART-USB bridge task: forwards data between UART1 (GPS) and USB CDC
void uart_usb_bridge_task(void *pvParameters) {
    // Wait for system to stabilize before starting
    vTaskDelay(pdMS_TO_TICKS(3000));
    
    const size_t bufsize = 128;
    uint8_t uart_buf[bufsize];
    uint8_t usb_buf[bufsize];

    while (true) {
        // printf("UART-USB Bridge Task Running\n");
        // fflush(stdout); // Ensure output is flushed
        // Forward UART->USB
        uart_read_blocking(uart1, uart_buf, 1); // read 1 byte
        // printf("UART RX: %02x\n", uart_buf[0]);
        putchar(uart_buf[0]); // USB CDC
        // if (uart_is_readable(uart1)) {
            
        // }
        // Forward USB->UART
        int c = getchar_timeout_us(0);
        if (c != PICO_ERROR_TIMEOUT) {
            uint8_t b = (uint8_t)c;
            uart_write_blocking(uart1, &b, 1);
        }
        // if (stdio_usb_connected()) {
            
        // }
        vTaskDelay(pdMS_TO_TICKS(1)); // Small delay to yield
    }
}

// I2C reserves some addresses for special purposes. We exclude these from the scan.
// These are any addresses of the form 000 0xxx or 111 1xxx
bool reserved_addr(uint8_t addr) {
    return (addr & 0x78) == 0 || (addr & 0x78) == 0x78;
}

// I2C Bus Scan task: scans for I2C devices periodically
void i2c_scan_task(void *pvParameters) {
    // Wait for system to stabilize before starting I2C scans
    vTaskDelay(pdMS_TO_TICKS(5000));
    
    // Define possible I2C pin combinations
    struct i2c_pins {
        uint8_t sda;
        uint8_t scl;
        const char* name;
    };
    
    struct i2c_pins i2c0_pins[] = {
        {0, 1, "GP0/GP1"},
        {4, 5, "GP4/GP5"},
        {8, 9, "GP8/GP9"},
        {12, 13, "GP12/GP13"},
        {16, 17, "GP16/GP17"},
        {20, 21, "GP20/GP21"}
    };
    
    struct i2c_pins i2c1_pins[] = {
        {2, 3, "GP2/GP3"},
        {6, 7, "GP6/GP7"},
        {10, 11, "GP10/GP11"},
        {14, 15, "GP14/GP15"},
        {18, 19, "GP18/GP19"},
        {26, 27, "GP26/GP27"}
    };

    while (true) {
        printf("\n========================================\n");
        printf("Starting comprehensive I2C bus scan...\n");
        printf("========================================\n");

        // Scan all I2C0 pin combinations
        for (size_t i = 0; i < sizeof(i2c0_pins) / sizeof(i2c0_pins[0]); i++) {
            printf("\n=== I2C0 Bus Scan (%s) ===\n", i2c0_pins[i].name);
            
            // Deinitialize current I2C0 configuration
            i2c_deinit(i2c0);
            
            // Reconfigure I2C0 with new pins
            i2c_init(i2c0, I2C_FREQUENCY);
            gpio_set_function(i2c0_pins[i].sda, GPIO_FUNC_I2C);
            gpio_set_function(i2c0_pins[i].scl, GPIO_FUNC_I2C);
            gpio_pull_up(i2c0_pins[i].sda);
            gpio_pull_up(i2c0_pins[i].scl);
            
            // Small delay to let I2C settle
            vTaskDelay(pdMS_TO_TICKS(100));
            
            printf("   0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F\n");
            
            bool found_device = false;
            for (int addr = 0; addr < (1 << 7); ++addr) {
                if (addr % 16 == 0) {
                    printf("%02x ", addr);
                }

                int ret;
                uint8_t rxdata;
                if (reserved_addr(addr))
                    ret = PICO_ERROR_GENERIC;
                else
                    ret = i2c_read_blocking(i2c0, addr, &rxdata, 1, false);

                if (ret >= 0) {
                    printf("@");
                    found_device = true;
                } else {
                    printf(".");
                }
                printf(addr % 16 == 15 ? "\n" : "  ");
            }
            
            if (found_device) {
                printf("*** DEVICES FOUND on I2C0 %s ***\n", i2c0_pins[i].name);
            } else {
                printf("No devices found on I2C0 %s\n", i2c0_pins[i].name);
            }
        }

        // Scan all I2C1 pin combinations
        for (size_t i = 0; i < sizeof(i2c1_pins) / sizeof(i2c1_pins[0]); i++) {
            printf("\n=== I2C1 Bus Scan (%s) ===\n", i2c1_pins[i].name);
            
            // Deinitialize current I2C1 configuration
            i2c_deinit(i2c1);
            
            // Reconfigure I2C1 with new pins
            i2c_init(i2c1, I2C_FREQUENCY);
            gpio_set_function(i2c1_pins[i].sda, GPIO_FUNC_I2C);
            gpio_set_function(i2c1_pins[i].scl, GPIO_FUNC_I2C);
            gpio_pull_up(i2c1_pins[i].sda);
            gpio_pull_up(i2c1_pins[i].scl);
            
            // Small delay to let I2C settle
            vTaskDelay(pdMS_TO_TICKS(100));
            
            printf("   0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F\n");
            
            bool found_device = false;
            for (int addr = 0; addr < (1 << 7); ++addr) {
                if (addr % 16 == 0) {
                    printf("%02x ", addr);
                }

                int ret;
                uint8_t rxdata;
                if (reserved_addr(addr))
                    ret = PICO_ERROR_GENERIC;
                else
                    ret = i2c_read_blocking(i2c1, addr, &rxdata, 1, false);

                if (ret >= 0) {
                    printf("@");
                    found_device = true;
                } else {
                    printf(".");
                }
                printf(addr % 16 == 15 ? "\n" : "  ");
            }
            
            if (found_device) {
                printf("*** DEVICES FOUND on I2C1 %s ***\n", i2c1_pins[i].name);
            } else {
                printf("No devices found on I2C1 %s\n", i2c1_pins[i].name);
            }
        }

        printf("\n========================================\n");
        printf("Comprehensive I2C scan complete!\n");
        printf("========================================\n");
        
        // Restore original pin configuration
        i2c_deinit(i2c0);
        i2c_deinit(i2c1);
        
        // Restore I2C0 to original pins
        i2c_init(i2c0, I2C_FREQUENCY);
        gpio_set_function(I2C0_SDA_PIN, GPIO_FUNC_I2C);
        gpio_set_function(I2C0_SCL_PIN, GPIO_FUNC_I2C);
        gpio_pull_up(I2C0_SDA_PIN);
        gpio_pull_up(I2C0_SCL_PIN);
        
        // Restore I2C1 to original pins
        i2c_init(i2c1, I2C_FREQUENCY);
        gpio_set_function(I2C1_SDA_PIN, GPIO_FUNC_I2C);
        gpio_set_function(I2C1_SCL_PIN, GPIO_FUNC_I2C);
        gpio_pull_up(I2C1_SDA_PIN);
        gpio_pull_up(I2C1_SCL_PIN);
        
        // Wait 30 seconds before next comprehensive scan
        vTaskDelay(pdMS_TO_TICKS(30000));
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
