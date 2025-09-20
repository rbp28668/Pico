#include "clock.hpp"

Clock::Clock()
: seconds(0), hasTime(false), deltaUs(1000000), ticksPerSec(1000000)
{
    t0 = get_absolute_time();
}

void Clock::onNtp(time_t now, uint32_t fraction)
{
    printf("== NTP received %d\n ", int(now));
 
    if(hasTime){
        t = get_absolute_time();
        uint64_t ntpUs = toUs(now, fraction);

        printf("Previous NTP uS %llu\n", uS);
        printf("NTP uS %llu\n", ntpUs);
        
        int64_t ntpDiff = ntpUs - uS;                       // Time difference between NTP calls in uS
        int64_t timerDiff = absolute_time_diff_us(t0, t);   // Same time difference as recorded by pico timer.

        printf("NTP dif uS %lld\n", ntpDiff);
        printf("Timer dif uS %lld\n", timerDiff);
        // Calculate number of timer ticks per second worth of NTP.
        int tps = int(timerDiff * 1000000 / ntpDiff);
        printf("TPS %d\n", tps);


        // update ticksPerSec value (calibrate our own clock against NTP) but use recursive averaging
        // so that this averages out jitter in the NTP value.
        ticksPerSec = (ticksPerSec * 10 + tps) / 11;
        printf("ticksPerSec %d\n", ticksPerSec);

        // Nominally, update the count 
        deltaUs = ticksPerSec;
        
        // Check time we report against ntp time (all in uS)
        uint64_t nowUs = uint64_t(_now )* 1000000;
        int diff = nowUs - ntpUs;   // +ve if we're ahead.
        deltaUs += diff / 1000;     // pull back in over next 1000 seconds.
                                    // If we're ahead want to make each "second"
                                    // a little bit longer.
  
        printf("Diff uS %d\n", diff);
        printf("Delta uS %d\n", deltaUs);

        // Reset base times for next NTP slot.
        this->seconds = now;
        this->fraction = fraction;
        this->t0 = t;
        this->uS = ntpUs;

    } else {
        this->seconds = now;
        this->fraction = fraction;
        this->t0 = get_absolute_time();
        this->uS = toUs(now,fraction);
        this->hasTime = true;

        this->t = t0;
        this->_now = now;
    }

}

absolute_time_t Clock::tick()
{
    t = delayed_by_us(t, deltaUs);
    ++ _now;
    return t;
}

time_t Clock::now() const{
    return _now;
}