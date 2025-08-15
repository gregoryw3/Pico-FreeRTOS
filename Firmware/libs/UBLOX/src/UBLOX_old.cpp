// #include "UBLOX.h"

// namespace UBLOX {

// // UBX Parser Implementation
// UBXParser::UBXParser() : buffer_pos(0), state(SYNC1), payload_counter(0), calculated_ck_a(0), calculated_ck_b(0) {
//     memset(buffer, 0, sizeof(buffer));
// }

// void UBXParser::calculate_checksum(const uint8_t* data, size_t length, uint8_t& ck_a, uint8_t& ck_b) {
//     ck_a = 0;
//     ck_b = 0;
//     for (size_t i = 2; i < length - 2; i++) {  // Skip sync chars and checksum
//         ck_a += data[i];
//         ck_b += ck_a;
//     }
// }

// bool UBXParser::verify_checksum(const UBXMessage& message) {
//     uint8_t ck_a, ck_b;
//     // Create temporary buffer with full message for checksum calculation
//     uint8_t temp_buffer[8 + message.length];
//     temp_buffer[0] = 0xB5; // SYNC1
//     temp_buffer[1] = 0x62; // SYNC2
//     temp_buffer[2] = message.msg_class;
//     temp_buffer[3] = message.msg_id;
//     temp_buffer[4] = message.length & 0xFF;
//     temp_buffer[5] = (message.length >> 8) & 0xFF;
//     memcpy(&temp_buffer[6], message.payload, message.length);
//     temp_buffer[6 + message.length] = message.ck_a;
//     temp_buffer[7 + message.length] = message.ck_b;
    
//     calculate_checksum(temp_buffer, 8 + message.length, ck_a, ck_b);
//     return (ck_a == message.ck_a && ck_b == message.ck_b);
// }

// void UBXParser::reset() {
//     state = SYNC1;
//     buffer_pos = 0;
//     payload_counter = 0;
//     calculated_ck_a = 0;
//     calculated_ck_b = 0;
// }

// // NMEA Parser Implementation
// NMEAParser::NMEAParser() : buffer_pos(0), in_message(false) {
//     memset(buffer, 0, sizeof(buffer));
// }

// void NMEAParser::reset() {
//     buffer_pos = 0;
//     in_message = false;
// }

// // Core UBLOX Implementation
// UBLOX::UBLOX(I2C_Interface* i2c, uint8_t address)
//     : i2c_interface(i2c), uart_interface(nullptr), spi_interface(nullptr),
//       i2c_address(address), interface_type(Interface::I2C), task_handle(nullptr), 
//       message_queue(nullptr), raw_data_queue(nullptr), mutex(nullptr), initialized(false) {
// }

// UBLOX::UBLOX(UART_Interface* uart)
//     : i2c_interface(nullptr), uart_interface(uart), spi_interface(nullptr),
//       i2c_address(0), interface_type(Interface::UART), task_handle(nullptr), 
//       message_queue(nullptr), raw_data_queue(nullptr), mutex(nullptr), initialized(false) {
// }

// UBLOX::UBLOX(SPI_Interface* spi)
//     : i2c_interface(nullptr), uart_interface(nullptr), spi_interface(spi),
//       i2c_address(0), interface_type(Interface::SPI), task_handle(nullptr), 
//       message_queue(nullptr), raw_data_queue(nullptr), mutex(nullptr), initialized(false) {
// }

// UBLOX::~UBLOX() {
//     shutdown();
// }

// void UBLOX::delay_ms(uint32_t ms) const {
//     vTaskDelay(pdMS_TO_TICKS(ms));
// }

// bool UBLOX::initialize(const Configuration& config) {
//     if (initialized) {
//         return false;
//     }
    
//     this->config = config;
    
//     // Initialize hardware interface based on type
//     bool interface_ok = false;
//     switch (interface_type) {
//         case Interface::I2C:
//             interface_ok = i2c_interface && i2c_interface->init(config.baudrate > 0 ? config.baudrate : 400000);
//             break;
//         case Interface::UART:
//             interface_ok = uart_interface && uart_interface->init(config.baudrate);
//             break;
//         case Interface::SPI:
//             interface_ok = spi_interface && spi_interface->init(config.baudrate);
//             break;
//     }
    
//     if (!interface_ok) {
//         printf("UBLOX: Failed to initialize interface\n");
//         return false;
//     }
    
//     delay_ms(250);  // Allow module to boot
    
