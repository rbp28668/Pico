#include <stdio.h>
#include "pico/stdlib.h"
#include "vario.h"
#include "telemetry.h"
#include "bmp085.h"
#include "ADXL362.h"

const uint LED_PIN = 25;
const uint SDA_PIN = 20;
const uint SCL_PIN = 21;
const uint MISO_PIN = 12;
const uint SCK_PIN = 14;
const uint MOSI_PIN = 15;
const uint ADXL_CS_PIN = 13;

float mean(float* data, int npoints){
    float total;
    for(int i=0; i<npoints; ++i){
        total += data[i];
    }
    return total/npoints;
}

uint64_t min(uint64_t* data, int npoints){
    uint64_t minval = UINT64_MAX;
    for(int i=0; i<npoints; ++i) {
        if(data[i]< minval){
            minval = data[i];
        }
    }
    return minval;
}

extern "C" int main() {

    /*
    stdio_init_all();
    for(int i=0; i<20; ++i) {
        busy_wait_us_32(100000);
        printf(".");
    }
    printf("\n");
    printf("Telemetry\n");
    */

    Slots& slots = telemetry_slots();

    Slot* slot = slots.getSlot(2);
    slot->setUnits(Slot::UNIT_METERS);
    slot->setValue(0);
    slot->setAlarm(false);
    slot->setEnabled(true);

    slot = slots.getSlot(3);
    slot->setUnits(Slot::UNIT_MS);
    slot->setValue(0);
    slot->setAlarm(false);
    slot->setEnabled(true);

    slot = slots.getSlot(4);
    slot->setUnits(Slot::UNIT_DEG_C);
    slot->setValue(0);
    slot->setAlarm(false);
    slot->setEnabled(true);

    slot = slots.getSlot(5);
    slot->setUnits(15);
    slot->setValue(0);
    slot->setAlarm(false);
    slot->setEnabled(true);

    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);
    gpio_put(LED_PIN, 0);

    start_telemetry();

    // Ring buffer for height/time samples.
    const int SIZE = 20;
    uint64_t time[SIZE];
    float height[SIZE];
    int bufferIndex = 0;

    I2C i2c(i2c0, SDA_PIN, SCL_PIN);
    BMP085 bmp085(&i2c);
    bmp085.begin(BMP085_ULTRAHIGHRES);

    // Initialise array by taking multiple samples over a short-ish period of time.
    for(int i=0; i<SIZE; ++i){
        float t = (float) time_us_64() / 1000000.0f; // time in seconds;
        float alt = bmp085.readAltitude(); // in metres
        time[i] = t;
        height[i] = alt;
        sleep_ms(10);
    }

    // Set start height as mean of those samples.
    float start_height = mean(height, SIZE);

    SPI spi(spi1, MISO_PIN, SCK_PIN, MOSI_PIN, 100*1000);
    ADXL362 g_sensor(&spi);
    g_sensor.begin(ADXL_CS_PIN);
    g_sensor.setSensitivity(8);  // -> 4mG per unit


    while (1) {
        uint64_t t = time_us_64(); // time in seconds;
        float alt = bmp085.readAltitude(); // in metres
        float temp = bmp085.readTemperature(); // degrees C
        
        //printf("%f, %f\n",alt, temp);

        // Store raw time (uS) and altitude (m)
        time[bufferIndex] = t;
        height[bufferIndex]= alt;
        ++bufferIndex;
        if(bufferIndex >= SIZE) bufferIndex = 0;

        // Rebase times so they start at zero.  Otherwise
        // we may start to lose precision over as we're looking
        // at LS bits of an increasingly large number.
        uint64_t startTime = min(time, SIZE);
        float x[SIZE];
        for(int i=0; i<SIZE; ++i){
            x[i] = (float)(time[i] - startTime) / 1000000.0;
        }

        // Basic line fit to get gradient - i.e. rate of climb/descent
        float X = mean(x, SIZE);
        float Y = mean(height,SIZE);
        float T = 0;
        float B = 0;
        for(int i=0; i<SIZE; ++i){
            T += (x[i] - X)*(height[i] - Y);
            B += (x[i] - X)*(x[i] - X);
        }
        float rate = T/B;

        slots.getSlot(2)->setValue((int)(alt - start_height + 0.5)); // integer metres.
        slots.getSlot(3)->setValue((int)(10 * rate));  //climb/descend
        slots.getSlot(4)->setValue( (int) (10 * temp + 0.5)); // temp to 1 decimal place

        g_sensor.beginMeasure();
        int z = g_sensor.readZData();
        float g = (4.0f * z)/1000;  // 4 mG per unit
        slots.getSlot(5)->setValue(int(g*100.0f));
        //slot->setAlarm(value > 150);
        sleep_ms(100);
    }
}
