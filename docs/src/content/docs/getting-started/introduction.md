---
title: Introduction
description: Welcome to Pico-FreeRTOS - A comprehensive embedded systems library for Raspberry Pi Pico
---

# Pico-FreeRTOS

Welcome to **Pico-FreeRTOS**, a comprehensive embedded systems library designed for the Raspberry Pi Pico microcontroller. This project integrates FreeRTOS with a rich collection of sensor libraries, GPS functionality, and wireless communication capabilities.

## 🚀 Key Features

- **FreeRTOS Integration**: Real-time operating system support with task management, queues, and semaphores
- **GPS & Navigation**: UBLOX GPS module support with NMEA and UBX protocol parsing and configuration
- **Rich Sensor Ecosystem**: Support for multiple IMUs, accelerometers, and environmental sensors
- **Wireless Communication**: Bluetooth connectivity with BTStack integration
- **Control Systems**: Built-in control theory implementations including PID controllers
- **Hardware Abstraction**: Clean APIs for I2C, SPI, and UART communication
- **Real-Time Data Logging**: Efficient data storage to flash memory and SD cards
- **Mobile App Integration**: Native iOS and Android apps for telemetry and configuration
- **Protobuf Support**: Easy integration with Protocol Buffers for data serialization over Bluetooth, UART, LoRa, and other radios

## 🔧 Supported Hardware

### Sensors

- **ADXL375**: High-G 3-axis accelerometer (±200g)
- **BMP390**: Precision barometric pressure sensor
- **BNO55**: 9-DOF absolute orientation sensor
- **ISM330DHCX**: 6-axis IMU with machine learning core
- **LSM6DSO32**: High-performance 6-axis IMU (±32g accelerometer)

### GPS Modules

- **UBLOX M10Q**: Multi-GNSS receiver with concurrent GPS, GLONASS, Galileo, and BeiDou support

### Communication

- **Bluetooth LE**: Full BTStack integration for wireless connectivity
- **UART/I2C/SPI**: Hardware abstraction layers for all communication protocols
- **LoRa**: Long-range radio communication support

## 🎯 Use Cases

This library is perfect for:

- **Aerospace Projects**: High-G logging, flight computers, rocket telemetry
- **Robotics**: Sensor fusion, navigation, real-time control
- **IoT Applications**: Wireless sensor networks, data logging
- **Research & Development**: Rapid prototyping of embedded systems
- **Educational Projects**: Learning embedded systems and real-time programming

## 📊 Architecture Overview

```txt
┌─────────────────┐    ┌─────────────────┐    ┌─────────────────┐
│   Application   │    │   Bluetooth     │    │   GPS/GNSS      │
│     Layer       │    │     Stack       │    │    Module       │
└─────────────────┘    └─────────────────┘    └─────────────────┘
         │                       │                       │
┌─────────────────────────────────────────────────────────────────┐
│                    FreeRTOS Kernel                              │
└─────────────────────────────────────────────────────────────────┘
         │                       │                       │
┌─────────────────┐    ┌─────────────────┐    ┌─────────────────┐
│  Sensor Fusion  │    │   Control       │    │   Hardware      │
│    & Filters    │    │   Systems       │    │  Abstraction    │
└─────────────────┘    └─────────────────┘    └─────────────────┘
         │                       │                       │
┌─────────────────────────────────────────────────────────────────┐
│              Raspberry Pi Pico Hardware                         │
└─────────────────────────────────────────────────────────────────┘
```

## 🛠️ Quick Start

Get up and running in minutes:

1. **Hardware Setup**: Connect your sensors and GPS module
2. **Build Environment**: Set up the Pico SDK and build tools
3. **Flash & Run**: Compile and deploy your first project

```bash
# Clone the repository
git clone https://github.com/gregoryw3/pico-freertos.git --recursive
cd pico-freertos

# Build the project
./release_build.sh
# Or
./debug_build.sh

# Flash to your Pico
picotool load -f -x build/debug/src/simple.uf2
```

## 📖 Documentation Structure

<!-- - **Core Systems**: FreeRTOS integration and hardware abstraction -->
- **Getting Started**: Setup, installation, and basic usage
- **GPS & Navigation**: Complete guide to GPS functionality
- **Sensor Libraries**: Individual sensor documentation and examples
- **Communication**: Bluetooth, UART, I2C, and SPI guides
- **Control Systems**: Theory and implementation of control algorithms
- **Examples**: Real-world projects and tutorials
- **API Reference**: Complete API documentation
<!-- 
## 🤝 Contributing

We welcome contributions! Whether it's adding new sensor support, improving documentation, or fixing bugs, your help makes this project better. -->

<!-- ## 📄 License

This project is open source and available under the MIT License.

---

Ready to get started? Head over to [Hardware Setup](/getting-started/hardware) to begin your journey with Pico-FreeRTOS! -->
