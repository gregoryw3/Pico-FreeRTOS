---
title: NMEA Protocol
description: Complete guide to NMEA 0183 protocol parsing for UBLOX GPS modules
---

# NMEA Protocol Support

Our UBLOX library provides some support for parsing NMEA 0183 messages from u-blox GPS modules. NMEA (National Marine Electronics Association) is a standard protocol used by GPS receivers to communicate position, time, and satellite information.

NMEA standards require paying for the specification, but our UBLOX library implements the most commonly used messages for GPS applications.

This page (and all GPS related documents from us) are based on u-blox published documents like the [M10 Interface Description](https://content.u-blox.com/sites/default/files/u-blox-M10-SPG-5.10_InterfaceDescription_UBX-21035062.pdf)

## NMEA Message Structure

NMEA messages are ASCII strings that follow a standardized format. Each message consists of five main components:

| Component | Description | Format | Example |
|-----------|-------------|---------|---------|
| **Start Character** | Message start delimiter | `$` | `$` |
| **Address Field** | Talker ID + Sentence Type | `TTSSS` | `GNGGA` |
| **Data Fields** | Comma-separated values | `field1,field2,...` | `123045.00,4722.80340,N` |
| **Checksum** | Error detection | `*XX` | `*6E` |
| **End Sequence** | Message terminator | `\r\n` | `\r\n` |

### Address Field Format

The address field consists of:

- **TT**: Two-letter Talker Identifier (GP, GN, etc.)
- **SSS**: Three-letter Sentence Formatter (GGA, RMC, etc.)

Example: `GNGGA` = GN (Multi-GNSS) + GGA (Global Positioning System Fix Data)

### NMEA Talker IDs

The Talker ID indicates which GNSS constellation(s) are being used:

| Talker ID | GNSS System | NMEA Version | Description |
|-----------|-------------|--------------|-------------|
| **GP** | GPS, SBAS | NMEA 2.3+ | Global Positioning System (USA) |
| **GL** | GLONASS | NMEA 2.3+ | GLONASS (Russia) |
| **GA** | Galileo | NMEA 4.10+ | Galileo (Europe) |
| **GB** | BeiDou | NMEA 4.10+ | BeiDou (China) |
| **GI** | NavIC | NMEA 4.11+ | NavIC (India) |
| **GQ** | QZSS | NMEA 4.11+ | QZSS (Japan) |
| **GN** | Multi-GNSS | NMEA 4.10+ | Any combination of GNSS |

### Data Fields

- Fields are separated by commas (`,`)
- Variable number of fields depending on message type
- Some fields may have repetitions
- Empty fields are allowed (represented as `,,`)
- Values can be null when data is invalid or unavailable

### Coordinate Format

NMEA uses degrees, minutes format for latitude and longitude:

| Format | Example | Conversion |
|--------|---------|------------|
| **NMEA Format** | `4722.80340,N` | 47°22.80340' North |
| **Decimal Degrees** | `47.38005667` | 47.38005667° |
| **Deg/Min/Sec** | `47°22'48.2040"` | 47° 22' 48.2040" |

**Conversion Formula:**

```text
Decimal Degrees = DD + (MM.MMMMM / 60)
```

Where:

- DD = Degrees (47)
- MM.MMMMM = Minutes and fractional minutes (22.80340)

## NMEA Message Types

### Standard NMEA Messages

Standard NMEA messages follow the official NMEA 0183 specification:

| Message | Class/ID | Description |
|---------|----------|-------------|
| **GGA** | 0xf0 0x00 | Global positioning system fix data |
| **GLL** | 0xf0 0x01 | Latitude and longitude, with time of position fix |
| **GSA** | 0xf0 0x02 | GNSS DOP and active satellites |
| **GSV** | 0xf0 0x03 | GNSS satellites in view |
| **RMC** | 0xf0 0x04 | Recommended minimum data |
| **VTG** | 0xf0 0x05 | Course over ground and ground speed |
| **GBS** | 0xf0 0x09 | GNSS satellite fault detection |
| **GST** | 0xf0 0x07 | GNSS pseudorange error statistics |
| **ZDA** | 0xf0 0x08 | Time and date |
| **GNS** | 0xf0 0x0d | GNSS fix data |
| **TXT** | 0xf0 0x41 | Text transmission |

### Proprietary NMEA Messages (PUBX)

u-blox proprietary messages use the manufacturer mnemonic **UBX** and have the address field **PUBX**:

| Message | Class/ID | Description |
|---------|----------|-------------|
| **PUBX,00** | 0xf1 0x00 | Lat/Long position data |
| **PUBX,03** | 0xf1 0x03 | Satellite status |
| **PUBX,04** | 0xf1 0x04 | Time of day and clock information |
| **PUBX,40** | 0xf1 0x40 | Set NMEA message output rate |
| **PUBX,41** | 0xf1 0x41 | Set protocols and baud rate |

### NMEA 4.10+ Extra Fields

Modern NMEA versions include additional fields for enhanced functionality:

| Message | Extra Fields | Description |
|---------|--------------|-------------|
| **GBS** | systemId, signalId | System and signal identification |
| **GNS** | navStatus | Navigation status |
| **GRS** | systemId, signalId | System and signal identification |
| **GSA** | systemId | System identification |
| **GSV** | signalId | Signal identification |
| **RMC** | navStatus | Navigation status |

## Data Validation and Error Handling

### Invalid Data Output

By default, u-blox receivers output empty fields for invalid or unknown data:

**Valid Position:**

```text
$GPGLL,4717.11634,N,00833.91297,E,124923.00,A,A*6E
```

**Invalid Position (valid time):**

```text
$GPGLL,,,,,124924.00,V,N*42
```

### Checksum Calculation

The checksum is calculated as XOR of all characters between `$` and `*`:

```cpp
uint8_t calculateChecksum(const char* sentence) {
    uint8_t checksum = 0;
    for (int i = 1; sentence[i] != '*' && sentence[i] != '\0'; i++) {
        checksum ^= sentence[i];
    }
    return checksum;
}
```

## Commonly Used NMEA Sentences

The following table shows the most frequently used NMEA sentences in GPS applications:

| Sentence | Full Name | Primary Use Case | Key Data |
|----------|-----------|------------------|----------|
| **GGA** | Global Positioning System Fix Data | Position and fix quality | Lat/Lon, altitude, satellites, HDOP |
| **RMC** | Recommended Minimum Navigation | Essential navigation data | Time, date, position, speed, course |
| **GSA** | GPS DOP and Active Satellites | Precision and satellite geometry | PDOP, HDOP, VDOP, satellite IDs |
| **GSV** | GPS Satellites in View | Satellite visibility and strength | SNR, elevation, azimuth for each satellite |
| **GLL** | Geographic Position - Lat/Lon | Basic position data | Latitude, longitude, time, validity |
| **VTG** | Track Made Good and Ground Speed | Movement information | Speed (knots/km/h), course |
| **ZDA** | Date & Time | Precise timing | UTC time, date, local time zone |
| **GNS** | GNSS Fix Data | Multi-constellation fix | Position, mode, satellites per system |

## NMEA Protocol Constants

The library defines essential protocol constants for parsing:

```cpp
namespace UBLOX::NMEA::NMEAProtocol {
    constexpr char START_CHAR = '$';              // Message start character
    constexpr char CHECKSUM_DELIMITER = '*';      // Checksum separator
    constexpr char FIELD_DELIMITER = ',';         // Field separator
    constexpr char CR = '\r';                     // Carriage return
    constexpr char LF = '\n';                     // Line feed
    constexpr size_t MAX_MESSAGE_LENGTH = 256;    // Maximum message length
    constexpr size_t MAX_FIELDS = 20;             // Maximum fields per message
    constexpr size_t MAX_FIELD_LENGTH = 32;       // Maximum field length
}
```

## GNSS Constellation Support

The library supports multiple GNSS constellations through talker IDs:

```cpp
namespace UBLOX::NMEA::NMEATalker {
    constexpr char GP[] = "GP";  // GPS (USA)
    constexpr char GL[] = "GL";  // GLONASS (Russia) 
    constexpr char GA[] = "GA";  // Galileo (Europe)
    constexpr char GB[] = "GB";  // BeiDou (China)
    constexpr char GN[] = "GN";  // Multiple GNSS combined
}
```

## Data Structures

### GGA - Global Positioning System Fix Data

Contains essential position and fix quality information:

```cpp
struct NMEAGGAData {
    double latitude;        // Latitude in decimal degrees
    double longitude;       // Longitude in decimal degrees
    uint8_t fix_quality;    // Fix quality indicator
    uint8_t num_satellites; // Number of satellites in use
    double hdop;           // Horizontal dilution of precision
    double altitude;       // Altitude above mean sea level (meters)
    double geoid_height;   // Height of geoid above WGS84 ellipsoid
    char time_str[16];     // Time as HHMMSS.sss
    bool valid;            // Data validity flag
};
```

#### Fix Quality Values

| Value | Description | Accuracy |
|-------|-------------|----------|
| 0 | Invalid | No fix available |
| 1 | GPS fix (SPS) | ±3-5 meters |
| 2 | DGPS fix | ±1-3 meters |
| 3 | PPS fix | High precision |

### RMC - Recommended Minimum Navigation

Essential navigation data including time, date, and movement:

```cpp
struct NMEARMCData {
    char time_str[16];     // Time as HHMMSS.sss
    char date_str[16];     // Date as DDMMYY
    char status;           // A=valid, V=invalid
    double latitude;       // Latitude in decimal degrees
    double longitude;      // Longitude in decimal degrees
    double speed_knots;    // Speed over ground in knots
    double course;         // Course over ground in degrees
    double magnetic_variation; // Magnetic variation
    bool valid;            // Data validity flag
};
```

### GSA - GPS DOP and Active Satellites

Provides information about satellite geometry and precision:

```cpp
struct NMEAGSAData {
    char mode;             // M=manual, A=automatic
    uint8_t fix_type;      // Fix type (1=no fix, 2=2D, 3=3D)
    uint8_t sat_ids[12];   // Satellite IDs used in solution
    double pdop;           // Position dilution of precision
    double hdop;           // Horizontal dilution of precision  
    double vdop;           // Vertical dilution of precision
    bool valid;            // Data validity flag
};
```

## Utility Functions

### Coordinate Conversion

Convert between NMEA coordinate format and decimal degrees:

```cpp
// Convert NMEA coordinate (DDMM.MMMM) to decimal degrees
double lat = NMEAUtils::nmea_coord_to_degrees("4807.038", 'N');
// Returns: 48.11730

// Convert decimal degrees to NMEA format
char coord_str[16];
NMEAUtils::degrees_to_nmea_coord(48.11730, coord_str, sizeof(coord_str), 'N', 'S');
```

### Time and Date Parsing

Parse NMEA time and date strings:

```cpp
uint8_t hour, minute, second;
uint16_t millisecond;
if (NMEAUtils::parse_time("123045.678", hour, minute, second, millisecond)) {
    // hour = 12, minute = 30, second = 45, millisecond = 678
}

uint8_t day, month;
uint16_t year;
if (NMEAUtils::parse_date("230825", day, month, year)) {
    // day = 23, month = 8, year = 2025
}
```

## Example Usage

### Basic NMEA Parsing

```cpp
#include "NMEAParser.h"

void handleNMEAMessage(const char* nmea_sentence) {
    using namespace UBLOX::NMEA;
    
    // Parse GGA sentence for position data
    if (strstr(nmea_sentence, "GGA") != nullptr) {
        NMEAGGAData gga_data;
        if (parseGGA(nmea_sentence, gga_data) && gga_data.valid) {
            printf("Position: %.6f°, %.6f°\n", 
                   gga_data.latitude, gga_data.longitude);
            printf("Altitude: %.2f m\n", gga_data.altitude);
            printf("Satellites: %d\n", gga_data.num_satellites);
            printf("HDOP: %.2f\n", gga_data.hdop);
        }
    }
    
    // Parse RMC sentence for speed and course
    if (strstr(nmea_sentence, "RMC") != nullptr) {
        NMEARMCData rmc_data;
        if (parseRMC(nmea_sentence, rmc_data) && rmc_data.valid) {
            printf("Speed: %.2f knots (%.2f km/h)\n", 
                   rmc_data.speed_knots, rmc_data.speed_knots * 1.852);
            printf("Course: %.2f°\n", rmc_data.course);
            printf("Date: %s Time: %s\n", rmc_data.date_str, rmc_data.time_str);
        }
    }
}
```

### FreeRTOS Integration

```cpp
void gps_task(void *pvParameters) {
    char nmea_buffer[NMEA_MAX_MESSAGE_LENGTH];
    
    while (1) {
        // Read NMEA sentence from UART
        if (uart_read_line(UART_GPS, nmea_buffer, sizeof(nmea_buffer))) {
            handleNMEAMessage(nmea_buffer);
        }
        
        vTaskDelay(pdMS_TO_TICKS(100)); // 10 Hz update rate
    }
}
```

## Performance Considerations

- **Memory Usage**: Each NMEA structure uses approximately 64-128 bytes
- **Parsing Speed**: Typical parsing time is <1ms per sentence on Pico
- **Update Rates**: GPS modules typically output at 1-10 Hz
- **Buffer Management**: Use circular buffers for continuous data streams

## Best Practices

1. **Validate Data**: Always check the `valid` flag before using parsed data
2. **Handle Multiple Constellations**: Support GN (multi-GNSS) messages for better accuracy
3. **Monitor Fix Quality**: Use HDOP and satellite count to assess positioning accuracy
4. **Time Synchronization**: Use RMC or ZDA messages for precise time references
5. **Error Handling**: Implement checksum validation and malformed message detection

## Next Steps

- Learn about [UBX Protocol](/gps/ubx) for advanced GPS features
- See [GPS Examples](/gps/examples) for complete implementation examples
- Explore [Sensor Fusion](/examples/sensor-fusion) combining GPS with IMU data
