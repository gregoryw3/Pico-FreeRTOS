add_library(LSM6DSO32 STATIC
    src/LSM6DSO32.cpp
)

target_compile_features(LSM6DSO32 PUBLIC cxx_std_23)

target_include_directories(LSM6DSO32 PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
)

target_link_libraries(LSM6DSO32
    PUBLIC pico_stdlib
    PRIVATE hardware_i2c
    PRIVATE FreeRTOS-Kernel
)