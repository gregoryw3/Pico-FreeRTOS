add_library(UBLOX STATIC
    src/UBLOX.cpp
)

target_compile_features(UBLOX PUBLIC cxx_std_23)

target_include_directories(UBLOX PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
)

target_link_libraries(UBLOX
    PUBLIC pico_stdlib
    PRIVATE hardware_i2c
    PRIVATE hardware_spi
    PRIVATE hardware_uart
    PRIVATE FreeRTOS-Kernel
)