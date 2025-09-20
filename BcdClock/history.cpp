#include "history.hpp"

History::History()
: minute(0), minuteBit(0), hourBit(0){
    for(int i=0; i<24; ++i) hours[i] = 0;
}

void History::add(bool active) {

    if(minuteBit == 60){
        // Promote to hour
        if(hourBit == 60) { // minutes in hour
            hourBit = 0;
            ++hourOfDay;
            if(hourOfDay == 24) hourOfDay = 0;
        } 

        // Record if anything happened this minute (i.e. minute is non zero) in the
        // correct minute of the current hour.
        hours[hourOfDay] |=  uint64_t( (minute != 0) ? 1 : 0) << hourBit;
        ++hourBit;

        // reset minute count, don't reset minute - always has last 60 secs of data.
        minuteBit = 0;   
        
    }
    minute <<= 1;
    minute &= ~0xF000000000000000; // 60 bits only
    minute |= active ? 1 : 0;      // and fold in this one.
    ++minuteBit;
}

