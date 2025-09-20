

cmake_minimum_required(VERSION 3.16)

# Pull in PICO SDK (must be before project)
include(pico_sdk_import.cmake)


project(telemetry C CXX ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# Initialize the SDK
pico_sdk_init()


add_executable(telemetry
        vario.cpp
        telemetry.cpp
        i2c.cpp
        bmp085.cpp
        spi.cpp
        ADXL362.cpp
        )

# Pull in our pico_stdlib which pulls in commonly used features
target_link_libraries(telemetry
        pico_stdlib 
        hardware_uart
        hardware_i2c
        hardware_spi
        )

# enable usb output, disable uart output
pico_enable_stdio_usb(telemetry 1)
pico_enable_stdio_uart(telemetry 0)

# create map/bin/hex file etc.
pico_add_extra_outputs(telemetry)

