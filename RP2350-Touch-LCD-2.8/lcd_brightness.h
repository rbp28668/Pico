#ifndef __BSP_LCD_BRIGHTNESS_H__
#define __BSP_LCD_BRIGHTNESS_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "pico/stdlib.h"

#define BSP_LCD_BL_PIN    16

class LcdBrightness {

    uint slice_num;
    uint pwm_channel;
    uint8_t _pin;
public:
    LcdBrightness(uint8_t pin);
    void set(uint8_t percent);
};


#endif //__BSP_LCD_BRIGHTNESS_H__

