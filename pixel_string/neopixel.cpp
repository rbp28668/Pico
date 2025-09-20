/**
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <assert.h>


#include "pico/stdlib.h"
#include "pico/time.h"
#include "hardware/clocks.h"


#include "ws2812.pio.h"
#include "neopixel.hpp"

#define IS_RGBW false
#define WS2812_PIN 15

/// @brief Utility function to add 2 RGB values together.
/// @param c1 
/// @param c2 
/// @return 
static uint32_t addRgb(uint32_t c1, uint32_t c2){

    uint32_t r = (c1 & 0xFF0000) + (c2 & 0xFF0000);
    uint32_t g = (c1 & 0x00FF00) + (c2 & 0x00FF00);
    uint32_t b = (c1 & 0x0000FF) + (c2 & 0x0000FF);

    if(r > 0xFF0000) r = 0xFF0000;
    if(g > 0x00FF00) g = 0x00FF00;
    if(b > 0x0000FF) b = 0x0000FF;

    return r | g | b;
}






////////////////////////////////////////////////////////////////////////////////////////////

Neopixels::Neopixels()
: pio(pio0)
, sm(0)
, pixels(buffer)
{

    // Setup PIO
    pio_sm_claim(pio, sm); // check not used by any other library in the future.
    uint offset = pio_add_program(pio, &ws2812_program);
    ws2812_program_init(pio, sm, offset, WS2812_PIN, 800000, IS_RGBW);

    // Setup DMA for writing to pixels.
    DmaConfig cfg = dma.getDefaultConfig();
    cfg.bswap(false);
    cfg.transferDataSize(DMA_SIZE_32);
    cfg.dreq(pio_get_dreq(pio,sm,true));
    cfg.enable(true);
    cfg.readIncrement(true);
    cfg.writeIncrement(false);
    dma.configure(cfg, &pio->txf[sm], 0, PIXEL_COUNT);
    
   
    // Zero the pixel buffer.
    for(int i=0; i<PIXEL_COUNT*2; ++i){
        buffer[i] = 0;
    }

}


uint32_t Neopixels::hsvToRgb(float hue, float saturation, float value){
    float r, g, b;

    if(hue > 1.0) hue = 1.0;
    if(saturation > 1.0) saturation = 1.0;
    if(value > 1.0) value = 1.0;

    int h = (int)(hue * 6);
    float f = hue * 6 - h;
    float p = value * (1 - saturation);
    float q = value * (1 - f * saturation);
    float t = value * (1 - (1 - f) * saturation);

    if (h == 0) {
        r = value;
        g = t;
        b = p;
    } else if (h == 1) {
        r = q;
        g = value;
        b = p;
    } else if (h == 2) {
        r = p;
        g = value;
        b = t;
    } else if (h == 3) {
        r = p;
        g = q;
        b = value;
    } else if (h == 4) {
        r = t;
        g = p;
        b = value;
    } else if (h <= 6) {
        r = value;
        g = p;
        b = q;
    } else {
        assert(false);
    }

    uint32_t rgb = 
        ((int)(r * 255) << 16) +
        ((int)(g * 255) << 8) +
        (int (b * 255));
    return rgb;
}

uint32_t Neopixels::hvToRgb(float hue, float value){
    float r, g, b;

    if(hue > 1.0) hue = 1.0;
    if(value > 1.0) value = 1.0;

    int h = (int)(hue * 6);
    float f = hue * 6 - h;
    float p = 0;
    float q = value * (1 - f );
    float t = value * f;

    if (h == 0) {
        r = value;
        g = t;
        b = p;
    } else if (h == 1) {
        r = q;
        g = value;
        b = p;
    } else if (h == 2) {
        r = p;
        g = value;
        b = t;
    } else if (h == 3) {
        r = p;
        g = q;
        b = value;
    } else if (h == 4) {
        r = t;
        g = p;
        b = value;
    } else if (h <= 6) {
        r = value;
        g = p;
        b = q;
    } else {
        assert(false);
    }

    uint32_t rgb = 
        ((int)(r * 255) << 16) +
        ((int)(g * 255) << 8) +
        (int (b * 255));
    return rgb;
}


/// @brief Sends the array of pixels to the display.
void Neopixels::send(){
    while(dma.isBusy()){
        ::sleep_ms(1);
    }
    //dma.waitForFinish();
    dma.fromBufferNow((void*)pixels, PIXEL_COUNT);
    
    // Flip buffer pointer for double buffering so we don't
    // write into the memory the DMA is writing.
    if(pixels == buffer) {
        pixels = buffer + PIXEL_COUNT;
    } else {
        pixels = buffer;
    }

    // Old implementation.
    // for(int i=0; i<PIXEL_COUNT; ++i){
    //     pio_sm_put_blocking(pio0, 0, pixels[i]);
    // }
}


/// @brief Sets an individual pixel.
/// @param idx 
/// @param rgb 
/// @param white 
void Neopixels::setPixel(int idx, uint32_t rgbPixel){
    assert(idx >= 0 && idx < PIXEL_COUNT);

    uint8_t r = (rgbPixel & 0xFF0000)>>16;
    uint8_t g = (rgbPixel & 0x00FF00)>>8;
    uint8_t b = (rgbPixel & 0x0000FF);

    pixels[idx] = rgb(r,g,b);
}

void Neopixels::setPixelRaw(int idx, uint32_t rgbw){
    assert(idx >= 0 && idx < PIXEL_COUNT);
    pixels[idx] = rgbw;
}

