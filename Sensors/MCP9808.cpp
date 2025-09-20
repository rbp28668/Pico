#include "MCP9808.h"

    MCP9808::MCP9808(I2C* i2c, uint8_t addr)
    : i2c(i2c)
    , _addr(addr)
    , _status(0)
    , _alarmBits(0)
    {
        write(1,0); // clear config
    }

    int MCP9808::check(int retCode){
        if(retCode == PICO_ERROR_GENERIC) _status |= NO_DEVICE;
        if(retCode == PICO_ERROR_TIMEOUT) _status |= IS_TIMEOUT;
        return retCode;
    }

    uint16_t MCP9808::read(uint8_t reg){
        uint8_t buffer[2];
        buffer[0] =  reg;
        check(i2c->write(_addr, buffer, 1, true, 2000));
        check(i2c->read(_addr, buffer, 2, false, 4000));
        uint16_t result = buffer[0];
        result <<= 8;
        result |= buffer[1];
        return result;
    }

    void MCP9808::write(uint8_t reg, uint16_t value){
        uint8_t buffer[3];
        buffer[0] =  reg;
        buffer[1] = (value >> 8) & 0xFF;
        buffer[2] = value & 0xFF;
        check(i2c->write(_addr, buffer, 3, false, 6000));

    }

    void MCP9808::setTempReg(uint8_t reg, int value){
        uint16_t t = value & 0x1FFF;
        write(reg, t);
    }
 
    MCP9808::Config MCP9808::getConfig(){
        MCP9808::Config config(read(1));
        return config;
    }

    void MCP9808::setConfig(MCP9808::Config config){
        write(1, config);
    }


    void MCP9808::setResolution(uint8_t resolution){
        if(resolution > 3) resolution = 3;
        uint8_t buffer[2];
        buffer[0] =  0x08;
        buffer[1] = resolution;
        check(i2c->write(_addr, buffer, 2, false, 4000));
    }

    uint16_t MCP9808::getManufacturerId(){
        return read(6);
    }

    uint16_t MCP9808::getDeviceIdAndRevision(){
        return read(7);
    }
   
    void MCP9808::setAlertLower(int lower){
        setTempReg(3, lower);
    }

    void MCP9808::setAlertUpper(int upper){
        setTempReg(2, upper);
    }

    void MCP9808::setAlertCritical(int critical){
        setTempReg(4, critical);
    }

    int MCP9808::readTempX16(){
        uint16_t raw = read(5);
        _alarmBits = (raw & 0xE000)>>13;
        // Data as twos complement so if sign bit set, 
        // sign extend -ve number to full 16 bits 
        // else truncate to 12 bit magnitude
        int temp =  (raw & 0x1000) ?  int16_t(raw | 0xF000) : int16_t(raw & 0x0FFF);
        return temp;
    }

    float MCP9808::readTemp(){
        return readTempX16() / 16.0f;
    }
    
 