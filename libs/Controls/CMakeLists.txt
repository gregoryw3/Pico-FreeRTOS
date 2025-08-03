add_library(Controls INTERFACE)

target_compile_features(Controls INTERFACE cxx_std_23)

target_include_directories(Controls INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/include/libfixmath
)

# target_link_libraries(Controls
# )