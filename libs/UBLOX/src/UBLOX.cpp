#include "UBLOX.h"

namespace UBLOX {

// Core UBLOX Implementation
UBLOX::UBLOX(I2C_Interface* i2c, uint8_t address)
    : i2c_interface(i2c), i2c_address(address), task_handle(nullptr), 
      data_queue(nullptr), mutex(nullptr), initialized(false) {
}

UBLOX::~UBLOX() {
    shutdown();
}

void UBLOX::delay_ms(uint32_t ms) const {
    vTaskDelay(pdMS_TO_TICKS(ms));
}

void UBLOX::calculate_checksum(const uint8_t* data, size_t length, uint8_t& ck_a, uint8_t& ck_b) const {
    ck_a = 0;
    ck_b = 0;
    for (size_t i = 2; i < length - 2; i++) {  // Skip sync chars and checksum
        ck_a += data[i];
        ck_b += ck_a;
    }
}

bool UBLOX::initialize(const Configuration& config) {
    if (initialized) {
        return false;
    }
    
    this->config = config;
    
    // Initialize hardware interface
    if (!i2c_interface || !i2c_interface->init(400000)) {
        printf("UBLOX: Failed to initialize I2C interface\n");
        return false;
    }
    
    delay_ms(250);  // Allow module to boot
    
    if (!is_connected()) {
        printf("UBLOX: Module not connected!\n");
        return false;
    }
    
    // Create FreeRTOS objects
    mutex = xSemaphoreCreateMutex();
    if (!mutex) {
        printf("UBLOX: Failed to create mutex\n");
        return false;
    }
    
    data_queue = xQueueCreate(10, sizeof(GPSData));
    if (!data_queue) {
        printf("UBLOX: Failed to create data queue\n");
        vSemaphoreDelete(mutex);
        return false;
    }
    
    // Create FreeRTOS task
    BaseType_t result = xTaskCreate(
        task_function,
        "UBLOX_Task",
        config.stack_size / sizeof(StackType_t),
        this,
        config.task_priority,
        &task_handle
    );
    
    if (result != pdPASS) {
        printf("UBLOX: Failed to create task\n");
        vQueueDelete(data_queue);
        vSemaphoreDelete(mutex);
        return false;
    }
    
    initialized = true;
    printf("UBLOX: Module initialized successfully\n");
    
    // Apply configuration
    if (config.output_ubx) {
        configure_for_airborne();
    }
    
    return true;
}

void UBLOX::shutdown() {
    if (!initialized) {
        return;
    }
    
    // Delete task
    if (task_handle) {
        vTaskDelete(task_handle);
        task_handle = nullptr;
    }
    
    // Delete FreeRTOS objects
    if (data_queue) {
        vQueueDelete(data_queue);
        data_queue = nullptr;
    }
    
    if (mutex) {
        vSemaphoreDelete(mutex);
        mutex = nullptr;
    }
    
    initialized = false;
    printf("UBLOX: Module shutdown complete\n");
}

bool UBLOX::is_connected() const {
    uint8_t test_data = 0;
    return i2c_interface->read(i2c_address, &test_data, 1);
}

uint16_t UBLOX::bytes_available() {
    uint8_t buffer[2];
    bool success = i2c_interface->write_read(i2c_address, &Register::NUMBER_BYTES_READY_MSB, 1, buffer, 2);
    if (!success) {
        return 0;
    }
    return (static_cast<uint16_t>(buffer[0]) << 8) | buffer[1];
}

bool UBLOX::get_data(GPSData& data, TickType_t timeout_ms) {
    if (!initialized || !data_queue) {
        return false;
    }
    
    return xQueueReceive(data_queue, &data, pdMS_TO_TICKS(timeout_ms)) == pdTRUE;
}

void UBLOX::task_function(void* parameters) {
    UBLOX* instance = static_cast<UBLOX*>(parameters);
    instance->run_task();
}

void UBLOX::run_task() {
    printf("UBLOX: Task started\n");
    
    while (true) {
        uint16_t available = bytes_available();
        if (available > 0) {
            // Read up to 128 bytes
            uint16_t to_read = (available > 128) ? 128 : available;
            
            GPSData gps_data;
            gps_data.length = to_read;
            gps_data.valid = i2c_interface->read(i2c_address, gps_data.data, to_read);
            gps_data.timestamp = xTaskGetTickCount();
            
            if (gps_data.valid) {
                // Try to put data in queue (non-blocking)
                if (xQueueSend(data_queue, &gps_data, 0) != pdTRUE) {
                    // Queue full, remove oldest item and try again
                    GPSData old_data;
                    xQueueReceive(data_queue, &old_data, 0);
                    xQueueSend(data_queue, &gps_data, 0);
                }
            }
        }
        
        // Task delay based on measurement rate
        vTaskDelay(pdMS_TO_TICKS(config.measurement_rate_ms));
    }
}

bool UBLOX::set_airborne_4g() {
    if (!mutex) return false;
    
    if (xSemaphoreTake(mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        const uint8_t ubx_cfg_valset_ram[17] = {
            0xB5, 0x62, 0x06, 0x8A, 0x09, 0x00, 0x01, 0x01,
            0x00, 0x00, 0x21, 0x00, 0x11, 0x20, 0x08, 0xF5, 0x5A
        };
        
        bool result = i2c_interface->write(i2c_address, ubx_cfg_valset_ram, sizeof(ubx_cfg_valset_ram));
        delay_ms(100);
        
        xSemaphoreGive(mutex);
        return result;
    }
    return false;
}

bool UBLOX::enable_i2c_ubx_nav_pvt() {
    if (!mutex) return false;
    
    if (xSemaphoreTake(mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        const uint8_t ubx_cfg_valset_ram[17] = {
            0xB5, 0x62, 0x06, 0x8A, 0x09, 0x00, 0x01, 0x01,
            0x00, 0x00, 0x06, 0x00, 0x91, 0x20, 0x01, 0x53, 0x4C
        };
        
        bool result = i2c_interface->write(i2c_address, ubx_cfg_valset_ram, sizeof(ubx_cfg_valset_ram));
        delay_ms(100);
        
        xSemaphoreGive(mutex);
        return result;
    }
    return false;
}

bool UBLOX::set_i2c_timeout_extended() {
    if (!mutex) return false;
    
    if (xSemaphoreTake(mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        const uint8_t ubx_cfg_valset_ram[17] = {
            0xB5, 0x62, 0x06, 0x8A, 0x09, 0x00, 0x01, 0x01,
            0x00, 0x00, 0x02, 0x00, 0x51, 0x10, 0x01, 0xFF, 0x58
        };
        
        bool result = i2c_interface->write(i2c_address, ubx_cfg_valset_ram, sizeof(ubx_cfg_valset_ram));
        delay_ms(100);
        
        xSemaphoreGive(mutex);
        return result;
    }
    return false;
}

bool UBLOX::disable_nmea_i2c() {
    if (!mutex) return false;
    
    if (xSemaphoreTake(mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        const uint8_t ubx_cfg_valset_ram[17] = {
            0xB5, 0x62, 0x06, 0x8A, 0x09, 0x00, 0x01, 0x01,
            0x00, 0x00, 0x02, 0x00, 0x72, 0x10, 0x00, 0x1F, 0xBA
        };
        
        bool result = i2c_interface->write(i2c_address, ubx_cfg_valset_ram, sizeof(ubx_cfg_valset_ram));
        delay_ms(100);
        
        xSemaphoreGive(mutex);
        return result;
    }
    return false;
}

bool UBLOX::enable_ubx_time_utc() {
    if (!mutex) return false;
    
    if (xSemaphoreTake(mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        const uint8_t ubx_cfg_valset_ram[48] = {
            0xB5, 0x62, 0x06, 0x8A, 0x28, 0x00, 0x01, 0x01,
            0x00, 0x00, 0x21, 0x00, 0x11, 0x20, 0x08, 0x06,
            0x00, 0x93, 0x10, 0x01, 0x01, 0x00, 0x21, 0x30,
            0xC8, 0x00, 0x02, 0x00, 0x51, 0x10, 0x01, 0x06,
            0x00, 0x91, 0x20, 0x01, 0x02, 0x00, 0x72, 0x10,
            0x00, 0x5B, 0x00, 0x91, 0x20, 0x01, 0x85, 0x14
        };
        
        bool result = i2c_interface->write(i2c_address, ubx_cfg_valset_ram, sizeof(ubx_cfg_valset_ram));
        delay_ms(100);
        
        xSemaphoreGive(mutex);
        return result;
    }
    return false;
}

bool UBLOX::enable_ubx_nav_pvt() {
    if (!mutex) return false;
    
    if (xSemaphoreTake(mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        const uint8_t ubx_cfg_valset_ram[43] = {
            0xB5, 0x62, 0x06, 0x8A, 0x23, 0x00,
            0x01, 0x01, 0x00, 0x00, 0x21, 0x00,
            0x11, 0x20, 0x08, 0x06, 0x00, 0x93,
            0x10, 0x01, 0x01, 0x00, 0x21, 0x30,
            0xC8, 0x00, 0x02, 0x00, 0x51, 0x10,
            0x01, 0x06, 0x00, 0x91, 0x20, 0x01,
            0x02, 0x00, 0x72, 0x10, 0x00, 0x73, 0x48
        };
        
        bool result = i2c_interface->write(i2c_address, ubx_cfg_valset_ram, sizeof(ubx_cfg_valset_ram));
        delay_ms(100);
        
        xSemaphoreGive(mutex);
        return result;
    }
    return false;
}

bool UBLOX::enable_i2c_ubx_nav_sat() {
    if (!mutex) return false;
    
    if (xSemaphoreTake(mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        const uint8_t ubx_cfg_valset_ram[17] = {
            0xB5, 0x62, 0x06, 0x8A, 0x09, 0x00, 0x01, 0x01, 
            0x00, 0x00, 0x15, 0x00, 0x91, 0x20, 0x01, 0x62, 0x97
        };
        
        bool result = i2c_interface->write(i2c_address, ubx_cfg_valset_ram, sizeof(ubx_cfg_valset_ram));
        delay_ms(100);
        
        xSemaphoreGive(mutex);
        return result;
    }
    return false;
}

bool UBLOX::poll_ubx_nav_pvt() {
    if (!mutex) return false;
    
    if (xSemaphoreTake(mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        const uint8_t ubx_poll_nav_pvt[8] = {
            0xB5, 0x62, 0x01, 0x07, 0x00, 0x00, 0x08, 0x19
        };
        
        bool result = i2c_interface->write(i2c_address, ubx_poll_nav_pvt, sizeof(ubx_poll_nav_pvt));
        delay_ms(100);
        
        xSemaphoreGive(mutex);
        return result;
    }
    return false;
}

bool UBLOX::configure_for_airborne() {
    printf("UBLOX: Configuring for airborne use...\n");
    
    bool success = true;
    success &= set_airborne_4g();
    success &= set_i2c_timeout_extended();
    success &= disable_nmea_i2c();
    success &= enable_i2c_ubx_nav_pvt();
    
    if (success) {
        printf("UBLOX: Airborne configuration complete!\n");
    } else {
        printf("UBLOX: Configuration failed!\n");
    }
    
    return success;
}

void UBLOX::get_task_stats(TaskStatus_t& task_status) const {
    if (task_handle) {
        vTaskGetInfo(task_handle, &task_status, pdTRUE, eInvalid);
    }
}

void UBLOX::print_status() const {
    printf("UBLOX Status:\n");
    printf("  I2C Address: 0x%02X\n", i2c_address);
    printf("  Initialized: %s\n", initialized ? "Yes" : "No");
    printf("  Connected: %s\n", is_connected() ? "Yes" : "No");
    printf("  Task Handle: %p\n", task_handle);
    
    if (task_handle) {
        TaskStatus_t task_status;
        get_task_stats(task_status);
        printf("  Task State: %d\n", task_status.eCurrentState);
        printf("  Stack High Water Mark: %lu\n", task_status.usStackHighWaterMark);
    }
}

#ifdef PICO_PLATFORM
// Pico-specific I2C Implementation
Pico_I2C::Pico_I2C(i2c_inst_t* i2c, uint8_t sda, uint8_t scl)
    : i2c_instance(i2c), sda_pin(sda), scl_pin(scl) {
}

bool Pico_I2C::init(uint32_t baudrate) {
    i2c_init(i2c_instance, baudrate);
    gpio_set_function(sda_pin, GPIO_FUNC_I2C);
    gpio_set_function(scl_pin, GPIO_FUNC_I2C);
    gpio_pull_up(sda_pin);
    gpio_pull_up(scl_pin);
    return true;
}

bool Pico_I2C::write(uint8_t address, const uint8_t* data, size_t length) {
    int result = i2c_write_blocking(i2c_instance, address, data, length, false);
    return result >= 0;
}

bool Pico_I2C::read(uint8_t address, uint8_t* data, size_t length) {
    int result = i2c_read_blocking(i2c_instance, address, data, length, false);
    return result >= 0;
}

bool Pico_I2C::write_read(uint8_t address, const uint8_t* write_data, size_t write_len, 
                         uint8_t* read_data, size_t read_len) {
    int result = i2c_write_blocking(i2c_instance, address, write_data, write_len, true);
    if (result < 0) return false;
    
    result = i2c_read_blocking(i2c_instance, address, read_data, read_len, false);
    return result >= 0;
}

// Pico-specific UART Implementation
Pico_UART::Pico_UART(uart_inst_t* uart, uint8_t tx_pin, uint8_t rx_pin)
    : uart_instance(uart), tx_pin(tx_pin), rx_pin(rx_pin) {
}

bool Pico_UART::init(uint32_t baudrate) {
    uart_init(uart_instance, baudrate);
    gpio_set_function(tx_pin, GPIO_FUNC_UART);
    gpio_set_function(rx_pin, GPIO_FUNC_UART);
    return true;
}

bool Pico_UART::write(const uint8_t* data, size_t length) {
    for (size_t i = 0; i < length; i++) {
        uart_putc_raw(uart_instance, data[i]);
    }
    return true;
}

bool Pico_UART::read(uint8_t* data, size_t length, TickType_t timeout_ms) {
    TickType_t start_time = xTaskGetTickCount();
    size_t count = 0;
    
    while (count < length && (xTaskGetTickCount() - start_time) < pdMS_TO_TICKS(timeout_ms)) {
        if (uart_is_readable(uart_instance)) {
            data[count++] = uart_getc(uart_instance);
        } else {
            vTaskDelay(pdMS_TO_TICKS(1));  // Small delay to prevent busy waiting
        }
    }
    
    return count == length;
}

bool Pico_UART::is_readable() {
    return uart_is_readable(uart_instance);
}
#endif

// UART UBLOX implementation would follow similar pattern...
// For brevity, I'll implement just the key parts

UBLOX_UART::UBLOX_UART(UART_Interface* uart)
    : uart_interface(uart), task_handle(nullptr), data_queue(nullptr), 
      mutex(nullptr), initialized(false) {
}

UBLOX_UART::~UBLOX_UART() {
    shutdown();
}

void UBLOX_UART::delay_ms(uint32_t ms) const {
    vTaskDelay(pdMS_TO_TICKS(ms));
}

bool UBLOX_UART::initialize(const Configuration& config) {
    if (initialized) {
        return false;
    }
    
    this->config = config;
    
    if (!uart_interface || !uart_interface->init(9600)) {
        printf("UBLOX_UART: Failed to initialize UART interface\n");
        return false;
    }
    
    // Create FreeRTOS objects (similar to I2C version)
    mutex = xSemaphoreCreateMutex();
    if (!mutex) {
        printf("UBLOX_UART: Failed to create mutex\n");
        return false;
    }
    
    data_queue = xQueueCreate(10, sizeof(GPSData));
    if (!data_queue) {
        printf("UBLOX_UART: Failed to create data queue\n");
        vSemaphoreDelete(mutex);
        return false;
    }
    
    initialized = true;
    printf("UBLOX_UART: Module initialized successfully\n");
    return true;
}

void UBLOX_UART::shutdown() {
    if (!initialized) {
        return;
    }
    
    if (task_handle) {
        vTaskDelete(task_handle);
        task_handle = nullptr;
    }
    
    if (data_queue) {
        vQueueDelete(data_queue);
        data_queue = nullptr;
    }
    
    if (mutex) {
        vSemaphoreDelete(mutex);
        mutex = nullptr;
    }
    
    initialized = false;
    printf("UBLOX_UART: Module shutdown complete\n");
}

bool UBLOX_UART::write(const uint8_t* data, size_t length) {
    if (!mutex) return false;
    
    if (xSemaphoreTake(mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        bool result = uart_interface->write(data, length);
        xSemaphoreGive(mutex);
        return result;
    }
    return false;
}

bool UBLOX_UART::get_data(GPSData& data, TickType_t timeout_ms) {
    if (!initialized || !data_queue) {
        return false;
    }
    
    return xQueueReceive(data_queue, &data, pdMS_TO_TICKS(timeout_ms)) == pdTRUE;
}

void UBLOX_UART::get_task_stats(TaskStatus_t& task_status) const {
    if (task_handle) {
        vTaskGetInfo(task_handle, &task_status, pdTRUE, eInvalid);
    }
}

} // namespace UBLOX