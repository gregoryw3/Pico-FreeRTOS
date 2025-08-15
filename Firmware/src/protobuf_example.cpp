// Example function showing how to use the generated protobuf code
// Add this to your bluetooth.cpp to test protobuf functionality

#include <stdio.h>
#include "data.pb.h"
#include "pb_encode.h"
#include "pb_decode.h"

void protobuf_example() {
    printf("=== Protobuf Example ===\n");
    
    // Create a GPS data packet
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
        
        // Print some encoded data as hex (first 20 bytes)
        printf("First 20 bytes: ");
        for (size_t i = 0; i < 20 && i < stream.bytes_written; i++) {
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
    
    printf("=== End Protobuf Example ===\n\n");
}

// Call this function from your bluetooth_task or main to test protobuf
// protobuf_example();
