#include "i2s.h"

#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/dma.h"
#include "hardware/clocks.h"
#include "i2s.pio.h"



I2S::I2S()
{
    // Initialize PIO pins
    pio_gpio_init(BSP_I2S_SOUND_PIO, BSP_I2S_DIN_PIN);
    pio_gpio_init(BSP_I2S_SOUND_PIO, BSP_I2S_BCK_PIN);
    pio_gpio_init(BSP_I2S_SOUND_PIO, BSP_I2S_LRCK_PIN);

    // Get the state machine and offset
    i2sSoundSm = pio_claim_unused_sm(BSP_I2S_SOUND_PIO, true);
    i2sSoundOffset = pio_add_program(BSP_I2S_SOUND_PIO, &i2s_pio_program);

    // Get PIO default settings
    pio_sm_config cfg = i2s_pio_program_get_default_config(i2sSoundOffset);

    // data pin
    sm_config_set_out_pins(&cfg, BSP_I2S_DIN_PIN, 1);

    // Additional pin settings
    sm_config_set_sideset_pins(&cfg, BSP_I2S_BCK_PIN);

    // Set to fetch data from the left (MSB), fetch automatically, fetch threshold 32 bits.
    sm_config_set_out_shift(&cfg, false, true, 32);

    // FIFO with combined input and output
    sm_config_set_fifo_join(&cfg, PIO_FIFO_JOIN_TX);

    pio_sm_init(BSP_I2S_SOUND_PIO, i2sSoundSm, i2sSoundOffset, &cfg);

    // Set the I2S pin direction to output.
    uint pinMask = (1u << BSP_I2S_DIN_PIN) | (3u << BSP_I2S_BCK_PIN);

    pio_sm_set_pindirs_with_mask(BSP_I2S_SOUND_PIO, i2sSoundSm, pinMask, pinMask);

    // Clear all I2S pins to 0.
    pio_sm_set_pins(BSP_I2S_SOUND_PIO, i2sSoundSm, 0);

    // Set state machine frequency
    // Crossover = Main unit frequency * 256 / (2 * number of channels * bit depth * frequency)
    // Frequency divider = Main unit frequency * 256 / (2 * 2 * 16 * 44100)
    // Frequency divider = Main unit frequency * 4 / 44100
    uint32_t freqDiv = clock_get_hz(clk_sys) * 4 / BSP_I2S_FREQ;
    pio_sm_set_clkdiv_int_frac(BSP_I2S_SOUND_PIO, i2sSoundSm, freqDiv >> 8, freqDiv & 0xFF);

    printf("I2S sound freq: %d, divider: %d\n", BSP_I2S_FREQ, freqDiv);
    pio_sm_set_enabled(BSP_I2S_SOUND_PIO, i2sSoundSm, true);

    // DMA
    dmaSoundTx = dma_claim_unused_channel(true);

    dma_channel_config c = dma_channel_get_default_config(dmaSoundTx);
    channel_config_set_dreq(&c, pio_get_dreq(BSP_I2S_SOUND_PIO, i2sSoundSm, true));
    dma_channel_configure(dmaSoundTx, &c, (uint32_t *)&(BSP_I2S_SOUND_PIO->txf[i2sSoundSm]), NULL, 0, false);
}


void I2S::output(const uint32_t *sound, size_t len, bool wait)
{
    if (wait)
    {
        dma_channel_wait_for_finish_blocking(dmaSoundTx);
    }
    else if (dma_channel_is_busy(dmaSoundTx))
        return;
    dma_channel_transfer_from_buffer_now(dmaSoundTx, (uint32_t *)sound, len);
}

void I2S::soundOutputDmaBlocking(const uint32_t *sound, size_t len)
{
    dma_channel_wait_for_finish_blocking(dmaSoundTx);
    dma_channel_transfer_from_buffer_now(dmaSoundTx, (uint32_t *)sound, len);
}
