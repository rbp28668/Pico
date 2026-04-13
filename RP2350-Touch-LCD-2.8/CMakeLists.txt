
#Build file for RP2350-Touch-LCD-2.8 from Waveshare test
cmake_minimum_required(VERSION 3.16)

set(PICO_PLATFORM rp2350)
set(PICO_BOARD pico2 CACHE STRING "Board type")

# Pull in PICO SDK (must be before project)
include(pico_sdk_import.cmake)


project(RPTouch28 C CXX ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)



# Initialize the SDK
pico_sdk_init()


add_executable(RPTouch28
        main.cpp
         ../PicoHardware/spi.cpp
         ../Sensors/QMI8658.cpp
        ../Displays/ST7735/ST7735_pico.cpp
        ../Displays/glcdfont.c
        ../Displays/print.cpp
        ../Displays/GFX.cpp
        ../Displays/TFT_Display.cpp
        ../Displays/fonts/font_Arial.c
        ../Displays/fonts/font_ArialBold.c
        )

# Pull in our pico_stdlib which pulls in commonly used features
target_link_libraries(RPTouch28
        pico_stdlib 
        hardware_i2c
        hardware_spi
        )

# enable usb output, disable uart output
pico_enable_stdio_usb(RPTouch28 1)
pico_enable_stdio_uart(RPTouch28 0)

# create map/bin/hex file etc.
pico_add_extra_outputs(RPTouch28)

