#ifndef RCWL0516_HPP
#define RCWL0516_HPP

#include "pico/stdlib.h"

class RCWL0516
{
    bool tick = false;

    int pin = 6;

    public:
    RCWL0516()
    {
        gpio_init(pin);
        gpio_set_dir(pin, GPIO_IN);
    }

    bool test()
    {
        return gpio_get(pin);
    }
};

#endif