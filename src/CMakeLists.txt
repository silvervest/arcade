cmake_minimum_required(VERSION 3.13)

include(pico_sdk_import.cmake)

project(lightgun C CXX ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
pico_sdk_init()

add_executable(stick
    stick.c
    usb_descriptors.c
)
pico_enable_stdio_uart(stick 0)
pico_enable_stdio_usb(stick 1)

target_include_directories(stick PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
)
target_link_libraries(stick PRIVATE
    pico_stdlib
    hardware_i2c
    tinyusb_device
    tinyusb_board
)

pico_add_extra_outputs(stick)

add_executable(vga
    vga.c
    usb_descriptors.c
)
# pico_generate_pio_header(controllers ${CMAKE_CURRENT_LIST_DIR}/spi.pio)
pico_enable_stdio_uart(vga 0)
pico_enable_stdio_usb(vga 1)

target_include_directories(vga PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
)
target_link_libraries(vga PRIVATE
    pico_stdlib
    # hardware_pio
    hardware_i2c
    tinyusb_device
    tinyusb_board
)

pico_add_extra_outputs(vga)
