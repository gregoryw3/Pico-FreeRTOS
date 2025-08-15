#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "pico/multicore.h"
#include "hardware/i2c.h"
#include "hardware/uart.h"
#include "hardware/gpio.h"
#include "hardware/watchdog.h"

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

#include "BNO55.hpp"

// Constants
#define PICO_DEFAULT_LED_PIN 25

// Pin definitions
#define I2C0_SDA_PIN 4
#define I2C0_SCL_PIN 5
#define I2C1_SDA_PIN 18
#define I2C1_SCL_PIN 19

#define UART1_TX_PIN 20
#define UART1_RX_PIN 21

#define I2C_FREQUENCY 400000
#define UART_BAUD_RATE 9600

static bool setup_hardware(void);
static void vLaunch(void);

// Task function prototypes
void simple_task(void *pvParameters);
void bno055_task(void *pvParameters);
void uart_usb_bridge_task(void *pvParameters);

// void vApplicationMallocFailedHook(TaskHandle_t xTask, signed portCHAR* pcTaskName);
void vApplicationStackOverflowHook(TaskHandle_t pxTask, char *pcTaskName);
void vApplicationIdleHook(void);
void vApplicationTickHook(void);

// Function to initialize hardware
static bool setup_hardware(void) {
    stdio_init_all();

    if (cyw43_arch_init()) {
        printf("ERROR: Wi-Fi init failed\n");
        return false;
    }

    // Initialize LED
    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);
    gpio_put(PICO_DEFAULT_LED_PIN, 1);

    // Initialize UART1 for GPS with error checking
    uint baud_actual = uart_init(uart1, UART_BAUD_RATE);
    if (baud_actual == 0) {
        printf("ERROR: UART1 initialization failed\n");
        return false;
    }
    printf("UART1 initialized at %u baud (requested %u)\n", baud_actual, UART_BAUD_RATE);
    
    gpio_set_function(UART1_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(UART1_RX_PIN, GPIO_FUNC_UART);
    uart_set_format(uart1, 8, 1, UART_PARITY_NONE);
    uart_set_hw_flow(uart1, false, false);
    uart_set_fifo_enabled(uart1, true);

    // Initialize I2C0 for device scanning with error checking
    uint i2c0_actual = i2c_init(i2c0, I2C_FREQUENCY);
    if (i2c0_actual == 0) {
        printf("ERROR: I2C0 initialization failed\n");
        return false;
    }
    printf("I2C0 initialized at %u Hz (requested %u)\n", i2c0_actual, I2C_FREQUENCY);
    
    gpio_set_function(I2C0_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(I2C0_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(I2C0_SDA_PIN);
    gpio_pull_up(I2C0_SCL_PIN);

    // Initialize I2C1 for device scanning with error checking
    uint i2c1_actual = i2c_init(i2c1, I2C_FREQUENCY);
    if (i2c1_actual == 0) {
        printf("ERROR: I2C1 initialization failed\n");
        return false;
    }
    printf("I2C1 initialized at %u Hz (requested %u)\n", i2c1_actual, I2C_FREQUENCY);
    
    gpio_set_function(I2C1_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(I2C1_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(I2C1_SDA_PIN);
    gpio_pull_up(I2C1_SCL_PIN);

    printf("Hardware initialization completed successfully\n");
    return true;
}

void vLaunch(void) {
    printf("%s starting on core %d\n", "FreeRTOS SMP", get_core_num());
    printf("Initial free heap: %zu bytes\n", xPortGetFreeHeapSize());
    
    // Create tasks with increased stack sizes to prevent overflow
    // In SMP mode, tasks can run on any core automatically
    xTaskCreate(simple_task, "SimpleTask", configMINIMAL_STACK_SIZE * 2, NULL, 2, NULL);
    // xTaskCreate(bno055_task, "BNO055Task", configMINIMAL_STACK_SIZE * 6, NULL, 2, NULL);
    // Create UART-USB bridge task with larger stack and lower priority
    xTaskCreate(uart_usb_bridge_task, "UARTUSBBridge", configMINIMAL_STACK_SIZE * 3, NULL, 1, NULL);
    
    printf("Tasks created on core %d. Free heap: %zu bytes\n", get_core_num(), xPortGetFreeHeapSize());

    // Start the FreeRTOS scheduler
    vTaskStartScheduler();
    
    // We should never get here
    while (1) {
        tight_loop_contents();
    }
}

int main(void) {
    // Initialize the hardware
    if (!setup_hardware()) {
        return -1;
    }

    printf("%s on both cores:\n", "FreeRTOS SMP");
    vLaunch();  // Start scheduler on core 0

    return 0;
}

void simple_task(void *pvParameters) {    
    // This is a simple task that just toggles the LED
    static uint32_t loop_count = 0;
    
    printf("Simple task started on core %d\n", get_core_num());
    
    while (true) {
        // printf("Simple task running on core %d\n", get_core_num());
        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
        vTaskDelay(pdMS_TO_TICKS(250));    // Delay for 250 ms
        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);
        vTaskDelay(pdMS_TO_TICKS(250));    // Delay for 250 ms
        
        // Periodic heartbeat message every 60 seconds
        loop_count++;
        if (loop_count % 120 == 0) {  // 120 * 500ms = 60 seconds
            printf("Simple task heartbeat (core %d) - loops: %lu, free heap: %zu bytes\n", 
                   get_core_num(), loop_count, xPortGetFreeHeapSize());
        }
    }
}

void bno055_task(void *pvParameters) {

    // Initialize BNO55 sensor
    BNO55::BNO55<I2C> bno55_sensor(i2c0, BNO55::Address::DEFAULT);
    bno55_sensor.pose_on_pcb = Pose(Eigen::Vector3f(0.0f, 0.0f, 0.0f), Eigen::Quaternionf::Identity());
    if (!bno55_sensor.init(BNO55::Settings {
        .power_mode = BNO55::PowerMode::NORMAL,
        .operation_mode = BNO55::OperationMode::NDOF,
        .unit_selection = BNO55::UnitSelection::METRIC,
        .axis_map_config = 0x00, // Default axis mapping
        .axis_map_sign = 0x00,   // Default axis sign
        .temp_source = 0x00      // Default temperature source
    })) {
        while(true) {
            printf("BNO55 initialization failed, retrying...\n");
            vTaskDelay(pdMS_TO_TICKS(1000)); // Retry every second
        }
    }

    while(true) {
        // Method 1: Read individual sensor components (as you're doing now)
        // Read accelerometer data
        AccelData accel_data = bno55_sensor.read_accel();
        Eigen::Vector3f accel = Eigen::Vector3f(accel_data.x, accel_data.y, accel_data.z);

        // Teleplot format for accelerometer data (separate lines)
        printf(">accel_x:%.3f\n", accel_data.x);
        printf(">accel_y:%.3f\n", accel_data.y);
        printf(">accel_z:%.3f\n", accel_data.z);

        // Method 2: Read all IMU9 data at once using the new read() method
        IMU9Data imu9_data = bno55_sensor.read();
        
        // Now you can access all 9-axis data from the single read
        printf(">imu9_accel_x:%.3f\n", imu9_data.accel.x);
        printf(">imu9_gyro_x:%.3f\n", imu9_data.gyro.x);
        printf(">imu9_mag_x:%.3f\n", imu9_data.mag.x);
        
        // This data could be stored in a shared structure for other tasks to access
        // (See SensorDataAggregator.hpp for a complete example)

        // Read quaternion data
        Quaternion quat = bno55_sensor.read_quaternion();
        // Teleplot format for quaternion data (separate lines)
        printf(">quat_w:%.3f\n", quat.w);
        printf(">quat_x:%.3f\n", quat.x);
        printf(">quat_y:%.3f\n", quat.y);
        printf(">quat_z:%.3f\n", quat.z);

        // Read roll, pitch, yaw data
        RollPitchYaw rpy = bno55_sensor.read_euler();
        // Teleplot format for Euler angles (in degrees)
        printf(">roll:%.2f\n", rpy.roll);
        printf(">pitch:%.2f\n", rpy.pitch);
        printf(">yaw:%.2f\n", rpy.yaw);

        // Read Temperature data
        float temperature = bno55_sensor.read_temperature();
        // Teleplot format for temperature
        printf(">temperature:%.2f\n", temperature);
        
        // 3D visualization: Send complete shape definition every time
        // This ensures all properties are always available, even if Teleplot refreshes
        // printf("3D|imu_orientation:S:cube:W:3:H:1:D:2:C:blue:P:0:0:0:Q:%.3f:%.3f:%.3f:%.3f\n", 
        //        quat.x, quat.y, quat.z, quat.w);

        // printf("3D|mySimpleCube:S:cube:P:1:1:1:R:0:0:0:W:2:H:2:D:2:C:#2ecc71\n");
        
        // vTaskDelay(pdMS_TO_TICKS(100)); // 10 Hz update rate (well below 60 FPS limit)
    }

    vTaskDelete(NULL); // Delete the task when done
}


// UART-USB bridge task: forwards data between UART1 (GPS) and USB CDC
void uart_usb_bridge_task(void *pvParameters) {
    // Wait for system to stabilize before starting
    vTaskDelay(pdMS_TO_TICKS(3000));
    
    const size_t bufsize = 64;  // Reduced buffer size for safety
    uint8_t uart_buf[bufsize];
    
    printf("UART-USB Bridge Task Started on core %d\n", get_core_num());

    while (true) {
        // printf("UART-USB Bridge running on core %d\n", get_core_num());
        // Forward UART->USB (non-blocking with timeout)
        if (uart_is_readable_within_us(uart1, 1000)) {  // 1ms timeout
            size_t bytes_read = 0;
            
            // Read available bytes (up to buffer size)
            while (uart_is_readable(uart1) && bytes_read < bufsize - 1) {
                uart_buf[bytes_read] = uart_getc(uart1);
                bytes_read++;
                
                // Prevent infinite loop - yield after reading some data
                if (bytes_read >= 32) {
                    break;
                }
            }
            
            // Output to USB if we got data
            if (bytes_read > 0) {
                for (size_t i = 0; i < bytes_read; i++) {
                    putchar(uart_buf[i]);
                }
                fflush(stdout); // Ensure data is sent
            }
        }
        
        // Forward USB->UART (non-blocking)
        int c = getchar_timeout_us(0);  // Non-blocking read
        if (c != PICO_ERROR_TIMEOUT && c >= 0) {
            uint8_t b = (uint8_t)c;
            if (uart_is_writable(uart1)) {  // Check if UART can accept data
                uart_putc(uart1, b);
            }
        }
        
        // Yield to other tasks more frequently
        vTaskDelay(pdMS_TO_TICKS(2)); // 2ms delay instead of 1ms
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

// Standard FreeRTOS hook functions
void vApplicationMallocFailedHook( void )
{
    /* Called if a call to pvPortMalloc() fails because there is insufficient
    free memory available in the FreeRTOS heap. This is a critical error. */
    
    printf("CRITICAL ERROR: Malloc failed - out of heap memory!\n");
    printf("Free heap space: %zu bytes\n", xPortGetFreeHeapSize());
    
    // Turn on LED to indicate error (use regular GPIO LED)
    gpio_put(PICO_DEFAULT_LED_PIN, 1);
    
    // Force an assert to halt system
    configASSERT( ( volatile void * ) NULL );
}

void vApplicationStackOverflowHook( TaskHandle_t pxTask, char *pcTaskName )
{
    ( void ) pcTaskName;
    ( void ) pxTask;

    /* Run time stack overflow checking is performed if
    configCHECK_FOR_STACK_OVERFLOW is defined to 1 or 2.  This hook
    function is called if a stack overflow is detected. */
    
    printf("CRITICAL ERROR: Stack overflow detected in task: %s\n", pcTaskName);
    printf("Free heap space: %zu bytes\n", xPortGetFreeHeapSize());
    
    // Turn on LED to indicate error (use regular GPIO LED)
    gpio_put(PICO_DEFAULT_LED_PIN, 1);

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
// #if mainCREATE_SIMPLE_BLINKY_DEMO_ONLY == 0
//     {
//         /* The full demo includes a software timer demo/test that requires
//         prodding periodically from the tick interrupt. */
//         #if (mainENABLE_TIMER_DEMO == 1)
//         vTimerPeriodicISRTests();
//         #endif

//         /* Call the periodic queue overwrite from ISR demo. */
//         #if (mainENABLE_QUEUE_OVERWRITE == 1)
//         vQueueOverwritePeriodicISRDemo();
//         #endif

//         /* Call the periodic event group from ISR demo. */
//         #if (mainENABLE_EVENT_GROUP == 1)
//         vPeriodicEventGroupsProcessing();
//         #endif

//         /* Call the code that uses a mutex from an ISR. */
//         #if (mainENABLE_INTERRUPT_SEMAPHORE == 1)
//         vInterruptSemaphorePeriodicTest();
//         #endif

//         /* Call the code that 'gives' a task notification from an ISR. */
//         #if (mainENABLE_TASK_NOTIFY == 1)
//         xNotifyTaskFromISR();
//         #endif
//     }
// #endif
}
