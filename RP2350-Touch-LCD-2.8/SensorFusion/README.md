# SensorFusion — QMI8658A IMU + GPS/INS Kalman Filter for Gliders

C++17 header-only library implementing a 15-state Multiplicative Extended Kalman Filter (MEKF) for orientation and position tracking using the QMI8658A 6-axis IMU. Designed for glider flight: handles sustained banked turns, variable g-loading, and intermittent or absent GPS, airspeed, and barometric data.

## Files

| File | Purpose |
|---|---|
| **Vec3.h** | 3D vector with dot, cross, normalize |
| **Quaternion.h** | Unit quaternion — multiply, rotate, axis-angle, Euler extract, rotation matrix |
| **Mat.h** | Compile-time-sized matrix template with multiply, transpose, inverse (Gauss-Jordan), block insert/extract, skew-symmetric constructor. Typedefs for 3x3, 6x6, 15x15 |
| **HalInterface.h** | Abstract I2C/SPI HAL — implement for your platform |
| **QMI8658A.h** | Full driver — init, register config, burst-read 6-axis + temperature, raw-to-SI conversion, timestamp, calibration-on-demand |
| **GpsData.h** | GPS fix struct, airspeed struct, glider polar, barometric altitude, accuracy estimates |
| **MEKF.h** | 15-state multiplicative EKF (see State Vector below) |
| **SensorFusion.h** | Top-level orchestrator tying everything together |

## State Vector

The MEKF uses an error-state formulation. The nominal state is maintained outside the filter; the Kalman filter estimates a 15-element error vector.

**Nominal state (16 components):**

| State | Size | Description |
|---|---|---|
| q | 4 | Attitude quaternion (body-to-NED) |
| p | 3 | Position in NED navigation frame (m) |
| v | 3 | Velocity in NED navigation frame (m/s) |
| bg | 3 | Gyroscope bias (rad/s) |
| ba | 3 | Accelerometer bias (m/s²) |

**Error state (15 components):**

```
dx = [dθ(3), dp(3), dv(3), dbg(3), dba(3)]
```

## Measurement Updates

| Update | Source | What it corrects | Rate |
|---|---|---|---|
| Gravity reference (adaptive) | Accelerometer | Roll, pitch, accel bias | Every IMU sample |
| GPS position + velocity | GPS receiver | Position, velocity, heading, all biases | 1-10 Hz, when available |
| GPS position only | GPS receiver | Position, biases | 1-10 Hz, when available |
| Airspeed (IAS) | Pitot/static | Velocity magnitude + heading (via body-x) | When available |
| Barometric altitude | Baro sensor | Vertical position | When available |
| Zero-velocity (ZUPT) | No-motion detector | Velocity, biases | When stationary |

All aiding sources (GPS, airspeed, baro) are optional and intermittent. The filter runs on IMU alone and incorporates corrections as they arrive.

## Glider-Specific Design

### Gravity Update: Centripetal Compensation + Adaptive Noise

These two mechanisms work together to keep the accelerometer gravity reference useful through banked turns.

**Centripetal compensation** subtracts the expected centripetal acceleration before comparing to the gravity reference:

```
a_centripetal = omega_body × v_body
a_corrected   = a_measured - a_centripetal
```

This uses the bias-corrected gyro rate and the current velocity estimate rotated into the body frame. In a coordinated banked turn the centripetal component fully explains the extra g-loading, so after subtraction the corrected measurement reads ~1g regardless of bank angle.

**Adaptive R scaling** then inflates the measurement noise based on the *corrected* (not raw) accelerometer magnitude:

```
excess = max(0, |a_corrected| - g - deadband)
R_effective = R_base * (1 + adaptiveScale * excess²)
```

Because the scaling uses the corrected measurement, a coordinated turn sees excess ~0 and R stays at baseline — the gravity update retains full authority throughout the thermal. Only genuinely unexplained dynamics (turbulence gusts, uncoordinated inputs, pull-ups) inflate R and down-weight the update. In extreme unmodelled manoeuvres the noise dominates and the update effectively goes to zero — a smooth transition rather than a hard gate.

### IMU Mounting Rotation

The QMI8658A PCB can be mounted at any orientation in the fuselage. Set `Config::mountingRotation` to the quaternion that rotates from PCB frame to body frame (x = forward, y = right, z = down). Raw accelerometer and gyroscope readings are rotated before entering the filter or calibration.

```cpp
// Example: PCB mounted with its X axis pointing right and Y axis pointing forward
cfg.mountingRotation = sf::Quaternion::fromEuler(0, 0, -1.5708f);  // -90 deg about Z
```

Leave as identity (default) if the PCB is already aligned with the body axes.

### Airspeed Fusion

