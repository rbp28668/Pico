
#ifndef BMP280_H
#define BMP280_H

#include <stdint.h>
#include "../PicoHardware/i2c.h"


class BMP280 {

  
struct calib_param {
    // temperature params
    uint16_t dig_t1;
    int16_t dig_t2;
    int16_t dig_t3;

    // pressure params
    uint16_t dig_p1;
    int16_t dig_p2;
    int16_t dig_p3;
    int16_t dig_p4;
    int16_t dig_p5;
    int16_t dig_p6;
    int16_t dig_p7;
    int16_t dig_p8;
    int16_t dig_p9;
};

  I2C *i2c_dev;
  struct calib_param calib;
  int32_t raw_temperature;
  int32_t raw_pressure;

  int32_t convert(int32_t temp, const BMP280::calib_param* params);
  int32_t convert_temp(int32_t temp, const BMP280::calib_param* params);
  int32_t convert_pressure(int32_t pressure, int32_t temp, const BMP280::calib_param* params);
  void get_calib_params(BMP280::calib_param* params); 
  void readRaw(int32_t* temp, int32_t* pressure);


public:

  enum Oversampling {
    OFF = 0,
    x1 = 1,
    x2 = 2,
    x4 = 3,
    x8 = 4,
    x16 = 5
  };

  enum PowerMode {
    Sleep = 0,
    Forced = 1,
    Normal = 3
  };

  enum StandbyTime {
    Standby_0_5mS = 0,
    Standby_62_5mS = 1,
    Standby_125mS = 2,
    Standby_250mS = 3,
    Standby_500mS = 4,
    Standby_1000mS = 5,
    Standby_2000mS = 6,
    Standby_4000mS = 7
  };

  enum IIR {
    IIR_OFF = 0,
    IIR_2 = 1,
    IIR_4 = 2,
    IIR_8 = 3,
    IIR_16 = 4
  };

  BMP280(I2C* i2c);
  bool devicePresent();
  bool isMeasuring();
  bool isImUpdate();
  void control(PowerMode powerMode, Oversampling temp, Oversampling pressure);
  void config(StandbyTime standby, IIR filter);

  void reset();
  void read();
  int32_t temperature();
  int32_t pressure();
};

#endif
