#pragma once

#include "../../include/Sensor.hpp"
#include "hardware/i2c.h"
#include "hardware/spi.h"
#include "hardware/gpio.h"
#include "FreeRTOS.h"
#include "task.h"

namespace BNO55 {
// BNO55 Register addresses
// Taken from the BNO55 datasheet and https://github.com/adafruit/Adafruit_BNO055/blob/master/Adafruit_BNO055.h
enum class Register : uint8_t {

    PAGE_ID_ADDR = 0x07, // Page ID register for BNO55
    BNO55_ID = 0xA0, // BNO55 device ID

    CHIP_ID = 0x00,
    ACCEL_REV_ID = 0x01,
    MAG_REV_ID = 0x02,
    GYRO_REV_ID = 0x03,
    SW_REV_ID_LSB = 0x04,
    SW_REV_ID_MSB = 0x05,
    BL_REV_ID = 0x06,

    /* Accel data register */
    ACCEL_DATA_X_LSB = 0x08,
    ACCEL_DATA_X_MSB = 0x09,
    ACCEL_DATA_Y_LSB = 0x0A,
    ACCEL_DATA_Y_MSB = 0x0B,
    ACCEL_DATA_Z_LSB = 0x0C,
    ACCEL_DATA_Z_MSB = 0x0D,

    /* Magnetometer data register */
    MAG_DATA_X_LSB = 0x0E,
    MAG_DATA_X_MSB = 0x0F,
    MAG_DATA_Y_LSB = 0x10,
    MAG_DATA_Y_MSB = 0x11,
    MAG_DATA_Z_LSB = 0x12,
    MAG_DATA_Z_MSB = 0x13,

    /* Gyroscope data register */
    GYRO_DATA_X_LSB = 0x14,
    GYRO_DATA_X_MSB = 0x15,
    GYRO_DATA_Y_LSB = 0x16,
    GYRO_DATA_Y_MSB = 0x17,
    GYRO_DATA_Z_LSB = 0x18,
    GYRO_DATA_Z_MSB = 0x19,

    /* Euler data registers */
    EULER_H_LSB = 0x1A,
    EULER_H_MSB = 0x1B,
    EULER_P_LSB = 0x1C,
    EULER_P_MSB = 0x1D,
    EULER_R_LSB = 0x1E,
    EULER_R_MSB = 0x1F,

    /* Quaternion data registers */
    QUATERNION_DATA_W_LSB = 0x20,
    QUATERNION_DATA_W_MSB = 0x21,
    QUATERNION_DATA_X_LSB = 0x22,
    QUATERNION_DATA_X_MSB = 0x23,
    QUATERNION_DATA_Y_LSB = 0x24,
    QUATERNION_DATA_Y_MSB = 0x25,
    QUATERNION_DATA_Z_LSB = 0x26,
    QUATERNION_DATA_Z_MSB = 0x27,

    /* Linear acceleration data registers */
    LINEAR_ACCEL_DATA_X_LSB = 0x28,
    LINEAR_ACCEL_DATA_X_MSB = 0x29,
    LINEAR_ACCEL_DATA_Y_LSB = 0x2A,
    LINEAR_ACCEL_DATA_Y_MSB = 0x2B,
    LINEAR_ACCEL_DATA_Z_LSB = 0x2C,
    LINEAR_ACCEL_DATA_Z_MSB = 0x2D,

    /* Gravity vector data registers */
    GRAVITY_DATA_X_LSB = 0x2E,
    GRAVITY_DATA_X_MSB = 0x2F,
    GRAVITY_DATA_Y_LSB = 0x30,
    GRAVITY_DATA_Y_MSB = 0x31,
    GRAVITY_DATA_Z_LSB = 0x32,
    GRAVITY_DATA_Z_MSB = 0x33,

    /* Temperature data register */
    TEMPERATURE = 0x34,

    /* Status registers */
    CALIB_STATUS = 0x35,
    SELFTEST_RESULT = 0x36,
    INTR_STAT = 0x37,
    SYS_CLK_STAT = 0x38,
    SYS_STAT = 0x39,
    SYS_ERR = 0x3A,

