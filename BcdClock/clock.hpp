
#ifndef CLOCK_HPP
#define CLOCK_HPP

#include "pico/time.h"
#include "ntp_client.hpp"

class Clock : public NtpClient::Callback{

    absolute_time_t t0;     // PicoTime NTP packet arrived 
    time_t seconds;         // from NTP
    uint32_t fraction;      // from NTP
    uint64_t uS;            // NTP seconds and fraction as microseconds

    absolute_time_t t;      // current estimate of next seconds rollover
    time_t _now;            // and seconds count to report as current time.

    int ticksPerSec;        // track number of clock ticks per NTP second.  Will be
                            // near 1000000 but not exactly due to crystal tolerances etc.

    int deltaUs;            // Number of PicoTime counts to bump t up every second
                            // Based on ticksPerSec but may also include an offset to 
                            // bring time back into line with NTP if it's drifted.

    bool hasTime;           // Set true by the first NTP packet

    inline uint64_t toUs(time_t now, uint32_t fraction) {
        return uint64_t(now) * 1000000l +         // seconds to uS
        ((uint64_t(fraction) * 1000000l) >> 32);    // fractional second to uS   
    }

    public:

    Clock();
    virtual void onNtp(time_t now, uint32_t fraction);
    absolute_time_t tick();
    time_t now() const;

    int tps() const { return ticksPerSec;}
    int dus() const { return deltaUs;}
};

#endif