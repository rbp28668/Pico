add_executable(lvgl_battery
    lvgl_battery.c
    ../lv_port/lv_port.c
    )

pico_enable_stdio_usb(lvgl_battery 1)
pico_enable_stdio_uart(lvgl_battery 0)

# pull in common dependencies
target_link_libraries(lvgl_battery 
    pico_stdlib
    bsp
    lvgl
    )

# create map/bin/hex/uf2 file etc.
pico_add_extra_outputs(lvgl_battery)