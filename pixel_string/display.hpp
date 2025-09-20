#ifndef LED_DISPLAY_HPP
#define LED_DISPLAY_HPP

#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include "neopixel.hpp"
#include <cstdint>

class LedDisplay {

    Neopixels neopixels;

    static constexpr unsigned int LENGTH = 50;
    uint32_t colours[LENGTH];
    unsigned long tick;
 
    // HSV colour space for current colour
    float h,s,v;

    bool chase;    // true if we run a chaser light through th string
    bool rainbow;  // true if we shift colours through the string
    float delta_v;
    bool sparkle;

    uint32_t convertToRaw(uint32_t rgb){
        uint8_t r = (rgb & 0xFF0000) >> 16;
        uint8_t g = (rgb & 0x00FF00) >> 8;
        uint8_t b = (rgb & 0x0000FF);
        return Neopixels::rgb(r,g,b);
    }

    void shiftOut(){
        for(int i=LENGTH-1; i>=1; --i) colours[i] = colours[i-1];
    }

    void shiftIn(){
        for(int i=0;i<LENGTH-1;++i) colours[i] = colours[i+1];
    }

    public:
    LedDisplay();

    inline void setChase(bool chase) {this->chase = chase;}
    inline void setRainbow(bool rainbow) {this->rainbow = rainbow;}
    inline void setSparkle(bool sparkle) {this->sparkle = sparkle;}
    inline void setHueChange(float dv) {
        if(dv < 0) dv = 0; 
        if(dv > 0.5f) dv = 0.5f; 
        this->delta_v = dv;
    }

    void setHSV(float hue, float sat, float val){
        if(hue < 0.0f) hue = 0.0f;
        if(hue > 1.0f) hue = 1.0f;
        if(sat < 0.0f) sat = 0.0f;
        if(sat > 1.0f) sat = 1.0f;
        if(val < 0.0f) val = 0.0f;
        if(val > 1.0f) val = 1.0f;
        this->h = hue;
        this->s = sat;
        this->v = val;
    }

    void update();

    void redraw();
    
};

#endif