#include "GY30.h"




GY30::GY30( I2C *i2c_dev, uint8_t addr) 
:  i2c_dev(i2c_dev)
, addr(addr)
{
}

void GY30::send( GY30::Opcodes op){
    uint8_t opcode = uint8_t(op);
    i2c_dev->write(addr, &opcode, 1, false);
}

void GY30::changeMeasurementTime(uint8_t t){
    uint8_t oph = uint8_t(ChangeMeasurementTimeH) | (t >> 5);   // 3 MSB
    uint8_t opl = uint8_t(ChangeMeasurementTimeL) | (t & 0x1F); // 5 LSB
    i2c_dev->write(addr, &oph, 1, false);
    i2c_dev->write(addr, &opl, 1, false);
}

uint16_t GY30::read(){
    uint8_t buff[2];
    i2c_dev->read(addr, buff, 2, false);
    return (uint16_t(buff[0]) << 8) | buff[1];
}