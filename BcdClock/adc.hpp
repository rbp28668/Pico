#ifndef ADC_HPP
#define ADC_HPP
#include <ctype.h>
#include "pico/stdlib.h"
#include "hardware/adc.h"
class Adc
{

public:
    Adc();
    Adc(int pin);
    uint16_t read();
    float readVolts();
    float readTempC();

    inline void usePin(uint gpio) { adc_gpio_init(gpio); }
    inline void selectInput(uint channel) { adc_select_input(channel); }
    inline uint selectedInput() const { return adc_get_selected_input(); }
    inline void setRoundRobin(uint input_mask) { adc_set_round_robin(input_mask); }
    inline void enableTempSensor(bool enable) { adc_set_temp_sensor_enabled(enable); }
    inline void run(bool run) { adc_run(run); }
    inline void setClkdiv(float clkdiv) { adc_set_clkdiv(clkdiv); }
    inline void setupFifo(bool en, bool dreq_en, uint16_t dreq_thresh, bool err_in_fifo, bool byte_shift) { adc_fifo_setup(en, dreq_en, dreq_thresh, err_in_fifo, byte_shift); }
    inline bool isFifoEmpty() { return adc_fifo_is_empty(); }
    inline uint8_t getFifoLevel() { return adc_fifo_get_level(); }
    inline uint16_t readFifo() { return adc_fifo_get(); }
    inline uint16_t readFifoBlocking() { return adc_fifo_get_blocking(); }
    inline void drainFifo() { adc_fifo_drain(); }
    inline void enableIrq(bool enabled) { adc_irq_set_enabled(enabled); }
};

#endif
