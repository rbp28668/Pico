

cmake_minimum_required(VERSION 3.16)

set(PICO_BOARD pico_w) # otherwise won't find <cyw43.h> etc.

# Pull in PICO SDK (must be before project)
include(pico_sdk_import.cmake)


project(weather C CXX ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

add_compile_definitions( 
        PICO_WIFI_SSID="$ENV{WIFI_SSID}" 
        PICO_WIFI_PASSWORD="$ENV{WIFI_PASSWORD}" 
)

# Initialize the SDK
pico_sdk_init()


add_executable(weather)

target_sources(weather PRIVATE
main.cpp
weather.cpp
weather_webapp.cpp
index.cpp
../PicoHardware/i2c.cpp
../Sensors/HDC1080.cpp
../Sensors/BMP280.cpp
../Sensors/MCP9808.cpp
../Sensors/GY30.cpp
../WebServer/wifi.cpp
../WebServer/server.cpp
../WebServer/webserver.cpp
../WebServer/block_malloc.cpp
../WebServer/block_list.cpp
../WebServer/webapp404.cpp
../WebServer/teapot.cpp
)

target_include_directories(weather PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# Pull in our pico_stdlib which pulls in commonly used features
target_link_libraries(weather PRIVATE
        pico_stdlib 
        hardware_i2c
        pico_cyw43_arch_lwip_poll
        pico_multicore
       )

# enable usb output, disable uart output
pico_enable_stdio_usb(weather 1)
pico_enable_stdio_uart(weather 0)

# create map/bin/hex file etc.
pico_add_extra_outputs(weather)

