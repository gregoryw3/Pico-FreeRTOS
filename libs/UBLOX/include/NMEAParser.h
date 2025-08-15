#pragma once

#include "UBLOX.h"

namespace UBLOX {

// M10Q Interface Description
// https://content.u-blox.com/sites/default/files/u-blox-M10-SPG-5.10_InterfaceDescription_UBX-21035062.pdf
/**
 * @brief 
 * 
 * # hi
 * 
 */
namespace NMEA {

/// NMEA Protocol Constants
namespace NMEAProtocol {
    constexpr char START_CHAR = '$';
    constexpr char CHECKSUM_DELIMITER = '*';
    constexpr char FIELD_DELIMITER = ',';
    constexpr char CR = '\r';
    constexpr char LF = '\n';
    constexpr size_t MAX_MESSAGE_LENGTH = 256;
    constexpr size_t MAX_FIELDS = 20;
    constexpr size_t MAX_FIELD_LENGTH = 32;
}

/// NMEA Talker IDs
namespace NMEATalker {
    constexpr char GP[] = "GP";  // GPS
    constexpr char GL[] = "GL";  // GLONASS
    constexpr char GA[] = "GA";  // Galileo
    constexpr char GB[] = "GB";  // BeiDou
    constexpr char GN[] = "GN";  // Multiple GNSS
}

/// NMEA Sentence Types
namespace NMEASentence {
    constexpr char GGA[] = "GGA";  // Global Positioning System Fix Data
    constexpr char GLL[] = "GLL";  // Geographic Position - Latitude/Longitude
    constexpr char GSA[] = "GSA";  // GPS DOP and active satellites
    constexpr char GSV[] = "GSV";  // GPS Satellites in view
    constexpr char RMC[] = "RMC";  // Recommended Minimum Navigation Information
    constexpr char VTG[] = "VTG";  // Track Made Good and Ground Speed
    constexpr char ZDA[] = "ZDA";  // Date & Time
}

/// NMEA Data structures for parsed messages
struct NMEAGGAData {
    double latitude;        // Degrees
    double longitude;       // Degrees
    uint8_t fix_quality;    // 0=invalid, 1=GPS, 2=DGPS
    uint8_t num_satellites; // Number of satellites in use
    double hdop;           // Horizontal dilution of precision
    double altitude;       // Altitude above mean sea level (meters)
    double geoid_height;   // Height of geoid above WGS84 ellipsoid (meters)
    char time_str[16];     // Time as HHMMSS.sss
    bool valid;
};

struct NMEARMCData {
    char time_str[16];     // Time as HHMMSS.sss
    char date_str[16];     // Date as DDMMYY
    char status;           // A=valid, V=invalid
    double latitude;       // Degrees
    double longitude;      // Degrees
    double speed_knots;    // Speed over ground in knots
    double course;         // Course over ground in degrees
    double magnetic_variation; // Magnetic variation in degrees
    bool valid;
};

struct NMEAGSAData {
    char mode;             // M=manual, A=automatic
    uint8_t fix_type;      // 1=no fix, 2=2D, 3=3D
    uint8_t sat_ids[12];   // Satellite IDs used in solution
    double pdop;           // Position dilution of precision
    double hdop;           // Horizontal dilution of precision
    double vdop;           // Vertical dilution of precision
    bool valid;
};

/// NMEA Parser utilities
namespace NMEAUtils {
    /// Convert NMEA coordinate format (DDMM.MMMM) to decimal degrees
    double nmea_coord_to_degrees(const char* coord_str, char hemisphere);
    
    /// Convert decimal degrees to NMEA coordinate format
    void degrees_to_nmea_coord(double degrees, char* coord_str, size_t buffer_size, 
                              char pos_hemisphere, char neg_hemisphere);
    
    /// Parse time string (HHMMSS.sss)
    bool parse_time(const char* time_str, uint8_t& hour, uint8_t& minute, 
                   uint8_t& second, uint16_t& millisecond);
    
    /// Parse date string (DDMMYY)
    bool parse_date(const char* date_str, uint8_t& day, uint8_t& month, uint16_t& year);
    
    /// Validate field is not empty
    bool is_field_valid(const char* field);
    
    /// Convert string to double with validation
    bool str_to_double(const char* str, double& value);
    
    /// Convert string to int with validation
    bool str_to_int(const char* str, int& value);
}

/// NMEA Message parsers for specific sentence types
namespace NMEAParsers {
    /// Parse GGA (Global Positioning System Fix Data) message
    bool parse_gga(const NMEAMessage& message, NMEAGGAData& data);
    
    /// Parse RMC (Recommended Minimum Navigation Information) message
    bool parse_rmc(const NMEAMessage& message, NMEARMCData& data);
    
    /// Parse GSA (GPS DOP and active satellites) message
    bool parse_gsa(const NMEAMessage& message, NMEAGSAData& data);
    
    /// Parse GSV (GPS Satellites in view) message
    bool parse_gsv(const NMEAMessage& message, uint8_t& total_messages, 
                  uint8_t& message_number, uint8_t& total_sats);
}

} // namespace NMEA

} // namespace UBLOX
