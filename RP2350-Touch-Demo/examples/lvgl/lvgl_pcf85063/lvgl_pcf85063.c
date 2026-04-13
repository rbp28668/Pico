#include <stdio.h>
#include "pico/stdlib.h"

#include "bsp_i2c.h"
#include "../lv_port/lv_port.h"
#include "demos/lv_demos.h"

#include "hardware/clocks.h"
#include "bsp_battery.h"
#include "bsp_lcd_brightness.h"
#include "bsp_pcf85063.h"

lv_obj_t *label_time;
lv_obj_t *label_date;

lv_timer_t *pcf85063_timer = NULL;

void set_cpu_clock(uint32_t freq_khz)
{
    set_sys_clock_khz(freq_khz, true);
    clock_configure(
        clk_peri,
        0,
        CLOCKS_CLK_PERI_CTRL_AUXSRC_VALUE_CLKSRC_PLL_SYS,
        freq_khz * 1000,
        freq_khz * 1000);
}

static void pcf85063_callback(lv_timer_t *timer)
{
    struct tm now_tm;
    bsp_pcf85063_get_time(&now_tm);
    lv_label_set_text_fmt(label_time, "%02d:%02d:%02d", now_tm.tm_hour, now_tm.tm_min, now_tm.tm_sec);
    lv_label_set_text_fmt(label_date, "%04d-%02d-%02d", now_tm.tm_year + 1900, now_tm.tm_mon + 1, now_tm.tm_mday);
}

void lvgl_pcf85063_ui_init(void)
{
    lv_obj_t *list = lv_list_create(lv_scr_act());
    lv_obj_set_size(list, lv_pct(100), lv_pct(100));

    lv_obj_t *list_item = lv_list_add_btn(list, NULL, "time");
    label_time = lv_label_create(list_item);
    lv_label_set_text(label_time, "12:00:00");

    list_item = lv_list_add_btn(list, NULL, "date");
    label_date = lv_label_create(list_item);
    lv_label_set_text(label_date, "2024-12-01");

    pcf85063_timer = lv_timer_create(pcf85063_callback, 1000, NULL);
}

int main()
{
    struct tm now_tm;
    stdio_init_all();
    bsp_battery_init(100);
    set_cpu_clock(220 * 1000);
    bsp_i2c_init();
    lv_port_init();
    bsp_lcd_brightness_init();
    bsp_lcd_brightness_set(50);
    
    bsp_pcf85063_init();

    lvgl_pcf85063_ui_init();

    bsp_pcf85063_get_time(&now_tm);

    if (now_tm.tm_year < 124 || now_tm.tm_year > 130)
    {
        now_tm.tm_year = 2024 - 1900; // The year starts from 1900
        now_tm.tm_mon = 12 - 1;       // Months start from 0 (November = 10)
        now_tm.tm_mday = 1;           // Day of the month
        now_tm.tm_hour = 12;          // Hour
        now_tm.tm_min = 0;            // Minute
        now_tm.tm_sec = 0;            // Second
        now_tm.tm_isdst = -1;         // Automatically detect daylight saving time
        bsp_pcf85063_set_time(&now_tm);
    }

    while (true)
    {
        lv_timer_handler();
        sleep_ms(1);
    }
}