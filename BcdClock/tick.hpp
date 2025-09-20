#ifndef TICK_HPP
#define TICK_HPP

#include "pico/stdlib.h"

class Tick
{
    bool tick = false;

    const int B0 = 10;
    const int B1 = 11;
    bool enabled = false;

public:
    Tick()
    {
        gpio_init(B0);
        gpio_init(B1);
        gpio_set_dir(B0, GPIO_OUT);
        gpio_set_dir(B1, GPIO_OUT);
    }

    void soundTick()
    {
        if (enabled)
        {
            gpio_put(B0, tick ? 1 : 0);
            gpio_put(B1, tick ? 0 : 1);
            tick = !tick;
        }
    }

    void enable(bool on)
    {
        enabled = on;
        if (!enabled)
        {
            gpio_put(B0, 0);
            gpio_put(B1, 0);
        }
    }
};

#endif