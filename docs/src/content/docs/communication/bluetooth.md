---
title: Bluetooth Setup
description: Complete guide to Bluetooth Low Energy integration using BTStack
---

The Pico-FreeRTOS library includes comprehensive Bluetooth Low Energy (BLE) support through the BTStack library. This enables wireless communication, data logging, and remote control capabilities for your embedded projects.

## BTStack Integration

BTStack is a complete Bluetooth protocol stack implementation that provides:

- Full Bluetooth Low Energy (BLE) support
- GATT (Generic Attribute Profile) server and client
- Multiple connection handling
- Security and pairing support
- Low power optimization

## Hardware Requirements

### Supported Hardware

- **Raspberry Pi Pico W**: Built-in CYW43439 wireless chip
- **External Bluetooth modules**: Via UART or SPI connection

### Pin Configuration

The Bluetooth implementation uses these standard pins:

```cpp
// Pin definitions from bluetooth.cpp
#define PICO_DEFAULT_LED_PIN 25   // Status LED

#define I2C0_SDA_PIN 4           // I2C for sensors
#define I2C0_SCL_PIN 5
#define I2C1_SDA_PIN 18
#define I2C1_SCL_PIN 19

#define UART0_TX_PIN 12          // UART for debug/communication
#define UART0_RX_PIN 13

#define I2C_FREQUENCY 400000     // 400 kHz I2C
#define UART_BAUD_RATE 115200    // Standard baud rate
```

## GATT Service Architecture

### Counter Service Example

The library includes a GATT counter service demonstrating BLE functionality:

```cpp
// GATT service definition
#include "gatt_counter.h"

// Service characteristics
- Counter Value (Read/Notify)
- Counter Control (Write)
- Device Information
```

### Service UUID Structure

```cpp
// Example GATT service UUIDs
#define COUNTER_SERVICE_UUID        0x1234
#define COUNTER_VALUE_CHAR_UUID     0x1235
#define COUNTER_CONTROL_CHAR_UUID   0x1236
```

## FreeRTOS Task Integration

### Bluetooth Task Implementation

```cpp
void bluetooth_task(void *pvParameters) {
    // Initialize CYW43 architecture
    if (cyw43_arch_init()) {
        printf("Wi-Fi/Bluetooth init failed\n");
        vTaskDelete(NULL);
        return;
    }
    
    // Setup BTStack
    l2cap_init();
    sm_init();
    att_server_init(profile_data, att_read_callback, att_write_callback);
    
    // Start advertising
    advertisements_enable = 1;
    
    while (1) {
        // Process Bluetooth events
        btstack_run_loop_execute_once();
        
        // Handle sensor data transmission
        if (connection_handle != HCI_CON_HANDLE_INVALID) {
            send_sensor_data();
        }
        
        vTaskDelay(pdMS_TO_TICKS(10)); // 100 Hz processing
    }
}
```

### Event Handling

```cpp
static void hci_event_handler(uint8_t packet_type, uint16_t channel, 
                             uint8_t *packet, uint16_t size) {
    switch (hci_event_packet_get_type(packet)) {
        case HCI_EVENT_LE_META:
            switch (hci_event_le_meta_get_subevent_code(packet)) {
                case HCI_SUBEVENT_LE_CONNECTION_COMPLETE:
                    connection_handle = hci_subevent_le_connection_complete_get_connection_handle(packet);
                    printf("BLE connection established\n");
                    break;
                    
                case HCI_SUBEVENT_LE_CONNECTION_UPDATE_COMPLETE:
                    printf("Connection parameters updated\n");
                    break;
            }
            break;
            
        case HCI_EVENT_DISCONNECTION_COMPLETE:
            connection_handle = HCI_CON_HANDLE_INVALID;
            advertisements_enable = 1; // Re-enable advertising
            printf("BLE connection terminated\n");
            break;
    }
}
```

## Data Transmission

### Sensor Data Broadcasting

```cpp
typedef struct {
    uint32_t timestamp;
    float acceleration[3];    // ADXL375 data
    float gyroscope[3];      // LSM6DSO32 data  
    float pressure;          // BMP390 data
    float temperature;       // BMP390 data
    double latitude;         // GPS data
    double longitude;        // GPS data
} __attribute__((packed)) SensorPacket;

void send_sensor_data(void) {
    if (connection_handle == HCI_CON_HANDLE_INVALID) return;
    
    SensorPacket packet;
    packet.timestamp = xTaskGetTickCount();
    
    // Collect sensor data
    read_all_sensors(&packet);
    
    // Send via GATT notification
    att_server_notify(connection_handle, SENSOR_DATA_CHAR_HANDLE, 
                     (uint8_t*)&packet, sizeof(packet));
}
```

### Command Processing

