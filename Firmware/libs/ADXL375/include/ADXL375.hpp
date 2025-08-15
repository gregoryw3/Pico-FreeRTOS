#pragma once

#include "../../include/Sensor.hpp"
#include "hardware/i2c.h"
#include "hardware/spi.h"
#include "hardware/gpio.h"
#include "FreeRTOS.h"
#include "task.h"

namespace ADXL375 {

// ADXL375 Register addresses
enum class Register : uint8_t {
    DEVID = 0x00,
    THRESH_TAP = 0x1D,
    OFSX = 0x1E,
    OFSY = 0x1F,
    OFSZ = 0x20,
    DUR = 0x21,
    LATENT = 0x22,
    WINDOW = 0x23,
    THRESH_ACT = 0x24,
    THRESH_INACT = 0x25,
    TIME_INACT = 0x26,
    ACT_INACT_CTL = 0x27,
    THRESH_FF = 0x28,
    TIME_FF = 0x29,
    TAP_AXES = 0x2A,
    ACT_TAP_STATUS = 0x2B,
    BW_RATE = 0x2C,
    POWER_CTL = 0x2D,
    INT_ENABLE = 0x2E,
    INT_MAP = 0x2F,
    INT_SOURCE = 0x30,
    DATA_FORMAT = 0x31,
    DATAX0 = 0x32,
    DATAX1 = 0x33,
    DATAY0 = 0x34,
    DATAY1 = 0x35,
    DATAZ0 = 0x36,
    DATAZ1 = 0x37,
    FIFO_CTL = 0x38,
    FIFO_STATUS = 0x39,
};

// ADXL375 I2C addresses
enum class Address : uint8_t {
    DEFAULT = 0x1D,
    ALTERNATE = 0x53  // When SDO/ALT_ADDRESS pin is high
};

// Bandwidth settings
enum class BandWidth : uint8_t {
    HZ_3200 = 0b1111,   // 3200 Hz
    HZ_1600 = 0b1110,   // 1600 Hz
    HZ_800 = 0b1101,    // 800 Hz
    HZ_400 = 0b1100,    // 400 Hz
    HZ_200 = 0b1011,    // 200 Hz
    HZ_100 = 0b1010,    // 100 Hz
    HZ_50 = 0b1001,     // 50 Hz
    HZ_25 = 0b1000,     // 25 Hz
    HZ_12_5 = 0b0111,   // 12.5 Hz
    HZ_6_25 = 0b0110,   // 6.25 Hz
    HZ_3_13 = 0b0101,   // 3.13 Hz
    HZ_1_56 = 0b0100,   // 1.56 Hz
    HZ_0_78 = 0b0011,   // 0.78 Hz
    HZ_0_39 = 0b0010,   // 0.39 Hz
    HZ_0_20 = 0b0001,   // 0.20 Hz
    HZ_0_10 = 0b0000,   // 0.10 Hz
};

// Power modes
enum class PowerMode : uint8_t {
    POWER_OFF = 0x00,
    STANDBY = 0x00,
    MEASUREMENT = 0x08  // Bit D3 in POWER_CTL register
};

// DATA_FORMAT register bit definitions (Register 0x31)
enum class DataFormatBits : uint8_t {
    SELF_TEST = 0x80,     // D7: Self-test force
    SPI_3_WIRE = 0x40,    // D6: 3-wire SPI mode (1) vs 4-wire (0)
    INT_INVERT = 0x20,    // D5: Interrupt polarity (1=active low, 0=active high)
    // D4 = 0 (reserved)
    // D3 = 1 (fixed)
    JUSTIFY = 0x04,       // D2: Left justified (1) vs right justified (0)
    // D1 = 1 (fixed)
    // D0 = 1 (fixed)
};

// SPI mode configuration
enum class SPIMode : uint8_t {
    FOUR_WIRE = 0,  // Standard 4-wire SPI (MOSI, MISO, SCK, CS)
    THREE_WIRE = 1  // 3-wire SPI (combined MOSI/MISO line)
};

// Data justification mode
enum class DataJustification : uint8_t {
    RIGHT_JUSTIFIED = 0,  // LSB mode with sign extension
    LEFT_JUSTIFIED = 1    // MSB mode
};

// Interrupt polarity
enum class InterruptPolarity : uint8_t {
    ACTIVE_HIGH = 0,
    ACTIVE_LOW = 1
};

constexpr float ADXL375_MG2G_MULTIPLIER = 0.049f; // 49 mg/LSB
constexpr float SENSORS_GRAVITY_EARTH = 9.80665f; // m/s^2

struct Settings {
    BandWidth bandwidth = BandWidth::HZ_100;
    PowerMode power_mode = PowerMode::MEASUREMENT;
    DataJustification justification = DataJustification::RIGHT_JUSTIFIED;
    InterruptPolarity int_polarity = InterruptPolarity::ACTIVE_HIGH;
    bool self_test = false;
    SPIMode spi_mode = SPIMode::FOUR_WIRE;
};

template<typename BusType>
class ADXL375 : public Sensor<BusType>, public AccelerometerInterface<ADXL375<BusType>> {
public:
    // I2C constructor
    template<typename T = BusType>
    ADXL375(typename std::enable_if_t<std::is_same_v<T, I2C>, i2c_inst_t*> i2c, 
            Address addr = Address::DEFAULT) 
        : Sensor<BusType>(i2c, static_cast<uint8_t>(addr)), address(addr) {}
    
