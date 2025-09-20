#ifndef _AHTX0_H_
#define _AHTX0_H_

#include "../PicoHardware/i2c.h"
class Ahtx0 {

    const uint8_t AHTX0_I2CADDR_DEFAULT = 0x38; // Default I2C address
    const uint8_t AHTX0_CMD_CALIBRATE = 0xE1;  // Calibration command
    const uint8_t AHTX0_CMD_TRIGGER = 0xAC;  // Trigger reading command
    const uint8_t AHTX0_CMD_SOFTRESET = 0xBA;  // Soft reset command
    const uint8_t AHTX0_STATUS_BUSY = 0x80;  // Status bit for busy
    const uint8_t AHTX0_STATUS_CALIBRATED = 0x08;  // Status bit for calibrated

   const uint8_t IS_TIMEOUT = 0x01;
   const uint8_t NO_DEVICE = 0x02;

    I2C* i2c;
    uint8_t _status;
    uint8_t addr;
    uint8_t buffer[6];
    int _temp;
    int _humidity;

    int check(int retCode);

    public:

    Ahtx0(I2C* i2c, uint8_t addr);
    inline Ahtx0(I2C* i2c) : Ahtx0(i2c, AHTX0_I2CADDR_DEFAULT){};

    void reset();
    bool calibrate();
    uint8_t status();
    int temperature();
    int relativeHumidity();
    void read();

    bool isError() {return _status != 0;}
    bool isTimeout() {return _status & IS_TIMEOUT;}
    bool isNoDevice() {return _status & NO_DEVICE;}
};
#endif