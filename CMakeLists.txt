cmake_minimum_required(VERSION 3.13)

set(PICO_BOARD pico_w)
set(PICO_PLATFORM rp2040)
# set(PICO_BOARD_HEADER_DIRS ${CMAKE_CURRENT_LIST_DIR}/include)
# Pull in SDK (must be before project)
include(${CMAKE_CURRENT_LIST_DIR}/include/pico-sdk/pico_sdk_init.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/include/FreeRTOS-Kernel/portable/ThirdParty/GCC/RP2040/FreeRTOS_Kernel_import.cmake)
# include_directories(${CMAKE_CURRENT_LIST_DIR}/include/eigen)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_C_COMPILER arm-none-eabi-gcc)
set(CMAKE_CXX_COMPILER arm-none-eabi-g++)
set(CMAKE_TRY_COMPILE_TARGET_TYPE "STATIC_LIBRARY")

set(BTSTACK_ROOT ${PICO_SDK_PATH}/lib/btstack)
set(BTSTACK_EXAMPLE_PATH ${BTSTACK_ROOT}/example)
set(BTSTACK_3RD_PARTY_PATH ${BTSTACK_ROOT}/3rd-party)
set(BT_EXAMPLE_COMMON_DIR "${CMAKE_CURRENT_LIST_DIR}")

project(example C CXX ASM)
set(CMAKE_C_STANDARD 23)
set(CMAKE_CXX_STANDARD 23)

pico_sdk_init()

include_directories(${CMAKE_CURRENT_LIST_DIR}/include)
add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/libs)

add_executable(main_full_smp
        src/bluetooth.cpp
        # src/simple.cpp
        # src/main.c
        # src/i2c_devices.c
        )

target_link_libraries(main_full_smp 
        ADXL375
        BMP390
        ISM330DHCX
        LSM6DSO32
        UBLOX
        pico_stdlib 
        pico_cyw43_arch_none
        hardware_clocks
        FreeRTOS-Kernel 
        FreeRTOS-Kernel-Heap4
        hardware_i2c
        hardware_uart
        hardware_gpio
        hardware_dma
        pico_multicore
        pico_btstack_ble
        pico_btstack_classic
)

pico_add_extra_outputs(main_full_smp)

target_compile_features(main_full_smp PRIVATE
        cxx_std_23
)

target_include_directories(main_full_smp PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}
        ${PICO_SDK_PATH}/lib/btstack/src
        ${CMAKE_CURRENT_LIST_DIR}/libs
)

target_compile_definitions(main_full_smp PRIVATE
        PICO_STDIO_STACK_BUFFER_SIZE=64
        # $<$<COMPILE_LANG_AND_ID:C,GNU>:-fdiagnostics-color=always>
        # $<$<COMPILE_LANG_AND_ID:C,Clang>:-fcolor-diagnostics>
        # $<$<COMPILE_LANG_AND_ID:C,Clang,GNU>:-Wall>
        # $<$<COMPILE_LANG_AND_ID:C,Clang,GNU>:-Wextra>
        # $<$<COMPILE_LANG_AND_ID:C,Clang,GNU>:-Werror>
        # $<$<COMPILE_LANG_AND_ID:C,Clang>:-Weverything>
)

# Use USB uart
pico_enable_stdio_usb(main_full_smp 1)
pico_enable_stdio_uart(main_full_smp 1)