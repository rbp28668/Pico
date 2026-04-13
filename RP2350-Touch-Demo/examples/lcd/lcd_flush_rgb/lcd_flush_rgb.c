#include <stdio.h>
#include "pico/stdlib.h"
#include "bsp_i2c.h"
#include "bsp_st7789.h"
#include "bsp_cst328.h"
#include "bsp_lcd_brightness.h"
#include "bsp_battery.h"

#define LCD_WIDTH 240
#define LCD_HEIGHT 320
#define LCD_ROTATION 0

uint16_t color_arr[6] = {0xf800, 0x07e0, 0x001f, 0xf800, 0x07e0, 0x001f};
uint8_t color_index = 0;

uint16_t lcd_framebuffer[LCD_WIDTH * LCD_HEIGHT];

bool lcd_flush_done = true;

void lcd_flush_done_callback(void)
{
    lcd_flush_done = true;
}

int main()
{
    stdio_init_all();
    bsp_battery_init(100);
    static bsp_st7789_info_t st7789_info;
    st7789_info.width = LCD_WIDTH;
    st7789_info.height = LCD_HEIGHT;
    st7789_info.rotation = LCD_ROTATION;
    st7789_info.x_offset = 0;
    st7789_info.y_offset = 0;
    st7789_info.enabled_dma = true;
    st7789_info.dma_flush_done_callback = lcd_flush_done_callback;
    bsp_st7789_init(&st7789_info);
    bsp_lcd_brightness_init();
    bsp_lcd_brightness_set(50);
    while (true)
    {
        for (int i = 0; i < st7789_info.width * st7789_info.height; i++)
        {
            lcd_framebuffer[i] = color_arr[color_index];
        }
        while (!lcd_flush_done)
            sleep_us(100);
        lcd_flush_done = false;
        bsp_st7789_flush_dma(0, 0, st7789_info.width - 1, st7789_info.height - 1,
            lcd_framebuffer);
        if (++color_index >= 3)
            color_index = 0;
        sleep_ms(1000);
    }
}
