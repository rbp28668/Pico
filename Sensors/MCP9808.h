#ifndef _MCP9808_H
#define _MCP9808_H

#include "../PicoHardware/i2c.h"


// Control the Microchip MCP9808 temperature sensor.
// Note all the integer temperatures are fixed point with
// 4 fractional bits - i.e. divide by 16 to get integer
// temperature in C & vice-versa.
// (C) R Bruce Porteous 2022
class MCP9808 {

    const uint8_t IS_TIMEOUT = 0x01;
    const uint8_t NO_DEVICE = 0x02;

    I2C* i2c;
    uint8_t _addr;
    uint8_t _status;
    uint8_t _alarmBits;


   int check(int retCode);

    uint16_t read(uint8_t reg);
    void write(uint8_t reg, uint16_t value);
    void setTempReg(uint8_t reg, int value);

 public:

    // Encapsulate the bit-twiddling needed to manipulate the config register.
    // setters return config reference so can chain methods.
    class Config {
        uint16_t _value;
        public:

        // Note - all zeros is power up default 
        Config(uint16_t value = 0) : _value(value) {};
        operator uint16_t() { return _value;}
        
        Config& hysteresis(int h) { _value = (_value & 0x600) | ((h & 3)<<9); return *this;}
        int hysteresis() const {return (_value & 0x600) >> 9;}
        
        Config& shutdown(bool shutdown) {(_value & ~0x0100) | (shutdown ? 0x0100 : 0); return *this;}
        bool shutdown() const {return _value & 0x0100;}

        Config& criticalLock(bool lock) {(_value & ~0x0080) | (lock ? 0x0080 : 0); return *this;}
        bool criticalLock() const {return _value & 0x0080;}

        Config& windowLock(bool lock) {(_value & ~0x0040) | (lock ? 0x0040 : 0); return *this;}
        bool windowLock() const {return _value & 0x0040;}

        Config& interruptClear(bool clr) {(_value & ~0x0020) | (clr ? 0x0020 : 0); return *this;}
        bool interruptClear() const {return _value & 0x0020;}
        
        Config& alertOutputStatus(bool status) {(_value & ~0x0010) | (status ? 0x0010 : 0); return *this;}
        bool alertOutputStatus() const {return _value & 0x0010;}
        
        Config& alertOutputControl(bool enable) {(_value & ~0x0008) | (enable ? 0x0008 : 0); return *this;}
        bool alertOutputControl() const {return _value & 0x0008;}
        
        Config& alertOutputSelect(bool criticalOnly) {(_value & ~0x0004) | (criticalOnly ? 0x0004 : 0); return *this;}
        bool alertOutputSelect() const {return _value & 0x0004;}
        
        Config& alertOutputPolarity(bool activeHigh) {(_value & ~0x0002) | (activeHigh ? 0x0002 : 0); return *this;}
        bool alertOutputPolarity() const {return _value & 0x0002;}
        
        Config& alertOutputMode(bool useInterrupts) {(_value & ~0x0001) | (useInterrupts ? 0x0001 : 0); return *this;}
        bool alertOutputMode() const {return _value & 0x0001;}
     
    };


    MCP9808(I2C* i2c, uint8_t addr=0x18);

    Config getConfig();
    void setConfig(Config config);

    // 00 = +0.5°C (tCONV = 30 ms typical)
    // 01 = +0.25°C (tCONV = 65 ms typical)
    // 10 = +0.125°C (tCONV = 130 ms typical)
    // 11 = +0.0625°C (power-up default, tCONV = 250 ms typical)
    void setResolution(uint8_t resolution);

    uint16_t getManufacturerId();
    uint16_t getDeviceIdAndRevision();
   
    // Set alert temperatures as fixed point temperatures
    void setAlertLower(int lower);
    void setAlertUpper(int upper);
    void setAlertCritical(int critical);

    // Read temperature as fixed and floating point.
    int readTempX16();
    float readTemp();
    
    // Alert temperature status.
    // These set when reading temperature.
    bool isLow() const {return _alarmBits & 1;}
    bool isHigh() const {return _alarmBits & 2;}
    bool isInRange() const {return (_alarmBits & 3) == 0;}
    bool isCritical() const {return _alarmBits & 4;}

    // I2C error status
    bool isError() {return _status != 0;}
    bool isTimeout() {return _status & IS_TIMEOUT;}
    bool isNoDevice() {return _status & NO_DEVICE;}

};

#endif
