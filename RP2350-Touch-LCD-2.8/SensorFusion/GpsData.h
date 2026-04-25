#pragma once
#include "Vec3.h"
#include <cstdint>
#include <cmath>

namespace sf {

struct GpsData {
    double latitude  = 0.0;     // degrees
    double longitude = 0.0;     // degrees
    float  altitudeMSL = 0.0f;  // metres above mean sea level

    // Barometric altitude in metres MSL.  Set to NaN when unavailable.
    float baroAltitude = std::numeric_limits<float>::quiet_NaN();

    Vec3  velocityNED;          // m/s in NED frame
    float horizontalAcc = 10.0f;// 1-sigma position accuracy (m)
    float verticalAcc   = 15.0f;// 1-sigma vertical accuracy (m)
    float velocityAcc   = 0.5f; // 1-sigma velocity accuracy (m/s)
    float hdop          = 99.0f;
    float vdop          = 99.0f;
    uint8_t fixType     = 0;    // 0=none  2=2D  3=3D
    uint8_t numSats     = 0;
    double  timestamp   = 0.0;  // seconds (GPS or system time)
    bool    valid       = false;
};

// Glider polar: sink_rate(V) = a*V² + b*V + c  (m/s, positive = sinking)
// V is true airspeed in m/s.  Set all to zero to disable netto computation.
struct GliderPolar {
    float a = 0.0f;
    float b = 0.0f;
    float c = 0.0f;

    float sinkRate(float tasMs) const {
        return a * tasMs * tasMs + b * tasMs + c;
    }

    bool valid() const { return a != 0.0f || b != 0.0f || c != 0.0f; }
};

struct AirspeedData {
    float ias         = 0.0f;   // indicated airspeed (m/s)
    float variance    = 4.0f;   // 1-sigma² measurement noise (m/s)²
    float pressureAlt = std::numeric_limits<float>::quiet_NaN();  // for IAS→TAS; NaN = use filter alt
    double timestamp  = 0.0;
    bool  valid       = false;
};

} // namespace sf
