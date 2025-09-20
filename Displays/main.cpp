#include <stdio.h>
#include "pico/stdlib.h"
#include "ILI9341_pico.h"
#include "fonts/font_Arial.h"
#include "gfx_fonts/FreeSerifBold12pt7b.h"
#include "gfx_fonts/FreeSerifBold24pt7b.h"

const uint8_t LED_PIN = 25;

const uint8_t SCL_PIN = 18; //IN 24, GPIO18 -> SCL
const uint8_t SDA_PIN = 19; //PIN 25, GPIO19 -> SDA
const uint8_t RES_PIN = 20; //PIN 26, GPIO20 -> RES
const uint8_t DC_PIN = 21; //PIN 27, GPIO21 -> DC
const uint8_t CS_PIN = 17; //PIN 22, GPIO17 (SPIO-CS) -> CS
const uint8_t BLK_PIN = 26; //PIN 31, GPIO26 ->  BLK

void testFills(GFX& screen, int delay) {
    // Test full screen fill
    uint16_t colours[] = {GFX::RED, GFX::GREEN, GFX::BLUE};
    for(int i=0; i<sizeof(colours)/sizeof(colours[0]); ++i){
        uint16_t colour = colours[i];
        screen.fillScreen(colour);  // Note 16 bit colour - just let it overflow to zero again.
        sleep_ms(250);
    }

    // fillrect Fills, all starting at top left.
    screen.fillScreen(GFX::BLACK );
    int w = screen.width();
    int h = screen.height();
    int x = 0;
    int y = 0;

    int idx = 0;
    while( w > 0 && h > 0){
        uint16_t colour = colours[idx];
        screen.fillRect( x, y, w, h, colour);
        w-=4;
        h-=4;
        idx = (idx + 1) % (sizeof(colours)/sizeof(colours[0]));
    }
    sleep_ms(delay);

    // Fills, centered
    screen.fillScreen(GFX::BLACK );
    w = screen.width();
    h = screen.height();
    x = 0;
    y = 0;

    idx = 0;
    while( w > 0 && h > 0){
        uint16_t colour = colours[idx];
        screen.fillRect( x, y, w, h, colour);
        w -= 8;
        h -= 8;
        x+=4;
        y+=4;
        idx = (idx + 1) % (sizeof(colours)/sizeof(colours[0]));
    }
    sleep_ms(delay);
    
    // hline and vline
    screen.fillScreen(GFX::BLACK );
    w = screen.width();
    h = screen.height();
    x = 0;
    y = 0;

    idx = 0;
    while( w > 0 && h > 0){
        uint16_t colour = colours[idx];
        screen.drawFastHLine(x,y,w,colour);
        screen.drawFastHLine(x,screen.height() - y, w, colour);
        screen.drawFastVLine(x,y,h, colour);
        screen.drawFastVLine(screen.width()-x,y,h,colour);
        w -= 8;
        h -= 8;
        x += 4;
        y += 4;
        idx = (idx + 1) % (sizeof(colours)/sizeof(colours[0]));
    }
    sleep_ms(delay);

	screen.fillScreenVGradient(GFX::RED, GFX::BLUE);
    sleep_ms(delay);

	screen.fillScreenHGradient(GFX::RED, GFX::BLUE);
    sleep_ms(delay);

}

void testLines(GFX& screen, int delay) {
            
    uint16_t colours[] = {GFX::RED, GFX::GREEN, GFX::BLUE};

    // drawPixel
    screen.fillScreen(GFX::BLACK );
    int16_t w = screen.width();
    int16_t h = screen.height();
    int16_t x = 0;
    int16_t y = 0;

    int16_t idx = 0;
    for(int j=0; j<h; ++j) {
        for(int i=0; i<w; ++i) {
            idx = ( (i+j)/4) % (sizeof(colours)/sizeof(colours[0]));
            uint16_t colour = colours[idx];
            screen.drawPixel(x+i, y+j, colour);
        }
    }
    sleep_ms(delay);

    // drawLine, horizontal, vertical and diagonal
    screen.fillScreen(GFX::BLACK );
    screen.drawLine(0,0, screen.width(), 0, GFX::RED);
    screen.drawLine(0,screen.height()-1, screen.width(), screen.height()-1, GFX::RED);
    screen.drawLine(0,0,0,screen.height()-1,GFX::GREEN);
    screen.drawLine(screen.width()-1, 0, screen.width()-1, screen.height()-1, GFX::GREEN);
        screen.drawLine(0,0, screen.width(), screen.height(), GFX::BLUE);
    screen.drawLine(0,screen.height(), screen.width(), 0, GFX::BLUE);
    sleep_ms(delay);
}

void testFonts(GFX& screen, int delay) {
       // Simple fonts

        screen.fillScreen(GFX::BLACK );
        screen.setTextColor(GFX::WHITE);
        screen.setFont();
 
        int16_t y = screen.height()/4;

        screen.setTextSize(1,1);
        screen.drawString("Default size", 1, y);
        y += screen.height()/4;

        screen.setTextSize(2,1);
        screen.drawString("Wider", 1, y);
        y += screen.height()/4;

        screen.setTextSize(1,2);
        screen.drawString("Higher", 1, y);
        y += screen.height()/4;
        sleep_ms(delay);

        // ILI93341 fonts
        screen.fillScreen(GFX::BLACK );
        screen.setTextColor(GFX::GREEN);
        y = 8+4;
        screen.setFont(Arial_8);
        y = screen.fontLineSpace() + 4;
        screen.drawString("Hello World Arial_8", 1, y);

        screen.setFont(Arial_12);
        y = y + screen.fontLineSpace() + 4;
        screen.drawString("Hello World Arial_12", 1, y);

       screen.setFont(Arial_18);
       y = y + screen.fontLineSpace() + 4;
       screen.drawString("Hello World Arial_18", 1, y);

       screen.setFont(Arial_32);
       y = y + screen.fontLineSpace() + 4;
       screen.drawString("Hello World Arial_32", 1, y);

       sleep_ms(delay);

        // GFX Fonts
        screen.fillScreen(GFX::BLACK );
        screen.setTextColor(GFX::RED);

        screen.setTextSize(1,1);
        screen.setFont(&FreeSerifBold12pt7b);
        y = screen.fontLineSpace() + 4;
        screen.drawString("Free Serif Bold 12pt", 1, y);
 
        screen.setFont(&FreeSerifBold24pt7b);
        y = screen.getCursorY() + screen.fontLineSpace();
        screen.setCursor(0, screen.getCursorY() + screen.fontLineSpace());
        screen.drawString("Free Serif Bold 24pt", 1, y);

       

       sleep_ms(delay);
}

extern "C" int main() {

    /*
    stdio_init_all();
    for(int i=0; i<20; ++i) {
        busy_wait_us_32(100000);
        printf(".");
    }
    printf("\n");
    printf("Telemetry\n");
    */


    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);
    gpio_put(LED_PIN, 0);

    HardwareSPI spi(spi0, 16, SCL_PIN, SDA_PIN, 20000000); // CS pin not part of SPI per se as really tied to addressable device
    spi.setDedicated(CS_PIN);  // not that CS is used on the sample display.

    ILI9341_pico screen(&spi,CS_PIN, DC_PIN,RES_PIN); // Hence CS is included here.
    screen.begin();
  
    screen.fillScreen(GFX::BLACK );

    sleep_ms(1000);

    int delay = 2000;

    while(true) {
        screen.setRotation(0);

        //testFills(screen, delay);

        //testLines(screen, delay);

        testFonts(screen, delay);
     
    }
  
}
