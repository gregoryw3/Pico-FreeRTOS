#pragma once

#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <stddef.h>

// FreeRTOS includes
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

// Hardware abstraction - these can be swapped for different platforms
#ifdef PICO_PLATFORM
    #include "hardware/i2c.h"
    #include "hardware/uart.h"
    #include "pico/stdlib.h"
#elif defined(STM32_PLATFORM)
    // STM32 HAL includes would go here
    // #include "stm32f4xx_hal.h"
#endif

namespace UBLOX {

/// UBLOX GPS module errors
enum class Error {
    I2C_ERROR,
    UART_ERROR,
    NO_DATA,
    TIMEOUT,
    INVALID_RESPONSE,
    FREERTOS_ERROR
};

/// UBLOX I2C addresses
enum class Address : uint8_t {
    DEFAULT = 0x42,
    CUSTOM = 0x42  // Can be customized
};

/// Configuration for UBLOX module
struct Configuration {
    bool output_nmea = false;
    bool output_ubx = true;
    bool output_rtcm = false;
    uint32_t measurement_rate_ms = 200;  // 5Hz default
    uint32_t task_priority = 2;          // FreeRTOS task priority
    uint32_t stack_size = 2048;          // FreeRTOS task stack size
};

/// Register addresses for UBLOX I2C communication
namespace Register {
    constexpr uint8_t NUMBER_BYTES_READY_MSB = 0xFD;
    constexpr uint8_t NUMBER_BYTES_READY_LSB = 0xFE;
    constexpr uint8_t DATA_STREAM = 0xFF;
}

/// Simple data container for GPS data
struct GPSData {
    uint8_t data[128];
    size_t length;
    bool valid;
    TickType_t timestamp;  // FreeRTOS timestamp
};

/// Hardware abstraction interface for I2C
class I2C_Interface {
public:
    virtual ~I2C_Interface() = default;
    virtual bool init(uint32_t baudrate) = 0;
    virtual bool write(uint8_t address, const uint8_t* data, size_t length) = 0;
    virtual bool read(uint8_t address, uint8_t* data, size_t length) = 0;
    virtual bool write_read(uint8_t address, const uint8_t* write_data, size_t write_len, 
                           uint8_t* read_data, size_t read_len) = 0;
};

/// Hardware abstraction interface for UART
class UART_Interface {
public:
    virtual ~UART_Interface() = default;
    virtual bool init(uint32_t baudrate) = 0;
    virtual bool write(const uint8_t* data, size_t length) = 0;
    virtual bool read(uint8_t* data, size_t length, TickType_t timeout_ms) = 0;
    virtual bool is_readable() = 0;
};

/// UBLOX GPS module driver using FreeRTOS abstractions
class UBLOX {
private:
    I2C_Interface* i2c_interface;
    uint8_t i2c_address;
    
    // FreeRTOS objects
    TaskHandle_t task_handle;
    QueueHandle_t data_queue;
    SemaphoreHandle_t mutex;
    
    Configuration config;
    bool initialized;
    
    /// FreeRTOS task function
    static void task_function(void* parameters);
    
    /// Internal task implementation
    void run_task();
    
    /// Calculate UBX checksum
    void calculate_checksum(const uint8_t* data, size_t length, uint8_t& ck_a, uint8_t& ck_b) const;
    
    /// Platform-independent delay using FreeRTOS
    void delay_ms(uint32_t ms) const;

public:
    /// Constructor
    UBLOX(I2C_Interface* i2c, uint8_t address = static_cast<uint8_t>(Address::DEFAULT));
    
    /// Destructor
    ~UBLOX();
    
    /// Initialize the UBLOX module and start FreeRTOS task
    bool initialize(const Configuration& config = Configuration{});
    
    /// Stop the module and cleanup FreeRTOS resources
    void shutdown();
    
    /// Check if module is connected
    bool is_connected() const;
    
    /// Get available data from the module (non-blocking, uses FreeRTOS queue)
    bool get_data(GPSData& data, TickType_t timeout_ms = 0);
    
    /// Check how many bytes are ready to read
    uint16_t bytes_available();
    
    /// Configuration methods
    bool set_airborne_4g();
    bool enable_i2c_ubx_nav_pvt();
    bool enable_ubx_nav_pvt();
    bool enable_ubx_time_utc();
    bool disable_nmea_i2c();
    bool set_i2c_timeout_extended();
    bool enable_i2c_ubx_nav_sat();
    bool poll_ubx_nav_pvt();
    
    /// Apply full configuration for airborne use
    bool configure_for_airborne();
    
    /// Get task statistics (FreeRTOS specific)
    void get_task_stats(TaskStatus_t& task_status) const;
    
    /// Debug methods
    void print_status() const;
};

/// UBLOX GPS module driver for UART communication using FreeRTOS
class UBLOX_UART {
private:
    UART_Interface* uart_interface;
    
    // FreeRTOS objects
    TaskHandle_t task_handle;
    QueueHandle_t data_queue;
    SemaphoreHandle_t mutex;
    
    Configuration config;
    bool initialized;
    
    /// FreeRTOS task function
    static void task_function(void* parameters);
    
    /// Internal task implementation
    void run_task();
    
    /// Platform-independent delay using FreeRTOS
    void delay_ms(uint32_t ms) const;

public:
    /// Constructor
    UBLOX_UART(UART_Interface* uart);
    
    /// Destructor
    ~UBLOX_UART();
    
    /// Initialize the UART interface and start FreeRTOS task
    bool initialize(const Configuration& config = Configuration{});
    
    /// Stop the module and cleanup FreeRTOS resources
    void shutdown();
    
    /// Write data to UART (thread-safe)
    bool write(const uint8_t* data, size_t length);
    
    /// Get data from UART (non-blocking, uses FreeRTOS queue)
    bool get_data(GPSData& data, TickType_t timeout_ms = 0);
    
    /// Configuration methods for UART
    bool set_airborne_4g();
    bool enable_ubx_uart();
    bool disable_nmea_uart();
    bool enable_uart_ubx_nav_pvt();
    bool enable_ubx_nav_pvt();
    bool enable_ubx_time_utc();
    
    /// Get task statistics (FreeRTOS specific)
    void get_task_stats(TaskStatus_t& task_status) const;
};

#ifdef PICO_PLATFORM
/// Pico-specific I2C implementation
class Pico_I2C : public I2C_Interface {
private:
    i2c_inst_t* i2c_instance;
    uint8_t sda_pin;
    uint8_t scl_pin;
    
public:
    Pico_I2C(i2c_inst_t* i2c, uint8_t sda = 4, uint8_t scl = 5);
    bool init(uint32_t baudrate) override;
    bool write(uint8_t address, const uint8_t* data, size_t length) override;
    bool read(uint8_t address, uint8_t* data, size_t length) override;
    bool write_read(uint8_t address, const uint8_t* write_data, size_t write_len, 
                   uint8_t* read_data, size_t read_len) override;
};

/// Pico-specific UART implementation
class Pico_UART : public UART_Interface {
private:
    uart_inst_t* uart_instance;
    uint8_t tx_pin;
    uint8_t rx_pin;
    
public:
    Pico_UART(uart_inst_t* uart, uint8_t tx_pin = 12, uint8_t rx_pin = 13);
    bool init(uint32_t baudrate) override;
    bool write(const uint8_t* data, size_t length) override;
    bool read(uint8_t* data, size_t length, TickType_t timeout_ms) override;
    bool is_readable() override;
};
#endif

} // namespace UBLOX