//     if (interface_type == Interface::I2C && !is_connected()) {
//         printf("UBLOX: Module not connected!\n");
//         return false;
//     }
    
//     // Create FreeRTOS objects
//     mutex = xSemaphoreCreateMutex();
//     if (!mutex) {
//         printf("UBLOX: Failed to create mutex\n");
//         return false;
//     }
    
//     // Create message queues
//     message_queue = xQueueCreate(config.queue_size, sizeof(ParsedMessage));
//     if (!message_queue) {
//         printf("UBLOX: Failed to create message queue\n");
//         vSemaphoreDelete(mutex);
//         return false;
//     }
    
//     raw_data_queue = xQueueCreate(config.queue_size, sizeof(GPSData));
//     if (!raw_data_queue) {
//         printf("UBLOX: Failed to create raw data queue\n");
//         vQueueDelete(message_queue);
//         vSemaphoreDelete(mutex);
//         return false;
//     }
    
//     // Create task
//     if (xTaskCreate(task_function, "UBLOX_Task", config.stack_size, this, config.task_priority, &task_handle) != pdPASS) {
//         printf("UBLOX: Failed to create task\n");
//         vQueueDelete(message_queue);
//         vQueueDelete(raw_data_queue);
//         vSemaphoreDelete(mutex);
//         return false;
//     }
    
//     initialized = true;
    
//     // Apply auto-configuration if enabled
//     if (config.auto_configure) {
//         delay_ms(1000);  // Allow task to start
//         configure_for_airborne();
//     }
    
//     printf("UBLOX: Initialized successfully\n");
//     return true;
// }

// void UBLOX::shutdown() {
//     if (!initialized) {
//         return;
//     }
    
//     // Delete task
//     if (task_handle) {
//         vTaskDelete(task_handle);
//         task_handle = nullptr;
//     }
    
//     // Delete FreeRTOS objects
//     if (message_queue) {
//         vQueueDelete(message_queue);
//         message_queue = nullptr;
//     }
    
//     if (raw_data_queue) {
//         vQueueDelete(raw_data_queue);
//         raw_data_queue = nullptr;
//     }
    
//     if (mutex) {
//         vSemaphoreDelete(mutex);
//         mutex = nullptr;
//     }
    
//     initialized = false;
//     printf("UBLOX: Module shutdown complete\n");
// }

// bool UBLOX::is_connected() const {
//     uint8_t test_data = 0;
//     return i2c_interface->read(i2c_address, &test_data, 1);
// }

// uint16_t UBLOX::bytes_available() {
//     uint8_t buffer[2];
//     bool success = i2c_interface->write_read(i2c_address, &Register::NUMBER_BYTES_READY_MSB, 1, buffer, 2);
//     if (!success) {
//         return 0;
//     }
//     return (static_cast<uint16_t>(buffer[0]) << 8) | buffer[1];
// }

// bool UBLOX::get_data(GPSData& data, TickType_t timeout_ms) {
//     if (!initialized || !raw_data_queue) {
//         return false;
//     }
    
//     return xQueueReceive(raw_data_queue, &data, pdMS_TO_TICKS(timeout_ms)) == pdTRUE;
// }

// void UBLOX::task_function(void* parameters) {
//     UBLOX* instance = static_cast<UBLOX*>(parameters);
//     instance->run_task();
// }

// void UBLOX::run_task() {
//     printf("UBLOX: Task started\n");
    
//     while (true) {
//         size_t bytes_read = 0;
//         if (read_interface_data(rx_buffer, sizeof(rx_buffer), bytes_read) && bytes_read > 0) {
//             process_incoming_data(rx_buffer, bytes_read);
            
//             // Also create legacy GPS data for backward compatibility
//             GPSData gps_data;
//             gps_data.length = (bytes_read > sizeof(gps_data.data)) ? sizeof(gps_data.data) : bytes_read;
//             memcpy(gps_data.data, rx_buffer, gps_data.length);
//             gps_data.valid = true;
//             gps_data.timestamp = xTaskGetTickCount();
            
//             // Try to put data in raw queue (non-blocking)
//             if (xQueueSend(raw_data_queue, &gps_data, 0) != pdTRUE) {
//                 // Queue full, remove oldest item and try again
//                 GPSData old_data;
//                 xQueueReceive(raw_data_queue, &old_data, 0);
//                 xQueueSend(raw_data_queue, &gps_data, 0);
//             }
//         }
        
