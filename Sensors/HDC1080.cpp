#include "pico/error.h"
#include "HDC1080.h"



HDC1080::HDC1080(I2C* i2c, uint8_t addr)
   : _i2c(i2c)
   , _addr(addr)
{
}
    
int HDC1080::check(int retCode){
    if(retCode == PICO_ERROR_GENERIC) _status |= NO_DEVICE;
    if(retCode == PICO_ERROR_TIMEOUT) _status |= IS_TIMEOUT;
    return retCode;
}

uint16_t HDC1080::read(uint8_t reg){
    uint8_t buffer[2];
    buffer[0] =  reg;
    check(_i2c->write(_addr, buffer, 1, true, 2000));
    check(_i2c->read(_addr, buffer, 2, false, 4000));
    uint16_t result = buffer[0];
    result <<= 8;
    result |= buffer[1];
    return result;
}

void HDC1080::write(uint8_t reg, uint16_t value){
    uint8_t buffer[3];
    buffer[0] =  reg;
    buffer[1] = (value >> 8) & 0xFF; // MS byte first
    buffer[2] = value & 0xFF;        // LS byte
    check(_i2c->write(_addr, buffer, 3, false, 6000));
}


HDC1080Config HDC1080::getConfig(){
    return HDC1080Config(read(HDC1080_CONFIGURATION));
}

void HDC1080::configure(HDC1080Config config){
    write(HDC1080_CONFIGURATION, config);
}


void HDC1080::trigger(){
    uint8_t buffer[1];
    buffer[0] = HDC1080_TEMPERATURE;
    check(_i2c->write(_addr, buffer, 1, false, 2000));
}

void HDC1080::readCombined(){
    uint8_t buffer[4];
    check(_i2c->read(_addr, buffer, 4, false, 8000));
    _temp = buffer[0];
    _temp <<= 8;
    _temp |= buffer[1];
    _humidity = buffer[2];
    _humidity <<= 8;
    _humidity |= buffer[3];
}

void HDC1080::triggerTemperature(){
    uint8_t buffer[1];
    buffer[0] = HDC1080_TEMPERATURE;
    check(_i2c->write(_addr, buffer, 1, false, 2000));
}

void HDC1080::triggerHumidity(){
    uint8_t buffer[1];
    buffer[0] = HDC1080_HUMIDITY;
    check(_i2c->write(_addr, buffer, 1, false, 2000));
}

void HDC1080::readTemperature(){
    uint8_t buffer[2];
    check(_i2c->read(_addr, buffer, 2, false, 4000));
    _temp = buffer[0];
    _temp <<= 8;
    _temp |= buffer[1];
}

void HDC1080::readHumidity(){
    uint8_t buffer[2];
    check(_i2c->read(_addr, buffer, 2, false, 4000));
    _humidity = buffer[0];
    _humidity <<= 8;
    _humidity |= buffer[1];
}

float HDC1080::temperature(){
    return (float(_temp) / 65536.0f) * 165.0f - 40.0f;
}

float HDC1080::humidity(){
    return (float(_humidity) / 65536.0f) * 100.0f;
}

unsigned long HDC1080::serialNumber(){
    uint16_t snHi = read(HDC1080_SERIAL_HI);
    uint16_t snMed = read(HDC1080_SERIAL_MED);
    uint16_t snLo = read(HDC1080_SERIAL_LO);

    unsigned long result = snHi;
    result <<= 16;
    result |= snMed;
    result <<= 16;
    result != snLo;
    return result;
}

uint16_t HDC1080::manufacturer(){
    return read(HDC1080_MANUFACTURER);
}

uint16_t HDC1080::deviceId(){
    return read(HDC1080_DEVICE);
}