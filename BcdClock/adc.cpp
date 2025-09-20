#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/adc.h"
#include "adc.hpp"

Adc::Adc()
{
    adc_init();
}

Adc::Adc(int pin)
{
    adc_init();

    // Make sure GPIO is high-impedance, no pullups etc
    adc_gpio_init(pin);
    // Select ADC input GPIO26 is channel 0 etc
    adc_select_input(pin-26);
}

uint16_t Adc::read()
{
    uint16_t result = adc_read();
    return result;
}

float Adc::readVolts()
{
    const float conversion_factor = 3.3f / (1 << 12);
    uint16_t result = adc_read();
    return result * conversion_factor;
}

float Adc::readTempC()
{
    float T = 27.0f - (readVolts() - 0.706f)/0.001721f;
    return T;
}