    /* Unit selection register */
    UNIT_SEL = 0x3B,

    /* Mode registers */
    OPERATION_MODE = 0x3D,
    POWER_MODE = 0x3E,

    SYS_TRIGGER = 0x3F,
    TEMP_SOURCE = 0x40,

    /* Axis remap registers */
    AXIS_MAP_CONFIG = 0x41,
    AXIS_MAP_SIGN = 0x42,

    /* SIC registers */
    SIC_MATRIX_0_LSB = 0x43,
    SIC_MATRIX_0_MSB = 0x44,
    SIC_MATRIX_1_LSB = 0x45,
    SIC_MATRIX_1_MSB = 0x46,
    SIC_MATRIX_2_LSB = 0x47,
    SIC_MATRIX_2_MSB = 0x48,
    SIC_MATRIX_3_LSB = 0x49,
    SIC_MATRIX_3_MSB = 0x4A,
    SIC_MATRIX_4_LSB = 0x4B,
    SIC_MATRIX_4_MSB = 0x4C,
    SIC_MATRIX_5_LSB = 0x4D,
    SIC_MATRIX_5_MSB = 0x4E,
    SIC_MATRIX_6_LSB = 0x4F,
    SIC_MATRIX_6_MSB = 0x50,
    SIC_MATRIX_7_LSB = 0x51,
    SIC_MATRIX_7_MSB = 0x52,
    SIC_MATRIX_8_LSB = 0x53,
    SIC_MATRIX_8_MSB = 0x54,

    /* Acceleromter Offset registers */
    ACCEL_OFFSET_X_LSB = 0x55,
    ACCEL_OFFSET_X_MSB = 0x56,
    ACCEL_OFFSET_Y_LSB = 0x57,
    ACCEL_OFFSET_Y_MSB = 0x58,
    ACCEL_OFFSET_Z_LSB = 0x59,
    ACCEL_OFFSET_Z_MSB = 0x5A,

    /* Magnetometer Offset registers */
    MAG_OFFSET_X_LSB = 0x5B,
    MAG_OFFSET_X_MSB = 0x5C,
    MAG_OFFSET_Y_LSB = 0x5D,
    MAG_OFFSET_Y_MSB = 0x5E,
    MAG_OFFSET_Z_LSB = 0x5F,
    MAG_OFFSET_Z_MSB = 0x60,

    /* Gyroscope Offset registers */
    GYRO_OFFSET_X_LSB = 0x61,
    GYRO_OFFSET_X_MSB = 0x62,
    GYRO_OFFSET_Y_LSB = 0x63,
    GYRO_OFFSET_Y_MSB = 0x64,
    GYRO_OFFSET_Z_LSB = 0x65,
    GYRO_OFFSET_Z_MSB = 0x66,

