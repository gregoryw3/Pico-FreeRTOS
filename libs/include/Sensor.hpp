#pragma once

#include <stdio.h>
#include <type_traits>
#include <utility>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "hardware/uart.h"
#include "hardware/spi.h"

#include "Pose.hpp"

// Tag types
struct I2C {};
struct SPI {};
struct UART {};

template<typename BusType, typename... Args>
auto make_sensor(Args&&... args) {
    return Sensor<BusType>(std::forward<Args>(args)...);
}

// Generic base template (not defined)
template<typename BusType>
class Sensor;

// Specialization for I2C
template<>
class Sensor<I2C> {
public:
    Sensor(i2c_inst_t* i2c, uint8_t addr) : i2c(i2c), addr(addr) {}
    void run() { /* start task or polling */ }
    void read() { /* read data over I2C */ }
protected:
    i2c_inst_t* i2c;
    uint8_t addr;
};

// Specialization for SPI
template<>
class Sensor<SPI> {
public:
    Sensor(spi_inst_t* spi, uint cs_pin) : spi(spi), cs_pin(cs_pin) {}
    void run() { /* start task or polling */ }
    void read() { /* read data over SPI */ }
protected:
    spi_inst_t* spi;
    uint cs_pin;
};

// Compile error for unsupported UART
template<>
class Sensor<UART> {
public:
    Sensor(uart_inst_t* uart, uint baud_rate) : uart(uart), baud_rate(baud_rate) {}
    void run() { /* start task or polling */ }
    void read() { /* read data over UART */ }
protected:
    uart_inst_t* uart;
    uint baud_rate;
};


class SensorBase {
public:
    Pose pose_on_pcb; // Position/orientation relative to PCB
};