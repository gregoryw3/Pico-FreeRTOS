#pragma once

#include <stdint.h>
#include "hardware/i2c.h"
#include "pico/stdlib.h"
#include "sensor_data.h"

// I2C Device structures and functions would go here

// ISM330DHCX functions
void ism330dhcx_init(i2c_inst_t* i2c, uint8_t addr);
void ism330dhcx_read(i2c_inst_t* i2c, uint8_t addr, ISM330DHCX* data);

// LSM6DSO32 functions
void lsm6dso32_init(i2c_inst_t* i2c, uint8_t addr);
void lsm6dso32_read(i2c_inst_t* i2c, uint8_t addr, LSM6DSO32* data);

// BMP390 functions
void bmp390_init(i2c_inst_t* i2c, uint8_t addr);
void bmp390_read(i2c_inst_t* i2c, uint8_t addr, BMP390* data);

// ADXL375 functions
void adxl375_init(i2c_inst_t* i2c, uint8_t addr);
void adxl375_read(i2c_inst_t* i2c, uint8_t addr, ADXL375* data);

// GPS functions (assuming I2C interface)
void gps_init(i2c_inst_t* i2c, uint8_t addr);
void gps_read(i2c_inst_t* i2c, uint8_t addr, GPS* data);