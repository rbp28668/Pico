
#ifndef BMP085_H
#define BMP085_H

#include <stdint.h>
#include "i2c.h"

#define BMP085_DEBUG 0 //!< Debug mode

#define BMP085_I2CADDR 0x77 //!< BMP085 I2C address

#define BMP085_ULTRALOWPOWER 0 //!< Ultra low power mode
#define BMP085_STANDARD 1      //!< Standard mode
#define BMP085_HIGHRES 2       //!< High-res mode
#define BMP085_ULTRAHIGHRES 3  //!< Ultra high-res mode


/*!
 * @brief Main BMP085 class
 */
class BMP085 {
public:
  BMP085(I2C* i2c);
  /*!
   * @brief Starts I2C connection
   * @param mode Mode to set, ultra high-res by default
    * @return Returns true if successful
   */
  bool begin(uint8_t mode = BMP085_ULTRAHIGHRES);
  /*!
   * @brief Gets the temperature over I2C from the BMP085
   * @return Returns the temperature
   */
  float readTemperature(void);
  /*!
   * @brief Gets the pressure over I2C from the BMP085
   * @return Returns the pressure
   */
  int32_t readPressure(void);
  /*!
   * @brief Calculates the pressure at sea level
   * @param altitude_meters Current altitude (in meters)
   * @return Returns the calculated pressure at sea level
   */
  int32_t readSealevelPressure(float altitude_meters = 0);
  /*!
   * @brief Reads the altitude
   * @param sealevelPressure Pressure at sea level, measured in pascals
   * @return Returns the altitude
   */
  float readAltitude(float sealevelPressure = 101325); // std atmosphere
  /*!
   * @brief Reads the raw temperature
   * @return Returns signed 16-bit integer of the raw temperature
   */
  uint16_t readRawTemperature(void);
  /*!
   * @brief Reads the raw pressure
   * @return Returns signed 32-bit integer of the raw temperature
   */
  uint32_t readRawPressure(void);

private:
  int32_t computeB5(int32_t UT);
  uint8_t read8(uint8_t addr);
  uint16_t read16(uint8_t addr);
  void write8(uint8_t addr, uint8_t data);

  I2C *i2c_dev;
  uint8_t oversampling;

  int16_t ac1, ac2, ac3, b1, b2, mb, mc, md;
  uint16_t ac4, ac5, ac6;
};

#endif