cmake_minimum_required(VERSION 3.13)
set(PICO_SDK_PATH $ENV{PICO_SDK_PATH})
include(pico_sdk_import.cmake)

project(menteviva C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
pico_sdk_init()

add_executable(menteviva
    main.c
    input.c
    buzzer.c
    led_matrix.c
    display.c
)

target_include_directories(menteviva PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/lib
)

target_link_libraries(menteviva
    pico_stdlib
    pico_cyw43_arch_lwip_poll
    hardware_adc
    hardware_i2c
    hardware_pwm
    hardware_pio
)

pico_enable_stdio_usb(menteviva 1)
pico_enable_stdio_uart(menteviva 1)
pico_add_extra_outputs(menteviva)
