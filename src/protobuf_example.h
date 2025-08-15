#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "FreeRTOS.h"
#include "task.h"

// Include the generated protobuf headers
#include "data.pb.h"
#include "pb_encode.h"
#include "pb_decode.h"

// Example function to create a GPS data packet
void create_gps_packet_example() {
    // Create a TrackerPacket
    rocketry_TrackerPacket packet = rocketry_TrackerPacket_init_zero;
    
    // Fill in common fields
    packet.device_id = 12345;
    packet.msg_num = 1;
    packet.time_since_boot = 1000000; // 1 second in microseconds
    packet.packet_type = rocketry_TrackerPacket_PacketType_GPS;
    
    // Fill in GPS data
    packet.which_payload = rocketry_TrackerPacket_gps_tag;
    packet.payload.gps.lat = 37.7749;      // San Francisco latitude
    packet.payload.gps.lon = -122.4194;   // San Francisco longitude
    packet.payload.gps.alt = 100.0;       // 100 meters altitude
    packet.payload.gps.num_sats = 8;      // 8 satellites
    packet.payload.gps.fix_type = rocketry_GpsFix_FIX_3D;
    packet.payload.gps.year = 2025;
    packet.payload.gps.month = 8;
    packet.payload.gps.day = 3;
    packet.payload.gps.hour = 12;
    packet.payload.gps.min = 30;
    packet.payload.gps.sec = 45;
    
    // Encode the packet
    uint8_t buffer[256];
    pb_ostream_t stream = pb_ostream_from_buffer(buffer, sizeof(buffer));
    
    bool status = pb_encode(&stream, rocketry_TrackerPacket_fields, &packet);
    
    if (status) {
        printf("GPS packet encoded successfully! Size: %zu bytes\n", stream.bytes_written);
        
        // Print the encoded data as hex
        printf("Encoded data: ");
        for (size_t i = 0; i < stream.bytes_written; i++) {
            printf("%02x ", buffer[i]);
        }
        printf("\n");
        
        // Now decode it back to verify
        pb_istream_t input_stream = pb_istream_from_buffer(buffer, stream.bytes_written);
        rocketry_TrackerPacket decoded_packet = rocketry_TrackerPacket_init_zero;
        
        bool decode_status = pb_decode(&input_stream, rocketry_TrackerPacket_fields, &decoded_packet);
        
        if (decode_status) {
            printf("Decoded packet successfully!\n");
            printf("Device ID: %lu\n", (unsigned long)decoded_packet.device_id);
            printf("Message Number: %lu\n", (unsigned long)decoded_packet.msg_num);
            printf("GPS Lat: %.6f\n", decoded_packet.payload.gps.lat);
            printf("GPS Lon: %.6f\n", decoded_packet.payload.gps.lon);
            printf("GPS Alt: %.2f m\n", decoded_packet.payload.gps.alt);
            printf("Satellites: %lu\n", (unsigned long)decoded_packet.payload.gps.num_sats);
        } else {
            printf("Failed to decode packet!\n");
        }
    } else {
        printf("Failed to encode GPS packet: %s\n", PB_GET_ERROR(&stream));
    }
}

// Example function to create an IMU data packet
void create_imu_packet_example() {
    rocketry_TrackerPacket packet = rocketry_TrackerPacket_init_zero;
    
    // Fill in common fields
    packet.device_id = 12345;
    packet.msg_num = 2;
    packet.time_since_boot = 2000000; // 2 seconds in microseconds
    packet.packet_type = rocketry_TrackerPacket_PacketType_ISM_PRIMARY;
    
    // Fill in IMU data
    packet.which_payload = rocketry_TrackerPacket_imu_tag;
    packet.payload.imu.accel_x = 0.1;   // 0.1 g
    packet.payload.imu.accel_y = 0.2;   // 0.2 g
    packet.payload.imu.accel_z = 9.8;   // 9.8 m/s^2 (gravity)
    packet.payload.imu.gyro_x = 0.01;   // 0.01 rad/s
    packet.payload.imu.gyro_y = 0.02;   // 0.02 rad/s
    packet.payload.imu.gyro_z = 0.03;   // 0.03 rad/s
    
    // Encode the packet
    uint8_t buffer[256];
    pb_ostream_t stream = pb_ostream_from_buffer(buffer, sizeof(buffer));
    
    bool status = pb_encode(&stream, rocketry_TrackerPacket_fields, &packet);
    
    if (status) {
        printf("IMU packet encoded successfully! Size: %zu bytes\n", stream.bytes_written);
        printf("Accel: (%.2f, %.2f, %.2f)\n", 
               packet.payload.imu.accel_x,
               packet.payload.imu.accel_y, 
               packet.payload.imu.accel_z);
        printf("Gyro: (%.3f, %.3f, %.3f)\n",
               packet.payload.imu.gyro_x,
               packet.payload.imu.gyro_y,
               packet.payload.imu.gyro_z);
    } else {
        printf("Failed to encode IMU packet: %s\n", PB_GET_ERROR(&stream));
    }
}
