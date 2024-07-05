cmake_minimum_required(VERSION 3.13)

include(pico_sdk_import.cmake)

project(lightgun C CXX ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
pico_sdk_init()

add_executable(controllers
    controllers.c
    usb_descriptors.c
)
pico_enable_stdio_uart(controllers 0)
pico_enable_stdio_usb(controllers 1)

target_include_directories(controllers PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
)
target_link_libraries(controllers PRIVATE
    pico_stdlib
    tinyusb_device
    tinyusb_board
)

pico_add_extra_outputs(controllers)

add_executable(accessory
    accessory.c
    usb_descriptors.c
)
# pico_generate_pio_header(controllers ${CMAKE_CURRENT_LIST_DIR}/spi.pio)
pico_enable_stdio_uart(accessory 0)
pico_enable_stdio_usb(accessory 1)

target_include_directories(accessory PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
)
target_link_libraries(accessory PRIVATE
    pico_stdlib
    # hardware_pio
    hardware_i2c
    tinyusb_device
    tinyusb_board
)

pico_add_extra_outputs(accessory)
