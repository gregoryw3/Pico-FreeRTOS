#pragma once

#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>

// FreeRTOS includes
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

// Hardware abstraction - these can be swapped for different platforms
#ifdef PICO_PLATFORM
    #include "hardware/i2c.h"
    #include "hardware/uart.h"
    #include "hardware/spi.h"
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
    SPI_ERROR,
    NO_DATA,
    TIMEOUT,
    INVALID_RESPONSE,
    FREERTOS_ERROR,
    CHECKSUM_ERROR,
    PARSE_ERROR,
    BUFFER_OVERFLOW
};

/// Communication interface types
enum class Interface {
    I2C,
    UART,
    SPI
};

/// UBX Message classes
enum class UBXClass : uint8_t {
    NAV = 0x01,     // Navigation Results Messages
    RXM = 0x02,     // Receiver Manager Messages
    INF = 0x04,     // Information Messages
    ACK = 0x05,     // Ack/Nak Messages
    CFG = 0x06,     // Configuration Input Messages
    UPD = 0x09,     // Firmware Update Messages
    MON = 0x0A,     // Monitoring Messages
    AID = 0x0B,     // AssistNow Aiding Messages
    TIM = 0x0D,     // Timing Messages
    ESF = 0x10,     // External Sensor Fusion Messages
    MGA = 0x13,     // Multiple GNSS Assistance Messages
    LOG = 0x21,     // Logging Messages
    SEC = 0x27,     // Security Feature Messages
    HNR = 0x28      // High Rate Navigation Results Messages
};

/// UBX Message IDs for common messages
namespace UBXMessages {
    // NAV messages
    constexpr uint8_t NAV_PVT = 0x07;
    constexpr uint8_t NAV_SAT = 0x35;
    constexpr uint8_t NAV_STATUS = 0x03;
    constexpr uint8_t NAV_POSLLH = 0x02;
    constexpr uint8_t NAV_VELNED = 0x12;
    constexpr uint8_t NAV_TIMEUTC = 0x21;
    
    // CFG messages
    constexpr uint8_t CFG_PRT = 0x00;
    constexpr uint8_t CFG_MSG = 0x01;
    constexpr uint8_t CFG_RATE = 0x08;
    constexpr uint8_t CFG_CFG = 0x09;
    constexpr uint8_t CFG_NAV5 = 0x24;
    constexpr uint8_t CFG_NMEA = 0x17;
    
    // ACK messages
    constexpr uint8_t ACK_ACK = 0x01;
    constexpr uint8_t ACK_NAK = 0x00;
}

/// UBX Message structure
struct UBXMessage {
    uint8_t msg_class;
    uint8_t msg_id;
    uint16_t length;
    uint8_t payload[512];  // Maximum payload size
    uint8_t ck_a;
    uint8_t ck_b;
    
    UBXMessage() : msg_class(0), msg_id(0), length(0), ck_a(0), ck_b(0) {
        memset(payload, 0, sizeof(payload));
    }
};

/// NMEA Message types
enum class NMEAType {
    UNKNOWN,
    GGA,    // Global Positioning System Fix Data
    GLL,    // Geographic Position - Latitude/Longitude
    GSA,    // GPS DOP and active satellites
    GSV,    // GPS Satellites in view
    RMC,    // Recommended Minimum Navigation Information
    VTG,    // Track Made Good and Ground Speed
    ZDA     // Date & Time
};

/// NMEA Message structure
struct NMEAMessage {
    NMEAType type;
    char raw_message[256];
    char fields[20][32];  // Up to 20 fields, 32 chars each
    uint8_t field_count;
    bool valid_checksum;
    
    NMEAMessage() : type(NMEAType::UNKNOWN), field_count(0), valid_checksum(false) {
        memset(raw_message, 0, sizeof(raw_message));
        memset(fields, 0, sizeof(fields));
    }
};

