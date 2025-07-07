add_library(BMP390 
    BMP390.cpp
)

target_compile_features(BMP390 PUBLIC cxx_std_23)

target_include_directories(BMP390 PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(BMP390
    pico_stdlib
    hardware_i2c
)