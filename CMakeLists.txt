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

set(PICO_LWIP_CONTRIB_PATH ${PICO_SDK_PATH}/lib/lwip/contrib)
add_subdirectory(${PICO_SDK_PATH}/lib/lwip ${CMAKE_BINARY_DIR}/lwip)

set(BTSTACK_ROOT ${PICO_SDK_PATH}/lib/btstack)
set(BTSTACK_EXAMPLE_PATH ${BTSTACK_ROOT}/example)
set(BTSTACK_3RD_PARTY_PATH ${BTSTACK_ROOT}/3rd-party)
set(BT_EXAMPLE_COMMON_DIR "${CMAKE_CURRENT_LIST_DIR}")

add_library(libfixmath INTERFACE)
file(GLOB libfixmath-srcs ${CMAKE_CURRENT_LIST_DIR}/include/libfixmath/libfixmath/*.c)
target_sources(libfixmath INTERFACE ${libfixmath-srcs})
target_include_directories(libfixmath INTERFACE
        ${CMAKE_CURRENT_LIST_DIR}/include/libfixmath/libfixmath
)
target_compile_definitions(libfixmath INTERFACE
        FIXMATH_FAST_SIN
        FIXMATH_NO_64BIT
        # FIXMATH_SIN_LUT
        # FIXMATH_NO_CACHE
        # FIXMATH_NO_HARD_DIVISION
        FIXMATH_NO_OVERFLOW
        # FIXMATH_NO_ROUNDING
        # FIXMATH_OPTIMIZE_8BIT
)

add_library(libfixmatrix INTERFACE)
file(GLOB libfixmatrix-srcs ${CMAKE_CURRENT_LIST_DIR}/include/libfixmatrix/*.c)
list(FILTER libfixmatrix-srcs EXCLUDE REGEX ".*unittests\\.c$")
target_sources(libfixmatrix INTERFACE ${libfixmatrix-srcs})
target_include_directories(libfixmatrix INTERFACE
        ${CMAKE_CURRENT_LIST_DIR}/include/libfixmatrix
)
target_compile_definitions(libfixmatrix INTERFACE FIXMATRIX_MAX_SIZE=3)
target_link_libraries(libfixmatrix INTERFACE libfixmath)

include_directories(${CMAKE_CURRENT_LIST_DIR}/include)
add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/libs)

add_subdirectory(src)

add_compile_options(
        -Wall 
        -Wno-format
        -fpermissive
        )