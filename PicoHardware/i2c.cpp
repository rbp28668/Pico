

#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "hardware/gpio.h"
#include "i2c.h"


I2C::I2C(i2c_inst_t *i2c, int gpio_sda, int gpio_scl, int bps ) 
: i2c(i2c)
{
    i2c_init(i2c, bps);
    gpio_set_function(gpio_sda, GPIO_FUNC_I2C);
    gpio_set_function(gpio_scl, GPIO_FUNC_I2C);
    gpio_pull_up(gpio_sda);
    gpio_pull_up(gpio_scl);

}

I2C::~I2C(){
    i2c_deinit(i2c);
}

uint I2C::setBaudrate(uint baudrate){
    return i2c_set_baudrate(i2c, baudrate);
}

int I2C::read (uint8_t addr, uint8_t *dst, size_t len, bool nostop) {
    return i2c_read_blocking (i2c, addr, dst, len, nostop);
}

int I2C::write (uint8_t addr, const uint8_t *src, size_t len, bool nostop){
    return i2c_write_blocking (i2c, addr, src, len, nostop);
}

int I2C::read (uint8_t addr, uint8_t *dst, size_t len, bool nostop, uint timeout_us){
    return i2c_read_timeout_us(i2c, addr, dst, len, nostop, timeout_us);
}

int I2C::write (uint8_t addr, const uint8_t *src, size_t len, bool nostop, uint timeout_us){
    return i2c_write_timeout_us(i2c, addr, src, len, nostop, timeout_us);

}

size_t I2C::readAvailable(){
    return i2c_get_read_available(i2c);
}

size_t I2C::writeAvailable(){
    return i2c_get_write_available(i2c);

}

