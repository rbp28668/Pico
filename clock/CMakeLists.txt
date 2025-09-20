

cmake_minimum_required(VERSION 3.16)

# Pull in PICO SDK (must be before project)
include(pico_sdk_import.cmake)


project(clock C CXX ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# Initialize the SDK
pico_sdk_init()


add_executable(clock
        clock.cpp
        )

# Pull in our pico_stdlib which pulls in commonly used features
target_link_libraries(clock
        pico_stdlib 
        )

# enable usb output, disable uart output
pico_enable_stdio_usb(clock 1)
pico_enable_stdio_uart(clock 0)

# create map/bin/hex file etc.
pico_add_extra_outputs(clock)

