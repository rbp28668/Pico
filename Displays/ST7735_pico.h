/***************************************************
  This is a library for the Adafruit 1.8" SPI display.
  This library works with the Adafruit 1.8" TFT Breakout w/SD card
  ----> http://www.adafruit.com/products/358
  as well as Adafruit raw 1.8" TFT display
  ----> http://www.adafruit.com/products/618

  Check out the links above for our tutorials and wiring diagrams
  These displays use SPI to communicate, 4 or 5 pins are required to
  interface (RST is optional)
  Adafruit invests time and resources providing this open source code,
  please support Adafruit and open-source hardware by purchasing
  products from Adafruit!

  Written by Limor Fried/Ladyada for Adafruit Industries.
  MIT license, all text above must be included in any redistribution
 ****************************************************/

#ifndef __ST7735_pico_H_
#define __ST7735_pico_H_

#include <cstdint>
#include <string.h>
#include <stdlib.h>
#include "../PicoHardware/spi.h"
#include "TFT_Display.h"
#include "fonts.h"
#include "gfxfont.h"


#define SPI_MODE0 0

#define ST7735_SPICLOCK 24000000
//#define ST7735_SPICLOCK 16000000

// some flags for initR() :(
#define INITR_GREENTAB 0x0
#define INITR_REDTAB   0x1
#define INITR_BLACKTAB 0x2

#define INITR_18GREENTAB    INITR_GREENTAB
#define INITR_18REDTAB      INITR_REDTAB
#define INITR_18BLACKTAB    INITR_BLACKTAB
#define INITR_144GREENTAB   0x1
#define INITR_144GREENTAB_OFFSET   0x4
#define INITR_MINI160x80  0x05
#define INITR_MINI160x80_ST7735S 0x06

#define INIT_ST7789_TABCOLOR 42  // Not used except as a indicator to the code... 

#define ST7735_TFTWIDTH  128
#define ST7735_TFTWIDTH_80     80 // for mini
// for 1.44" display
#define ST7735_TFTHEIGHT_144 128
// for 1.8" display and mini
#define ST7735_TFTHEIGHT_160  160 // for 1.8" and mini display



// Color definitions
#define ST7735_BLACK   0x0000
#define ST7735_BLUE    0x001F
#define ST7735_RED     0xF800
#define ST7735_GREEN   0x07E0
#define ST7735_CYAN    0x07FF
#define ST7735_MAGENTA 0xF81F
#define ST7735_YELLOW  0xFFE0
#define ST7735_WHITE   0xFFFF

// Also define them in a non specific ST77XX specific name
#define ST77XX_BLACK      0x0000
#define ST77XX_WHITE      0xFFFF
#define ST77XX_RED        0xF800
#define ST77XX_GREEN      0x07E0
#define ST77XX_BLUE       0x001F
#define ST77XX_CYAN       0x07FF
#define ST77XX_MAGENTA    0xF81F
#define ST77XX_YELLOW     0xFFE0
#define ST77XX_ORANGE     0xFC00
#define ST77XX_PINK       0xF81F

// Map fonts that were modified back to the ILI9341 font
#define ST7735_pico_font_t ILI9341_t3_font_t




#define CL(_r,_g,_b) ((((_r)&0xF8)<<8)|(((_g)&0xFC)<<3)|((_b)>>3))


 

class ST7735_pico : public TFTDisplay
{
 
 public:

  // Initialisation
  ST7735_pico(SPI* pspi, uint8_t CS, uint8_t RS, uint8_t RST = -1);
  void     initB(void);                             // for ST7735B displays
  void     initR(uint8_t options = INITR_GREENTAB); // for ST7735R
 
  // GFX CONTROL API
  virtual void setRotation(uint8_t r);
  virtual void invertDisplay(bool i);
   	
	//////
	//virtual size_t write(uint8_t);		
	//virtual size_t write(const uint8_t *buffer, size_t size);
	
 	// Useful methods added from ili9341_t3 
  void writeRect(int16_t x, int16_t y, int16_t w, int16_t h, const uint16_t *pcolors);


 protected:
  uint8_t  tabcolor;


  void commandList(const uint8_t *addr);
  void commonInit(const uint8_t *cmdList, uint8_t mode=SPI_MODE0);
   //uint8_t  spiread(void);


 


 
  

};




#endif	 
