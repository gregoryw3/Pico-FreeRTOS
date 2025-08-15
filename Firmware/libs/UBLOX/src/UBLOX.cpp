#include "UBLOX.h"

namespace UBLOX {

// UBX Parser Implementation
UBXParser::UBXParser() : buffer_pos(0), state(SYNC1), payload_counter(0), calculated_ck_a(0), calculated_ck_b(0) {
    memset(buffer, 0, sizeof(buffer));
}

void UBXParser::calculate_checksum(const uint8_t* data, size_t length, uint8_t& ck_a, uint8_t& ck_b) {
    ck_a = 0;
    ck_b = 0;
    for (size_t i = 2; i < length - 2; i++) {  // Skip sync chars and checksum
        ck_a += data[i];
        ck_b += ck_a;
    }
}

bool UBXParser::verify_checksum(const UBXMessage& message) {
    uint8_t ck_a, ck_b;
    // Create temporary buffer with full message for checksum calculation
    uint8_t temp_buffer[8 + message.length];
    temp_buffer[0] = 0xB5; // SYNC1
    temp_buffer[1] = 0x62; // SYNC2
    temp_buffer[2] = message.msg_class;
    temp_buffer[3] = message.msg_id;
    temp_buffer[4] = message.length & 0xFF;
    temp_buffer[5] = (message.length >> 8) & 0xFF;
    memcpy(&temp_buffer[6], message.payload, message.length);
    temp_buffer[6 + message.length] = message.ck_a;
    temp_buffer[7 + message.length] = message.ck_b;
    
    calculate_checksum(temp_buffer, 8 + message.length, ck_a, ck_b);
    return (ck_a == message.ck_a && ck_b == message.ck_b);
}

void UBXParser::reset() {
    state = SYNC1;
    buffer_pos = 0;
    payload_counter = 0;
    calculated_ck_a = 0;
    calculated_ck_b = 0;
}

bool UBXParser::parse_byte(uint8_t byte, UBXMessage& message) {
    // Simplified parser implementation - stub for now
    return false;
}

bool UBXParser::generate_message(uint8_t msg_class, uint8_t msg_id, 
                                const uint8_t* payload, uint16_t payload_length,
                                uint8_t* output_buffer, size_t& output_length) {
    // Simplified generator implementation - stub for now
    return false;
}

// NMEA Parser Implementation
NMEAParser::NMEAParser() : buffer_pos(0), in_message(false) {
    memset(buffer, 0, sizeof(buffer));
}

void NMEAParser::reset() {
    buffer_pos = 0;
    in_message = false;
}

bool NMEAParser::parse_byte(uint8_t byte, NMEAMessage& message) {
    // Simplified parser implementation - stub for now
    return false;
}

// Core UBLOX Implementation
UBLOX::UBLOX(I2C_Interface* i2c, uint8_t address)
    : i2c_interface(i2c), uart_interface(nullptr), spi_interface(nullptr),
      i2c_address(address), interface_type(Interface::I2C), task_handle(nullptr), 
      message_queue(nullptr), raw_data_queue(nullptr), mutex(nullptr), initialized(false) {
}

UBLOX::UBLOX(UART_Interface* uart)
    : i2c_interface(nullptr), uart_interface(uart), spi_interface(nullptr),
      i2c_address(0), interface_type(Interface::UART), task_handle(nullptr), 
      message_queue(nullptr), raw_data_queue(nullptr), mutex(nullptr), initialized(false) {
}

UBLOX::UBLOX(SPI_Interface* spi)
    : i2c_interface(nullptr), uart_interface(nullptr), spi_interface(spi),
      i2c_address(0), interface_type(Interface::SPI), task_handle(nullptr), 
      message_queue(nullptr), raw_data_queue(nullptr), mutex(nullptr), initialized(false) {
}

UBLOX::~UBLOX() {
    shutdown();
}

void UBLOX::delay_ms(uint32_t ms) const {
    vTaskDelay(pdMS_TO_TICKS(ms));
}

bool UBLOX::initialize(const Configuration& config) {
    if (initialized) {
        return false;
    }
    
    this->config = config;
    
    // Initialize hardware interface based on type
    bool interface_ok = false;
    switch (interface_type) {
        case Interface::I2C:
            interface_ok = i2c_interface && i2c_interface->init(config.baudrate > 0 ? config.baudrate : 400000);
            break;
        case Interface::UART:
            interface_ok = uart_interface && uart_interface->init(config.baudrate);
            break;
        case Interface::SPI:
            interface_ok = spi_interface && spi_interface->init(config.baudrate);
            break;
    }
    
    if (!interface_ok) {
        printf("UBLOX: Failed to initialize interface\n");
        return false;
    }
    
    delay_ms(250);  // Allow module to boot
    
    if (interface_type == Interface::I2C && !is_connected()) {
        printf("UBLOX: Module not connected!\n");
        return false;
    }
    
    // Create FreeRTOS objects
    mutex = xSemaphoreCreateMutex();
    if (!mutex) {
        printf("UBLOX: Failed to create mutex\n");
        return false;
    }
    
    // Create message queues
    message_queue = xQueueCreate(config.queue_size, sizeof(ParsedMessage));
    if (!message_queue) {
        printf("UBLOX: Failed to create message queue\n");
        vSemaphoreDelete(mutex);
        return false;
    }
    
    raw_data_queue = xQueueCreate(config.queue_size, sizeof(GPSData));
    if (!raw_data_queue) {
        printf("UBLOX: Failed to create raw data queue\n");
        vQueueDelete(message_queue);
        vSemaphoreDelete(mutex);
        return false;
    }
    
    // Create task
    if (xTaskCreate(task_function, "UBLOX_Task", config.stack_size, this, config.task_priority, &task_handle) != pdPASS) {
        printf("UBLOX: Failed to create task\n");
        vQueueDelete(message_queue);
        vQueueDelete(raw_data_queue);
        vSemaphoreDelete(mutex);
        return false;
    }
    
    initialized = true;
    
    // Apply auto-configuration if enabled
    if (config.auto_configure) {
        delay_ms(1000);  // Allow task to start
        configure_for_airborne();
    }
    
    printf("UBLOX: Initialized successfully\n");
    return true;
}

void UBLOX::shutdown() {
    if (!initialized) {
        return;
    }
    
    initialized = false;
    
    // Delete task first
    if (task_handle) {
        vTaskDelete(task_handle);
        task_handle = nullptr;
    }
    
    // Clean up queues
    if (message_queue) {
        vQueueDelete(message_queue);
        message_queue = nullptr;
    }
    
    if (raw_data_queue) {
        vQueueDelete(raw_data_queue);
        raw_data_queue = nullptr;
    }
    
    // Clean up mutex
    if (mutex) {
        vSemaphoreDelete(mutex);
        mutex = nullptr;
    }
    
    printf("UBLOX: Shutdown complete\n");
}

bool UBLOX::is_connected() const {
    if (interface_type != Interface::I2C || !i2c_interface) {
        return true; // Assume connected for UART/SPI
    }
    uint8_t test_data = 0;
    return i2c_interface->read(i2c_address, &test_data, 1);
}

uint16_t UBLOX::bytes_available() {
    if (interface_type != Interface::I2C || !i2c_interface) {
        return 0;
    }
    uint8_t buffer[2];
    bool success = i2c_interface->write_read(i2c_address, &Register::NUMBER_BYTES_READY_MSB, 1, buffer, 2);
    if (!success) {
        return 0;
    }
    return (static_cast<uint16_t>(buffer[0]) << 8) | buffer[1];
}

bool UBLOX::get_data(GPSData& data, TickType_t timeout_ms) {
    if (!initialized || !raw_data_queue) {
        return false;
    }
    
    return xQueueReceive(raw_data_queue, &data, pdMS_TO_TICKS(timeout_ms)) == pdTRUE;
}

bool UBLOX::get_parsed_message(ParsedMessage& message, TickType_t timeout_ms) {
    if (!initialized || !message_queue) {
        return false;
    }
    
    return xQueueReceive(message_queue, &message, pdMS_TO_TICKS(timeout_ms)) == pdTRUE;
}

bool UBLOX::get_position(GPSPosition& position, TickType_t timeout_ms) {
    // Stub implementation - would parse NAV-PVT messages
    return false;
}

void UBLOX::task_function(void* parameters) {
    UBLOX* instance = static_cast<UBLOX*>(parameters);
    instance->run_task();
}

void UBLOX::run_task() {
    printf("UBLOX: Task started\n");
    
    while (true) {
        size_t bytes_read = 0;
        if (read_interface_data(rx_buffer, sizeof(rx_buffer), bytes_read) && bytes_read > 0) {
            process_incoming_data(rx_buffer, bytes_read);
            
            // Also create legacy GPS data for backward compatibility
            GPSData gps_data;
            gps_data.length = (bytes_read > sizeof(gps_data.data)) ? sizeof(gps_data.data) : bytes_read;
            memcpy(gps_data.data, rx_buffer, gps_data.length);
            gps_data.valid = true;
            gps_data.timestamp = xTaskGetTickCount();
            
            // Try to put data in raw queue (non-blocking)
            if (xQueueSend(raw_data_queue, &gps_data, 0) != pdTRUE) {
                // Queue full, remove oldest item and try again
                GPSData old_data;
                xQueueReceive(raw_data_queue, &old_data, 0);
                xQueueSend(raw_data_queue, &gps_data, 0);
            }
        }
        
        // Task delay based on measurement rate
        vTaskDelay(pdMS_TO_TICKS(config.measurement_rate_ms));
    }
}

bool UBLOX::read_interface_data(uint8_t* buffer, size_t max_length, size_t& bytes_read) {
    bytes_read = 0;
    
    switch (interface_type) {
        case Interface::I2C: {
            if (!i2c_interface) return false;
            uint16_t available = bytes_available();
            if (available > 0) {
                size_t to_read = (available > max_length) ? max_length : available;
                bool success = i2c_interface->read(i2c_address, buffer, to_read);
                if (success) {
                    bytes_read = to_read;
                }
                return success;
            }
            return true; // No data available but no error
        }
        case Interface::UART: {
            if (!uart_interface) return false;
            if (uart_interface->is_readable()) {
                return uart_interface->read(buffer, max_length, 10); // 10ms timeout
            }
            return true;
        }
        case Interface::SPI: {
            if (!spi_interface) return false;
            // SPI implementation would need specific protocol handling
            return false;
        }
    }
    return false;
}

bool UBLOX::write_interface_data(const uint8_t* data, size_t length) {
    switch (interface_type) {
        case Interface::I2C:
            return i2c_interface && i2c_interface->write(i2c_address, data, length);
        case Interface::UART:
            return uart_interface && uart_interface->write(data, length);
        case Interface::SPI:
            return spi_interface && spi_interface->write(data, length);
    }
    return false;
}

void UBLOX::process_incoming_data(const uint8_t* data, size_t length) {
    // Process data through parsers and create ParsedMessage objects
    // This is a stub implementation
    for (size_t i = 0; i < length; i++) {
        UBXMessage ubx_msg;
        NMEAMessage nmea_msg;
        
        if (ubx_parser.parse_byte(data[i], ubx_msg)) {
            ParsedMessage parsed;
            parsed.type = ParsedMessage::UBX_MESSAGE;
            parsed.ubx = ubx_msg;
            parsed.timestamp = xTaskGetTickCount();
            parsed.valid = true;
            
            xQueueSend(message_queue, &parsed, 0);
        }
        
        if (nmea_parser.parse_byte(data[i], nmea_msg)) {
            ParsedMessage parsed;
            parsed.type = ParsedMessage::NMEA_MESSAGE;
            parsed.nmea = nmea_msg;
            parsed.timestamp = xTaskGetTickCount();
            parsed.valid = true;
            
            xQueueSend(message_queue, &parsed, 0);
        }
    }
}

// Configuration methods (enhanced API)
bool UBLOX::set_measurement_rate(uint16_t rate_ms) {
    // Stub implementation
    return false;
}

bool UBLOX::set_dynamic_model(uint8_t model) {
    // Stub implementation
    return false;
}

bool UBLOX::enable_message(uint8_t msg_class, uint8_t msg_id, uint8_t rate) {
    // Stub implementation
    return false;
}

bool UBLOX::disable_message(uint8_t msg_class, uint8_t msg_id) {
    // Stub implementation
    return false;
}

bool UBLOX::configure_for_airborne() {
    // Stub implementation
    return set_airborne_4g();
}

bool UBLOX::set_airborne_4g() {
    // Simplified implementation - would need actual UBX commands
    return true;
}

bool UBLOX::enable_nav_pvt(uint8_t rate) {
    return enable_message(static_cast<uint8_t>(UBXClass::NAV), UBXMessages::NAV_PVT, rate);
}

bool UBLOX::enable_nav_sat(uint8_t rate) {
    return enable_message(static_cast<uint8_t>(UBXClass::NAV), UBXMessages::NAV_SAT, rate);
}

bool UBLOX::disable_nmea_messages() {
    // Stub implementation
    return true;
}

bool UBLOX::poll_nav_pvt() {
    // Stub implementation
    return false;
}

bool UBLOX::poll_nav_status() {
    // Stub implementation
    return false;
}

void UBLOX::get_task_stats(TaskStatus_t& task_status) const {
    if (task_handle) {
        vTaskGetInfo(task_handle, &task_status, pdTRUE, eInvalid);
    }
}

void UBLOX::get_parser_stats(ParserStats& stats) const {
    // Stub implementation
    memset(&stats, 0, sizeof(stats));
}

void UBLOX::print_status() const {
    printf("UBLOX Status:\n");
    printf("  Initialized: %s\n", initialized ? "Yes" : "No");
    printf("  Interface: %s\n", 
           (interface_type == Interface::I2C) ? "I2C" : 
           (interface_type == Interface::UART) ? "UART" : "SPI");
    printf("  Measurement rate: %lu ms\n", config.measurement_rate_ms);
}

void UBLOX::print_message_info(const ParsedMessage& message) const {
    printf("Message type: %s, timestamp: %lu\n",
           (message.type == ParsedMessage::UBX_MESSAGE) ? "UBX" : 
           (message.type == ParsedMessage::NMEA_MESSAGE) ? "NMEA" : "RAW",
           message.timestamp);
}

#ifdef PICO_PLATFORM
// Pico-specific I2C implementation
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
    return i2c_write_blocking(i2c_instance, address, data, length, false) == length;
}

