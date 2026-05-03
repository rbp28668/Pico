#ifndef __BSP_CST328_H__
#define __BSP_CST328_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "../PicoHardware/i2c.h"

#define BSP_CST328_RST_PIN 17
#define BSP_CST328_INT_PIN 18

#define CST328_LCD_TOUCH_MAX_POINTS (5)

#define CST328_DEVICE_ADDR 0x1A

///  CST328 Touch Controller
///  Derived from waveshare example code for 2.8in LCD board

class CST328
{

public:
    struct Data
    {
        uint8_t points; // Number of touch points
        struct
        {
            uint16_t x;        /*!< X coordinate */
            uint16_t y;        /*!< Y coordinate */
            uint16_t pressure; /*!< pressure */
        } coords[CST328_LCD_TOUCH_MAX_POINTS];
    };

private:
    Data _data;

    uint8_t _rotation;
    uint16_t _width;
    uint16_t _height;

    I2C *_pi2c;
    uint8_t _rst_pin;
    uint8_t _int_pin;

    void read_byte(uint16_t reg_addr, uint8_t *data, size_t len);
    void write_byte(uint16_t reg_addr, uint8_t *data, size_t len);

public:
    // Single bit to determine data available.
    static bool read_data_done;

    CST328(I2C *pi2c, uint8_t rst_pin, uint8_t int_pin, uint16_t rotation, uint16_t width, uint16_t height);

    void reset();
    void set_rotation(uint16_t rotation);
    bool get_touch_data(CST328::Data *cst328_data);
    void read(void);
};

#endif // __BSP_CST328_H__