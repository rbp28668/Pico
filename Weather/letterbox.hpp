//#include "pico/stdlib.h"

struct Letterbox {

    // mcp9808
    float primaryTemp;

    // bmp280
    float pressure;
    float temp2;

    // hdc1080
    float humidity;
    float temp3;

    // gy30
    float lux;

    //absolute_time_t readingTime;
 };

 extern Letterbox letterbox;