Indicated airspeed (IAS) is converted to true airspeed (TAS) via ISA atmosphere density correction, then fused as a scalar measurement of the body-x component of velocity:

```
h(x) = e_x^T · R(q)^T · v_NED
innovation = TAS - h(x)
```

This constrains both speed and heading (since it measures the projection of velocity onto the body x-axis). Readings below 5 m/s are rejected. The IAS-to-TAS conversion uses pressure altitude from the airspeed sensor if available, otherwise from the filter's altitude estimate.

### Barometric Altitude (GPS-independent)

Barometric altitude no longer requires a GPS fix. Set `Config::baroRefAltitude` to the field elevation or QNH-derived reference altitude at startup:

```cpp
cfg.baroRefAltitude = 450.0f;  // metres MSL from QNH
```

The filter uses this as the vertical datum immediately. If GPS arrives later, the first GPS fix also seeds the reference (if one wasn't already set). Set to `NaN` to revert to the old behaviour of waiting for GPS.

### Wind Estimation

When both GPS velocity and airspeed are available, wind is estimated as:

```
wind_NED = GPS_velocity_NED - TAS * heading_unit_vector
```

The heading unit vector comes from the attitude quaternion's body-x axis rotated to NED. The estimate is smoothed with a complementary filter (alpha = 0.05 per GPS update). Output includes wind vector, horizontal wind speed, and the bearing the wind is coming FROM.

Wind is the single most valuable derived output for a glider pilot — thermal centering, final glide calculation, and ridge soaking all depend on it.

### Total-Energy Variometer

```
TE_vario = climb_rate + (V · V_dot) / g
```

Compensates for stick-pull zoom climbs. A sustained positive TE reading means the glider is in rising air, not just converting speed to altitude.

### Netto Variometer

Subtracts the glider's expected polar sink rate from the TE vario to show what the air mass is doing:

```
netto = TE_vario + polar_sink(TAS)
```

(Polar sink is positive-down, so adding it subtracts the sink.) Requires a glider polar configured via `Config::polar`:

```cpp
// Quadratic polar: sink = a*V² + b*V + c  (m/s, positive = sinking)
// Typical 15m glider (ASW 28, LS8, etc.)
cfg.polar = {0.000293f, -0.0439f, 1.895f};
```

If no polar is configured, netto falls back to TE vario.

### Load Factor

Instantaneous load factor (`|a_measured| / g`). At wings-level 1g flight this reads 1.0; in a 45-degree coordinated turn ~1.41; in a 60-degree bank ~2.0. Useful for structural awareness and thermal turn monitoring.

### Turn Rate

Bias-corrected yaw rate in degrees/second (positive = right turn), computed by projecting the body-frame gyro rate onto the NED down axis. Useful for thermal centering algorithms and turn-and-bank instruments.

## Quick Start

### 1. Implement the HAL

```cpp
#include "SensorFusion.h"

class MyHal : public sf::HalInterface {
public:
    bool writeRegister(uint8_t devAddr, uint8_t reg, uint8_t value) override {
        // Your I2C/SPI write
    }
    bool readRegister(uint8_t devAddr, uint8_t reg, uint8_t* value) override {
        // Your I2C/SPI single-byte read
    }
    bool burstRead(uint8_t devAddr, uint8_t startReg,
                   uint8_t* buffer, uint8_t length) override {
        // Your I2C/SPI multi-byte read
    }
    uint32_t millis() override {
        // Your monotonic millisecond timer
    }
};
```

### 2. Initialise and run

```cpp
MyHal hal;
sf::SensorFusion fusion(hal);

sf::SensorFusion::Config cfg;
cfg.accelScale = sf::AccelScale::G8;
cfg.accelOdr   = sf::AccelODR::Hz448;
cfg.gyroScale  = sf::GyroScale::DPS2048;
cfg.gyroOdr    = sf::GyroODR::Hz448;

// Mounting: PCB rotated 90 deg clockwise when viewed from above
cfg.mountingRotation = sf::Quaternion::fromEuler(0, 0, 1.5708f);

// Baro reference from QNH (works before GPS)
cfg.baroRefAltitude = 450.0f;

// Glider polar for netto vario (optional)
cfg.polar = {0.000293f, -0.0439f, 1.895f};

fusion.init(cfg);

// Main loop — call at IMU rate
while (true) {
    fusion.processImu();

    // Feed GPS when available (intermittent is fine)
    if (gpsFixReady()) {
        sf::GpsData gps;
        gps.latitude      = getLatitude();
        gps.longitude     = getLongitude();
        gps.altitudeMSL   = getAltitude();
        gps.baroAltitude  = getBaroAlt();    // NaN if unavailable
        gps.velocityNED   = {vn, ve, vd};
        gps.horizontalAcc = getHAcc();
        gps.verticalAcc   = getVAcc();
        gps.velocityAcc   = getVelAcc();
        gps.fixType       = 3;
        gps.valid         = true;
        fusion.processGps(gps);
    }

    // Feed airspeed when available (intermittent is fine)
    if (airspeedReady()) {
        sf::AirspeedData as;
        as.ias       = getIAS();              // m/s
        as.variance  = 2.0f;                  // (m/s)²
        as.valid     = true;
        // as.pressureAlt = ...;              // set if known, else NaN (default)
        fusion.processAirspeed(as);
    }

    // Standalone baro feed (if not using GpsData::baroAltitude)
    if (baroReady())
        fusion.processBaroAltitude(baroAltMSL, 1.0f);

    // Stationary detection from QMI8658A no-motion engine or external logic
    if (deviceStationary())
        fusion.notifyStationary();

    // Read fused output at any time
    auto out = fusion.output();
    // out.attitude      — quaternion
    // out.euler         — Vec3(roll, pitch, yaw) in radians
    // out.latitude      — degrees (valid after first GPS fix)
    // out.longitude     — degrees
    // out.altitude      — metres MSL (from GPS or baro)
    // out.speed         — ground speed, m/s
    // out.course        — radians from north
    // out.verticalSpeed — m/s, positive up
    // out.teVario       — total-energy variometer, m/s, positive up
    // out.nettoVario    — netto variometer, m/s (airmass vertical)
    // out.loadFactor    — |accel| / g  (1.0 at 1g)
    // out.turnRate      — deg/s, positive = right
    // out.wind          — wind vector NED (m/s)
    // out.windSpeed     — horizontal wind speed (m/s)
    // out.windBearing   — direction wind comes FROM (rad from north)
    // out.windValid     — true once both GPS vel + airspeed are available
    // out.gyroBias      — estimated gyro bias (rad/s)
    // out.accelBias     — estimated accel bias (m/s²)
}
```

## Tuning

Key parameters in `MEKF::Config`:

| Parameter | Default | Effect |
|---|---|---|
| `gyroNoiseDensity` | 2.27e-4 rad/s/√Hz | From QMI8658A datasheet. Increase if gyro is noisier than spec. |
| `accelNoiseDensity` | 1.47e-3 m/s²/√Hz | From QMI8658A datasheet. Increase in high-vibration environments. |
| `gyroBiasWalk` | 3e-5 rad/s²/√Hz | How fast gyro bias can change. Loosened for thermal turbulence. |
| `accelBiasWalk` | 3e-4 m/s³/√Hz | How fast accel bias can change. Loosened for in-flight shifts. |
| `adaptiveScale` | 20.0 | How fast gravity-update R inflates with *residual* g-deviation (after centripetal subtraction). Coordinated turns read ~1g after correction, so this only fires on unmodelled dynamics. Higher = more aggressive down-weighting. |
| `adaptiveDeadband` | 0.3 m/s² | Residual g-deviation tolerated before R starts inflating. Accounts for sensor noise and imperfect centripetal estimates. |
| `centripetalCompensation` | true | Subtract omega x v from accel before gravity comparison and R scaling. Disable if the velocity estimate is unreliable (e.g. no GPS and no airspeed). |
| `airspeedVarianceFloor` | 1.0 (m/s)² | Minimum airspeed measurement noise. Prevents over-trusting noisy pitot readings. |

GPS measurement noise is set from the receiver's reported accuracy fields (`horizontalAcc`, `verticalAcc`, `velocityAcc` in `GpsData`), so no manual tuning is needed there.

## Coordinate Frames

- **Body frame:** x = forward, y = right, z = down. If the QMI8658A PCB is not aligned with the body, use `Config::mountingRotation` to rotate raw readings into body frame before they enter the filter.
- **Navigation frame:** NED (North-East-Down). Origin at first GPS fix, or at the IMU location if no GPS is used.
- **WGS-84 conversion:** Latitude/longitude/altitude are converted to/from local NED using meridional and prime-vertical radii of curvature at the reference latitude.

## Yaw Observability

Without a magnetometer, yaw is only constrained by:
- GPS ground velocity (needs >~5 m/s ground speed)
- Airspeed (weakly — constrains body-x projection of velocity)

A glider thermalling in strong wind may have low ground speed despite good airspeed, or vice versa. In these conditions yaw will drift. This is inherent to a 6-axis IMU and cannot be solved without adding a magnetometer. The wind estimate degrades proportionally to yaw drift since it depends on the heading vector.

## Dependencies

None beyond C++17 standard library (`<cmath>`, `<cstdint>`, `<algorithm>`, `<limits>`, `<utility>`).
