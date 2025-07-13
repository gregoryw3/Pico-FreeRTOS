cmake_minimum_required(VERSION 3.13)

set(PICO_BOARD pico_w)
set(PICO_PLATFORM rp2040)
# set(PICO_BOARD_HEADER_DIRS ${CMAKE_CURRENT_LIST_DIR}/include)
# Pull in SDK (must be before project)
include(${CMAKE_CURRENT_LIST_DIR}/include/pico-sdk/pico_sdk_init.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/include/FreeRTOS-Kernel/portable/ThirdParty/GCC/RP2040/FreeRTOS_Kernel_import.cmake)
include_directories(${CMAKE_CURRENT_LIST_DIR}/include/eigen)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_C_COMPILER arm-none-eabi-gcc)
set(CMAKE_CXX_COMPILER arm-none-eabi-g++)
set(CMAKE_TRY_COMPILE_TARGET_TYPE "STATIC_LIBRARY")

project(example C CXX ASM)

pico_sdk_init()

set(CMAKE_C_STANDARD 23)
set(CMAKE_CXX_STANDARD 23)

if (PICO_SDK_VERSION_STRING VERSION_LESS "1.3.0")
    message(FATAL_ERROR "Raspberry Pi Pico SDK version 1.3.0 (or later) required. Your version is ${PICO_SDK_VERSION_STRING}")
endif()

set(BTSTACK_ROOT ${PICO_SDK_PATH}/lib/btstack)
set(BTSTACK_EXAMPLE_PATH ${BTSTACK_ROOT}/example)
set(BTSTACK_3RD_PARTY_PATH ${BTSTACK_ROOT}/3rd-party)
set(BT_EXAMPLE_COMMON_DIR "${CMAKE_CURRENT_LIST_DIR}")

include_directories(${CMAKE_CURRENT_LIST_DIR}/include)
add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/libs)

add_subdirectory(src)

add_compile_options(
        -Wall 
        -Wno-format
        -fpermissive
        )