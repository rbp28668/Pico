

cmake_minimum_required(VERSION 3.16)

set(PICO_BOARD pico_w) # otherwise won't find <cyw43.h> etc.

# Pull in PICO SDK (must be before project)
include(pico_sdk_import.cmake)


project(bcd_clock C CXX ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

add_compile_definitions( 
        PICO_WIFI_SSID="$ENV{WIFI_SSID}" 
        PICO_WIFI_PASSWORD="$ENV{WIFI_PASSWORD}" 
        CYW43_HOST_NAME="bcd-clock"
)

# Initialize the SDK
pico_sdk_init()


add_executable(bcd_clock)

pico_generate_pio_header(bcd_clock ${CMAKE_CURRENT_LIST_DIR}/ws2812.pio)


target_sources(bcd_clock PRIVATE
main.cpp
display.cpp
neopixel.cpp
adc.cpp
adc_webapp.cpp
dma.cpp
index.cpp
clock_webapp.cpp
ntp_client.cpp
clock.cpp
bst.cpp
mqtt.cpp
history.cpp
history_webapp.cpp
../WebServer/wifi.cpp
../WebServer/server.cpp
../WebServer/webserver.cpp
../WebServer/block_malloc.cpp
../WebServer/block_list.cpp
../WebServer/webapp404.cpp
../WebServer/teapot.cpp
)

target_include_directories(bcd_clock PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# Pull in our pico_stdlib which pulls in commonly used features
target_link_libraries(bcd_clock PRIVATE
        pico_stdlib 
        hardware_pio
        hardware_dma
        hardware_adc
        pico_cyw43_arch_lwip_poll
        pico_multicore
        pico_lwip_mdns
)

# enable usb output, disable uart output
pico_enable_stdio_usb(bcd_clock 0)
pico_enable_stdio_uart(bcd_clock 1)

# create map/bin/hex file etc.
pico_add_extra_outputs(bcd_clock)

