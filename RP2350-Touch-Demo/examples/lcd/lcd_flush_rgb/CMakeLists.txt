add_executable(lcd_flush_rgb
    lcd_flush_rgb.c
    )

pico_enable_stdio_usb(lcd_flush_rgb 1)
pico_enable_stdio_uart(lcd_flush_rgb 0)

# pull in common dependencies
target_link_libraries(lcd_flush_rgb 
    pico_stdlib
    bsp)

# create map/bin/hex/uf2 file etc.
pico_add_extra_outputs(lcd_flush_rgb)