add_library(ADXL375 STATIC

)

target_compile_features(ADXL375 PUBLIC cxx_std_23)

target_include_directories(ADXL375 PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
)

target_link_libraries(ADXL375
    PUBLIC pico_stdlib
    PRIVATE hardware_i2c
    PRIVATE hardware_spi
    PUBLIC FreeRTOS-Kernel
)