//         // Task delay based on measurement rate
//         vTaskDelay(pdMS_TO_TICKS(config.measurement_rate_ms));
//     }
// }

// bool UBLOX::set_airborne_4g() {
//     if (!mutex) return false;
    
//     if (xSemaphoreTake(mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
//         const uint8_t ubx_cfg_valset_ram[17] = {
//             0xB5, 0x62, 0x06, 0x8A, 0x09, 0x00, 0x01, 0x01,
//             0x00, 0x00, 0x21, 0x00, 0x11, 0x20, 0x08, 0xF5, 0x5A
//         };
        
//         bool result = i2c_interface->write(i2c_address, ubx_cfg_valset_ram, sizeof(ubx_cfg_valset_ram));
//         delay_ms(100);
        
//         xSemaphoreGive(mutex);
//         return result;
//     }
//     return false;
// }

// bool UBLOX::enable_i2c_ubx_nav_pvt() {
//     if (!mutex) return false;
    
//     if (xSemaphoreTake(mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
//         const uint8_t ubx_cfg_valset_ram[17] = {
//             0xB5, 0x62, 0x06, 0x8A, 0x09, 0x00, 0x01, 0x01,
//             0x00, 0x00, 0x06, 0x00, 0x91, 0x20, 0x01, 0x53, 0x4C
//         };
        
//         bool result = i2c_interface->write(i2c_address, ubx_cfg_valset_ram, sizeof(ubx_cfg_valset_ram));
//         delay_ms(100);
        
//         xSemaphoreGive(mutex);
//         return result;
//     }
//     return false;
// }

// bool UBLOX::set_i2c_timeout_extended() {
//     if (!mutex) return false;
    
//     if (xSemaphoreTake(mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
//         const uint8_t ubx_cfg_valset_ram[17] = {
//             0xB5, 0x62, 0x06, 0x8A, 0x09, 0x00, 0x01, 0x01,
//             0x00, 0x00, 0x02, 0x00, 0x51, 0x10, 0x01, 0xFF, 0x58
//         };
        
//         bool result = i2c_interface->write(i2c_address, ubx_cfg_valset_ram, sizeof(ubx_cfg_valset_ram));
//         delay_ms(100);
        
//         xSemaphoreGive(mutex);
//         return result;
//     }
//     return false;
// }

// bool UBLOX::disable_nmea_i2c() {
//     if (!mutex) return false;
    
//     if (xSemaphoreTake(mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
//         const uint8_t ubx_cfg_valset_ram[17] = {
//             0xB5, 0x62, 0x06, 0x8A, 0x09, 0x00, 0x01, 0x01,
//             0x00, 0x00, 0x02, 0x00, 0x72, 0x10, 0x00, 0x1F, 0xBA
//         };
        
//         bool result = i2c_interface->write(i2c_address, ubx_cfg_valset_ram, sizeof(ubx_cfg_valset_ram));
//         delay_ms(100);
        
//         xSemaphoreGive(mutex);
//         return result;
//     }
//     return false;
// }

// bool UBLOX::enable_ubx_time_utc() {
//     if (!mutex) return false;
    
//     if (xSemaphoreTake(mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
//         const uint8_t ubx_cfg_valset_ram[48] = {
//             0xB5, 0x62, 0x06, 0x8A, 0x28, 0x00, 0x01, 0x01,
//             0x00, 0x00, 0x21, 0x00, 0x11, 0x20, 0x08, 0x06,
//             0x00, 0x93, 0x10, 0x01, 0x01, 0x00, 0x21, 0x30,
//             0xC8, 0x00, 0x02, 0x00, 0x51, 0x10, 0x01, 0x06,
//             0x00, 0x91, 0x20, 0x01, 0x02, 0x00, 0x72, 0x10,
//             0x00, 0x5B, 0x00, 0x91, 0x20, 0x01, 0x85, 0x14
//         };
        
//         bool result = i2c_interface->write(i2c_address, ubx_cfg_valset_ram, sizeof(ubx_cfg_valset_ram));
//         delay_ms(100);
        
//         xSemaphoreGive(mutex);
//         return result;
//     }
//     return false;
// }

// bool UBLOX::enable_ubx_nav_pvt() {
//     if (!mutex) return false;
    
