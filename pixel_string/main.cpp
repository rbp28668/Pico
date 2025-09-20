
extern "C"
{
#include <cyw43.h>
#include "lwip/apps/mdns.h"
}
#include "pico/multicore.h"
#include "../WebServer/wifi.hpp"
#include "../WebServer/server.hpp"
#include "../WebServer/webserver.hpp"
#include "../WebServer/teapot.hpp"
#include "index.hpp"
#include "display.hpp"
#include "display_webapp.hpp"

WifiStation station;
Webserver webserver;
Teapot teapot; // respondes to /coffee with 418...
IndexPage indexPage;
DisplayWebapp displayWebapp;

LedDisplay display;

// TODO GET /favicon.ico HTTP/1.1

#if LWIP_MDNS_RESPONDER
static void
// void (*)(netif *netif, u8_t result, s8_t slot)
mdns_example_report(struct netif *netif, u8_t result, s8_t slot)
{
    LWIP_PLATFORM_DIAG(("mdns status[netif %d]: %d\n", netif->num, result));
}
#endif

int main()
{

    stdio_init_all();

    ::sleep_ms(2000);
    printf("\n\nPixel String Startup\n");
    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);

    uint8_t mac[6];
    if (cyw43_wifi_get_mac(&cyw43_state, CYW43_ITF_STA, mac) == 0)
    {
        printf("MAC ADDR: ");
        for (int i = 0; i < 5; ++i)
            printf("%02.2x:", mac[i]);
        printf("%2.2x\n", mac[5]);
    }

    // start weather sampling on cpu 1
    // printf("Starting data acquisition\n");
    // multicore_launch_core1(run_weather);

    const char *ssid = PICO_WIFI_SSID;
    const char *password = PICO_WIFI_PASSWORD;

    //         mdns_resp_register_name_result_cb(mdns_example_report);
    mdns_resp_init();

    while (true)
    {
        if (station.connect(ssid, password, 30000))
        {

            webserver.addApplication(&teapot);
            webserver.addApplication(&displayWebapp);
            webserver.addApplication(&indexPage);
            

            TcpServer server(&webserver);
            if (server.open(80))
            {

                mdns_resp_add_netif(netif_default, CYW43_HOST_NAME);
                mdns_resp_announce(netif_default); // I'm here!

                absolute_time_t next = get_absolute_time();
                while (true)
                {

                    display.update();

                    next = delayed_by_ms(next, 100); // 10Hz cycle time (100mS)

                    do
                    {
                        cyw43_arch_wait_for_work_until(next);
                        cyw43_arch_poll();
                    } while (absolute_time_diff_us(get_absolute_time(), next) > 0);
                }
            }

            mdns_resp_remove_netif(netif_default);

            server.close();
        }
        else
        {
            // Couldn't connect to wifi so just run default
            absolute_time_t next = get_absolute_time();
            while (true)
            {
                display.update();
                next = delayed_by_ms(next, 100); // 10Hz cycle time (100mS)
                sleep_until(next);
            }
        }
    }
}