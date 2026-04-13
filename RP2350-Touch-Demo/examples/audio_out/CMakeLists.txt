add_executable(audio_out
    audio_out.c
    )

pico_enable_stdio_usb(audio_out 1)
pico_enable_stdio_uart(audio_out 0)

# Add the standard include files to the build
target_include_directories(audio_out PRIVATE
  ${CMAKE_CURRENT_LIST_DIR}
)

# pull in common dependencies
target_link_libraries(audio_out 
    pico_stdlib
    bsp)

    
# create map/bin/hex/uf2 file etc.
pico_add_extra_outputs(audio_out)