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

    static bsp_cst328_info_t cst328_info;
    cst328_info.width = LCD_WIDTH;
    cst328_info.height = LCD_HEIGHT;
    cst328_info.rotation = LCD_ROTATION;

    bsp_cst328_data_t cst328_data;

    bsp_i2c_init();
    bsp_st7789_init(&st7789_info);
    bsp_cst328_init(&cst328_info);
    bsp_lcd_brightness_init();
    bsp_lcd_brightness_set(50);

    uint16_t sleep_conut = 30;
    // bsp_st7789_clear(0, 0, st7789_info.width, st7789_info.height, 0x001f);
    for (int i = 0; i < st7789_info.width * st7789_info.height; i++)
    {
        lcd_framebuffer[i] = color_arr[0];
    }
    while (true)
    {
        bsp_cst328_read();
        if (bsp_cst328_get_touch_data(&cst328_data))
        {
            // printf("points:%d x:%d y:%d pressure:%d\r\n", cst328_data.points, cst328_data.coords[0].x, cst328_data.coords[0].y, cst328_data.coords[0].pressure);
            if (cst328_data.coords[0].x > st7789_info.width - 4)
                cst328_data.coords[0].x = st7789_info.width - 4;
            if (cst328_data.coords[0].y > st7789_info.height - 4)
                cst328_data.coords[0].y = st7789_info.height - 4;

            for (int w = 0; w < 4; w++)
            {
                for (int h = 0; h < 4; h++)
                {
                    *(lcd_framebuffer + st7789_info.width * (cst328_data.coords[0].y + h) + cst328_data.coords[0].x + w) = color_arr[1];
                }
            }
        }
        if (++sleep_conut >= 20)
        {
            while (!lcd_flush_done)
                sleep_us(50);
            lcd_flush_done = false;
            sleep_conut = 0;
            bsp_st7789_flush_dma(0, 0, st7789_info.width - 1, st7789_info.height - 1,
                                 lcd_framebuffer);
        }

        sleep_ms(1);
    }
}