bool Pico_I2C::read(uint8_t address, uint8_t* data, size_t length) {
    return i2c_read_blocking(i2c_instance, address, data, length, false) == length;
}

bool Pico_I2C::write_read(uint8_t address, const uint8_t* write_data, size_t write_len, 
                         uint8_t* read_data, size_t read_len) {
    return (i2c_write_blocking(i2c_instance, address, write_data, write_len, true) == write_len) &&
           (i2c_read_blocking(i2c_instance, address, read_data, read_len, false) == read_len);
}

// Pico-specific UART implementation
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
    uart_write_blocking(uart_instance, data, length);
    return true;
}

bool Pico_UART::read(uint8_t* data, size_t length, TickType_t timeout_ms) {
    // Simplified implementation
    size_t bytes_read = 0;
    uint32_t start_time = to_ms_since_boot(get_absolute_time());
    
    while (bytes_read < length) {
        if (uart_is_readable(uart_instance)) {
            data[bytes_read++] = uart_getc(uart_instance);
        } else {
            if ((to_ms_since_boot(get_absolute_time()) - start_time) > timeout_ms) {
                break;
            }
            sleep_ms(1);
        }
    }
    
    return bytes_read > 0;
}

bool Pico_UART::is_readable() {
    return uart_is_readable(uart_instance);
}