/// Parsed GPS Position data from UBX NAV-PVT
struct GPSPosition {
    uint32_t iTOW;          // GPS time of week (ms)
    uint16_t year;
    uint8_t month;
    uint8_t day;
    uint8_t hour;
    uint8_t min;
    uint8_t sec;
    uint8_t valid;          // Validity flags
    uint32_t tAcc;          // Time accuracy estimate (ns)
    int32_t nano;           // Fraction of second (-1e9..1e9 ns)
    uint8_t fixType;        // GNSS fix type
    uint8_t flags;          // Fix status flags
    uint8_t flags2;         // Additional flags
    uint8_t numSV;          // Number of satellites used
    int32_t lon;            // Longitude (deg * 1e-7)
    int32_t lat;            // Latitude (deg * 1e-7)
    int32_t height;         // Height above ellipsoid (mm)
    int32_t hMSL;           // Height above mean sea level (mm)
    uint32_t hAcc;          // Horizontal accuracy estimate (mm)
    uint32_t vAcc;          // Vertical accuracy estimate (mm)
    int32_t velN;           // NED north velocity (mm/s)
    int32_t velE;           // NED east velocity (mm/s)
    int32_t velD;           // NED down velocity (mm/s)
    int32_t gSpeed;         // Ground speed (2-D) (mm/s)
    int32_t headMot;        // Heading of motion (2-D) (deg * 1e-5)
    uint32_t sAcc;          // Speed accuracy estimate (mm/s)
    uint32_t headAcc;       // Heading accuracy estimate (deg * 1e-5)
    uint16_t pDOP;          // Position DOP (* 0.01)
    uint8_t reserved1[6];
    int32_t headVeh;        // Heading of vehicle (2-D) (deg * 1e-5)
    int16_t magDec;         // Magnetic declination (deg * 1e-2)
    uint16_t magAcc;        // Magnetic declination accuracy (deg * 1e-2)
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
    uint32_t stack_size = 4096;          // FreeRTOS task stack size (increased for parsing)
    uint32_t queue_size = 10;            // Message queue size
    Interface interface_type = Interface::I2C;
    uint32_t baudrate = 38400;           // For UART/SPI
    bool auto_configure = true;          // Automatically configure for optimal settings
    bool enable_nav_pvt = true;          // Enable NAV-PVT messages
    bool enable_nav_sat = false;         // Enable NAV-SAT messages
    bool disable_nmea = true;            // Disable NMEA output by default
};

/// Message container for parsed data
struct ParsedMessage {
    enum Type {
        UBX_MESSAGE,
        NMEA_MESSAGE,
        RAW_DATA
    } type;
    
    union {
        UBXMessage ubx;
        NMEAMessage nmea;
        struct {
            uint8_t data[256];
            size_t length;
        } raw;
    };
    
    TickType_t timestamp;
    bool valid;
    
    ParsedMessage() : type(RAW_DATA), timestamp(0), valid(false) {}
};

/// Register addresses for UBLOX I2C communication
namespace Register {
    constexpr uint8_t NUMBER_BYTES_READY_MSB = 0xFD;
    constexpr uint8_t NUMBER_BYTES_READY_LSB = 0xFE;
    constexpr uint8_t DATA_STREAM = 0xFF;
}

/// Simple data container for GPS data (legacy support)
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
    virtual void flush() = 0;
};

/// Hardware abstraction interface for SPI
class SPI_Interface {
public:
    virtual ~SPI_Interface() = default;
    virtual bool init(uint32_t baudrate) = 0;
    virtual bool write(const uint8_t* data, size_t length) = 0;
    virtual bool read(uint8_t* data, size_t length) = 0;
    virtual bool write_read(const uint8_t* write_data, size_t write_len, 
                           uint8_t* read_data, size_t read_len) = 0;
    virtual void select() = 0;
    virtual void deselect() = 0;
};

/// UBX Message Parser and Generator
class UBXParser {
private:
    uint8_t buffer[1024];
    size_t buffer_pos;
    
    enum ParseState {
        SYNC1,
        SYNC2,
        CLASS,
        ID,
        LENGTH_LOW,
        LENGTH_HIGH,
        PAYLOAD,
        CHECKSUM_A,
        CHECKSUM_B
    } state;
    
    UBXMessage current_message;
    uint16_t payload_counter;
    uint8_t calculated_ck_a, calculated_ck_b;

public:
    UBXParser();
    
