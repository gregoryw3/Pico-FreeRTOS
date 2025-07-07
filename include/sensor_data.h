#pragma once

#include <stdint.h>
#include <stdbool.h>

// Define GPS Fix types
typedef enum {
    GPS_FIX_NONE = 0,
    GPS_FIX_2D = 2,
    GPS_FIX_3D = 3
} GpsFix;

// UTC Time structure
typedef struct {
    uint32_t itow;
    uint32_t time_accuracy_estimate_ns;
    int32_t nanos;
    uint16_t year;
    uint8_t month;
    uint8_t day;
    uint8_t hour;
    uint8_t min;
    uint8_t sec;
    uint8_t valid;
} UTC;

// Sensor data structures
typedef struct {
    float temp;
    float gyro_x;
    float gyro_y;
    float gyro_z;
    float accel_x;
    float accel_y;
    float accel_z;
} ISM330DHCX;

typedef struct {
    float gyro_x;
    float gyro_y;
    float gyro_z;
    float accel_x;
    float accel_y;
    float accel_z;
} LSM6DSO32;

typedef struct {
    float temperature;
    float pressure;
    float altitude;
} BMP390;

typedef struct {
    float latitude;
    float longitude;
    float altitude;
    float altitude_msl;
    uint8_t num_sats;
    GpsFix fix_type;
    UTC utc_time;
} GPS;

typedef struct {
    float accel_x;
    float accel_y;
    float accel_z;
} ADXL375;

// Union for sensor updates
typedef enum {
    UPDATE_TYPE_ISM330DHCX = 0,
    UPDATE_TYPE_LSM6DSO32 = 1,
    UPDATE_TYPE_BMP390 = 2,
    UPDATE_TYPE_GPS = 3,
    UPDATE_TYPE_ADXL375 = 4,
    UPDATE_TYPE_ISM330DHCX2 = 5
} SensorUpdateType;

typedef struct {
    SensorUpdateType type;
    union {
        ISM330DHCX ism330dhcx;
        LSM6DSO32 lsm6dso32;
        BMP390 bmp390;
        GPS gps;
        ADXL375 adxl375;
        ISM330DHCX ism330dhcx2;
    } data;
} SensorUpdate;

// Mini data structure for LoRa transmission
typedef struct {
    uint8_t device_id;
    uint32_t msg_num;
    uint32_t time_since_boot;
    
    float lat;
    float lon;
    float alt;
    uint8_t num_sats;
    GpsFix gps_fix;
    UTC gps_time;
    
    float baro_alt;
    
    float ism_axel_x;
    float ism_axel_y;
    float ism_axel_z;
    float ism_gyro_x;
    float ism_gyro_y;
    float ism_gyro_z;
    
    float lsm_axel_x;
    float lsm_axel_y;
    float lsm_axel_z;
    float lsm_gyro_x;
    float lsm_gyro_y;
    float lsm_gyro_z;
    
    float adxl_axel_x;
    float adxl_axel_y;
    float adxl_axel_z;
    
    float ism_axel_x2;
    float ism_axel_y2;
    float ism_axel_z2;
    float ism_gyro_x2;
    float ism_gyro_y2;
    float ism_gyro_z2;
} MiniData;