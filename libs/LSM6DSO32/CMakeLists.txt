add_library(LSM6DSO32 
    LSM6DSO32.cpp
)

target_compile_features(LSM6DSO32 PUBLIC cxx_std_23)

target_include_directories(LSM6DSO32 PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(LSM6DSO32
    pico_stdlib
    hardware_i2c
)