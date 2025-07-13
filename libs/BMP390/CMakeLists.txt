add_library(BMP390 STATIC
    src/BMP390.cpp
)

target_compile_features(BMP390 PUBLIC cxx_std_23)

target_include_directories(BMP390 PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
)

target_link_libraries(BMP390
    PUBLIC pico_stdlib
    PRIVATE hardware_i2c
)