/// timer tick

#include <stdio.h>
#include <assert.h>
#include "pico/stdlib.h"
#include "../PicoHardware/i2c.h"
#include "../Sensors/HDC1080.h"
#include "../Sensors/BMP280.h"
#include "../Sensors/MCP9808.h"
#include "../Sensors/GY30.h"

#include "letterbox.hpp"



// Inbuilt LED
const uint LED_PIN = 25;

// Sensors on I2C
const uint8_t I2C_SDA_PIN = 4;
const uint8_t I2C_SCL_PIN = 5;

Letterbox letterbox;

extern void run_weather() {

   printf("Acquisition Starting (core 1)\n");
 
    bool state = true;
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN,GPIO_OUT);
    gpio_put(LED_PIN,0);

    I2C i2c(i2c0, I2C_SDA_PIN, I2C_SCL_PIN);
   
    printf("Initialising HDC1080\n");
    HDC1080 hdc1080(&i2c);
    HDC1080Config cfg = hdc1080.getConfig();
    cfg.modeBoth();
    hdc1080.configure(cfg);
    printf("HDC1080 Device ID: %04x\n",hdc1080.deviceId());
    printf("HDC1080 Manufacturer: %04x\n",hdc1080.manufacturer());
    printf("HDC1080 Serial: %012lx\n",hdc1080.serialNumber());


    printf("Initialising BMP280\n");
    BMP280 bmp280(&i2c);
    if(bmp280.devicePresent()) printf("Found BMP280\n");

    printf("Initialising MCP9808\n");
    MCP9808 mcp9808(&i2c);
    printf("MCP9808 Device: %04x\n",mcp9808.getDeviceIdAndRevision());
    printf("MCP9808 Manufacturer: %04x\n",mcp9808.getManufacturerId());
    mcp9808.setResolution(3);

    printf("Initialising GY30\n");
    GY30 gy30(&i2c);
    gy30.send(GY30::ContinuousHResolutionMode2);
    printf("GY30 initialised\n");
    
    while(true) {

        printf("TICK: starting read\n");
        hdc1080.trigger();
        ::sleep_ms(HDC1080_MEASUREMENT_DELAY * 2);
        hdc1080.readCombined();
        letterbox.humidity = hdc1080.humidity();
        letterbox.temp3 = hdc1080.temperature();

        bmp280.read();
        letterbox.pressure = bmp280.pressure() / 100.0f;
        letterbox.temp2 = bmp280.temperature() / 100.0f;

        letterbox.primaryTemp = mcp9808.readTemp();

        letterbox.lux = gy30.readLux();

        state = !state;
        //gpio_put(LED_PIN, state ? 1 : 0);

        printf("TICK: end\n");
        ::sleep_ms(1000);
       } 
}