    /* Radius registers */
    ACCEL_RADIUS_LSB = 0x67,
    ACCEL_RADIUS_MSB = 0x68,
    MAG_RADIUS_LSB = 0x69,
    MAG_RADIUS_MSB = 0x6A,
};

enum class Address : uint8_t {
    DEFAULT = 0x28, // Default I2C address
    ALTERNATE = 0x29 // Alternate address when SDO pin is high
};

enum class PowerMode : uint8_t {
    NORMAL = 0x00,
    LOW_POWER = 0x01,
    SUSPEND = 0x02,
};

// TODO: This seems wrong, check datasheet
enum class AxisRemap : uint8_t {
    CONFIG_P0 = 0x21,
    CONFIG_P1 = 0x24, // P1 is the default configuration
    CONFIG_P2 = 0x24,
    CONFIG_P3 = 0x21,
    CONFIG_P4 = 0x24,
    CONFIG_P5 = 0x21,
    CONFIG_P6 = 0x21,
    CONFIG_P7 = 0x24,
};

enum class AxisSign : uint8_t {
    SIGN_P0 = 0x04,
    SIGN_P1 = 0x00, // P1 is the default configuration
    SIGN_P2 = 0x06,
    SIGN_P3 = 0x02,
    SIGN_P4 = 0x03,
    SIGN_P5 = 0x01,
    SIGN_P6 = 0x07,
    SIGN_P7 = 0x05,
};

enum class OperationMode : uint8_t {
    CONFIG = 0x00, // Configuration mode
    ACCELEROMETER = 0x01, // Accelerometer only
    MAGNETOMETER = 0x02, // Magnetometer only
    GYROSCOPE = 0x03, // Gyroscope only
    ACCEL_MAG = 0x04, // Accelerometer + Magnetometer
    ACCEL_GYRO = 0x05, // Accelerometer + Gyroscope
    MAG_GYRO = 0x06, // Magnetometer + Gyroscope
    ACCEL_MAG_GYRO = 0x07, // Accelerometer + Magnetometer
    IMU_PLUS = 0x08, // IMU with additional features
    COMPASS = 0x09, // Compass mode
    M4G = 0x0A, // 4-DOF mode
    NDOF_FMC_OFF = 0x0B, // NDOF mode
    NDOF = 0x0C // NDOF mode with full sensor fusion
};

// TODO: Check these modes against the datasheet
enum class UnitSelection : uint8_t {
    METRIC = 0x00, // Metric units (m/s^2, degrees)
    US = 0x01      // Imperial units (in/s^2, degrees)
};

struct Settings {
    PowerMode power_mode = PowerMode::NORMAL;
    OperationMode operation_mode = OperationMode::NDOF;
    UnitSelection unit_selection = UnitSelection::METRIC;
    uint8_t axis_map_config = 0x00; // Default axis mapping
    uint8_t axis_map_sign = 0x00;   // Default axis sign
    uint8_t temp_source = 0x00;     // Default temperature source
};

template<typename BusType>
class BNO55 : public Sensor<BusType>, public IMU9Interface<BNO55<BusType>> {
public:
    // I2C constructor
    template<typename T = BusType>
    BNO55(typename std::enable_if_t<std::is_same_v<T, I2C>, i2c_inst_t*> i2c, 
          Address addr = Address::DEFAULT) 
        : Sensor<BusType>(i2c, static_cast<uint8_t>(addr)), address(addr) {}

    // SPI constructor
    template<typename T = BusType>
    BNO55(typename std::enable_if_t<std::is_same_v<T, SPI>, spi_inst_t*> spi, 
          uint cs_pin) 
        : Sensor<BusType>(spi, cs_pin), address(Address::DEFAULT) {}

