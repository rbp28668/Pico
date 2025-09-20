

cmake_minimum_required(VERSION 3.16)

set(PICO_BOARD pico_w) # otherwise won't find <cyw43.h> etc.

# Pull in PICO SDK (must be before project)
include(pico_sdk_import.cmake)


project(pixel_string C CXX ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

add_compile_definitions( 
        PICO_WIFI_SSID="$ENV{WIFI_SSID}" 
        PICO_WIFI_PASSWORD="$ENV{WIFI_PASSWORD}" 
        CYW43_HOST_NAME="pixel_string"
)

# Initialize the SDK
pico_sdk_init()


add_executable(pixel_string)

pico_generate_pio_header(pixel_string ${CMAKE_CURRENT_LIST_DIR}/ws2812.pio)


target_sources(pixel_string PRIVATE
main.cpp
display.cpp
neopixel.cpp
dma.cpp
index.cpp
display_webapp.cpp
../WebServer/wifi.cpp
../WebServer/server.cpp
../WebServer/webserver.cpp
../WebServer/block_malloc.cpp
../WebServer/block_list.cpp
../WebServer/webapp404.cpp
../WebServer/teapot.cpp
)

target_include_directories(pixel_string PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# Pull in our pico_stdlib which pulls in commonly used features
target_link_libraries(pixel_string PRIVATE
        pico_stdlib 
        hardware_pio
        hardware_dma
        hardware_adc
        pico_cyw43_arch_lwip_poll
        pico_multicore
        pico_lwip_mdns
)

# enable usb output, disable uart output
pico_enable_stdio_usb(pixel_string 1)
pico_enable_stdio_uart(pixel_string 0)

# create map/bin/hex file etc.
pico_add_extra_outputs(pixel_string)

