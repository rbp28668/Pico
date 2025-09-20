

cmake_minimum_required(VERSION 3.16)

# Pull in PICO SDK (must be before project)
include(pico_sdk_import.cmake)


project(displays C CXX ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# Initialize the SDK
pico_sdk_init()


add_executable(displays
        main.cpp
        ../PicoHardware/spi.cpp
        ILI9341_pico.cpp
        ST7735_pico.cpp
        glcdfont.c
        print.cpp
        GFX.cpp
        TFT_Display.cpp
        fonts/font_Arial.c
        fonts/font_ArialBold.c
        )

# Pull in our pico_stdlib which pulls in commonly used features
target_link_libraries(displays
        pico_stdlib 
        hardware_i2c
        hardware_spi
        )

# enable usb output, disable uart output
pico_enable_stdio_usb(displays 1)
pico_enable_stdio_uart(displays 0)

# create map/bin/hex file etc.
pico_add_extra_outputs(displays)

