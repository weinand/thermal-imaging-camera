cmake_minimum_required(VERSION 3.12)

# initialize the SDK based on PICO_SDK_PATH
# note: this must happen before project()
include($ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake)

project(hello C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

add_executable(thermocam
    main.cpp
    MLX90640_PICO_I2C_Driver.cpp
    ./mlx90640-library/functions/MLX90640_API.c
    SSD1351_PICO_SPI_Driver.cpp
    SSD1351_API.cpp
)

target_include_directories(thermocam PUBLIC
    ./mlx90640-library/headers
)

# Add pico_stdlib library which aggregates commonly used features
target_link_libraries(thermocam
    pico_stdlib
    hardware_i2c
    hardware_spi
    hardware_gpio
    pico_multicore
)

# initialize the Raspberry Pi Pico SDK
pico_sdk_init()

# Enable USB, UART output
pico_enable_stdio_usb(thermocam 0)
pico_enable_stdio_uart(thermocam 1)

# create map/bin/hex/uf2 file in addition to ELF.
pico_add_extra_outputs(thermocam)
