#include "battery.h"
#include "hardware/adc.h"
#include "hardware/gpio.h"

#define BATTERY_ADC_SIZE 9

// Sorting function - now a selection sort.
void Battery::sort(uint16_t *data, uint16_t size)
{

    // Only sort arrays > 1 in length.
    for (int i=size; i>1; --i){
        
        // put the largest value at the top of range 0..i-1
        int pos = 0;
        uint16_t maxVal = data[pos];
        for (int j=1; j<i; ++j){
            if(data[j] > maxVal){
                pos = j;
                maxVal = data[pos];
            }
        }
        // swap the topmost element with the largest (maxVal at pos)
        data[pos] = data[i-1];
        data[i-1] = maxVal;
    }
}

uint16_t Battery::average_filter(uint16_t *samples)
{
    uint16_t out;
    sort(samples, BATTERY_ADC_SIZE);
    for (int i = 1; i < BATTERY_ADC_SIZE - 1; i++)
    {
        out += samples[i] / (BATTERY_ADC_SIZE - 2);
    }
    return out;
}

uint16_t Battery::read_raw(void)
{
    uint16_t samples[BATTERY_ADC_SIZE];
    adc_select_input(BSP_BAT_ADC_PIN - 26);
    for (int i = 0; i < BATTERY_ADC_SIZE; i++)
    {
        samples[i] = adc_read();
    }
    return average_filter(samples); // Use median filtering
}

void Battery::read(float *voltage, uint16_t *adc_raw)
{
    uint16_t result = read_raw();
    if (adc_raw)
    {
        *adc_raw = result;
    }
    if (voltage)
    {
        *voltage = result * (3.3 / (1 << 12)) * 3.0;
    }
}


void Battery::enabled(bool enabled)
{
    gpio_put(BSP_BAT_EN_PIN, enabled);
}


Battery::Battery(uint16_t key_wait_ms)
{
    adc_init();
    adc_gpio_init(BSP_BAT_ADC_PIN);
    gpio_init(BSP_BAT_EN_PIN);
    gpio_set_dir(BSP_BAT_EN_PIN, GPIO_OUT);
    gpio_init(BSP_BAT_KEY_PIN);
    gpio_set_dir(BSP_BAT_KEY_PIN, GPIO_IN);

    enabled(false);
    sleep_ms(key_wait_ms);
    enabled(true);
}


bool Battery::get_key_level(void)
{
    return gpio_get(BSP_BAT_KEY_PIN);
}

