#include <stdio.h>
#include <assert.h>
#include "pico/stdlib.h"
#include "../Displays/ILI9341_pico.h"
#include "../Displays/fonts/font_Arial.h"
#include "../Displays/gfx_fonts/FreeSerifBold12pt7b.h"
#include "../Displays/gfx_fonts/FreeSerifBold24pt7b.h"

// Pins for driving the screen via SPI
const uint8_t SCL_PIN = 18; //IN 24, GPIO18 -> SCL
const uint8_t SDA_PIN = 19; //PIN 25, GPIO19 -> SDA
const uint8_t RES_PIN = 20; //PIN 26, GPIO20 -> RES
const uint8_t DC_PIN = 21; //PIN 27, GPIO21 -> DC
const uint8_t CS_PIN = 17; //PIN 22, GPIO17 (SPIO-CS) -> CS
const uint8_t BLK_PIN = 26; //PIN 31, GPIO26 ->  BLK

extern "C" int main() {

    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN,GPIO_OUT);
    gpio_put(LED_PIN,0);


    //Initialise I/O
    stdio_init_all(); 

    // Initialise ILI9341 Screen on SPI0
    HardwareSPI spi(spi0, 16, SCL_PIN, SDA_PIN, 20000000); // CS pin not part of SPI per se as really tied to addressable device
    spi.setDedicated(CS_PIN);  // not that CS is used on the sample display.

    ILI9341_pico screen(&spi,CS_PIN, DC_PIN,RES_PIN); // Hence CS is included here.
    screen.begin();
  
    screen.fillScreen(GFX::BLACK );
    screen.setRotation(0);

    screen.drawString("Initialised",1,300);
    
    bool stop = false;
    while(!stop) {

   
        screen.fillScreen(GFX::BLACK );
        screen.setTextColor(GFX::GREEN);

     
        int y = 20;
        int x = 1;
        screen.setFont(Arial_18);
        x = 120;
    
        screen.drawString("Pressure ", 1, y);
        screen.drawFloat(42, 2, x, y);
        y += screen.fontLineSpace() + 4;
     

        ::sleep_ms(1000);
       } 
}