    // SPI constructor  
    template<typename T = BusType>
    ADXL375(typename std::enable_if_t<std::is_same_v<T, SPI>, spi_inst_t*> spi, 
            uint cs_pin) 
        : Sensor<BusType>(spi, cs_pin), address(Address::DEFAULT) {}
    
    // // Required by AccelerometerInterface (CRTP)
    // AccelData read_impl() {
    //     return read_accel();
    // }
    
    // void start_impl() {
    //     if (!initialized) {
    //         // Initialize the sensor
    //         if (init_sensor()) {
    //             initialized = true;
    //             printf("ADXL375 started successfully\n");
    //         } else {
    //             printf("ADXL375 initialization failed\n");
    //         }
    //     }
    // }
    
    // void stop_impl() {
    //     if (initialized) {
    //         set_power_mode(PowerMode::POWER_OFF);
    //         initialized = false;
    //         printf("ADXL375 stopped\n");
    //     }
    // }

    bool init(const Settings& settings = Settings()) {
        // Use the standardized device ID verification from Sensor base class
        if (!this->verify_device_id(Register::DEVID, 0xE5, "ADXL375")) {
            return false;
        }
    
        // Set bandwidth
        if (!set_bandwidth(settings.bandwidth)) {
            printf("ADXL375: Failed to set bandwidth\n");
            return false;
        }
        
        // Set power mode
        if (!set_power_mode(settings.power_mode)) {
            printf("ADXL375: Failed to set power mode\n");
            return false;
        }
        
        // Set data format
        // if (!set_data_format(settings.self_test, settings.spi_mode, settings.int_polarity, settings.justification)) {
        //     printf("ADXL375: Failed to set data format\n");
        //     return false;
        // }
        
        // Mark as initialized
        initialized = true;
        printf("ADXL375 initialized successfully\n");
        return true;
    }
    
    // Specific accelerometer method
    AccelData read_accel() {
        if (!initialized) {
            printf("Warning: ADXL375 not initialized\n");
            return {0.0f, 0.0f, 0.0f};
        }
        
        uint8_t buffer[6];
        
        // Read all 6 bytes at once as recommended to prevent data changes between reads
        if constexpr (std::is_same_v<BusType, I2C>) {
            if (!read_registers_i2c(Register::DATAX0, buffer, 6)) {
                printf("ADXL375: Failed to read acceleration data via I2C\n");
                return {0.0f, 0.0f, 0.0f};
            }
        } else if constexpr (std::is_same_v<BusType, SPI>) {
            if (!read_registers_spi(Register::DATAX0, buffer, 6)) {
                printf("ADXL375: Failed to read acceleration data via SPI\n");
                return {0.0f, 0.0f, 0.0f};
            }
        }
        
        // Convert little-endian 16-bit values (DATAx0 is LSB, DATAx1 is MSB)
        int16_t raw_x = static_cast<int16_t>(buffer[0] | (buffer[1] << 8));
        int16_t raw_y = static_cast<int16_t>(buffer[2] | (buffer[3] << 8));
        int16_t raw_z = static_cast<int16_t>(buffer[4] | (buffer[5] << 8));
        
        // Handle data justification based on current format
        if (data_justification == DataJustification::LEFT_JUSTIFIED) {
            // In left-justified mode, shift data appropriately
            raw_x = raw_x >> 6;  // Shift MSB data to proper position
            raw_y = raw_y >> 6;
            raw_z = raw_z >> 6;
        }
        
        // For 3200Hz and 1600Hz rates, LSB is always 0, so we can check for this
        if (current_bandwidth == BandWidth::HZ_3200 || current_bandwidth == BandWidth::HZ_1600) {
            // LSB should be 0 for these high data rates
            if ((raw_x & 0x01) || (raw_y & 0x01) || (raw_z & 0x01)) {
                printf("ADXL375: Warning - LSB not zero at high data rate\n");
            }
        }
        
        // Convert to m/s^2 using the scaling factor
        AccelData data;
        data.x = (static_cast<float>(raw_x) * ADXL375_MG2G_MULTIPLIER) * SENSORS_GRAVITY_EARTH;
        data.y = (static_cast<float>(raw_y) * ADXL375_MG2G_MULTIPLIER) * SENSORS_GRAVITY_EARTH;
        data.z = (static_cast<float>(raw_z) * ADXL375_MG2G_MULTIPLIER) * SENSORS_GRAVITY_EARTH;
        
        return data;
    }
    
