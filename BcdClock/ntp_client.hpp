#ifndef NTP_CLIENT_HPP
#define NTP_CLIENT_HPP

#include <string.h>
#include <time.h>
#include "pico/stdlib.h"

#include "lwip/dns.h"
#include "lwip/pbuf.h"
#include "lwip/udp.h"

class NtpClient {

    public:
    enum class State {
        STARTUP,
        IDLE,
        FAILED,
        WAIT_DNS,
        WAIT_NTP,
        INVALID_NTP,
        INVALID_DNS
    };


    struct Callback {
        virtual void onNtp(time_t now, uint32_t fraction) = 0;
    };

    static constexpr const char* DEFAULT_SERVER = "pool.ntp.org";



    private:

    const char* ntp_server_host;
    ip_addr_t ntp_server_address;
    struct udp_pcb *ntp_pcb;
    State state;
    Callback* pCallback;

    alarm_id_t dns_alarm_id;

    static void dns_found(const char *hostname, const ip_addr_t *ipaddr, void *arg);
    static int64_t dns_timeout(alarm_id_t id, void *user_data);
    static void ntp_received(void *arg, struct udp_pcb *pcb, struct pbuf *p, const ip_addr_t *addr, u16_t port);

    void onDnsFound(const char *hostname, const ip_addr_t *ipaddr);
    int64_t onDnsTimeout(alarm_id_t id);
    void onNtpReceived(struct udp_pcb *pcb, struct pbuf *p, const ip_addr_t *addr, u16_t port);
    
    public:


 
    NtpClient(Callback* callback);
    NtpClient(Callback* callback, const char* host);
    NtpClient(Callback* callback, ip_addr_t addr);
    ~NtpClient();

    void bind(const char* host);
    void bind(ip_addr_t addr);
    State getState() const { return state;}
    void request();

};
#endif