    /// Parse incoming bytes, returns true when complete message is ready
    bool parse_byte(uint8_t byte, UBXMessage& message);
    
    /// Generate UBX message from class, id, and payload
    bool generate_message(uint8_t msg_class, uint8_t msg_id, 
                         const uint8_t* payload, uint16_t payload_length,
                         uint8_t* output_buffer, size_t& output_length);
    
    /// Calculate UBX checksum
    static void calculate_checksum(const uint8_t* data, size_t length, uint8_t& ck_a, uint8_t& ck_b);
    
    /// Verify UBX checksum
    static bool verify_checksum(const UBXMessage& message);
    
    /// Reset parser state
    void reset();
    
    /// Create common configuration messages
    bool create_cfg_msg(uint8_t msg_class, uint8_t msg_id, uint8_t rate, 
                       uint8_t* output_buffer, size_t& output_length);
    bool create_cfg_rate(uint16_t meas_ms, uint16_t nav_rate, uint16_t time_ref,
                        uint8_t* output_buffer, size_t& output_length);
    bool create_cfg_nav5(uint8_t dyn_model, uint8_t* output_buffer, size_t& output_length);
    bool create_cfg_prt_i2c(uint8_t* output_buffer, size_t& output_length);
    bool create_cfg_prt_uart(uint32_t baudrate, uint8_t* output_buffer, size_t& output_length);
};

/// NMEA Message Parser
class NMEAParser {
private:
    char buffer[256];
    size_t buffer_pos;
    bool in_message;

public:
    NMEAParser();
    
    /// Parse incoming bytes, returns true when complete message is ready
    bool parse_byte(uint8_t byte, NMEAMessage& message);
    
    /// Parse NMEA message fields
    static bool parse_fields(NMEAMessage& message);
    
    /// Verify NMEA checksum
    static bool verify_checksum(const char* message);
    
    /// Calculate NMEA checksum
    static uint8_t calculate_checksum(const char* message, size_t length);
    
    /// Determine NMEA message type from talker and sentence
    static NMEAType determine_type(const char* message);
    
    /// Reset parser state
    void reset();
};
/// Enhanced UBLOX GPS module driver with comprehensive parsing
class UBLOX {
private:
    // Hardware interfaces (only one will be used)
    I2C_Interface* i2c_interface;
    UART_Interface* uart_interface;
    SPI_Interface* spi_interface;
    
    uint8_t i2c_address;
    Interface interface_type;
    
    // Parsers
    UBXParser ubx_parser;
    NMEAParser nmea_parser;
    
    // FreeRTOS objects
    TaskHandle_t task_handle;
    QueueHandle_t message_queue;     // For parsed messages
    QueueHandle_t raw_data_queue;    // For raw data (legacy support)
    SemaphoreHandle_t mutex;
    
    Configuration config;
    bool initialized;
    
    // Receive buffer
    uint8_t rx_buffer[1024];
    
    /// FreeRTOS task function
    static void task_function(void* parameters);
    
    /// Internal task implementation
    void run_task();
    
    /// Read data from active interface
    bool read_interface_data(uint8_t* buffer, size_t max_length, size_t& bytes_read);
    
    /// Write data to active interface
    bool write_interface_data(const uint8_t* data, size_t length);
    
    /// Process incoming data and parse messages
    void process_incoming_data(const uint8_t* data, size_t length);
    
    /// Send UBX configuration message and wait for ACK
    bool send_ubx_message_with_ack(uint8_t msg_class, uint8_t msg_id, 
                                   const uint8_t* payload, uint16_t payload_length,
                                   TickType_t timeout_ms = 1000);
    
    /// Platform-independent delay using FreeRTOS
    void delay_ms(uint32_t ms) const;

public:
    /// Constructor for I2C interface
    UBLOX(I2C_Interface* i2c, uint8_t address = static_cast<uint8_t>(Address::DEFAULT));
    
    /// Constructor for UART interface
    UBLOX(UART_Interface* uart);
    
    /// Constructor for SPI interface
    UBLOX(SPI_Interface* spi);
    
    /// Destructor
    ~UBLOX();
    
    /// Initialize the UBLOX module and start FreeRTOS task
    bool initialize(const Configuration& config = Configuration{});
    
