#include "pico/stdlib.h"
#include <cstdio>
#include "AHTX0.h"


Ahtx0::Ahtx0(I2C* i2c, uint8_t addr)
: i2c(i2c),
_status(0),
addr(addr),
_temp(0),
_humidity(0)
{
    printf("AHTX0 on %d\n", (int)addr);
    ::sleep_ms(20); // wakeup time
    printf("Reset\n");
   reset();
   printf("Calibrate\n");
   calibrate();
}

int Ahtx0::check(int retCode){
    if(retCode == PICO_ERROR_GENERIC) _status |= NO_DEVICE;
    if(retCode == PICO_ERROR_TIMEOUT) _status |= IS_TIMEOUT;
    return retCode;
}

void Ahtx0::reset(){
    buffer[0] = AHTX0_CMD_SOFTRESET;
    check(i2c->write(addr, buffer, 1, false, 2000));
    ::sleep_ms(20); // 20ms delay to wake up
}

bool Ahtx0::calibrate(){
    buffer[0] = AHTX0_CMD_CALIBRATE;
    buffer[1] = 0x08;
    buffer[2] = 0x00;
    check(i2c->write(addr, buffer, 3, false, 6000));
 
    int count = 0;
    while(status() & AHTX0_STATUS_BUSY) {
        ::sleep_ms(10);
        ++count;
        if(count > 100) return false;
    }
   return (status() & AHTX0_STATUS_CALIBRATED);
   

}
uint8_t Ahtx0::status(){
    check(i2c->read(addr, buffer, 1, false, 2000));
    //printf("Status %d\n", (int)buffer[0]);
    return buffer[0];
}
int Ahtx0::temperature(){
    return _temp;
}

int Ahtx0::relativeHumidity(){
    return _humidity;
}

void Ahtx0::read(){
    buffer[0] = AHTX0_CMD_TRIGGER;
    buffer[1] = 0x33;
    buffer[2] = 0x00;
    check(i2c->write(addr, buffer, 3, false,6000));

	int count = 0;
    while (status() & AHTX0_STATUS_BUSY) {
        sleep_ms(10);
        ++count;
        if(count > 100) return;
    }

    check(i2c->read(addr,buffer,6,false, 6*2000));
        
    _humidity = ((buffer[1] << 12) | (buffer[2] << 4) | (buffer[3] >> 4));
    _humidity = (_humidity * 100) / 0x100000;
    _temp = ((buffer[3] & 0xF) << 16) | (buffer[4] << 8) | buffer[5];
    _temp = ((_temp * 200.0) / 0x100000) - 50;

}
