# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.2.0)
set(toolchainVersion 14_2_Rel1)
set(picotoolVersion 2.2.0-a4)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================

set(PICO_BOARD pico_w CACHE STRING "Board type")

cmake_minimum_required(VERSION 3.13)
include(pico_sdk_import.cmake)

project(menteviva C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()

add_executable(menteviva
    main.c
    input.c
    buzzer.c
    display.c
    led_matrix.c
    lib/neopixel_pio/neopixel_pio.c
    lib/ssd1306/inc/ssd1306.c
)

target_include_directories(menteviva PRIVATE
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/lib
    ${CMAKE_SOURCE_DIR}/lib/neopixel_pio
    ${CMAKE_SOURCE_DIR}/lib/ssd1306
    ${CMAKE_SOURCE_DIR}/lib/ssd1306/inc
)

pico_generate_pio_header(menteviva
    ${CMAKE_SOURCE_DIR}/lib/neopixel_pio/ws2818b.pio
)

target_link_libraries(menteviva
    pico_stdlib
    pico_cyw43_arch_lwip_poll
    hardware_adc
    hardware_i2c
    hardware_pwm
    hardware_pio
    hardware_flash
    hardware_sync
)

pico_enable_stdio_usb(menteviva 1)
pico_enable_stdio_uart(menteviva 1)
pico_add_extra_outputs(menteviva)