---
title: Sensor Overview
description: Comprehensive guide to the sensor ecosystem in Pico-FreeRTOS
---

The Pico-FreeRTOS library provides a rich ecosystem of sensor drivers designed for high-performance embedded applications. Each sensor library is built with FreeRTOS integration, thread-safety, and real-time performance in mind.

## Supported Sensors

### High-G Accelerometers

#### ADXL375 - ±200g 3-Axis Accelerometer

**Applications**: Rocket flights, impact detection, crash testing, high-shock environments

**Key Features**:

- Measurement range: ±200g
- Resolution: 49 mg/LSB
- Interface: I2C/SPI
- Shock survival: 10,000g
- Built-in FIFO buffer

**Typical Use Cases**:

```cpp
#include "ADXL375.hpp"

ADXL375::Accelerometer accel(i2c0, ADXL375_I2C_ADDRESS);

void high_g_monitoring_task(void *pvParameters) {
    ADXL375::AccelData data;
    
    while (1) {
        if (accel.readAcceleration(data)) {
            if (data.magnitude > 50.0) { // 50g threshold
                printf("High-G event detected: %.2fg\n", data.magnitude);
                // Trigger data logging or emergency protocols
            }
        }
        vTaskDelay(pdMS_TO_TICKS(10)); // 100 Hz sampling
    }
}
```

### Environmental Sensors

#### BMP390 - Precision Barometric Pressure Sensor

**Applications**: Altitude measurement, weather monitoring, flight computers

**Key Features**:

- Pressure range: 300-1250 hPa
- Accuracy: ±0.03 hPa (±0.25m altitude)
- Temperature compensation
- Low power consumption
- Integrated temperature sensor

**Example Implementation**:

```cpp
#include "BMP390.hpp"

BMP390::Sensor barometer(i2c1, BMP390_I2C_ADDRESS);

void altitude_task(void *pvParameters) {
    float pressure, temperature, altitude;
    
    barometer.setOversamplingPressure(BMP390::OVERSAMPLING_X8);
    barometer.setOversamplingTemperature(BMP390::OVERSAMPLING_X2);
    
    while (1) {
        if (barometer.readData(pressure, temperature)) {
            altitude = barometer.pressureToAltitude(pressure);
            printf("Altitude: %.2f m, Pressure: %.2f hPa\n", altitude, pressure);
        }
        vTaskDelay(pdMS_TO_TICKS(100)); // 10 Hz updates
    }
}
```

### Inertial Measurement Units (IMUs)

#### BNO55 - 9-DOF Absolute Orientation Sensor

**Applications**: Drone stabilization, robotics, navigation systems

**Key Features**:

- 9-DOF sensor fusion (accel + gyro + magnetometer)
- Built-in sensor fusion processor
- Quaternion and Euler angle output
- Calibration status indicators
- Multiple output modes

#### ISM330DHCX - 6-Axis IMU with Machine Learning

**Applications**: Activity recognition, gesture detection, advanced motion sensing

**Key Features**:

- 6-DOF (3-axis accelerometer + 3-axis gyroscope)
- Machine Learning Core (MLC)
- Finite State Machine (FSM)  
- Anti-aliasing filter
- Timestamp functionality

#### LSM6DSO32 - High-Performance 6-Axis IMU

**Applications**: High-acceleration environments, sports analytics, impact detection

**Key Features**:

- Extended accelerometer range: ±32g
- High-performance gyroscope: ±4000 dps
- Advanced digital filtering
- FIFO with compression
- Programmable interrupts

## Sensor Integration Architecture

```text
┌─────────────────────────────────────────────────────────────┐
│                    Application Layer                        │
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐          │
│  │   Sensor    │  │   Sensor    │  │     GPS     │          │
│  │   Fusion    │  │  Logging    │  │ Integration │          │
│  └─────────────┘  └─────────────┘  └─────────────┘          │
└─────────────────────────────────────────────────────────────┘
              │                │                │
┌─────────────────────────────────────────────────────────────┐
│                  Sensor Abstraction Layer                   │
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐          │
│  │   ADXL375   │  │   BMP390    │  │   BNO55     │          │
│  │   Driver    │  │   Driver    │  │   Driver    │          │
│  └─────────────┘  └─────────────┘  └─────────────┘          │
└─────────────────────────────────────────────────────────────┘
              │                │                │
┌─────────────────────────────────────────────────────────────┐
│                 Hardware Abstraction Layer                  │
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐          │
│  │     I2C     │  │     SPI     │  │    UART     │          │
│  │   Manager   │  │   Manager   │  │   Manager   │          │
│  └─────────────┘  └─────────────┘  └─────────────┘          │
└─────────────────────────────────────────────────────────────┘
```