```cpp
static int att_write_callback(hci_con_handle_t connection_handle,
                             uint16_t att_handle, uint16_t transaction_mode,
                             uint16_t offset, uint8_t *buffer, uint16_t buffer_size) {
    
    if (att_handle == COUNTER_CONTROL_CHAR_HANDLE) {
        uint8_t command = buffer[0];
        
        switch (command) {
            case CMD_START_LOGGING:
                start_data_logging();
                break;
                
            case CMD_STOP_LOGGING:
                stop_data_logging();
                break;
                
            case CMD_RESET_COUNTERS:
                reset_all_counters();
                break;
                
            case CMD_SET_SAMPLE_RATE:
                if (buffer_size >= 2) {
                    set_sample_rate(buffer[1]);
                }
                break;
        }
    }
    
    return 0;
}
```

## Power Management

### Low Power BLE

```cpp
void configure_low_power_ble(void) {
    // Set connection interval for power savings
    gap_set_connection_parameters(
        100,    // min_interval (125ms)
        200,    // max_interval (250ms) 
        0,      // latency
        400     // supervision_timeout (4s)
    );
    
    // Enable sleep mode
    cyw43_arch_enable_sta_mode();
}
```

### Advertising Configuration

```cpp
void setup_advertising(void) {
    // Set advertising parameters
    gap_advertisements_set_params(
        800,    // min_interval (500ms)
        800,    // max_interval (500ms)
        0,      // type (connectable)
        0,      // direct_address_type
        NULL,   // direct_address
        0x07,   // channel_map (all channels)
        0       // filter_policy
    );
    
    // Set advertising data
    const char device_name[] = "Pico-FreeRTOS";
    gap_advertisements_set_data(sizeof(adv_data), (uint8_t*)adv_data);
    gap_scan_response_set_data(sizeof(scan_resp_data), (uint8_t*)scan_resp_data);
}
```

## Security Features

### Pairing and Bonding

```cpp
void setup_security(void) {
    // Enable pairing
    sm_set_io_capabilities(IO_CAPABILITY_NO_INPUT_NO_OUTPUT);
    sm_set_authentication_requirements(SM_AUTHREQ_BONDING);
    
    // Set security requirements for characteristics
    att_server_set_security_requirements(SENSOR_DATA_CHAR_HANDLE, 
                                        ATT_SECURITY_ENCRYPTED);
}
```

## Performance Optimization

### Connection Parameters

```cpp
// Optimized for sensor data streaming
#define MIN_CONNECTION_INTERVAL 20   // 25ms
#define MAX_CONNECTION_INTERVAL 40   // 50ms  
#define SLAVE_LATENCY 0             // No latency
#define SUPERVISION_TIMEOUT 500      // 5s timeout
```

### Throughput Considerations

| Data Type | Size (bytes) | Max Rate | Throughput |
|-----------|--------------|----------|-------------|
| Sensor packet | 64 | 50 Hz | 25.6 kbps |
| GPS data | 32 | 10 Hz | 2.56 kbps |
| Status updates | 8 | 1 Hz | 64 bps |
| Commands | 4 | On-demand | Variable |

## Example Applications

### Remote Sensor Monitoring

```cpp
void sensor_monitoring_task(void *pvParameters) {
    SensorData sensor_data;
    
    while (1) {
        // Read sensors
        if (read_all_sensors(&sensor_data)) {
            // Send via Bluetooth if connected
            if (is_bluetooth_connected()) {
                send_sensor_notification(&sensor_data);
            }
            
            // Also log locally
            log_sensor_data(&sensor_data);
        }
        
        vTaskDelay(pdMS_TO_TICKS(100)); // 10 Hz updates
    }
}
```

### Command and Control

```cpp
void process_remote_command(uint8_t *command_data, uint16_t length) {
    CommandPacket *cmd = (CommandPacket*)command_data;
    
    switch (cmd->type) {
        case CMD_SET_LED_STATE:
            gpio_put(PICO_DEFAULT_LED_PIN, cmd->data[0]);
            break;
            
        case CMD_CONFIGURE_SENSOR:
            configure_sensor(cmd->data[0], cmd->data[1]);
            break;
            
        case CMD_START_CALIBRATION:
            start_sensor_calibration();
            break;
    }
    
    // Send acknowledgment
    send_command_response(cmd->id, STATUS_OK);
}
```

## Debugging and Troubleshooting

### Common Issues

1. **Connection failures**: Check advertising parameters and power management
2. **Data corruption**: Verify packet alignment and endianness
3. **High power consumption**: Optimize connection intervals and disable when not needed
4. **Range limitations**: Consider antenna placement and interference sources

### Debug Output

```cpp
void enable_bluetooth_debug(void) {
    // Enable BTStack debug output
    hci_dump_open(NULL, HCI_DUMP_STDOUT);
    
    printf("Bluetooth debugging enabled\n");
    printf("Connection handle: 0x%04x\n", connection_handle);
    printf("Advertising enabled: %d\n", advertisements_enable);
}
```

## Integration Examples

- [Bluetooth Data Logging](/examples/bluetooth-logging)
- [Remote Sensor Control](/examples/remote-control)
- [Multi-device Communication](/examples/multi-device)
- [Power-Optimized BLE](/examples/low-power-ble)

## Next Steps

- Learn about [I2C Communication](/communication/i2c) for sensor integration
- Explore [UART Communication](/communication/uart) for debug interfaces
- See [Multi-Task System](/examples/multitask) for complete implementations
