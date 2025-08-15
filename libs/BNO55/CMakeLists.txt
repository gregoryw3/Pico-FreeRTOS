add_library(BNO55 STATIC

)

target_compile_features(BNO55 PUBLIC cxx_std_23)

target_include_directories(BNO55 PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
)

target_link_libraries(BNO55
    PUBLIC pico_stdlib
    PRIVATE hardware_i2c
    PRIVATE hardware_spi
    PUBLIC FreeRTOS-Kernel
)