//     if (xSemaphoreTake(mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
//         const uint8_t ubx_cfg_valset_ram[43] = {
//             0xB5, 0x62, 0x06, 0x8A, 0x23, 0x00,
//             0x01, 0x01, 0x00, 0x00, 0x21, 0x00,
//             0x11, 0x20, 0x08, 0x06, 0x00, 0x93,
//             0x10, 0x01, 0x01, 0x00, 0x21, 0x30,
//             0xC8, 0x00, 0x02, 0x00, 0x51, 0x10,
//             0x01, 0x06, 0x00, 0x91, 0x20, 0x01,
//             0x02, 0x00, 0x72, 0x10, 0x00, 0x73, 0x48
//         };
        
//         bool result = i2c_interface->write(i2c_address, ubx_cfg_valset_ram, sizeof(ubx_cfg_valset_ram));
//         delay_ms(100);
        
//         xSemaphoreGive(mutex);
//         return result;
//     }
//     return false;
// }

// bool UBLOX::enable_i2c_ubx_nav_sat() {
//     if (!mutex) return false;
    
//     if (xSemaphoreTake(mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
//         const uint8_t ubx_cfg_valset_ram[17] = {
//             0xB5, 0x62, 0x06, 0x8A, 0x09, 0x00, 0x01, 0x01, 
//             0x00, 0x00, 0x15, 0x00, 0x91, 0x20, 0x01, 0x62, 0x97
//         };
        
//         bool result = i2c_interface->write(i2c_address, ubx_cfg_valset_ram, sizeof(ubx_cfg_valset_ram));
//         delay_ms(100);
        
//         xSemaphoreGive(mutex);
//         return result;
//     }
//     return false;
// }

// bool UBLOX::poll_ubx_nav_pvt() {
//     if (!mutex) return false;
    
//     if (xSemaphoreTake(mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
//         const uint8_t ubx_poll_nav_pvt[8] = {
//             0xB5, 0x62, 0x01, 0x07, 0x00, 0x00, 0x08, 0x19
//         };
        
//         bool result = i2c_interface->write(i2c_address, ubx_poll_nav_pvt, sizeof(ubx_poll_nav_pvt));
//         delay_ms(100);
        
//         xSemaphoreGive(mutex);
//         return result;
//     }
//     return false;
// }

// void UBLOX::get_task_stats(TaskStatus_t& task_status) const {
//     if (task_handle) {
//         vTaskGetInfo(task_handle, &task_status, pdTRUE, eInvalid);
//     }
// }

// void UBLOX::print_status() const {
//     printf("UBLOX Status:\n");
//     printf("  I2C Address: 0x%02X\n", i2c_address);
//     printf("  Initialized: %s\n", initialized ? "Yes" : "No");
//     printf("  Connected: %s\n", is_connected() ? "Yes" : "No");
//     printf("  Task Handle: %p\n", task_handle);
    
//     if (task_handle) {
//         TaskStatus_t task_status;
//         get_task_stats(task_status);
//         printf("  Task State: %d\n", task_status.eCurrentState);
//         printf("  Stack High Water Mark: %lu\n", task_status.usStackHighWaterMark);
//     }
// }

// #ifdef PICO_PLATFORM
// // Pico-specific I2C Implementation
// Pico_I2C::Pico_I2C(i2c_inst_t* i2c, uint8_t sda, uint8_t scl)
//     : i2c_instance(i2c), sda_pin(sda), scl_pin(scl) {
// }

// bool Pico_I2C::init(uint32_t baudrate) {
//     i2c_init(i2c_instance, baudrate);
//     gpio_set_function(sda_pin, GPIO_FUNC_I2C);
//     gpio_set_function(scl_pin, GPIO_FUNC_I2C);
//     gpio_pull_up(sda_pin);
//     gpio_pull_up(scl_pin);
//     return true;
// }

// bool Pico_I2C::write(uint8_t address, const uint8_t* data, size_t length) {
//     int result = i2c_write_blocking(i2c_instance, address, data, length, false);
//     return result >= 0;
// }

// bool Pico_I2C::read(uint8_t address, uint8_t* data, size_t length) {
//     int result = i2c_read_blocking(i2c_instance, address, data, length, false);
//     return result >= 0;
// }

// bool Pico_I2C::write_read(uint8_t address, const uint8_t* write_data, size_t write_len, 
//                          uint8_t* read_data, size_t read_len) {
//     int result = i2c_write_blocking(i2c_instance, address, write_data, write_len, true);
//     if (result < 0) return false;
    
