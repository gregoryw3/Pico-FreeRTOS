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

struct Quaternion {
    float w, x, y, z;
    
    Quaternion(float w = 0.0f, float x = 0.0f, float y = 0.0f, float z = 0.0f)
        : w(w), x(x), y(y), z(z) {}
};

struct RollPitchYaw {
    float roll, pitch, yaw;
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

// Device ID verification structure - each sensor defines these
struct DeviceIdInfo {
    uint8_t register_address;
    uint8_t expected_value;
    const char* device_name;
};

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
    
    // I2C implementation of device ID register reading
    template<typename RegisterType>
    uint8_t read_device_id_register(RegisterType reg) {
        uint8_t device_id = 0;
        uint8_t reg_addr = static_cast<uint8_t>(reg);
        
        // Write register address
        int result = i2c_write_blocking(i2c, addr, &reg_addr, 1, true);
        if (result != 1) {
            printf("I2C: Failed to write device ID register address\n");
            return 0;
        }
        
        // Read device ID
        result = i2c_read_blocking(i2c, addr, &device_id, 1, false);
        if (result != 1) {
            printf("I2C: Failed to read device ID\n");
            return 0;
        }
        
        return device_id;
    }
    
    // Helper method for derived classes to implement device ID checking
    template<typename RegisterType>
    bool verify_device_id(RegisterType id_register, uint8_t expected_value, const char* device_name = "Unknown") {
        uint8_t device_id = read_device_id_register(id_register);
        if (device_id != expected_value) {
            printf("%s: Device ID check failed. Expected: 0x%02X, Got: 0x%02X\n", 
                   device_name, expected_value, device_id);
            return false;
        }
        printf("%s: Device ID verified (0x%02X)\n", device_name, device_id);
        return true;
    }
};

// Specialization for SPI
template<>
class Sensor<SPI> : public Sensor<void> {
public:
    Sensor(spi_inst_t* spi, uint cs_pin) : spi(spi), cs_pin(cs_pin) {}
    
protected:
    spi_inst_t* spi;
    uint cs_pin;
    
    // SPI implementation of device ID register reading
    template<typename RegisterType>
    uint8_t read_device_id_register(RegisterType reg) {
        uint8_t device_id = 0;
        uint8_t reg_addr = static_cast<uint8_t>(reg) | 0x80; // Set read bit for SPI
        
        gpio_put(cs_pin, 0); // Assert CS
        spi_write_blocking(spi, &reg_addr, 1);
        spi_read_blocking(spi, 0, &device_id, 1);
        gpio_put(cs_pin, 1); // Deassert CS
        
        return device_id;
    }
    
    // Helper method for derived classes to implement device ID checking
    template<typename RegisterType>
    bool verify_device_id(RegisterType id_register, uint8_t expected_value, const char* device_name = "Unknown") {
        uint8_t device_id = read_device_id_register(id_register);
        if (device_id != expected_value) {
            printf("%s: Device ID check failed. Expected: 0x%02X, Got: 0x%02X\n", 
                   device_name, expected_value, device_id);
            return false;
        }
        printf("%s: Device ID verified (0x%02X)\n", device_name, device_id);
        return true;
    }
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