    /// Stop the module and cleanup FreeRTOS resources
    void shutdown();
    
    /// Check if module is connected
    bool is_connected() const;
    
    /// Get parsed message from the module (non-blocking, uses FreeRTOS queue)
    bool get_parsed_message(ParsedMessage& message, TickType_t timeout_ms = 0);
    
    /// Get raw data from the module (non-blocking, legacy support)
    bool get_data(GPSData& data, TickType_t timeout_ms = 0);
    
    /// Get latest GPS position data (parsed from NAV-PVT)
    bool get_position(GPSPosition& position, TickType_t timeout_ms = 100);
    
    /// Check how many bytes are ready to read (I2C only)
    uint16_t bytes_available();
    
    /// Send raw UBX message
    bool send_ubx_message(uint8_t msg_class, uint8_t msg_id, 
                         const uint8_t* payload, uint16_t payload_length);
    
    /// Configuration methods with enhanced functionality
    bool set_measurement_rate(uint16_t rate_ms);
    bool set_dynamic_model(uint8_t model);  // 0=portable, 6=airborne<1g, 7=airborne<2g, 8=airborne<4g
    bool enable_message(uint8_t msg_class, uint8_t msg_id, uint8_t rate = 1);
    bool disable_message(uint8_t msg_class, uint8_t msg_id);
    bool configure_port_i2c();
    bool configure_port_uart(uint32_t baudrate);
    bool save_configuration();
    bool reset_to_defaults();
    
    /// Convenience configuration methods
    bool set_airborne_4g();
    bool enable_nav_pvt(uint8_t rate = 1);
    bool enable_nav_sat(uint8_t rate = 1);
    bool disable_nmea_messages();
    bool configure_for_airborne();
    
    /// Poll for specific messages
    bool poll_nav_pvt();
    bool poll_nav_status();
    
    /// Get task statistics (FreeRTOS specific)
    void get_task_stats(TaskStatus_t& task_status) const;
    
    /// Get parser statistics
    struct ParserStats {
        uint32_t ubx_messages_parsed;
        uint32_t nmea_messages_parsed;
        uint32_t parse_errors;
        uint32_t checksum_errors;
        uint32_t buffer_overflows;
    };
    void get_parser_stats(ParserStats& stats) const;
    
    /// Debug methods
    void print_status() const;
    void print_message_info(const ParsedMessage& message) const;
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
    void flush() override;
};

/// Pico-specific SPI implementation
class Pico_SPI : public SPI_Interface {
private:
    spi_inst_t* spi_instance;
    uint8_t miso_pin;
    uint8_t mosi_pin;
    uint8_t sck_pin;
    uint8_t cs_pin;
    
public:
    Pico_SPI(spi_inst_t* spi, uint8_t miso = 16, uint8_t mosi = 19, 
             uint8_t sck = 18, uint8_t cs = 17);
    bool init(uint32_t baudrate) override;
    bool write(const uint8_t* data, size_t length) override;
    bool read(uint8_t* data, size_t length) override;
    bool write_read(const uint8_t* write_data, size_t write_len, 
                   uint8_t* read_data, size_t read_len) override;
    void select() override;
    void deselect() override;
};
#endif

/// Utility functions for GPS data conversion
namespace Utils {
    /// Convert UBX coordinate to degrees (double precision)
    double ubx_coord_to_degrees(int32_t coord);
    
    /// Convert UBX velocity to m/s
    double ubx_velocity_to_ms(int32_t vel_mm_s);
    
    /// Convert UBX heading to degrees
    double ubx_heading_to_degrees(int32_t heading_1e5);
    
    /// Format GPS time as string
    bool format_gps_time(const GPSPosition& pos, char* buffer, size_t buffer_size);
    
    /// Validate GPS fix quality
    bool is_valid_fix(const GPSPosition& pos, uint8_t min_fix_type = 3);
    
    /// Calculate distance between two GPS positions (Haversine formula)
    double calculate_distance(double lat1, double lon1, double lat2, double lon2);
    
    /// Calculate bearing between two GPS positions
    double calculate_bearing(double lat1, double lon1, double lat2, double lon2);
}

} // namespace UBLOX
