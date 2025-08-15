#pragma once

#include "UBLOX.h"

namespace UBLOX {

/// UBX Protocol Constants
namespace UBXProtocol {
    constexpr uint8_t SYNC1 = 0xB5;
    constexpr uint8_t SYNC2 = 0x62;
    constexpr size_t MIN_MESSAGE_SIZE = 8;  // Header + checksum
    constexpr size_t MAX_PAYLOAD_SIZE = 512;
}

/// UBX Dynamic Models for CFG-NAV5
namespace DynamicModel {
    constexpr uint8_t PORTABLE = 0;
    constexpr uint8_t STATIONARY = 2;
    constexpr uint8_t PEDESTRIAN = 3;
    constexpr uint8_t AUTOMOTIVE = 4;
    constexpr uint8_t SEA = 5;
    constexpr uint8_t AIRBORNE_1G = 6;
    constexpr uint8_t AIRBORNE_2G = 7;   
    constexpr uint8_t AIRBORNE_4G = 8;
}

/// UBX Fix Types
namespace FixType {
    constexpr uint8_t NO_FIX = 0;
    constexpr uint8_t DEAD_RECKONING = 1;
    constexpr uint8_t FIX_2D = 2;
    constexpr uint8_t FIX_3D = 3;
    constexpr uint8_t GNSS_DEAD_RECKONING = 4;
    constexpr uint8_t TIME_ONLY = 5;
}

/// UBX Configuration templates
struct UBXConfigTemplates {
    /// Generate CFG-MSG message to enable/disable specific message types
    static bool create_cfg_msg(uint8_t msg_class, uint8_t msg_id, uint8_t rate, 
                              uint8_t* buffer, size_t& length);
    
    /// Generate CFG-RATE message to set measurement and navigation rates
    static bool create_cfg_rate(uint16_t meas_ms, uint16_t nav_rate, uint16_t time_ref,
                               uint8_t* buffer, size_t& length);
    
    /// Generate CFG-NAV5 message to set dynamic model
    static bool create_cfg_nav5(uint8_t dyn_model, uint8_t* buffer, size_t& length);
    
    /// Generate CFG-PRT message for I2C configuration
    static bool create_cfg_prt_i2c(uint8_t* buffer, size_t& length);
    
    /// Generate CFG-PRT message for UART configuration
    static bool create_cfg_prt_uart(uint32_t baudrate, uint8_t* buffer, size_t& length);
    
    /// Generate CFG-CFG message to save configuration
    static bool create_cfg_save(uint8_t* buffer, size_t& length);
    
    /// Generate CFG-CFG message to reset to defaults
    static bool create_cfg_reset(uint8_t* buffer, size_t& length);
    
    /// Generate poll message (empty payload)
    static bool create_poll_message(uint8_t msg_class, uint8_t msg_id, 
                                   uint8_t* buffer, size_t& length);
};

/// Helper functions for UBX data parsing
namespace UBXDataParser {
    /// Parse NAV-PVT message payload into GPSPosition structure
    bool parse_nav_pvt(const uint8_t* payload, uint16_t length, GPSPosition& position);
    
    /// Parse ACK-ACK/ACK-NAK messages
    bool parse_ack(const uint8_t* payload, uint16_t length, uint8_t& acked_class, uint8_t& acked_id);
    
    /// Parse NAV-STATUS message
    bool parse_nav_status(const uint8_t* payload, uint16_t length, 
                         uint8_t& fix_type, uint8_t& flags, uint32_t& ttff);
    
    /// Parse MON-VER message (version information)
    bool parse_mon_ver(const uint8_t* payload, uint16_t length, 
                      char* sw_version, size_t sw_len, char* hw_version, size_t hw_len);
}

} // namespace UBLOX