    // Initialize the sensor
    bool init(const Settings& settings = Settings()) {

        // Wait for 850ms after power-up (FreeRTOS compatible)
        vTaskDelay(pdMS_TO_TICKS(850));

        // Use the standardized device ID verification from Sensor base class
        if (!this->verify_device_id(Register::CHIP_ID, 0xA0, "BNO55")) {
            return false;
        }

        // Switch to configuration mode
        if (!set_operation_mode(OperationMode::CONFIG)) {
            printf("BNO55: Failed to set operation mode to CONFIG\n");
            return false;
        }

        // Reset the sensor
        if constexpr (std::is_same_v<BusType, I2C>) {
            write_register_i2c(Register::SYS_TRIGGER, 0x20); // Trigger software reset
        } else if constexpr (std::is_same_v<BusType, SPI>) {
            write_register_spi(Register::SYS_TRIGGER, 0x20); // Trigger software reset
        }
        vTaskDelay(pdMS_TO_TICKS(30)); // Wait for reset to complete
        while(true) {
            // Check if the reset was successful by reading the CHIP_ID register
            // This is a blocking check, so we can use a local variable to avoid multiple reads
            uint8_t chip_id = 0;
            if constexpr (std::is_same_v<BusType, I2C>) {
                this->read_registers_i2c(Register::CHIP_ID, &chip_id, 1);
            } else if constexpr (std::is_same_v<BusType, SPI>) {
                this->read_registers_spi(Register::CHIP_ID, &chip_id, 1);
            }
            if (chip_id == static_cast<uint8_t>(Register::BNO55_ID)) {
                printf("BNO55: Reset successful, CHIP_ID = 0x%02X\n", chip_id);
                break; // Exit loop if reset was successful
            } else {
                printf("BNO55: Reset failed, CHIP_ID = 0x%02X\n", chip_id);
                vTaskDelay(pdMS_TO_TICKS(10)); // Wait before retrying
            }
        }
        vTaskDelay(pdMS_TO_TICKS(50)); // Wait for sensor to stabilize

        // Set to normal power mode
        if (!set_power_mode(PowerMode::NORMAL)) {
            printf("BNO55: Failed to set power mode\n");
            return false;
        }
        vTaskDelay(pdMS_TO_TICKS(10));

        // TODO: Figure out why adafruit sets page ID to 0
        if constexpr (std::is_same_v<BusType, I2C>) {
            if (!write_register_i2c(Register::PAGE_ID_ADDR, 0x00)) {
                printf("BNO55: Failed to set page ID to 0\n");
                return false;
            }
        } else if constexpr (std::is_same_v<BusType, SPI>) {
            if (!write_register_spi(Register::PAGE_ID_ADDR, 0x00)) {
                printf("BNO55: Failed to set page ID to 0\n");
                return false;
            }
        }

        // TODO: Figure out why adafruit sets SYS_TRIGGER to 0
        if constexpr (std::is_same_v<BusType, I2C>) {
            if (!write_register_i2c(Register::SYS_TRIGGER, 0x00)) {
                printf("BNO55: Failed to set SYS_TRIGGER to 0\n");
                return false;
            }
        } else if constexpr (std::is_same_v<BusType, SPI>) {
            if (!write_register_spi(Register::SYS_TRIGGER, 0x00)) {
                printf("BNO55: Failed to set SYS_TRIGGER to 0\n");
                return false;
            }
        }
        vTaskDelay(pdMS_TO_TICKS(10));

        // Set operation mode
        if (!set_operation_mode(settings.operation_mode)) {
            printf("BNO55: Failed to set operation mode\n");
            return false;
        }
        vTaskDelay(pdMS_TO_TICKS(20));

        printf("BNO55 initialized successfully\n");
        return true;
    }

    // Read accelerometer data
    AccelData read_accel() {
        AccelData data;
        uint8_t buffer[6];
        if constexpr (std::is_same_v<BusType, I2C>) {
            if (!read_registers_i2c(Register::ACCEL_DATA_X_LSB, buffer, 6)) {
                printf("BNO55: Failed to read accelerometer data via I2C\n");
                return {0.0f, 0.0f, 0.0f};
            }
        } else if constexpr (std::is_same_v<BusType, SPI>) {
            if (!read_registers_spi(Register::ACCEL_DATA_X_LSB, buffer, 6)) {
                printf("BNO55: Failed to read accelerometer data via SPI\n");
                return {0.0f, 0.0f, 0.0f};
            }
        }
        // Convert little-endian 16-bit values
        data.x = static_cast<float>(static_cast<int16_t>(buffer[0] | (buffer[1] << 8))) * 0.01f; // Convert to m/s^2
        data.y = static_cast<float>(static_cast<int16_t>(buffer[2] | (buffer[3] << 8))) * 0.01f; // Convert to m/s^2
        data.z = static_cast<float>(static_cast<int16_t>(buffer[4] | (buffer[5] << 8))) * 0.01f; // Convert to m/s^2
        return data;
    }

