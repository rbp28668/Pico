#pragma once

#include "hardware/i2c.h"

class I2C {
    i2c_inst_t* i2c;
    
public:

  I2C(i2c_inst_t *i2c, int gpio_sda, int gpio_scl, int bps = 100*1000);
  ~I2C();

  uint setBaudrate(uint baudrate); 
		

  // Set nostop true when doing a multi-stage transaction and you want 
  // to keep hold of the bus.  E.g.  writing a register address then reading data for SMBus.
  int read (uint8_t addr, uint8_t *dst, size_t len, bool nostop);
  int write (uint8_t addr, const uint8_t *src, size_t len, bool nostop);

  // read and write but with a timeout in uS.
  int read (uint8_t addr, uint8_t *dst, size_t len, bool nostop, uint timeout_us);
  int write (uint8_t addr, const uint8_t *src, size_t len, bool nostop, uint timeout_us);

  size_t readAvailable();
  size_t writeAvailable();


};

