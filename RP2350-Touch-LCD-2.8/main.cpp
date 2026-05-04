#include <ctype.h>
#include <stdint.h>
#include "lcd_brightness.h"
#include "battery.h"
#include "../PicoHardware/i2c.h"
#include "../PicoHardware/spi.h"
#include "../Displays/ST_LCD/ST7789T3_pico.h"
#include "../Sensors/QMI8658.h"
#include "../Sensors/CST328.h"
#include "main.h"


void show_colours(ST7789T3_pico &display);
void show_imu(Battery &battery, QMI8658 &imu, ST7789T3_pico &display);
void draw(Battery &battery, CST328& touch, ST7789T3_pico &display);
void set_brightness(Battery &battery, CST328 &touch, LcdBrightness& brightness, ST7789T3_pico &display);


void showBar(TFTDisplay &display, int value, int y)
{
    int bar = (120 * value) / (16 * 8192); // scale summed reading to pixels
    if (bar > 0)
    {
        display.fillRect(120, y, bar, 10, 0);
        display.fillRect(120 + bar, y, 120 - bar, 10, 0xF81F);
        display.fillRect(0, y, 120, 10, 0xF81F);
    }
    else
    {
        display.fillRect(120 + bar, y, -bar, 10, 0xFFFF);
        display.fillRect(0, y, 120 + bar, 10, 0xF81F);
        display.fillRect(120, y, 120, 10, 0xF81F);
    }
}
int main(int argc, char **argv)
{

    // Pinouts

    // I2S sound
    const uint8_t I2S_BCK = 2;
    const uint8_t I2S_LRCK = 3;
    const uint8_t I2s_DIN = 4;

    // RTC
    const uint8_t RTC_INT = 5;

    // SDA/SCL for device I2C - shared
    const uint8_t DEV_SDA = 6;
    const uint8_t DEV_SCL = 7;

    // IMU
    const uint8_t IMU_INT1 = 8;
    const uint8_t LCD_INT2 = 9;

    // LCD
    const uint8_t LCD_SCK = 10;  // SPI
    const uint8_t LCD_MOSI = 11; // SPI
    const uint8_t LCD_MISO = 12; // SPI
    const uint8_t LCD_CS = 13;   // LCD chip select
    const uint8_t LCD_D_C = 14;  // Data / Command
    const uint8_t LCD_RST = 15;  // Reset
    const uint8_t LCD_BL = 16;   // Backlight

    // Touchpad
    const uint8_t TP_RST = 17; // touchpad reset
    const uint8_t TP_INT = 18; // touchpad interrupt

    // SD Card
    const uint8_t SD_SCK = 19;
    const uint8_t SD_CMD = 20;
    const uint8_t SD_D0 = 21;
    const uint8_t SD_D1 = 22;
    const uint8_t SD_D2 = 23;
    const uint8_t SD_D3 = 24;

    // I2d Addresses
    const uint8_t QMI8658_DEVICE_ADDR = QMI8658_SECONDARY_I2C;   // IMU
    const uint8_t PCF85063_DEVICE_ADDR = 0x51;                   // RTC

    HardwareSPI spi(spi1, LCD_MISO, LCD_SCK, LCD_MOSI, 2500000);
    spi.setDedicated(LCD_CS);

    ST7789T3_pico display(&spi, LCD_CS, LCD_D_C, LCD_RST);
    display.fillScreen(GFX::color565(GFX::CYAN));

    LcdBrightness backlight(LCD_BL);
    backlight.set(100);


    I2C i2c(i2c1, DEV_SDA, DEV_SCL);
    QMI8658 imu(&i2c, QMI8658_DEVICE_ADDR);
    imu.reset();

    CST328 touch(&i2c,TP_RST,TP_INT, 0, 240, 320);  // no rotation,

    Battery battery;  // not using battery but want to read the switch

    display.setTextColor(GFX::color565(GFX::BLACK));



    show_colours(display);
    while(true){
        show_imu(battery, imu, display);
        draw(battery, touch, display);
        set_brightness(battery, touch, backlight, display);
    }
    return 0;
}

void show_colours(ST7789T3_pico& display){
    display.fillScreen(0xF800);
    display.setCursor(10, 30);
    display.print("RED");
    sleep_ms(1000);

    display.fillScreen(0x07E0); // GREEN 0x07E0
    display.setCursor(10, 30);
    display.print("GREEN");
    sleep_ms(1000);

    display.fillScreen(0x001F); // BLUE 0x001F
    display.setCursor(10, 30);
    display.print("BLUE");
    sleep_ms(1000);
}
void show_imu(Battery &battery, QMI8658 &imu, ST7789T3_pico &display)
{
    display.fillScreen(0xF81F); // magenta 0xF81F : GFX::color565(GFX::MAGENTA)
    display.setTextColor(GFX::color565(GFX::BLACK));

    while (battery.get_key_level())
    {

        int acc_x = 0, acc_y = 0, acc_z = 0;
        int gyro_x = 0, gyro_y = 0, gyro_z = 0;

        for (int i = 0; i < 16; ++i)
        {
            imu.read();
            acc_x += (int16_t)imu.acc_x();
            acc_y += (int16_t)imu.acc_y();
            acc_z += (int16_t)imu.acc_z();
            gyro_x += (int16_t)imu.gyro_x();
            gyro_y += (int16_t)imu.gyro_y();
            gyro_z += (int16_t)imu.gyro_z();
        }

        display.fillRect(0, 20, 240, 40, 0xF81F);
        display.setCursor(10, 30);
        display.print(acc_x);
        display.setCursor(80, 30);
        display.print(acc_y);
        display.setCursor(150, 30);
        display.print(acc_z);

        display.setCursor(10, 50);
        display.print(gyro_x);
        display.setCursor(80, 50);
        display.print(gyro_y);
        display.setCursor(150, 50);
        display.print(gyro_z);

        int y = 60;
        showBar(display, acc_x, y);
        y += 15;
        showBar(display, acc_y, y);
        y += 15;
        showBar(display, acc_z, y);
        y += 40;

        showBar(display, gyro_x, y);
        y += 15;
        showBar(display, gyro_y, y);
        y += 15;
        showBar(display, gyro_z, y);

        sleep_ms(100);
    }
}


void draw(Battery &battery, CST328 &touch, ST7789T3_pico &display){
    display.fillScreen(0xFFFF);  // GFX::color565(GFX::WHITE)
    while (battery.get_key_level()) {
        touch.read();
        while(!touch.read_data_done);
        CST328::Data data;
        touch.get_touch_data(&data);
        int size = 2 * data.points;
         
        for(int i=0; i<data.points; ++i){
            display.fillRect(data.coords[i].x, data.coords[i].y, size, size, data.coords[i].pressure);
           
        }
    }

}

void set_brightness(Battery &battery, CST328 &touch, LcdBrightness& brightness, ST7789T3_pico &display){
    display.fillScreen(0xFFFF);  // GFX::color565(GFX::WHITE)
    while (battery.get_key_level()) {
        touch.read();
        while(!touch.read_data_done);
        CST328::Data data;
        touch.get_touch_data(&data);
        if(data.points > 0){
            int pct = (data.coords[0].y * 100) / 320;  // y coordinate to percentage
            brightness.set(pct);
        }
        
    }

}


