#ifndef _HDC1080_H
#define _HDC1080_H

#include <stdint.h>
#include "../PicoHardware/i2c.h"

#define HDC1080_TEMPERATURE 0
#define HDC1080_HUMIDITY 1
#define HDC1080_CONFIGURATION 2
#define HDC1080_SERIAL_HI 0xFB
#define HDC1080_SERIAL_MED 0xFC
#define HDC1080_SERIAL_LO 0xFD
#define HDC1080_MANUFACTURER 0xFE
#define HDC1080_DEVICE 0xFF

#define HDC1080_ID 0x1050       // Expected device ID
#define HDC1080_MFG_TI 0x5449   // Expected Manufacturer - Texas Instruments

#define HDC1080_STARTUP_DELAY 15
#define HDC1080_MEASUREMENT_DELAY 7 // worst case in mS for 14 bit.

// See https://www.ti.com/product/HDC1080


class HDC1080Config {
    uint16_t cfg;

    public:
    const static uint16_t TEMP_14_BITS = 0x0000;
    const static uint16_t TEMP_11_BITS = 0x0400;
    const static uint16_t HUMIDITY_14_BITS = 0x0000;
    const static uint16_t HUMIDITY_11_BITS = 0x0100;
    const static uint16_t HUMIDITY_8_BITS = 0x0200;
 
    HDC1080Config(uint16_t config) : cfg(config) {};
    operator uint16_t() const {return cfg;}

    void reset() { cfg |= 0x8000;}
    void heat() {cfg |= 0x2000;}
    void modeBoth() {cfg |= 1000;}
    bool batteryLow() {return cfg & 0x0800;}
    void tempResolution(uint16_t res) {cfg |= res;}
    void humidityResolution(uint16_t res) { cfg |= res;}
};

class HDC1080 {

    const uint8_t IS_TIMEOUT = 0x01;
    const uint8_t NO_DEVICE = 0x02;

    I2C* _i2c;
    uint8_t _addr;
    uint8_t _status;
    uint16_t _temp;
    uint16_t _humidity;

    void write(uint8_t pr, uint16_t value);
    uint16_t read(uint8_t pr);
    int check(int retCode);

    public:

    HDC1080(I2C* i2c, uint8_t addr=0x40);

    uint8_t status() { return _status;}

    void configure(HDC1080Config config);
    HDC1080Config getConfig();

    void trigger();
    void readCombined();

    void triggerTemperature();
    void readTemperature();
    
    void triggerHumidity();
    void readHumidity();

    float temperature();
    float humidity();

    unsigned long serialNumber();
    uint16_t manufacturer();
    uint16_t deviceId();
};

#endif
