
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
#include "clock_webapp.hpp"
#include "ntp_client.hpp"
#include "clock.hpp"
#include "display.hpp"
#include "bst.hpp"
#include "tick.hpp"
#include "rcwl0516.hpp"
#include "history.hpp"
#include "history_webapp.hpp"
#include "adc.hpp"
#include "adc_webapp.hpp"

WifiStation station;
Webserver webserver;
Teapot teapot; // respondes to /coffee with 418...
IndexPage indexPage;
ClockWebapp clockPage;
HistoryWebapp historyPage;
AdcWebapp adcPage;

Clock ntpClock;
NtpClient ntp(&ntpClock);
BcdDisplay display;
Tick ticker;
RCWL0516 movementSensor;
History history;
Adc adc(28); // GPIO 28 - channel 2

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
    printf("\n\nBCD Clock Startup\n");
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

            webserver.addApplication(&clockPage);
            webserver.addApplication(&teapot);
            webserver.addApplication(&historyPage);
            webserver.addApplication(&adcPage);
            webserver.addApplication(&indexPage);

            TcpServer server(&webserver);
            if (server.open(80))
            {

                ntp.bind(NtpClient::DEFAULT_SERVER);

                mdns_resp_add_netif(netif_default, CYW43_HOST_NAME);
                mdns_resp_announce(netif_default); // I'm here!

                int ntpDelay = 0;
                while (true)
                {
                    if (ntp.getState() == NtpClient::State::INVALID_DNS)
                    {
                        ntp.bind(NtpClient::DEFAULT_SERVER);
                    }

                    absolute_time_t next = ntpClock.tick();

                    ticker.soundTick();

                    bool active = movementSensor.test();
                    history.add(active);
                    // display.setNotify1(active ? 0xFF0000 : 0x00FF00);

                    uint16_t light = adc.read();
                    display.setLightLevel(light);

                    time_t now = ntpClock.now();
                    // struct tm *utc = gmtime(&now);
                    //  printf("GM Time: %02d/%02d/%04d %02d:%02d:%02d", utc->tm_mday, utc->tm_mon + 1, utc->tm_year + 1900,
                    //         utc->tm_hour, utc->tm_min, utc->tm_sec);
                    //  printf(" (%d, %d)", ntpClock.tps(), ntpClock.dus());
                    //  printf("\n");

                    // Allow for British Summer Time.
                    int offset = BST::offset(now);
                    now = now + offset;

                    display.update(now);
                    ++ntpDelay;
                    if (ntpDelay == 600)
                    {
                        if (ntp.getState() != NtpClient::State::FAILED)
                        {
                            ntp.request();
                        }

                        ntpDelay = 0;
                    }

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
        printf("BCDClock Restart\n");
        sleep_ms(1000);
        station.restart();
    }
}