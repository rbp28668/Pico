#include <stdio.h>
#include "pico/stdlib.h"

#include "bsp_i2c.h"
#include "../lv_port/lv_port.h"
#include "demos/lv_demos.h"

#include "hardware/clocks.h"
#include "bsp_battery.h"
#include "bsp_lcd_brightness.h"

lv_obj_t *label_adc_raw;
lv_obj_t *label_voltage;
lv_obj_t *label_key;

lv_timer_t *battery_timer = NULL;
lv_timer_t *battery_key_timer = NULL;

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

static void battery_timer_callback(lv_timer_t *timer)
{
    char str_buffer[20];
    float voltage;
    uint16_t adc_raw;
    bsp_battery_read(&voltage, &adc_raw);
    lv_label_set_text_fmt(label_adc_raw, "%d", adc_raw);
    sprintf(str_buffer, "%.1f", voltage);
    lv_label_set_text(label_voltage, str_buffer);
}

static void battery_key_timer_callback(lv_timer_t *timer)
{
    if (bsp_battery_get_key_level())
        lv_label_set_text(label_key, "released");
    else
        lv_label_set_text(label_key, "pressed");
}

void lvgl_battery_ui_init(void)
{
    lv_obj_t *list = lv_list_create(lv_scr_act());
    lv_obj_set_size(list, lv_pct(100), lv_pct(100));

    lv_obj_t *list_item = lv_list_add_btn(list, NULL, "adc_raw");
    label_adc_raw = lv_label_create(list_item);
    lv_label_set_text(label_adc_raw, "0");

    list_item = lv_list_add_btn(list, NULL, "voltage");
    label_voltage = lv_label_create(list_item);
    lv_label_set_text(label_voltage, "0.0");

    list_item = lv_list_add_btn(list, NULL, "key");
    label_key = lv_label_create(list_item);
    lv_label_set_text(label_key, "released");

    battery_timer = lv_timer_create(battery_timer_callback, 1000, NULL);
    battery_key_timer = lv_timer_create(battery_key_timer_callback, 100, NULL);
}

int main()
{
    stdio_init_all();
    bsp_battery_init(100);
    set_cpu_clock(220 * 1000);
    bsp_i2c_init();
    lv_port_init();
    bsp_lcd_brightness_init();
    bsp_lcd_brightness_set(50);

    lvgl_battery_ui_init();
    while (true)
    {
        lv_timer_handler();
        sleep_ms(5);
    }
}