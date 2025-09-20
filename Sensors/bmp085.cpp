/*!
 * @file bmp085.cpp
 *
 * @mainpage BMP085 Library
 *
 * @section intro_sec Introduction
 *
 * This is a Bosch BMP085/BMP180 Barometric Pressure + Temp
 * sensor
 *
 * Modified from the Adafruit BMP085 library for Adafruit BMP085 or BMP180 Breakout
 * ----> http://www.adafruit.com/products/391
 * ----> http://www.adafruit.com/products/1603
 *
 * These sensors use I2C to communicate, 2 pins are required to
 * interface
 * Adafruit invests time and resources providing this open source code,
 * please support Adafruit and open-source hardware by purchasing
 * products from Adafruit!
 *
 * @section author Author
 *
 * Written by Limor Fried/Ladyada for Adafruit Industries.
 * Updated by Samy Kamkar for cross-platform support.
 * Translated to RPI Pico by Bruce Porteous
 * @section license License
 *
 * BSD license, all text above must be included in any redistribution
 */

#include <cmath>
#include "pico/stdlib.h"
#include "bmp085.h"


#if (BMP085_DEBUG & 1)
#include <stdio.h>
#endif
#define BMP085_I2CADDR 0x77 //!< BMP085 I2C address

#define BMP085_CAL_AC1 0xAA    //!< R   Calibration data (16 bits)
#define BMP085_CAL_AC2 0xAC    //!< R   Calibration data (16 bits)
#define BMP085_CAL_AC3 0xAE    //!< R   Calibration data (16 bits)
#define BMP085_CAL_AC4 0xB0    //!< R   Calibration data (16 bits)
#define BMP085_CAL_AC5 0xB2    //!< R   Calibration data (16 bits)
#define BMP085_CAL_AC6 0xB4    //!< R   Calibration data (16 bits)
#define BMP085_CAL_B1 0xB6     //!< R   Calibration data (16 bits)
#define BMP085_CAL_B2 0xB8     //!< R   Calibration data (16 bits)
#define BMP085_CAL_MB 0xBA     //!< R   Calibration data (16 bits)
#define BMP085_CAL_MC 0xBC     //!< R   Calibration data (16 bits)
#define BMP085_CAL_MD 0xBE     //!< R   Calibration data (16 bits)

#define BMP085_CONTROL 0xF4         //!< Control register
#define BMP085_TEMPDATA 0xF6        //!< Temperature data register
#define BMP085_PRESSUREDATA 0xF6    //!< Pressure data register
#define BMP085_READTEMPCMD 0x2E     //!< Read temperature control register value
#define BMP085_READPRESSURECMD 0x34 //!< Read pressure control register value


// Shim function
static void delay(int mS){
    sleep_ms(mS);
}

BMP085::BMP085(I2C* i2c)
: i2c_dev(i2c)
 {}

bool BMP085::begin(uint8_t mode) {
  if (mode > BMP085_ULTRAHIGHRES)
    mode = BMP085_ULTRAHIGHRES;
  oversampling = mode;

#if BMP085_DEBUG & 2
  // use datasheet numbers!
  ac6 = 23153;
  ac5 = 32757;
  mc = -8711;
  md = 2868;
  b1 = 6190;
  b2 = 4;
  ac3 = -14383;
  ac2 = -72;
  ac1 = 408;
  ac4 = 32741;
  oversampling = 0;
#else 

  if (read8(0xD0) != 0x55)
    return false;

  /* read calibration data */
  ac1 = read16(BMP085_CAL_AC1);
  ac2 = read16(BMP085_CAL_AC2);
  ac3 = read16(BMP085_CAL_AC3);
  ac4 = read16(BMP085_CAL_AC4);
  ac5 = read16(BMP085_CAL_AC5);
  ac6 = read16(BMP085_CAL_AC6);

  b1 = read16(BMP085_CAL_B1);
  b2 = read16(BMP085_CAL_B2);

  mb = read16(BMP085_CAL_MB);
  mc = read16(BMP085_CAL_MC);
  md = read16(BMP085_CAL_MD);

#endif

#if (BMP085_DEBUG & 1)
  printf("ac1 = %d\n", ac1);
  printf("ac2 = %d\n", ac2);
  printf("ac3 = %d\n", ac3);
  printf("ac4 = %d\n", ac4);
  printf("ac5 = %d\n", ac5);
  printf("ac6 = %d\n", ac6);

  printf("b1 = %d\n", b1);
  printf("b2 = %d\n", b2);
 
  printf("mb = %d\n", mb);
  printf("mc = %d\n", mc);
  printf("md = %d\n", md);
 #endif

  return true;
}

int32_t BMP085::computeB5(int32_t UT) {
  int32_t X1 = ( (UT - int32_t(ac6)) * int32_t(ac5)) >> 15;
  int32_t X2 = (int32_t(mc) << 11) / (X1 + int32_t(md));
  return X1 + X2;
}