    // Set bandwidth
    bool set_bandwidth(BandWidth bw) {
        bool success = write_register(Register::BW_RATE, static_cast<uint8_t>(bw));
        if (success) {
            current_bandwidth = bw;
        }
        return success;
    }
    
    // Set power mode
    bool set_power_mode(PowerMode mode) {
        return write_register(Register::POWER_CTL, static_cast<uint8_t>(mode));
    }
    
    // Set data format with configurable options
    bool set_data_format(bool self_test = false, 
                        SPIMode spi_mode = SPIMode::FOUR_WIRE,
                        InterruptPolarity int_polarity = InterruptPolarity::ACTIVE_HIGH,
                        DataJustification justification = DataJustification::RIGHT_JUSTIFIED) {
        
        uint8_t format_value = 0b00001011; // Base value: D4=0, D3=1, D1=1, D0=1 (fixed bits)
        
        if (self_test) {
            format_value |= static_cast<uint8_t>(DataFormatBits::SELF_TEST);
        }
        
        if constexpr (std::is_same_v<BusType, SPI>) {
            if (spi_mode == SPIMode::THREE_WIRE) {
                format_value |= static_cast<uint8_t>(DataFormatBits::SPI_3_WIRE);
            }
        }
        
        if (int_polarity == InterruptPolarity::ACTIVE_LOW) {
            format_value |= static_cast<uint8_t>(DataFormatBits::INT_INVERT);
        }
        
        if (justification == DataJustification::LEFT_JUSTIFIED) {
            format_value |= static_cast<uint8_t>(DataFormatBits::JUSTIFY);
        }
        
        bool success = write_register(Register::DATA_FORMAT, format_value);
        if (success) {
            data_justification = justification;
            vTaskDelay(pdMS_TO_TICKS(10)); // 10ms delay (FreeRTOS compatible)
        }
        return success;
    }
    
    // Simplified data format setter (backward compatibility)
    bool set_data_format() {
        return set_data_format(false, SPIMode::FOUR_WIRE, InterruptPolarity::ACTIVE_HIGH, DataJustification::RIGHT_JUSTIFIED);
    }
    
    // Enable/disable self-test
    bool set_self_test(bool enable) {
        return set_data_format(enable, SPIMode::FOUR_WIRE, InterruptPolarity::ACTIVE_HIGH, data_justification);
    }
    
    // Read raw acceleration data (before conversion to m/s^2)
    bool read_raw_accel(int16_t& x, int16_t& y, int16_t& z) {
        if (!initialized) {
            return false;
        }
        
        uint8_t buffer[6];
        
        // Read all 6 bytes at once as recommended
        bool success = false;
        if constexpr (std::is_same_v<BusType, I2C>) {
            success = read_registers_i2c(Register::DATAX0, buffer, 6);
        } else if constexpr (std::is_same_v<BusType, SPI>) {
            success = read_registers_spi(Register::DATAX0, buffer, 6);
        }
        
        if (!success) {
            return false;
        }
        
        // Convert little-endian 16-bit values
        x = static_cast<int16_t>(buffer[0] | (buffer[1] << 8));
        y = static_cast<int16_t>(buffer[2] | (buffer[3] << 8));
        z = static_cast<int16_t>(buffer[4] | (buffer[5] << 8));
        
        // Handle data justification
        if (data_justification == DataJustification::LEFT_JUSTIFIED) {
            x = x >> 6;  // Shift MSB data to proper position
            y = y >> 6;
            z = z >> 6;
        }
        
        return true;
    }

private:
    Address address;
    bool initialized = false;
    DataJustification data_justification = DataJustification::RIGHT_JUSTIFIED;
    BandWidth current_bandwidth = BandWidth::HZ_100;
    
    // Write a single register
    bool write_register(Register reg, uint8_t value) {
        if constexpr (std::is_same_v<BusType, I2C>) {
            return write_register_i2c(reg, value);
        } else if constexpr (std::is_same_v<BusType, SPI>) {
            return write_register_spi(reg, value);
        }
        return false;
    }
    
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

    // Compile-time check: only allow I2C or SPI
    static_assert(
        std::is_same<BusType, I2C>::value || std::is_same<BusType, SPI>::value,
        "ADXL375 only supports I2C or SPI"
    );
};

} // namespace ADXL375
