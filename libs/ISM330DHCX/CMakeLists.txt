add_library(ISM330DHCX 
    ISM330DHCX.cpp
)

target_compile_features(ISM330DHCX PUBLIC cxx_std_23)

target_include_directories(ISM330DHCX PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(ISM330DHCX
    pico_stdlib
    hardware_i2c
)