#include "i2c_devices.h"
#include <stdio.h>

// ISM330DHCX Implementation
void ism330dhcx_init(i2c_inst_t* i2c, uint8_t addr) {
    uint8_t id;
    uint8_t who_am_i_reg = 0x0F;
    i2c_write_blocking(i2c, addr, &who_am_i_reg, 1, true);
    i2c_read_blocking(i2c, addr, &id, 1, false);
    
    printf("ISM330DHCX ID: 0x%02X (should be 0x6B)\n", id);
    
    // Initialize the sensor with appropriate settings
    // This would include setting the data rate, scale, etc.
    // For brevity, we're just showing the pattern
    
    // Example: set accelerometer data rate and range
    uint8_t ctrl1_xl_reg = 0x10;
    uint8_t ctrl1_xl_val = 0x48; // 26Hz, 4G
    uint8_t buf[2] = {ctrl1_xl_reg, ctrl1_xl_val};
    i2c_write_blocking(i2c, addr, buf, 2, false);
    
    // Example: set gyroscope data rate and range
    uint8_t ctrl2_g_reg = 0x11;
    uint8_t ctrl2_g_val = 0x48; // 26Hz, 500dps
    buf[0] = ctrl2_g_reg;
    buf[1] = ctrl2_g_val;
    i2c_write_blocking(i2c, addr, buf, 2, false);
}

void ism330dhcx_read(i2c_inst_t* i2c, uint8_t addr, ISM330DHCX* data) {
    // Read accelerometer data
    uint8_t accel_data_reg = 0x28; // OUTX_L_A register
    uint8_t raw_data[6];
    
    i2c_write_blocking(i2c, addr, &accel_data_reg, 1, true);
    i2c_read_blocking(i2c, addr, raw_data, 6, false);
    
    // Convert to meaningful values (specifics depend on range setting)
    int16_t accel_x = (int16_t)(raw_data[1] << 8 | raw_data[0]);
    int16_t accel_y = (int16_t)(raw_data[3] << 8 | raw_data[2]);
    int16_t accel_z = (int16_t)(raw_data[5] << 8 | raw_data[4]);
    
    // Convert to m/s^2 (assuming ±4g range)
    data->accel_x = (float)accel_x * 0.122f / 1000.0f * 9.81f;
    data->accel_y = (float)accel_y * 0.122f / 1000.0f * 9.81f;
    data->accel_z = (float)accel_z * 0.122f / 1000.0f * 9.81f;
    
    // Read gyroscope data
    uint8_t gyro_data_reg = 0x22; // OUTX_L_G register
    
    i2c_write_blocking(i2c, addr, &gyro_data_reg, 1, true);
    i2c_read_blocking(i2c, addr, raw_data, 6, false);
    
    // Convert to meaningful values
    int16_t gyro_x = (int16_t)(raw_data[1] << 8 | raw_data[0]);
    int16_t gyro_y = (int16_t)(raw_data[3] << 8 | raw_data[2]);
    int16_t gyro_z = (int16_t)(raw_data[5] << 8 | raw_data[4]);
    
    // Convert to dps (assuming ±500dps range)
    data->gyro_x = (float)gyro_x * 17.5f / 1000.0f;
    data->gyro_y = (float)gyro_y * 17.5f / 1000.0f;
    data->gyro_z = (float)gyro_z * 17.5f / 1000.0f;
    
    // Read temperature data
    uint8_t temp_data_reg = 0x20; // OUT_TEMP_L register
    uint8_t temp_raw[2];
    
    i2c_write_blocking(i2c, addr, &temp_data_reg, 1, true);
    i2c_read_blocking(i2c, addr, temp_raw, 2, false);
    
    int16_t temp = (int16_t)(temp_raw[1] << 8 | temp_raw[0]);
    
    // Convert to Celsius
    data->temp = 25.0f + (float)temp / 256.0f;
}

// Similar implementations would be needed for other sensors