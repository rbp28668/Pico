#ifndef NEOPIXEL_HPP
#define NEOPIXEL_HPP


#include "pico/stdlib.h"
#include "pico/util/queue.h"
#include "hardware/pio.h"
#include "dma.hpp"


#define PIXEL_COUNT (50)
#define SCALE (10000.0f)  // for sending floats via ints


   // Pixel format is:  GGRRBBWW



class Neopixels {

    uint32_t colour;
    uint32_t white;

    uint32_t buffer[2*PIXEL_COUNT]; // Allow for double buffering.
    uint32_t* pixels;

    PIO pio; // which PIO is in use to drive the pixels.
    uint sm; // and which statemachine is in use.

    Dma dma; // to shovel pixels to the PIO

    const uint32_t RED = 0x00FF0000;
    const uint32_t GREEN = 0xFF000000;
    const uint32_t BLUE = 0x0000FF00;
    const uint32_t WHITE = 0x000000FF;


    // inline void put_pixel(uint32_t pixel) {
    //     pio_sm_put_blocking(pio, sm, pixel);
    // }

    public:
    
    static inline uint32_t rgb(uint8_t r, uint8_t g, uint8_t b) {
        return
                ((uint32_t) (r) << 24) |
                ((uint32_t) (g) << 16) |
                ((uint32_t) (b) << 8);
    }

 
   

    Neopixels();
    void send();
   void setPixel(int idx, uint32_t rgb);  // single pixel
    void setPixelRaw(int idx, uint32_t pixel);  // single pixel in Neopixel format
    
   
    static uint32_t hsvToRgb(float h, float s, float v); 
    static uint32_t hvToRgb(float hue, float value); // assumes s = 1
   
   

};


#endif