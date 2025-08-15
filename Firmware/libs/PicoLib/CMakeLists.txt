add_library(PicoLib STATIC
    src/PicoLib.cpp
)

target_compile_features(PicoLib PUBLIC cxx_std_23)

target_include_directories(PicoLib PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
)

target_link_libraries(PicoLib
    PUBLIC pico_stdlib
    PRIVATE hardware_i2c
    PRIVATE hardware_spi
    PRIVATE hardware_uart
    PRIVATE hardware_dma
    PRIVATE FreeRTOS-Kernel
)