
#ifndef __ST7789T3_pico_H_
#define __ST7789T3_pico_H_

#include <cstdint>
#include <string.h>
#include <stdlib.h>
#include "../..//PicoHardware/spi.h"
#include "TFT_Display.h"


class ST7789T3_pico : public TFTDisplay
{
 
 public:

  // Initialisation
  // pspi - SPI to use
  // CS - chip select pin to use
  // DC - display / command pin to use
  // RST - optional reset pin
  ST7789T3_pico(SPI* pspi, uint8_t CS, uint8_t DC, uint8_t RST = -1);
   // GFX CONTROL API
  //virtual void setRotation(uint8_t r);
  //virtual void invertDisplay(bool i);
   	

};


#endif