    // Read gyroscope data
    GyroData read_gyro() {
        GyroData data;
        uint8_t buffer[6];
        if constexpr (std::is_same_v<BusType, I2C>) {
            if (!this->read_registers_i2c(Register::GYRO_DATA_X_LSB, buffer, 6)) {
                printf("BNO55: Failed to read gyroscope data via I2C\n");
                return {0.0f, 0.0f, 0.0f};
            }
        } else if constexpr (std::is_same_v<BusType, SPI>) {
            if (!this->read_registers_spi(Register::GYRO_DATA_X_LSB, buffer, 6)) {
                printf("BNO55: Failed to read gyroscope data via SPI\n");
                return {0.0f, 0.0f, 0.0f};
            }
        }
        // Convert little-endian 16-bit values
        data.x = static_cast<float>(static_cast<int16_t>(buffer[0] | (buffer[1] << 8))) * 0.01f; // Convert to rad/s
        data.y = static_cast<float>(static_cast<int16_t>(buffer[2] | (buffer[3] << 8))) * 0.01f; // Convert to rad/s
        data.z = static_cast<float>(static_cast<int16_t>(buffer[4] | (buffer[5] << 8))) * 0.01f; // Convert to rad/s
        return data;
    }

    // Read magnetometer data
    MagData read_mag() {
        MagData data;
        uint8_t buffer[6];
        if constexpr (std::is_same_v<BusType, I2C>) {
            if (!this->read_registers_i2c(Register::MAG_DATA_X_LSB, buffer, 6)) {
                printf("BNO55: Failed to read magnetometer data via I2C\n");
                return {0.0f, 0.0f, 0.0f};
            }
        } else if constexpr (std::is_same_v<BusType, SPI>) {
            if (!this->read_registers_spi(Register::MAG_DATA_X_LSB, buffer, 6)) {
                printf("BNO55: Failed to read magnetometer data via SPI\n");
                return {0.0f, 0.0f, 0.0f};
            }
        }
        // Convert little-endian 16-bit values
        data.x = static_cast<float>(static_cast<int16_t>(buffer[0] | (buffer[1] << 8))) * 0.01f; // Convert to uT
        data.y = static_cast<float>(static_cast<int16_t>(buffer[2] | (buffer[3] << 8))) * 0.01f; // Convert to uT
        data.z = static_cast<float>(static_cast<int16_t>(buffer[4] | (buffer[5] << 8))) * 0.01f; // Convert to uT
        return data;
    }

    // Read temperature
    float read_temperature() {
        uint8_t temp;
        if constexpr (std::is_same_v<BusType, I2C>) {
            if (!this->read_registers_i2c(Register::TEMPERATURE, &temp, 1)) {
                printf("BNO55: Failed to read temperature via I2C\n");
                return 0.0f;
            }
        } else if constexpr (std::is_same_v<BusType, SPI>) {
            if (!this->read_registers_spi(Register::TEMPERATURE, &temp, 1)) {
                printf("BNO55: Failed to read temperature via SPI\n");
                return 0.0f;
            }
        }
        // Convert to Celsius?
        return static_cast<float>(temp);
    }

    // Read Euler angles
    RollPitchYaw read_euler() {
        uint8_t buffer[6];
        if constexpr (std::is_same_v<BusType, I2C>) {
            if (!this->read_registers_i2c(Register::EULER_H_LSB, buffer, 6)) {
                printf("BNO55: Failed to read Euler angles via I2C\n");
                return RollPitchYaw();
            }
        } else if constexpr (std::is_same_v<BusType, SPI>) {
            if (!this->read_registers_spi(Register::EULER_H_LSB, buffer, 6)) {
                printf("BNO55: Failed to read Euler angles via SPI\n");
                return RollPitchYaw();
            }
        }
        // Convert little-endian 16-bit values
        RollPitchYaw euler;
        euler.roll = static_cast<float>(static_cast<int16_t>(buffer[0] | (buffer[1] << 8))) * 0.01f; // Convert to
        euler.pitch = static_cast<float>(static_cast<int16_t>(buffer[2] | (buffer[3] << 8))) * 0.01f; // Convert to degrees
        euler.yaw = static_cast<float>(static_cast<int16_t>(buffer[4] | (buffer[5] << 8))) * 0.01f; // Convert to degrees
        return euler;
    }

