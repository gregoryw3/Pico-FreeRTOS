#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "hardware/i2c.h"
#include "hardware/uart.h"
#include "hardware/gpio.h"

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

#include "sensor_data.h"
#include "i2c_devices.h"

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

// Queue handles
QueueHandle_t sensor_queue;
QueueHandle_t lora_queue;

// Mutex handles
SemaphoreHandle_t i2c0_mutex;
SemaphoreHandle_t i2c1_mutex;

// Task function prototypes
void ism330dhcx_task(void *pvParameters);
void lsm6dso32_task(void *pvParameters);
void bmp390_task(void *pvParameters);
void gps_task(void *pvParameters);
void adxl375_task(void *pvParameters);
void ism330dhcx_task2(void *pvParameters);
void aggregator_task(void *pvParameters);
void lora_task(void *pvParameters);

void vApplicationMallocFailedHook(void);
void vApplicationStackOverflowHook(TaskHandle_t pxTask, char *pcTaskName);
void vApplicationIdleHook(void);
void vApplicationTickHook(void);

// Function to initialize hardware
static void setup_hardware(void) {
    stdio_init_all();

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

    // Initialize I2C1
    i2c_init(i2c1, I2C_FREQUENCY);
    gpio_set_function(I2C1_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(I2C1_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(I2C1_SDA_PIN);
    gpio_pull_up(I2C1_SCL_PIN);

    // Initialize UART0
    uart_init(uart0, UART_BAUD_RATE);
    gpio_set_function(UART0_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(UART0_RX_PIN, GPIO_FUNC_UART);
}

void main(void) {
    // Initialize the hardware
    setup_hardware();
    
    printf("FreeRTOS SMP starting on Raspberry Pi Pico\n");
    
    // Create mutexes for I2C bus access
    i2c0_mutex = xSemaphoreCreateMutex();
    i2c1_mutex = xSemaphoreCreateMutex();
    
    // Create queues for communication
    sensor_queue = xQueueCreate(20, sizeof(SensorUpdate));
    lora_queue = xQueueCreate(1, sizeof(MiniData));
    
    // Create tasks for sensors
    xTaskCreate(ism330dhcx_task, "ISM330DHCX", configMINIMAL_STACK_SIZE * 3, NULL, 2, NULL);
    xTaskCreate(lsm6dso32_task, "LSM6DSO32", configMINIMAL_STACK_SIZE * 3, NULL, 2, NULL);
    xTaskCreate(bmp390_task, "BMP390", configMINIMAL_STACK_SIZE * 3, NULL, 2, NULL);
    xTaskCreate(gps_task, "GPS", configMINIMAL_STACK_SIZE * 3, NULL, 2, NULL);
    xTaskCreate(adxl375_task, "ADXL375", configMINIMAL_STACK_SIZE * 3, NULL, 2, NULL);
    xTaskCreate(ism330dhcx_task2, "ISM330DHCX2", configMINIMAL_STACK_SIZE * 3, NULL, 2, NULL);
    
    // Create aggregator task
    xTaskCreate(aggregator_task, "Aggregator", configMINIMAL_STACK_SIZE * 4, NULL, 3, NULL);
    
    // Create LoRa task
    xTaskCreate(lora_task, "LoRa", configMINIMAL_STACK_SIZE * 3, NULL, 3, NULL);
    
    // Start the FreeRTOS scheduler
    vTaskStartScheduler();
    
    // We should never get here
    while (true) {
        printf("Error: FreeRTOS scheduler failed to start\n");
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

// Implement some example tasks to show the pattern
void ism330dhcx_task(void *pvParameters) {
    ISM330DHCX data;
    SensorUpdate update;
    
    // Configure update type
    update.type = UPDATE_TYPE_ISM330DHCX;
    
    // Initialize the sensor
    xSemaphoreTake(i2c0_mutex, portMAX_DELAY);
    ism330dhcx_init(i2c0, 0x6A);
    xSemaphoreGive(i2c0_mutex);
    
    vTaskDelay(pdMS_TO_TICKS(100)); // Wait for sensor to initialize
    
    while (1) {
        // Take mutex before accessing I2C bus
        xSemaphoreTake(i2c0_mutex, portMAX_DELAY);
        
        // Read sensor data
        ism330dhcx_read(i2c0, 0x6A, &data);
        
        // Release mutex
        xSemaphoreGive(i2c0_mutex);
        
        // Copy data to update structure
        memcpy(&update.data.ism330dhcx, &data, sizeof(ISM330DHCX));
        
        // Send update to queue
        xQueueSend(sensor_queue, &update, portMAX_DELAY);
        
        // Wait for next reading (25Hz = 40ms)
        vTaskDelay(pdMS_TO_TICKS(40));
    }
}

void lsm6dso32_task(void *pvParameters) {
    LSM6DSO32 data;
    SensorUpdate update;
    
    // Configure update type
    update.type = UPDATE_TYPE_LSM6DSO32;
    
    // Initialize the sensor
    xSemaphoreTake(i2c0_mutex, portMAX_DELAY);
    lsm6dso32_init(i2c0, 0x6B);
    xSemaphoreGive(i2c0_mutex);
    
    vTaskDelay(pdMS_TO_TICKS(100)); // Wait for sensor to initialize
    
    while (1) {
        // Take mutex before accessing I2C bus
        xSemaphoreTake(i2c0_mutex, portMAX_DELAY);
        
        // Read sensor data
        lsm6dso32_read(i2c0, 0x6B, &data);
        
        // Release mutex
        xSemaphoreGive(i2c0_mutex);
        
        // Copy data to update structure
        memcpy(&update.data.lsm6dso32, &data, sizeof(LSM6DSO32));
        
        // Send update to queue
        xQueueSend(sensor_queue, &update, portMAX_DELAY);
        
        // Wait for next reading (25Hz = 40ms)
        vTaskDelay(pdMS_TO_TICKS(40));
    }
}

void bmp390_task(void *pvParameters) {
    BMP390 data;
    SensorUpdate update;
    
    // Configure update type
    update.type = UPDATE_TYPE_BMP390;
    
    // Initialize the sensor
    xSemaphoreTake(i2c0_mutex, portMAX_DELAY);
    bmp390_init(i2c0, 0x77);  // BMP390 address is typically 0x77
    xSemaphoreGive(i2c0_mutex);
    
    vTaskDelay(pdMS_TO_TICKS(100)); // Wait for sensor to initialize
    
    while (1) {
        // Take mutex before accessing I2C bus
        xSemaphoreTake(i2c0_mutex, portMAX_DELAY);
        
        // Read sensor data
        bmp390_read(i2c0, 0x77, &data);
        
        // Release mutex
        xSemaphoreGive(i2c0_mutex);
        
        // Copy data to update structure
        memcpy(&update.data.bmp390, &data, sizeof(BMP390));
        
        // Send update to queue
        xQueueSend(sensor_queue, &update, portMAX_DELAY);
        
        // Wait for next reading (25Hz = 40ms)
        vTaskDelay(pdMS_TO_TICKS(40));
    }
}

void gps_task(void *pvParameters) {
    GPS data;
    SensorUpdate update;
    
    // Configure update type
    update.type = UPDATE_TYPE_GPS;
    
    // Initialize the sensor
    xSemaphoreTake(i2c1_mutex, portMAX_DELAY);
    gps_init(i2c1, 0x42);  // GPS address is typically 0x42
    xSemaphoreGive(i2c1_mutex);
    
    vTaskDelay(pdMS_TO_TICKS(500)); // Wait longer for GPS to initialize
    
    while (1) {
        // Take mutex before accessing I2C bus
        xSemaphoreTake(i2c1_mutex, portMAX_DELAY);
        
        // Read sensor data
        gps_read(i2c1, 0x42, &data);
        
        // Release mutex
        xSemaphoreGive(i2c1_mutex);
        
        // Copy data to update structure
        memcpy(&update.data.gps, &data, sizeof(GPS));
        
        // Send update to queue
        xQueueSend(sensor_queue, &update, portMAX_DELAY);
        
        // GPS typically updates at 1Hz, but we'll poll more frequently
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

void adxl375_task(void *pvParameters) {
    ADXL375 data;
    SensorUpdate update;
    
    // Configure update type
    update.type = UPDATE_TYPE_ADXL375;
    
    // Initialize the sensor
    xSemaphoreTake(i2c1_mutex, portMAX_DELAY);
    adxl375_init(i2c1, 0x53);  // ADXL375 address is typically 0x53
    xSemaphoreGive(i2c1_mutex);
    
    vTaskDelay(pdMS_TO_TICKS(100)); // Wait for sensor to initialize
    
    while (1) {
        // Take mutex before accessing I2C bus
        xSemaphoreTake(i2c1_mutex, portMAX_DELAY);
        
        // Read sensor data
        adxl375_read(i2c1, 0x53, &data);
        
        // Release mutex
        xSemaphoreGive(i2c1_mutex);
        
        // Copy data to update structure
        memcpy(&update.data.adxl375, &data, sizeof(ADXL375));
        
        // Send update to queue
        xQueueSend(sensor_queue, &update, portMAX_DELAY);
        
        // Wait for next reading (25Hz = 40ms)
        vTaskDelay(pdMS_TO_TICKS(40));
    }
}

void ism330dhcx_task2(void *pvParameters) {
    ISM330DHCX data;
    SensorUpdate update;
    
    // Configure update type
    update.type = UPDATE_TYPE_ISM330DHCX2;
    
    // Initialize the sensor
    xSemaphoreTake(i2c1_mutex, portMAX_DELAY);
    ism330dhcx_init(i2c1, 0x6A);  // Second ISM330DHCX also at 0x6A but on i2c1
    xSemaphoreGive(i2c1_mutex);
    
    vTaskDelay(pdMS_TO_TICKS(100)); // Wait for sensor to initialize
    
    while (1) {
        // Take mutex before accessing I2C bus
        xSemaphoreTake(i2c1_mutex, portMAX_DELAY);
        
        // Read sensor data
        ism330dhcx_read(i2c1, 0x6A, &data);
        
        // Release mutex
        xSemaphoreGive(i2c1_mutex);
        
        // Copy data to update structure
        memcpy(&update.data.ism330dhcx2, &data, sizeof(ISM330DHCX));
        
        // Send update to queue
        xQueueSend(sensor_queue, &update, portMAX_DELAY);
        
        // Wait for next reading (25Hz = 40ms)
        vTaskDelay(pdMS_TO_TICKS(40));
    }
}

void aggregator_task(void *pvParameters) {
    SensorUpdate update;
    MiniData mini_data = {0};
    
    // Set device ID
    mini_data.device_id = 0; // Top is 0, Bottom is 1, Mobile is 2
    
    uint32_t msg_num = 0;
    
    while (1) {
        // Wait for sensor updates
        if (xQueueReceive(sensor_queue, &update, pdMS_TO_TICKS(10)) == pdTRUE) {
            // Process the sensor update based on its type
            switch (update.type) {
                case UPDATE_TYPE_ISM330DHCX:
                    mini_data.ism_axel_x = update.data.ism330dhcx.accel_x;
                    mini_data.ism_axel_y = update.data.ism330dhcx.accel_y;
                    mini_data.ism_axel_z = update.data.ism330dhcx.accel_z;
                    mini_data.ism_gyro_x = update.data.ism330dhcx.gyro_x;
                    mini_data.ism_gyro_y = update.data.ism330dhcx.gyro_y;
                    mini_data.ism_gyro_z = update.data.ism330dhcx.gyro_z;
                    break;
                    
                case UPDATE_TYPE_LSM6DSO32:
                    mini_data.lsm_axel_x = update.data.lsm6dso32.accel_x;
                    mini_data.lsm_axel_y = update.data.lsm6dso32.accel_y;
                    mini_data.lsm_axel_z = update.data.lsm6dso32.accel_z;
                    mini_data.lsm_gyro_x = update.data.lsm6dso32.gyro_x;
                    mini_data.lsm_gyro_y = update.data.lsm6dso32.gyro_y;
                    mini_data.lsm_gyro_z = update.data.lsm6dso32.gyro_z;
                    break;
                    
                case UPDATE_TYPE_BMP390:
                    mini_data.baro_alt = update.data.bmp390.altitude;
                    break;
                    
                case UPDATE_TYPE_GPS:
                    mini_data.lat = update.data.gps.latitude;
                    mini_data.lon = update.data.gps.longitude;
                    mini_data.alt = update.data.gps.altitude;
                    mini_data.num_sats = update.data.gps.num_sats;
                    mini_data.gps_fix = update.data.gps.fix_type;
                    memcpy(&mini_data.gps_time, &update.data.gps.utc_time, sizeof(UTC));
                    break;
                    
                case UPDATE_TYPE_ADXL375:
                    mini_data.adxl_axel_x = update.data.adxl375.accel_x;
                    mini_data.adxl_axel_y = update.data.adxl375.accel_y;
                    mini_data.adxl_axel_z = update.data.adxl375.accel_z;
                    break;
                    
                case UPDATE_TYPE_ISM330DHCX2:
                    mini_data.ism_axel_x2 = update.data.ism330dhcx2.accel_x;
                    mini_data.ism_axel_y2 = update.data.ism330dhcx2.accel_y;
                    mini_data.ism_axel_z2 = update.data.ism330dhcx2.accel_z;
                    mini_data.ism_gyro_x2 = update.data.ism330dhcx2.gyro_x;
                    mini_data.ism_gyro_y2 = update.data.ism330dhcx2.gyro_y;
                    mini_data.ism_gyro_z2 = update.data.ism330dhcx2.gyro_z;
                    break;
            }
        }
        
        // Update message number and timestamp
        mini_data.msg_num = msg_num++;
        mini_data.time_since_boot = xTaskGetTickCount() * portTICK_PERIOD_MS;
        
        // Send mini data to LoRa task
        xQueueSend(lora_queue, &mini_data, 0); // Don't block if queue is full
        
        // Run at 100Hz (10ms interval)
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void lora_task(void *pvParameters) {
    MiniData data;
    
    while (1) {
        if (xQueueReceive(lora_queue, &data, portMAX_DELAY) == pdTRUE) {
            // This is where we would serialize the data using COBS
            // Since we don't have postcard/COBS in C, we'll just print the data
            printf("Sending data: lat=%f, lon=%f, alt=%f, sats=%d\n", 
                data.lat, data.lon, data.alt, data.num_sats);
                
            // Here we would serialize the data and send it via UART
            // For simplicity, we'll just send some raw data
            uint8_t buffer[16];
            memcpy(buffer, &data.msg_num, sizeof(data.msg_num));
            uart_write_blocking(uart0, buffer, sizeof(data.msg_num));
        }
        
        // Short delay to prevent busy-waiting
        vTaskDelay(pdMS_TO_TICKS(5));
    }
}

// Standard FreeRTOS hook functions already implemented in the existing main.c
/*-----------------------------------------------------------*/

void vApplicationMallocFailedHook( void )
{
    /* Called if a call to pvPortMalloc() fails because there is insufficient
    free memory available in the FreeRTOS heap.  pvPortMalloc() is called
    internally by FreeRTOS API functions that create tasks, queues, software
    timers, and semaphores.  The size of the FreeRTOS heap is set by the
    configTOTAL_HEAP_SIZE configuration constant in FreeRTOSConfig.h. */

    /* Force an assert. */
    configASSERT( ( volatile void * ) NULL );
}
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
