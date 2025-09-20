#include <stdio.h>
#include "pico/stdlib.h"
#include "ILI9341_pico.h"
#include "font_Arial.h"
const uint8_t LED_PIN = 25;

const uint8_t SCL_PIN = 18; //IN 24, GPIO18 -> SCL
const uint8_t SDA_PIN = 19; //PIN 25, GPIO19 -> SDA
const uint8_t RES_PIN = 20; //PIN 26, GPIO20 -> RES
const uint8_t DC_PIN = 21; //PIN 27, GPIO21 -> DC
const uint8_t CS_PIN = 17; //PIN 22, GPIO17 (SPIO-CS) -> CS
const uint8_t BLK_PIN = 26; //PIN 31, GPIO26 ->  BLK



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
  
    screen.fillScreen(ILI9341_RED );

    sleep_ms(1000);

    while(true) {
        uint16_t colour = 1;
        for(int i=0; i<32; ++i){
            bool msbit = (colour & 0x8000) != 0;
            colour <<=1;
            colour |= msbit ? 1 : 0;
            screen.fillScreen(colour);  // Note 16 bit colour - just let it overflow to zero again.
            sleep_ms(40);
        }
        screen.fillScreen(ILI9341_BLACK );
        screen.drawLine(0,0, screen.width(), screen.height(), ILI9341_BLUE);
        screen.drawLine(0,screen.height(), screen.width(), 0, ILI9341_BLUE);
        sleep_ms(1000);

        screen.fillScreen(ILI9341_BLACK );
        screen.setTextColor(ILI9341_WHITE);
        screen.drawString("Hello World", 1, screen.height()/2);
        sleep_ms(1000);

        screen.setTextColor(ILI9341_GREEN);
        screen.setFont(Arial_18);
        screen.setRotation(3);
        screen.setCursor(1,6);
        screen.drawString("Hello World", 1, screen.height()/2);
     
       sleep_ms(1000);
    }
  
}
