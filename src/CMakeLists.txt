# Bluetooth Executable
# set(BTSTACK_EXAMPLE_TYPE freertos CACHE STRING "BTstack example type")

add_executable(bluetooth
        ${PROJECT_SOURCE_DIR}/src/bluetooth.cpp
        )

# pico_set_binary_type(bluetooth copy_to_ram)

target_link_libraries(bluetooth 
        pico_stdlib 
        pico_cyw43_arch_none
        # pico_cyw43_arch_lwip_sys_freertos
        hardware_clocks
        FreeRTOS-Kernel 
        FreeRTOS-Kernel-Heap4
        hardware_i2c
        hardware_spi
        hardware_uart
        hardware_gpio
        hardware_dma
        pico_multicore
        pico_btstack_ble
        pico_btstack_classic
        pico_btstack_cyw43
        test_bluetooth
        # pico_lwip_freertos
        # pico_lwip_core
)

target_include_directories(bluetooth PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}
        ${PICO_SDK_PATH}/lib/btstack/src
        ${CMAKE_CURRENT_LIST_DIR}/libs
        ${CMAKE_CURRENT_LIST_DIR}/../include
)

target_compile_definitions(bluetooth PRIVATE
        USE_FREERTOS=1
        DEBUG=1
        PICO_STDIO_STACK_BUFFER_SIZE=64
)

target_compile_features(bluetooth PRIVATE
        cxx_std_23
)

target_link_options(bluetooth PRIVATE
        -Wl,--print-memory-usage
)

# Use USB uart
pico_enable_stdio_usb(bluetooth 1)
pico_enable_stdio_uart(bluetooth 1)
# Add extra outputs for the executables
pico_add_extra_outputs(bluetooth)

# Sensors Executable

add_executable(sensors
        ${PROJECT_SOURCE_DIR}/src/sensors.cpp
)


target_link_libraries(sensors 
        # Pico SDK libraries
        pico_stdlib 
        hardware_i2c
        hardware_uart
        hardware_spi
        hardware_gpio
        hardware_dma
        pico_multicore
        pico_cyw43_arch_none
        hardware_clocks
        # FreeRTOS libraries
        FreeRTOS-Kernel 
        FreeRTOS-Kernel-Heap4
        # Sensor libraries
        ADXL375
        BMP390
        ISM330DHCX
        LSM6DSO32
        UBLOX
)

target_include_directories(sensors PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}
        ${PICO_SDK_PATH}/lib/btstack/src
        ${CMAKE_CURRENT_LIST_DIR}/libs
)
target_compile_definitions(sensors PRIVATE
        USE_FREERTOS=1
        DEBUG=1
        PICO_STDIO_STACK_BUFFER_SIZE=64
)

target_compile_features(sensors PRIVATE
        cxx_std_23
)

target_link_options(sensors PRIVATE
        -Wl,--print-memory-usage
)

pico_enable_stdio_usb(sensors 1)
pico_enable_stdio_uart(sensors 1)
# Add extra outputs for the executables
pico_add_extra_outputs(sensors)

# Simple exacutable

add_executable(simple
        ${PROJECT_SOURCE_DIR}/src/simple.cpp
)


target_link_libraries(simple 
        # Pico SDK libraries
        pico_stdlib 
        hardware_i2c
        hardware_uart
        hardware_spi
        hardware_gpio
        hardware_dma
        pico_multicore
        pico_cyw43_arch_none
        hardware_clocks
        # FreeRTOS libraries
        FreeRTOS-Kernel 
        FreeRTOS-Kernel-Heap4
        # Sensor libraries
        # ADXL375
        # BMP390
        # ISM330DHCX
        # LSM6DSO32
        # UBLOX
)

target_include_directories(simple PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}
        ${PICO_SDK_PATH}/lib/btstack/src
        ${CMAKE_CURRENT_LIST_DIR}/libs
)
target_compile_definitions(simple PRIVATE
        USE_FREERTOS=1
        DEBUG=1
        PICO_STDIO_STACK_BUFFER_SIZE=64
)

target_compile_features(simple PRIVATE
        cxx_std_23
)

target_link_options(simple PRIVATE
        -Wl,--print-memory-usage
)

pico_enable_stdio_usb(simple 1)
pico_enable_stdio_uart(simple 0)
# Add extra outputs for the executables
pico_add_extra_outputs(simple)