void Pico_UART::flush() {
    // Pico SDK doesn't have a direct flush function for UART
}

// Pico-specific SPI implementation
Pico_SPI::Pico_SPI(spi_inst_t* spi, uint8_t miso, uint8_t mosi, uint8_t sck, uint8_t cs)
    : spi_instance(spi), miso_pin(miso), mosi_pin(mosi), sck_pin(sck), cs_pin(cs) {
}

bool Pico_SPI::init(uint32_t baudrate) {
    spi_init(spi_instance, baudrate);
    gpio_set_function(miso_pin, GPIO_FUNC_SPI);
    gpio_set_function(mosi_pin, GPIO_FUNC_SPI);
    gpio_set_function(sck_pin, GPIO_FUNC_SPI);
    gpio_init(cs_pin);
    gpio_set_dir(cs_pin, GPIO_OUT);
    gpio_put(cs_pin, 1); // Deselect
    return true;
}

bool Pico_SPI::write(const uint8_t* data, size_t length) {
    return spi_write_blocking(spi_instance, data, length) == length;
}

bool Pico_SPI::read(uint8_t* data, size_t length) {
    return spi_read_blocking(spi_instance, 0, data, length) == length;
}

bool Pico_SPI::write_read(const uint8_t* write_data, size_t write_len, 
                         uint8_t* read_data, size_t read_len) {
    return spi_write_read_blocking(spi_instance, write_data, read_data, 
                                  (write_len > read_len) ? write_len : read_len) >= 0;
}

void Pico_SPI::select() {
    gpio_put(cs_pin, 0);
}

void Pico_SPI::deselect() {
    gpio_put(cs_pin, 1);
}
#endif

} // namespace UBLOX