//     result = i2c_read_blocking(i2c_instance, address, read_data, read_len, false);
//     return result >= 0;
// }

// // Pico-specific UART Implementation
// Pico_UART::Pico_UART(uart_inst_t* uart, uint8_t tx_pin, uint8_t rx_pin)
//     : uart_instance(uart), tx_pin(tx_pin), rx_pin(rx_pin) {
// }

// bool Pico_UART::init(uint32_t baudrate) {
//     uart_init(uart_instance, baudrate);
//     gpio_set_function(tx_pin, GPIO_FUNC_UART);
//     gpio_set_function(rx_pin, GPIO_FUNC_UART);
//     return true;
// }

// bool Pico_UART::write(const uint8_t* data, size_t length) {
//     for (size_t i = 0; i < length; i++) {
//         uart_putc_raw(uart_instance, data[i]);
//     }
//     return true;
// }

// bool Pico_UART::read(uint8_t* data, size_t length, TickType_t timeout_ms) {
//     TickType_t start_time = xTaskGetTickCount();
//     size_t count = 0;
    
//     while (count < length && (xTaskGetTickCount() - start_time) < pdMS_TO_TICKS(timeout_ms)) {
//         if (uart_is_readable(uart_instance)) {
//             data[count++] = uart_getc(uart_instance);
//         } else {
//             vTaskDelay(pdMS_TO_TICKS(1));  // Small delay to prevent busy waiting
//         }
//     }
    
//     return count == length;
// }

// bool Pico_UART::is_readable() {
//     return uart_is_readable(uart_instance);
// }
// #endif

// // UART UBLOX implementation would follow similar pattern...
// // For brevity, I'll implement just the key parts

// UBLOX_UART::UBLOX_UART(UART_Interface* uart)
//     : uart_interface(uart), task_handle(nullptr), data_queue(nullptr), 
//       mutex(nullptr), initialized(false) {
// }

// UBLOX_UART::~UBLOX_UART() {
//     shutdown();
// }

// void UBLOX_UART::delay_ms(uint32_t ms) const {
//     vTaskDelay(pdMS_TO_TICKS(ms));
// }

// bool UBLOX_UART::initialize(const Configuration& config) {
//     if (initialized) {
//         return false;
//     }
    
//     this->config = config;
    
//     if (!uart_interface || !uart_interface->init(9600)) {
//         printf("UBLOX_UART: Failed to initialize UART interface\n");
//         return false;
//     }
    
//     // Create FreeRTOS objects (similar to I2C version)
//     mutex = xSemaphoreCreateMutex();
//     if (!mutex) {
//         printf("UBLOX_UART: Failed to create mutex\n");
//         return false;
//     }
    
//     data_queue = xQueueCreate(10, sizeof(GPSData));
//     if (!data_queue) {
//         printf("UBLOX_UART: Failed to create data queue\n");
//         vSemaphoreDelete(mutex);
//         return false;
//     }
    
//     initialized = true;
//     printf("UBLOX_UART: Module initialized successfully\n");
//     return true;
// }

// void UBLOX_UART::shutdown() {
//     if (!initialized) {
//         return;
//     }
    
//     if (task_handle) {
//         vTaskDelete(task_handle);
//         task_handle = nullptr;
//     }
    
//     if (data_queue) {
//         vQueueDelete(data_queue);
//         data_queue = nullptr;
//     }
    
//     if (mutex) {
//         vSemaphoreDelete(mutex);
//         mutex = nullptr;
//     }
    
//     initialized = false;
//     printf("UBLOX_UART: Module shutdown complete\n");
// }

// bool UBLOX_UART::write(const uint8_t* data, size_t length) {
//     if (!mutex) return false;
    
//     if (xSemaphoreTake(mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
//         bool result = uart_interface->write(data, length);
//         xSemaphoreGive(mutex);
//         return result;
//     }
//     return false;
// }

// bool UBLOX_UART::get_data(GPSData& data, TickType_t timeout_ms) {
//     if (!initialized || !data_queue) {
//         return false;
//     }
    
//     return xQueueReceive(data_queue, &data, pdMS_TO_TICKS(timeout_ms)) == pdTRUE;
// }

// void UBLOX_UART::get_task_stats(TaskStatus_t& task_status) const {
//     if (task_handle) {
//         vTaskGetInfo(task_handle, &task_status, pdTRUE, eInvalid);
//     }
// }

// } // namespace UBLOX