#ifndef GY30_H
#define GY30_H

#include <stdint.h>
#include "../PicoHardware/i2c.h"


class GY30 {

    I2C *i2c_dev;
    uint8_t addr;

    public:

    enum Opcodes{
        PowerDown =     0b00000000,             // No active state.
        PowerOn =       0b00000001,             // Waiting for measurement command.
        Reset =         0b00000111,             // Reset Data register value. Reset command is not acceptable in Power Down mode.
        ContinuousHResolutionMode  = 0b00010000,// Start measurement at 1lx resolution.
                                                // Measurement Time is typically 120ms.
        ContinuousHResolutionMode2 = 0b00010001,// Start measurement at 0.5lx resolution.
                                                // Measurement Time is typically 120ms.
        ContinuousLResolutionMode = 0b00010011, // Start measurement at 4lx resolution.
                                                // Measurement Time is typically 16ms.
        OneTimeHResolutionMode = 0b00100000,    // Start measurement at 1lx resolution.
                                                // Measurement Time is typically 120ms.
                                                // It is automatically set to Power Down mode after measurement.
        OneTimeHResolutionMode2 = 0b00100001,   // Start measurement at 0.5lx resolution.
                                                // Measurement Time is typically 120ms.
                                                // It is automatically set to Power Down mode after measurement.
        OneTimeLResolutionMode = 0b00100011,    // Start measurement at 4lx resolution.
                                                // Measurement Time is typically 16ms.
                                                // It is automatically set to Power Down mode after measurement.
        ChangeMeasurementTimeH = 0b01000000,    // ( High bit ) 01000_MT[7,6,5] Change measurement time.
                                                // Please refer "adjust measurement result for influence of optical window."
        ChangeMeasurementTimeL = 0b01100000     // ( Low bit ) 011_MT[4,3,2,1,0] Change measurement time.
                                                // Please refer "adjust measurement result for influence of optical window."
    };

    const uint8_t AltAddr = 0x5C;

    GY30(I2C* i2c, uint8_t addr = 0x23);

    void send( enum Opcodes op);
    void changeMeasurementTime(uint8_t t);
    uint16_t read();
    float readLux() { return read() / 1.2f;}
};

#endif
