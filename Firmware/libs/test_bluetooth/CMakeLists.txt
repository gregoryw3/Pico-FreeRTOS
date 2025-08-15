# set(BTSTACK_EXAMPLE_TYPE freertos CACHE STRING "BTstack example type")

add_library(test_bluetooth STATIC
    src/gatt_counter.c
)

target_compile_features(test_bluetooth PUBLIC cxx_std_23)

target_include_directories(test_bluetooth PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
    ${PICO_SDK_PATH}/lib/btstack/src
    ${PICO_SDK_PATH}/lib/btstack/platform/freertos
)

target_link_libraries(test_bluetooth
    PUBLIC pico_stdlib
    PUBLIC FreeRTOS-Kernel
    PUBLIC pico_btstack_ble
    # PUBLIC pico_btstack_cyw43
)