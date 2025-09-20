
extern "C" {
#include <cyw43.h>
}
#include "pico/multicore.h"
#include "../WebServer/wifi.hpp"
#include "../WebServer/server.hpp"
#include "../WebServer/webserver.hpp"
#include "../WebServer/teapot.hpp"
#include "index.hpp"
#include "weather_webapp.hpp"

WifiStation station;
Webserver webserver;
WeatherWebapp webapp;
Teapot teapot; // respondes to /coffee with 418...
IndexPage indexPage;

// TODO GET /favicon.ico HTTP/1.1


extern void run_weather();

int main() {

    stdio_init_all();

    ::sleep_ms(2000);
    printf("\n\nWeather Startup\n");


    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);

    uint8_t mac[6];
    if(cyw43_wifi_get_mac(&cyw43_state, CYW43_ITF_STA, mac) == 0){
        printf("MAC ADDR: " );
        for(int i=0;i<5;++i) printf("%02.2x:",mac[i]);
        printf("%2.2x\n",mac[5]);
    }
  
    // start weather sampling on cpu 1
    printf("Starting data acquisition\n");
    multicore_launch_core1(run_weather);

    const char* ssid = PICO_WIFI_SSID;
    const char* password = PICO_WIFI_PASSWORD;
    while(true) {
        if(station.connect(ssid, password, 30000)) {

            webserver.addApplication(&webapp);
            webserver.addApplication(&teapot);
            webserver.addApplication(&indexPage);

            TcpServer server(&webserver);
            if(server.open(80)){
                 server.run();
            }
            server.close();
        }
        printf("Weather Restart\n");
        sleep_ms(1000);
        station.restart();
    }
}