/**
 * Copyright (c) 2022 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"

// #include "lwip/dns.h"
// #include "lwip/pbuf.h"
// #include "lwip/udp.h"

#include "ntp_client.hpp"

#define NTP_MSG_LEN 48
#define NTP_PORT 123
#define NTP_DELTA 2208988800 // seconds between 1 Jan 1900 and 1 Jan 1970

/// @brief Create an NTP client that's not bound to any NTP server
/// @param callback
NtpClient::NtpClient(Callback *callback)
    : pCallback(callback), ntp_pcb(nullptr), state(State::STARTUP)
{
    ntp_pcb = udp_new_ip_type(IPADDR_TYPE_ANY);
    if (!ntp_pcb)
    {
        printf("failed to create pcb\n");
        state = State::FAILED;
    }
}

/// @brief Create an NTP client that's bound to the given host.
/// @param callback
/// @param host
NtpClient::NtpClient(Callback *callback, const char *host)
    : NtpClient(callback)
{
    bind(host);
}

/// @brief Creat an NTP client that's bound to the given IP address.
/// @param callback
/// @param addr
NtpClient::NtpClient(Callback *callback, ip_addr_t addr)
    : NtpClient(callback)
{
    bind(addr);
}

/// @brief Destroy an NtpClient
NtpClient::~NtpClient()
{
    if (ntp_pcb)
    {
        // TODO
    }
    pCallback = nullptr;
}

/// @brief Bind to a given host and, once DNS lookup is complete, make a request.
/// @param host
void NtpClient::bind(const char *host)
{
    printf("binding to %s\n", host);
    ntp_server_host = host;

    // Ensure we start listening before any requests sent
    udp_recv(ntp_pcb, NtpClient::ntp_received, this);
    
    // No timeout alarm yet.
    dns_alarm_id = 0; 

    bool retry = false;
    
    cyw43_arch_lwip_begin();
    int err = dns_gethostbyname(host, &ntp_server_address, NtpClient::dns_found, this);
    cyw43_arch_lwip_end();

    // Now see if DNS worked....
    if (err == ERR_OK)
    {
        request(); // Cached result
    }
    else if (err != ERR_INPROGRESS)
    { // ERR_INPROGRESS means expect a callback
        printf("dns request failed\n");
        state = State::INVALID_DNS;
    }
    else
    {
        printf("Waiting for DNS\n");
        state = State::WAIT_DNS;
        dns_alarm_id = add_alarm_in_ms(10 * 1000, NtpClient::dns_timeout, this, true);
    }
}

/// @brief Bind to a given IP address and make a request.
void NtpClient::bind(ip_addr_t addr)
{
    ntp_server_address = addr;
    udp_recv(ntp_pcb, NtpClient::ntp_received, this);
    request();
}

/// @brief Sends an NTP request.
void NtpClient::request()
{
    printf("Requesting NTP\n");
    cyw43_arch_lwip_begin();
    struct pbuf *p = pbuf_alloc(PBUF_TRANSPORT, NTP_MSG_LEN, PBUF_RAM);
    uint8_t *req = (uint8_t *)p->payload;
    memset(req, 0, NTP_MSG_LEN);
    req[0] = 0x1b;
    udp_sendto(ntp_pcb, p, &ntp_server_address, NTP_PORT);
    pbuf_free(p);
    cyw43_arch_lwip_end();
    state = State::WAIT_NTP;
}

void NtpClient::dns_found(const char *hostname, const ip_addr_t *ipaddr, void *arg)
{
    NtpClient *pClient = static_cast<NtpClient *>(arg);
    pClient->onDnsFound(hostname, ipaddr);
}

void NtpClient::onDnsFound(const char *hostname, const ip_addr_t *ipaddr)
{

    // Cancel timeout alarm.
    if(dns_alarm_id != 0){
        cancel_alarm(dns_alarm_id);
        dns_alarm_id = 0;
    }

    if (ipaddr)
    {
        ntp_server_address = *ipaddr;
        printf("ntp address %s\n", ipaddr_ntoa(ipaddr));
        request();
    }
    else
    {
        printf("ntp dns request failed\n");
        state = State::INVALID_DNS;
    }
}

int64_t NtpClient::dns_timeout(alarm_id_t id, void *user_data)
{
    NtpClient *pClient = static_cast<NtpClient *>(user_data);
    return pClient->onDnsTimeout(id);
}

int64_t NtpClient::onDnsTimeout(alarm_id_t id)
{
    printf("DNS timeout\n");
    bind(ntp_server_host);  // try again!
    return 0;
}

void NtpClient::ntp_received(void *arg, struct udp_pcb *pcb, struct pbuf *p, const ip_addr_t *addr, u16_t port)
{
    NtpClient *pClient = static_cast<NtpClient *>(arg);
    pClient->onNtpReceived(pcb, p, addr, port);
}

/*
See https://www.rfc-editor.org/rfc/rfc1769

                           1                   2                   3
       0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
      |LI | VN  |Mode |    Stratum    |     Poll      |   Precision   |
      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
      |                          Root Delay                           |
      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
      |                       Root Dispersion                         |
      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
      |                    Reference Identifier                       |
      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
      |                                                               |
      |                   Reference Timestamp (64)                    |
      |                                                               |
      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
      |                                                               |
      |                   Originate Timestamp (64)                    |
      |                                                               |
      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
      |                                                               |
      |                    Receive Timestamp (64)                     |
      |                                                               |
      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
      |                                                               |
      |                    Transmit Timestamp (64)                    |
      |                                                               |
      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
      |                                                               |
      |                                                               |
      |                  Authenticator (optional) (96)                |
      |                                                               |
      |                                                               |
      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*/
