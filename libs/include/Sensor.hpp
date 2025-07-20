#pragma once

#include "Board.hpp"

// Tag types
struct I2C {};
struct SPI {};
struct UART {};

// Sensor Tag types
struct AccelerometerTag {};
struct GyroscopeTag {};
struct IMU_6_AXISTag {};
struct MagnetometerTag {};
struct IMU_9_AXISTag {};
struct BarometerTag {};
struct IMU_10_AXISTag {};

// Data structures for different sensor readings
struct AccelData {
    float x, y, z;
};

struct GyroData {
    float x, y, z;
};

struct MagData {
    float x, y, z;
};

struct PressureData {
    float pressure;
    float temperature;
};

struct IMU6Data {
    AccelData accel;
    GyroData gyro;
};

struct IMU9Data {
    AccelData accel;
    GyroData gyro;
    MagData mag;
};

struct IMU10Data {
    AccelData accel;
    GyroData gyro;
    MagData mag;
    PressureData pressure;
};

template<typename BusType, typename... Args>
auto make_sensor(Args&&... args) {
    return Sensor<BusType>(std::forward<Args>(args)...);
}

// Generic base template (not defined)
template<typename BusType>
class Sensor {
public:
    Pose pose_on_pcb; // Position/orientation relative to PCB
};

// Specialization for I2C
template<>
class Sensor<I2C> : public Sensor<void> {
public:
    Sensor(i2c_inst_t* i2c, uint8_t addr) : i2c(i2c), addr(addr) {}
protected:
    i2c_inst_t* i2c;
    uint8_t addr;
};

// Specialization for SPI
template<>
class Sensor<SPI> : public Sensor<void> {
public:
    Sensor(spi_inst_t* spi, uint cs_pin) : spi(spi), cs_pin(cs_pin) {}
protected:
    spi_inst_t* spi;
    uint cs_pin;
};

// Compile error for unsupported UART
template<>
class Sensor<UART> : public Sensor<void> {
public:
    Sensor(uart_inst_t* uart, uint baud_rate) : uart(uart), baud_rate(baud_rate) {}
protected:
    uart_inst_t* uart;
    uint baud_rate;
};

// Compile-time sensor type traits
template<typename SensorType>
struct SensorTraits;

template<>
struct SensorTraits<AccelerometerTag> {
    using DataType = AccelData;
};

template<>
struct SensorTraits<GyroscopeTag> {
    using DataType = GyroData;
};

template<>
struct SensorTraits<IMU_6_AXISTag> {
    using DataType = IMU6Data;
};

template<>
struct SensorTraits<MagnetometerTag> {
    using DataType = MagData;
};

template<>
struct SensorTraits<IMU_9_AXISTag> {
    using DataType = IMU9Data;
};

template<>
struct SensorTraits<BarometerTag> {
    using DataType = PressureData;
};

template<>
struct SensorTraits<IMU_10_AXISTag> {
    using DataType = IMU10Data;
};

// CRTP base classes for compile-time polymorphism
template<typename Derived, typename SensorTypeTag>
class SensorInterface {
public:
    using DataType = typename SensorTraits<SensorTypeTag>::DataType;
    
    // CRTP pattern - calls derived class implementation
    // DataType read() {
    //     return static_cast<Derived*>(this)->read_impl();
    // }
    
    // void start() {
    //     static_cast<Derived*>(this)->start_impl();
    // }
    
    // void stop() {
    //     static_cast<Derived*>(this)->stop_impl();
    // }
};

// Specific sensor interface templates
template<typename Derived>
using AccelerometerInterface = SensorInterface<Derived, AccelerometerTag>;

template<typename Derived>
using GyroscopeInterface = SensorInterface<Derived, GyroscopeTag>;

template<typename Derived>
using IMU6Interface = SensorInterface<Derived, IMU_6_AXISTag>;

template<typename Derived>
using MagnetometerInterface = SensorInterface<Derived, MagnetometerTag>;

template<typename Derived>
using IMU9Interface = SensorInterface<Derived, IMU_9_AXISTag>;

template<typename Derived>
using BarometerInterface = SensorInterface<Derived, BarometerTag>;

template<typename Derived>
using IMU10Interface = SensorInterface<Derived, IMU_10_AXISTag>;

template<typename T>
struct is_accelerometer {
    template<typename U>
    static auto test(int) -> decltype(std::declval<U>().read_accel(), std::true_type{});
    template<typename>
    static std::false_type test(...);
    using type = decltype(test<T>(0));
    static constexpr bool value = type::value;
};

template<typename T>
struct is_gyroscope {
    template<typename U>
    static auto test(int) -> decltype(std::declval<U>().read_gyro(), std::true_type{});
    template<typename>
    static std::false_type test(...);
    using type = decltype(test<T>(0));
    static constexpr bool value = type::value;
};

template<typename T>
struct is_imu {
    template<typename U>
    static auto test(int) -> decltype(std::declval<U>().read_imu(), std::true_type{});
    template<typename>
    static std::false_type test(...);
    using type = decltype(test<T>(0));
    static constexpr bool value = type::value;
};

// Generic sensor function templates that work with any sensor type
// template<typename SensorType>
// auto read_sensor_data(SensorType& sensor) -> decltype(sensor.read()) {
//     return sensor.read();
// }

// template<typename SensorType>
// void start_sensor(SensorType& sensor) {
//     sensor.start();
// }

// template<typename SensorType>
// void stop_sensor(SensorType& sensor) {
//     sensor.stop();
// }