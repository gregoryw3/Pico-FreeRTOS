add_library(ISM330DHCX STATIC
    src/ISM330DHCX.cpp
)

target_compile_features(ISM330DHCX PUBLIC cxx_std_23)

target_include_directories(ISM330DHCX PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
)

target_link_libraries(ISM330DHCX
    PUBLIC pico_stdlib
    PRIVATE hardware_i2c
    PRIVATE FreeRTOS-Kernel
)