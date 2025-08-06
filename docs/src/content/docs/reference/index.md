---
title: API Reference
description: Complete API documentation for Pico-FreeRTOS libraries
---

This section contains comprehensive API documentation for all components of the Pico-FreeRTOS library.

## Library Organization

The API is organized into the following modules:

### Core Systems
- **FreeRTOS Integration**: Task management, synchronization primitives
- **Hardware Abstraction**: I2C, SPI, UART interfaces
- **Memory Management**: Heap allocation, buffer management

### GPS & Navigation
- **NMEA Parser**: Complete NMEA 0183 protocol support
- **UBX Protocol**: u-blox binary protocol implementation
- **Coordinate Systems**: Geodetic transformations and utilities

### Sensor Libraries
- **ADXL375**: High-G accelerometer (±200g)
- **BMP390**: Precision barometric pressure sensor
- **BNO55**: 9-DOF absolute orientation sensor
- **ISM330DHCX**: 6-axis IMU with machine learning
- **LSM6DSO32**: High-performance 6-axis IMU

### Communication
- **Bluetooth**: BTStack integration and GATT services
- **Serial Communication**: UART, I2C, SPI drivers
- **Wireless**: WiFi and radio communication protocols

### Control Systems
- **PID Controllers**: Proportional-integral-derivative control
- **Kalman Filters**: State estimation and sensor fusion
- **Signal Processing**: Digital filters and transforms

## API Conventions

### Return Values
- `true`/`false` for success/failure operations
- Specific error codes for detailed diagnostics
- Null pointers indicate allocation failures

### Thread Safety
- All sensor drivers are thread-safe by default
- Shared resources use FreeRTOS mutexes
- Interrupt-safe variants noted in documentation

### Memory Management
- Stack allocation preferred for performance
- Heap allocation clearly documented
- Automatic cleanup in destructors

### Error Handling
- Comprehensive error codes and messages
- Graceful degradation for non-critical failures
- Debug output available in development builds

## Quick Navigation

- Browse by **component** using the sidebar navigation
- Use the **search function** to find specific APIs
- Check **examples** for usage patterns
- Refer to **best practices** for optimal performance

## Code Examples

All API documentation includes:
- Complete function signatures
- Parameter descriptions
- Return value explanations
- Usage examples
- Performance notes
- Thread safety information

## Integration Notes

This API reference is auto-generated from the source code and is always up-to-date with the latest implementation. For higher-level guides and tutorials, see the main documentation sections.