void NtpClient::onNtpReceived(struct udp_pcb *pcb, struct pbuf *p, const ip_addr_t *addr, u16_t port)
{
    printf("ntp received");
    uint8_t mode = pbuf_get_at(p, 0) & 0x7;
    uint8_t stratum = pbuf_get_at(p, 1);

    // Check the result
    if (ip_addr_cmp(addr, &ntp_server_address) && port == NTP_PORT && p->tot_len == NTP_MSG_LEN &&
        mode == 0x4 && stratum != 0)
    {
        // Use transmit timestamp.
        uint8_t seconds_buf[4] = {0};
        pbuf_copy_partial(p, seconds_buf, sizeof(seconds_buf), 40);
        uint32_t seconds_since_1900 = seconds_buf[0] << 24 | seconds_buf[1] << 16 | seconds_buf[2] << 8 | seconds_buf[3];

        uint8_t fraction_buf[4] = {0};
        pbuf_copy_partial(p, fraction_buf, sizeof(fraction_buf), 44);
        uint32_t fraction = fraction_buf[0] << 24 | fraction_buf[1] << 16 | fraction_buf[2] << 8 | fraction_buf[3];

        // NTP epoch is 1900 whereas time_t uses 1970.
        uint32_t seconds_since_1970 = seconds_since_1900 - NTP_DELTA;
        time_t epoch = seconds_since_1970;

        pCallback->onNtp(epoch, fraction);
        state = State::IDLE;
    }
    else
    {
        printf("invalid ntp response\n");
        state = State::INVALID_NTP;
    }
    pbuf_free(p);
}

// // Called with results of operation
// static void ntp_result(NTP_T *state, int status, time_t *result)
// {
//     if (status == 0 && result)
//     {
//         struct tm *utc = gmtime(result);
//         printf("got ntp response: %02d/%02d/%04d %02d:%02d:%02d\n", utc->tm_mday, utc->tm_mon + 1, utc->tm_year + 1900,
//                utc->tm_hour, utc->tm_min, utc->tm_sec);
//     }

//     if (state->ntp_resend_alarm > 0)
//     {
//         cancel_alarm(state->ntp_resend_alarm);
//         state->ntp_resend_alarm = 0;
//     }
//     state->ntp_test_time = make_timeout_time_ms(NTP_TEST_TIME);
//     state->dns_request_sent = false;
// }
