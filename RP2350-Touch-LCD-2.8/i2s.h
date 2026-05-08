#ifndef __BSP_I2S_H__
#define __BSP_I2S_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "pico/stdlib.h"


class I2S {
    
    int i2sSoundSm;
    uint i2sSoundOffset;
    int dmaSoundTx;

    public:
    I2S();
    void output(const uint32_t *sound, size_t len, bool wait);
    void soundOutputDmaBlocking(const uint32_t *sound, size_t len);
};

#endif