    // Read quaternion data
    Quaternion read_quaternion() {
        uint8_t buffer[8];
        if constexpr (std::is_same_v<BusType, I2C>) {
            if (!this->read_registers_i2c(Register::QUATERNION_DATA_W_LSB, buffer, 8)) {
                printf("BNO55: Failed to read quaternion data via I2C\n");
                return Quaternion();
            }
        } else if constexpr (std::is_same_v<BusType, SPI>) {
            if (!this->read_registers_spi(Register::QUATERNION_DATA_W_LSB, buffer, 8)) {
                printf("BNO55: Failed to read quaternion data via SPI\n");
                return Quaternion();
            }
        }
        // Convert little-endian 16-bit values
        float w = static_cast<float>(static_cast<int16_t>(buffer[0] | (buffer[1] << 8))) * 0.01f; // Convert to unit quaternion
        float x = static_cast<float>(static_cast<int16_t>(buffer[2] | (buffer[3] << 8))) * 0.01f; // Convert to unit quaternion
        float y = static_cast<float>(static_cast<int16_t>(buffer[4] | (buffer[5] << 8))) * 0.01f; // Convert to unit quaternion
        float z = static_cast<float>(static_cast<int16_t>(buffer[6] | (buffer[7] << 8))) * 0.01f; // Convert to unit quaternion
        return Quaternion(w, x, y, z);
    }

    // Read linear acceleration data
    AccelData read_linear_accel() {
        AccelData data;
        uint8_t buffer[6];
        if constexpr (std::is_same_v<BusType, I2C>) {
            if (!this->read_registers_i2c(Register::LINEAR_ACCEL_DATA_X_LSB, buffer, 6)) {
                printf("BNO55: Failed to read linear acceleration data via I2C\n");
                return {0.0f, 0.0f, 0.0f};
            }
        } else if constexpr (std::is_same_v<BusType, SPI>) {
            if (!this->read_registers_spi(Register::LINEAR_ACCEL_DATA_X_LSB, buffer, 6)) {
                printf("BNO55: Failed to read linear acceleration data via SPI\n");
                return {0.0f, 0.0f, 0.0f};
            }
        }
        // Convert little-endian 16-bit values
        data.x = static_cast<float>(static_cast<int16_t>(buffer[0] | (buffer[1] << 8))) * 0.01f; // Convert to m/s^2
        data.y = static_cast<float>(static_cast<int16_t>(buffer[2] | (buffer[3] << 8))) * 0.01f; // Convert to m/s^2
        data.z = static_cast<float>(static_cast<int16_t>(buffer[4] | (buffer[5] << 8))) * 0.01f; // Convert to m/s^2
        return data;
    }

    // Read gravity vector data
    AccelData read_gravity() {
        AccelData data;
        uint8_t buffer[6];
        if constexpr (std::is_same_v<BusType, I2C>) {
            if (!this->read_registers_i2c(Register::GRAVITY_DATA_X_LSB, buffer, 6)) {
                printf("BNO55: Failed to read gravity data via I2C\n");
                return {0.0f, 0.0f, 0.0f};
            }
        } else if constexpr (std::is_same_v<BusType, SPI>) {
            if (!this->read_registers_spi(Register::GRAVITY_DATA_X_LSB, buffer, 6)) {
                printf("BNO55: Failed to read gravity data via SPI\n");
                return {0.0f, 0.0f, 0.0f};
            }
        }
        // Convert little-endian 16-bit values
        data.x = static_cast<float>(static_cast<int16_t>(buffer[0] | (buffer[1] << 8))) * 0.01f; // Convert to m/s^2
        data.y = static_cast<float>(static_cast<int16_t>(buffer[2] | (buffer[3] << 8))) * 0.01f; // Convert to m/s^2
        data.z = static_cast<float>(static_cast<int16_t>(buffer[4] | (buffer[5] << 8))) * 0.01f; // Convert to m/s^2
        return data;
    }

    // IMU9Interface implementation - read all 9-axis sensor data
    IMU9Data read() {
        IMU9Data data;
        data.accel = read_accel();
        data.gyro = read_gyro();
        data.mag = read_mag();
        return data;
    }

