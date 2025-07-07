add_library(UBLOX 
    UBLOX.cpp
)

target_compile_features(UBLOX PUBLIC cxx_std_23)

target_compile_options(UBLOX PUBLIC
    $<$<COMPILE_LANGUAGE:CXX>:-fno-exceptions>
    $<$<COMPILE_LANGUAGE:CXX>:-fno-rtti>
)

target_compile_definitions(UBLOX PUBLIC
    PICO_PLATFORM=1
)

target_include_directories(UBLOX PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(UBLOX
    pico_stdlib
    hardware_i2c
    hardware_uart
    hardware_gpio
    FreeRTOS-Kernel
)