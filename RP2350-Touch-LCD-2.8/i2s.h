#ifndef __BSP_I2S_H__
#define __BSP_I2S_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "pico/stdlib.h"

#define BSP_I2S_BCK_PIN     2
#define BSP_I2S_LRCK_PIN    (BSP_I2S_BCK_PIN + 1)
#define BSP_I2S_DIN_PIN     4


// I2S
#define BSP_I2S_SOUND_PIO           pio1    // The pio module used


#define BSP_I2S_FREQ                16000   // channel frequency




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