## Common Sensor Patterns

### Thread-Safe Sensor Access

All sensor drivers are designed for FreeRTOS environments:

```cpp
class SensorManager {
private:
    SemaphoreHandle_t sensor_mutex;
    
public:
    SensorManager() {
        sensor_mutex = xSemaphoreCreateMutex();
    }
    
    bool readSensorSafe(SensorData& data) {
        if (xSemaphoreTake(sensor_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
            bool result = sensor.readData(data);
            xSemaphoreGive(sensor_mutex);
            return result;
        }
        return false;
    }
};
```

### Multi-Sensor Data Fusion

Combine multiple sensors for enhanced accuracy:

```cpp
struct FusedSensorData {
    float acceleration[3];    // From ADXL375
    float angular_velocity[3]; // From LSM6DSO32
    float magnetic_field[3];   // From BNO55
    float pressure;           // From BMP390
    float altitude;           // Calculated
    uint32_t timestamp;       // System timestamp
};

void sensor_fusion_task(void *pvParameters) {
    FusedSensorData fused_data;
    
    while (1) {
        // Read from all sensors
        fused_data.timestamp = xTaskGetTickCount();
        
        adxl375.readAcceleration(fused_data.acceleration);
        lsm6dso32.readGyroscope(fused_data.angular_velocity);
        bno55.readMagnetometer(fused_data.magnetic_field);
        bmp390.readPressure(fused_data.pressure);
        
        // Calculate derived values
        fused_data.altitude = bmp390.pressureToAltitude(fused_data.pressure);
        
        // Apply sensor fusion algorithms
        kalman_filter.update(fused_data);
        
        vTaskDelay(pdMS_TO_TICKS(20)); // 50 Hz fusion rate
    }
}
```

## Performance Characteristics

| Sensor | Interface | Max Sample Rate | Current Draw | Response Time |
|--------|-----------|----------------|--------------|---------------|
| ADXL375 | I2C/SPI | 3.2 kHz | 145 µA | <1 ms |
| BMP390 | I2C/SPI | 200 Hz | 3.4 µA | 5 ms |
| BNO55 | I2C | 100 Hz | 12.3 mA | 10 ms |
| ISM330DHCX | I2C/SPI | 6.66 kHz | 55 µA | <1 ms |
| LSM6DSO32 | I2C/SPI | 6.66 kHz | 55 µA | <1 ms |

## Best Practices

### Power Management

```cpp
// Put sensors in low-power mode when not needed
void enter_low_power_mode() {
    adxl375.setPowerMode(ADXL375::POWER_STANDBY);
    bmp390.setPowerMode(BMP390::SLEEP_MODE);
    lsm6dso32.setPowerMode(LSM6DSO32::POWER_DOWN);
}
```

### Error Handling

```cpp
void robust_sensor_read() {
    const int MAX_RETRIES = 3;
    int retry_count = 0;
    
    while (retry_count < MAX_RETRIES) {
        if (sensor.readData(data)) {
            // Success - process data
            break;
        } else {
            retry_count++;
            printf("Sensor read failed, retry %d/%d\n", retry_count, MAX_RETRIES);
            vTaskDelay(pdMS_TO_TICKS(10)); // Brief delay before retry
        }
    }
    
    if (retry_count >= MAX_RETRIES) {
        // Handle persistent sensor failure
        handle_sensor_error();
    }
}
```

### Calibration and Validation

```cpp
bool validate_sensor_data(const SensorData& data) {
    // Check for reasonable value ranges
    if (abs(data.acceleration_x) > 200.0) return false; // Beyond ADXL375 range
    if (data.pressure < 300 || data.pressure > 1250) return false; // BMP390 range
    
    // Check for sensor communication errors
    if (data.status != SENSOR_OK) return false;
    
    return true;
}
```

## Next Steps

Explore individual sensor documentation:

- [ADXL375 High-G Accelerometer](/sensors/adxl375)
- [BMP390 Pressure Sensor](/sensors/bmp390)  
- [BNO55 9-DOF IMU](/sensors/bno55)
- [ISM330DHCX Machine Learning IMU](/sensors/ism330dhcx)
- [LSM6DSO32 High-Performance IMU](/sensors/lsm6dso32)

Or see practical implementations:

- [Sensor Fusion Example](/examples/sensor-fusion)
- [Data Logging Tutorial](/examples/bluetooth-logging)
- [Multi-Task Sensor System](/examples/multitask)
