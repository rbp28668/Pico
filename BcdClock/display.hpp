#ifndef BCD_DISPLAY_HPP
#define BCD_DISPLAY_HPP

#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include "neopixel.hpp"
#include <cstdint>

class BcdDisplay {

    Neopixels neopixels;

    uint32_t colours[30];
    time_t lastTime;

    // HSV colour space for digits
    float dh,ds,dv;

    // HSV colour space for colon
    float ch,cs,cv;

    bool autoBrightness = false;
    float brightness = 0.0f;

    int setDigit(int idx, int digit, bool up);

    uint32_t convertToRaw(uint32_t rgb){
        uint8_t r = (rgb & 0xFF0000) >> 16;
        uint8_t g = (rgb & 0x00FF00) >> 8;
        uint8_t b = (rgb & 0x0000FF);
        return Neopixels::rgb(r,g,b);
    }

    public:
    BcdDisplay();

    void update(time_t t);

    void setDigitColour(uint32_t rgb);
    void setColonColour(uint32_t rgb);
    
    void setDigitColour(float h, float s, float v);
    void setColonColour(float h, float s, float v);
    
    void setBrightness(float v);

    void setNotify1(uint32_t rgb);
    void setNotify2(uint32_t rgb);
    
    void setAutoBrightness(bool on);
    void setLightLevel(uint32_t light);

    void redraw();
    
};

#endif