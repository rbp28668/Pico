#ifndef __BSP_BATTERY_H__
#define __BSP_BATTERY_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"

// Controls the battery hardware on the board.
// Potentially allows a key to wakeup the board if there's a battery, or just be read.
// See schematic.
class Battery
{
    static constexpr uint8_t BSP_BAT_ADC_PIN = 27;
    static constexpr uint8_t BSP_BAT_EN_PIN = 26;
    static constexpr uint8_t BSP_BAT_KEY_PIN = 25;

    void sort(uint16_t *data, uint16_t size);
    uint16_t average_filter(uint16_t *samples);
    uint16_t read_raw(void);


    public:

    Battery(uint16_t key_wait_ms = 10);
    void enabled(bool enabled);
    void read(float *voltage, uint16_t *adc_raw);
    bool get_key_level(void);
};

#endif // __BSP_BATTERY_H__