    // Set power mode
    bool set_power_mode(PowerMode mode) {
        uint8_t value = static_cast<uint8_t>(mode);
        if constexpr (std::is_same_v<BusType, I2C>) {
            return this->write_register_i2c(Register::POWER_MODE, value);
        } else if constexpr (std::is_same_v<BusType, SPI>) {
            return this->write_register_spi(Register::POWER_MODE, value);
        }
        return false;
    }

    // Set operation mode
    // TODO: CHECK THIS
    bool set_operation_mode(OperationMode mode) {
        uint8_t value = static_cast<uint8_t>(mode);
        if constexpr (std::is_same_v<BusType, I2C>) {
            return this->write_register_i2c(Register::OPERATION_MODE, value);
        } else if constexpr (std::is_same_v<BusType, SPI>) {
            return this->write_register_spi(Register::OPERATION_MODE, value);
        }
        return false;
    }

    // Set unit selection
    bool set_unit_selection(UnitSelection unit) {
        uint8_t value = static_cast<uint8_t>(unit);
        if constexpr (std::is_same_v<BusType, I2C>) {
            return this->write_register_i2c(Register::UNIT_SEL, value);
        } else if constexpr (std::is_same_v<BusType, SPI>) {
            return this->write_register_spi(Register::UNIT_SEL, value);
        }
        return false;
    }

    // TODO: Implement axis mapping
    
    // TODO: Implement axis sign mapping

    // TODO: Implement temperature source setting

    // TODO: Implement self-test

    // TODO: Implement calibration status reading

    // TODO: Implement interrupt status reading

    // TODO: Implement system clock status reading

    // TODO: Implement system status reading

    // TODO: Implement system error reading

    // TODO: Implement SIC (Soft Iron Calibration)

    // TODO: Implement accelerometer offset setting

    // TODO: Implement magnetometer offset setting

    // TODO: Implement gyroscope offset setting

    // TODO: Implement accelerometer radius setting

    // TODO: Implement magnetometer radius setting

    // TODO: Implement other features as needed

private:
    Address address; // I2C address or SPI chip select pin

    // I2C register write
    bool write_register_i2c(Register reg, uint8_t value) {
        uint8_t data[2] = {static_cast<uint8_t>(reg), value};
        int result = i2c_write_blocking(this->i2c, static_cast<uint8_t>(address), data, 2, false);
        return result == 2;
    }
    
    // I2C register read
    bool read_registers_i2c(Register reg, uint8_t* buffer, size_t length) {
        uint8_t reg_addr = static_cast<uint8_t>(reg);
        int result = i2c_write_blocking(this->i2c, static_cast<uint8_t>(address), &reg_addr, 1, true);
        if (result != 1) return false;
        
        result = i2c_read_blocking(this->i2c, static_cast<uint8_t>(address), buffer, length, false);
        return result == static_cast<int>(length);
    }
    
    // SPI register write
    bool write_register_spi(Register reg, uint8_t value) {
        gpio_put(this->cs_pin, 0); // Assert CS
        
        uint8_t reg_addr = static_cast<uint8_t>(reg); // Write bit is 0 (default)
        spi_write_blocking(this->spi, &reg_addr, 1);
        spi_write_blocking(this->spi, &value, 1);
        
        gpio_put(this->cs_pin, 1); // Deassert CS
        return true;
    }
    
    // SPI register read  
    bool read_registers_spi(Register reg, uint8_t* buffer, size_t length) {
        gpio_put(this->cs_pin, 0); // Assert CS
        
        uint8_t reg_addr = static_cast<uint8_t>(reg) | 0x80; // Set read bit (bit 7)
        if (length > 1) {
            reg_addr |= 0x40; // Set multi-byte bit (bit 6) for burst reads
        }
        
        spi_write_blocking(this->spi, &reg_addr, 1);
        spi_read_blocking(this->spi, 0, buffer, length);
        
        gpio_put(this->cs_pin, 1); // Deassert CS
        return true;
    }
};

}