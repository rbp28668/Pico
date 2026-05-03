#include "lcd_brightness.h"
#include "hardware/pwm.h"
#include "hardware/clocks.h"

#define PWM_FREQ    10000
#define PWM_WRAP    1000



LcdBrightness::LcdBrightness(uint8_t pin)
: _pin(pin)
{
        // Turn on the display backlight
    gpio_init(pin);
    gpio_set_dir(pin, GPIO_OUT);
    gpio_put(pin, 1); // on


    // Now set up as pwm.
    float sys_clk = clock_get_hz(clk_sys);
    gpio_set_function(_pin, GPIO_FUNC_PWM);

    // Find out which PWM slice is connected to GPIO 0 (it's slice 0)
    slice_num = pwm_gpio_to_slice_num(_pin);
    pwm_channel = pwm_gpio_to_channel(_pin);


    pwm_set_clkdiv(slice_num, sys_clk / (PWM_FREQ * PWM_WRAP));  

    pwm_set_wrap(slice_num, PWM_WRAP);  

    pwm_set_chan_level(slice_num, pwm_channel, 0);

    pwm_set_enabled(slice_num, true);
    
}

void LcdBrightness::set(uint8_t percent)
{
    if (percent > 100)
    {
        percent = 100;
    }
    pwm_set_chan_level(slice_num, pwm_channel, PWM_WRAP / 100 * percent);
}