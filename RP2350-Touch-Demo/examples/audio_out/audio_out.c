#include <stdio.h>
#include "pico/stdlib.h"
#include "bsp_i2s.h"
#include "i2stest8bit.h"
#include "bsp_battery.h"
// 测试程序
uint16_t runCount = 5;
uint32_t sample32bit[SOUND_LENGTH];

void initSample()
{
    // 转换成双声道
    for (size_t index = 0; index < SOUND_LENGTH; ++index)
    {
        sample32bit[index] = (i2stest8bit[index] << 24) | (i2stest8bit[index] << 8);

        sample32bit[index] >>= 1;
    }
}
void playDma()
{
    for (size_t i = 0; i < runCount; ++i)
    {
        i2sSoundOutputDmaBlocking(sample32bit, SOUND_LENGTH);
    }
}

int main()
{
    stdio_init_all();
    bsp_battery_init(100);
    bsp_i2s_init();
    initSample();

    playDma();
    
    return 0;
}
