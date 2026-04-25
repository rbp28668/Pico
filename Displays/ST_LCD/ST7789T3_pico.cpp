#include "ST7789T3_pico.h"
#include "ST7789T3_commands.h"

static uint8_t init_commands[] = {

8,  // command count    
0x01, TFTDisplay::DELAY, 150,     // SWRESET

0x11, TFTDisplay::DELAY, 120,     // SLPOUT


0x3A, 1,            // COLMOD
0x55,          // 16-bit/pixel (RGB565)

0x36, 1,           // MADCTL
0x00,          // MV=0 → native 240x320 portrait, RGB

0x21, 0,           // INVON (if panel needs it)

0x2A, 4,           // CASET
0x00,          // XS = 0
0x00,
0x00,          // XE = 239
0xEF,

0x2B, 4,           // RASET
0x00,          // YS = 0
0x00,
0x01,          // YE = 319
0x3F,

0x29, TFTDisplay::DELAY, 20           // DISPON


};




 ST7789T3_pico::ST7789T3_pico(SPI* pspi, uint8_t CS, uint8_t DC, uint8_t RST )
 :TFTDisplay(240,320,pspi,CS,DC,RST){

    gpio_init(DC);
    gpio_init(CS);
    gpio_init(RST);

    gpio_set_dir(DC, GPIO_OUT);
    gpio_set_dir(CS, GPIO_OUT);
    gpio_set_dir(RST, GPIO_OUT);


    reset();               // RESX low 10µs, high, wait 120ms
    //commandList(init_commands);

    writecommand(ST7789_DISPON); // Display on
    delay(10);

    writecommand(ST7789_SLPOUT);
    delay(10); // ms
    
    writecommand(ST7789_MADCTL);
    writedata(0x00);

    writecommand(ST7789_COLMOD);
    writedata(0x55); // 16 bit data 

    writecommand(ST7789_RAMCTRL);
    writedata(0x00);
    writedata(0xE0); // 5 to 6-bit conversion: r0 = r5, b0 = b5, no byte swap

    writecommand(ST7789_PORCTRL);
    writedata(0x0C);
    writedata(0x0C);
    writedata(0x00);
    writedata(0x33);
    writedata(0x33);

    writecommand(ST7789_GCTRL);
    writedata(0x75); // VGH=14.97V,VGL=-7.67V

    writecommand(ST7789_VCOMS);
    writedata(0x1A);

    writecommand(ST7789_LCMCTRL);
    writedata(0x2C);

    writecommand(ST7789_VDVVRHEN);
    writedata(0x01);
    writedata(0xFF);

    writecommand(ST7789_VRHS);
    writedata(0x13);

    writecommand(ST7789_VDVS);
    writedata(0x20);

    writecommand(ST7789_FRCTRL2);
    writedata(0x0F);

    writecommand(ST7789_PWCTRL1);
    writedata(0xA4);
    writedata(0xA1);

    writecommand(0xD6);
    writedata(0xA1);

    writecommand(ST7789_PVGAMCTRL);
    writedata(0xD0);
    writedata(0x0D);
    writedata(0x14);
    writedata(0x0D);
    writedata(0x0D);
    writedata(0x09);
    writedata(0x38);
    writedata(0x44);
    writedata(0x4E);
    writedata(0x3A);
    writedata(0x17);
    writedata(0x18);
    writedata(0x2F);
    writedata(0x30);

    writecommand(ST7789_NVGAMCTRL);
    writedata(0xD0);
    writedata(0x09);
    writedata(0x0F);
    writedata(0x08);
    writedata(0x07);
    writedata(0x14);
    writedata(0x37);
    writedata(0x44);
    writedata(0x4D);
    writedata(0x38);
    writedata(0x15);
    writedata(0x16);
    writedata(0x2C);
    writedata(0x2E);

    writecommand(ST7789_INVON);

    writecommand(ST7789_DISPON);

    writecommand(ST7789_RAMWR);

 }
