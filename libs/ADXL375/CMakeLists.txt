# Create the ADXL375 library
add_library(ADXL375 
    ADXL375.cpp
)

target_compile_features(ADXL375 PUBLIC cxx_std_23)

target_include_directories(ADXL375 PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(ADXL375
    pico_stdlib
    hardware_i2c
)