uint16_t BMP085::readRawTemperature(void) {
#if BMP085_DEBUG & 2
  uint16_t UT = 27898;
#else
  write8(BMP085_CONTROL, BMP085_READTEMPCMD);
  delay(5);
  delay(5); // Extra.
  
  uint16_t UT = read16(BMP085_TEMPDATA);
#endif

#if BMP085_DEBUG & 1
  printf("Raw temp: %d\n",UT);
#endif
  return UT;
}

uint32_t BMP085::readRawPressure(void) {

#if BMP085_DEBUG & 2
   uint32_t raw = 23843;
#else
  write8(BMP085_CONTROL, BMP085_READPRESSURECMD + (oversampling << 6));

  if (oversampling == BMP085_ULTRALOWPOWER)
    delay(5);
  else if (oversampling == BMP085_STANDARD)
    delay(8);
  else if (oversampling == BMP085_HIGHRES)
    delay(14);
  else
    delay(26);

  delay(5); // Extra.

  uint32_t raw = read16(BMP085_PRESSUREDATA);

  raw <<= 8;
  raw |= read8(BMP085_PRESSUREDATA + 2);
  raw >>= (8 - oversampling);
#endif

 #if BMP085_DEBUG & 1
  printf("Raw pressure: %d\n",raw);
#endif
  return raw;
}

int32_t BMP085::readPressure(void) {
  int32_t UT, UP, B3, B5, B6, X1, X2, X3, p;
  uint32_t B4, B7;
  UT = readRawTemperature();
  delay(5); // Extra.
  UP = readRawPressure();
 

  B5 = computeB5(UT);

  // do pressure calcs
  B6 = B5 - 4000;
  X1 = ((int32_t)b2 * ((B6 * B6) >> 12)) >> 11;
  X2 = ((int32_t)ac2 * B6) >> 11;
  X3 = X1 + X2;
  B3 = ((((int32_t)ac1 * 4 + X3) << oversampling) + 2) / 4;

#if BMP085_DEBUG & 1
  printf("B5 = %d\n",B5);
  printf("B6 = %d\n",B6);
  printf("X1 = %d\n",X1);
  printf("X2 = %d\n",X2);
  printf("B3 = %d\n",B3);
#endif

  X1 = (int32_t(ac3) * B6) >> 13;
  X2 = (int32_t(b1) * ((B6 * B6) >> 12)) >> 16;
  X3 = ((X1 + X2) + 2) >> 2;
  B4 = (uint32_t(ac4) * uint32_t(X3 + 32768)) >> 15;
  B7 = (uint32_t(UP) - B3) * uint32_t(50000UL >> oversampling);

#if BMP085_DEBUG & 1
  printf("X1 = %d\n",X1);
  printf("X2 = %d\n",X2);
  printf("B4 = %d\n",B4);
  printf("B7 = %d\n",B7);
#endif

  if (B7 < 0x80000000) {
    p = (B7 * 2) / B4;
  } else {
    p = (B7 / B4) * 2;
  }
  X1 = (p >> 8) * (p >> 8);
  X1 = (X1 * 3038) >> 16;
  X2 = (-7357 * p) >> 16;

#if BMP085_DEBUG & 1
  printf("p = %d\n",p);
  printf("X1 = %d\n",X1);
  printf("X2 = %d\n",X2);
#endif

  p = p + ((X1 + X2 + 3791) >> 4);
#if BMP085_DEBUG & 1
  printf("p = %d\n",p);
#endif
  return p;
}

int32_t BMP085::readSealevelPressure(float altitude_meters) {
  float pressure = readPressure();
  return (int32_t)(pressure / pow(1.0 - altitude_meters / 44330, 5.255));
}

float BMP085::readTemperature(void) {
  int32_t UT = readRawTemperature();

  int32_t B5 = computeB5(UT);
  float temp = (B5 + 8) >> 4;
  temp /= 10;

  return temp;
}

float BMP085::readAltitude(float sealevelPressure) {
  float altitude;

  float pressure = readPressure();

  altitude = 44330 * (1.0 - pow(pressure / sealevelPressure, 0.1903));

  return altitude;
}

/*********************************************************************/

uint8_t BMP085::read8(uint8_t a) {
  uint8_t ret;

  
  // send 1 byte, reset i2c, read 1 byte
  i2c_dev->write(BMP085_I2CADDR,&a,1,true, 2000);
  i2c_dev->read(BMP085_I2CADDR,&ret,1,false, 2000);
 
  return ret;
}

uint16_t BMP085::read16(uint8_t a) {
  uint8_t retbuf[2];
  uint16_t ret;

  // send 1 byte, reset i2c, read 2 bytes
  // we could typecast uint16_t as uint8_t array but would need to ensure proper
  // endianness
  i2c_dev->write(BMP085_I2CADDR,&a,1,true, 2000);
  i2c_dev->read(BMP085_I2CADDR, retbuf,2,false, 4000);
 
  // write_then_read uses uint8_t array
  ret = uint16_t(retbuf[1]) | ( uint16_t(retbuf[0]) << 8);

  return ret;
}

void BMP085::write8(uint8_t a, uint8_t d) {
    uint8_t buff[2];
    buff[0] = a;
    buff[1] = d;
  // send d prefixed with a (a d [stop])
  i2c_dev->write(BMP085_I2CADDR,buff,2,false,2000);
 }
