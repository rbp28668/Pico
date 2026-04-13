#ifndef __ST7796_H__
#define __ST7796_H__

#include "DEV_Config.h"
#include <stdint.h>

#include <stdlib.h>		//itoa()
#include <stdio.h>

#include "../../PicoHardware/spi.h"
#include "../TFT_Display.h"

#define ST7796_WIDTH        320
#define ST7796_HEIGHT       480

class ST7796 : public TFTDisplay {

    public:
    ST7796(SPI* pspi, uint8_t CS, uint8_t RS, uint8_t RST = -1);

    void writeRect(int16_t x, int16_t y, int16_t w, int16_t h, const uint16_t *pcolors);

}
void st7796_init(void);
void st7796_draw_rectangle(uint16_t x_start, uint16_t y_start, uint16_t x_end, uint16_t y_end, uint16_t color);
void st7796_clear(uint16_t color);

#endif  // __ST7796_H__

