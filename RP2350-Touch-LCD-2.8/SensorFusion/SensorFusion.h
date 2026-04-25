#pragma once
#include "QMI8658A.h"
#include "MEKF.h"
#include "GpsData.h"
#include <cmath>

namespace sf {

class SensorFusion {
public:
    struct Config {
        AccelScale accelScale = AccelScale::G8;
        AccelODR   accelOdr   = AccelODR::Hz448;
        GyroScale  gyroScale  = GyroScale::DPS2048;
        GyroODR    gyroOdr    = GyroODR::Hz448;
        bool       enableLpf  = true;
        LPFMode    lpfMode    = LPFMode::BW_5_39;

        MEKF::Config mekf;

        // IMU mounting rotation: rotates raw sensor readings from PCB frame
        // into body frame (x = forward, y = right, z = down).
        // Identity means the PCB is already aligned with the body axes.
        Quaternion mountingRotation = Quaternion::identity();

        // GPS antenna -> IMU offset in body frame (metres)
        Vec3 leverArm;

        // Glider polar for netto variometer.  Leave default (zeros) to disable.
        GliderPolar polar;

        // QNH-based reference altitude for baro-only operation (metres MSL).
        // When GPS is unavailable, baro readings are referenced to this value.
        // Set to NaN to disable baro until first GPS fix (old behaviour).
        float baroRefAltitude = 0.0f;

        // Samples to average during startup calibration (~1 s at 448 Hz)
        int calibrationSamples = 500;
    };

    struct Output {
        Quaternion attitude;
        Vec3  euler;            // roll, pitch, yaw  (rad)
        Vec3  positionNED;      // metres from reference
        double latitude  = 0;   // degrees
        double longitude = 0;   // degrees
        float  altitude  = 0;   // metres MSL
        Vec3  velocityNED;      // m/s
        float speed      = 0;   // m/s ground
        float course     = 0;   // rad from north
        float verticalSpeed = 0;// m/s (positive = up)
        float teVario    = 0;   // total-energy compensated vario (m/s, positive = up)
        float nettoVario = 0;   // netto vario: airmass vertical (m/s, positive = up)
        float loadFactor = 1.0f;// |accel| / g  (1.0 = 1g wings-level)
        float turnRate   = 0;   // yaw rate (deg/s, positive = right)
        Vec3  wind;             // estimated wind vector NED (m/s)
        float windSpeed  = 0;   // horizontal wind speed (m/s)
        float windBearing = 0;  // direction wind is coming FROM (rad from north)
        Vec3  gyroBias;
        Vec3  accelBias;
        bool  gpsAvailable = false;
        bool  windValid    = false;
        bool  initialized  = false;
    };

    SensorFusion(HalInterface& hal, uint8_t imuAddr = QMI8658A::DEFAULT_ADDR)
        : imu_(hal, imuAddr) {}

    bool init(const Config& cfg = Config{}) {
        cfg_ = cfg;
        if (!imu_.init(cfg.accelScale, cfg.accelOdr,
                       cfg.gyroScale,  cfg.gyroOdr,
                       cfg.enableLpf,  cfg.lpfMode))
            return false;

        hasMounting_ = !isIdentity(cfg.mountingRotation);

        if (!std::isnan(cfg.baroRefAltitude)) {
            baroRef_    = cfg.baroRefAltitude;
            hasBaroRef_ = true;
        }

        mekf_.configure(cfg.mekf, imu_.odrHz());
        calibrating_ = true;
        calibCount_  = 0;
        calibAccel_  = {};
        calibGyro_   = {};
        return true;
    }

    // ---- Main loop entry points --------------------------------------------

    bool processImu() {
        if (!imu_.dataReady()) return false;
        ImuSample s = imu_.read();
        if (!s.valid) return false;

        applyMounting(s);
        lastSample_ = s;

        float dt = computeDt(s.timestamp);

        if (calibrating_) {
            calibAccel_ += s.accel;
            calibGyro_  += s.gyro;
            if (++calibCount_ >= cfg_.calibrationSamples)
                finishCalibration();
            return true;
        }

        mekf_.processImu(s.gyro, s.accel, dt);
        return true;
    }

    void processGps(const GpsData& gps) {
        if (!gps.valid || gps.fixType < 2) return;

        if (!hasGpsRef_) {
            refLat_ = gps.latitude;
            refLon_ = gps.longitude;
            refAlt_ = gps.altitudeMSL;
            computeRadii(refLat_);
            hasGpsRef_ = true;

            if (!hasBaroRef_) {
                baroRef_    = gps.altitudeMSL;
                hasBaroRef_ = true;
            }
        }

        Vec3 posNED = gpsToNED(gps.latitude, gps.longitude, gps.altitudeMSL);

        if (cfg_.leverArm.normSq() > 1e-6f)
            posNED -= mekf_.attitude().rotate(cfg_.leverArm);

        float hVar   = gps.horizontalAcc * gps.horizontalAcc;
        float vVar   = gps.verticalAcc   * gps.verticalAcc;
        float velVar = gps.velocityAcc   * gps.velocityAcc;

        mekf_.updateGps(posNED, gps.velocityNED,
                        Vec3(hVar, hVar, vVar),
                        Vec3(velVar, velVar, velVar));
        gpsAvailable_ = true;

        updateWind(gps.velocityNED);

        if (!std::isnan(gps.baroAltitude))
            processBaroAltitude(gps.baroAltitude, 1.0f);
    }

    // Barometric altitude in metres MSL.
    // Works before GPS: uses baroRefAltitude from Config, or first GPS alt.
    void processBaroAltitude(float altMSL, float variance = 1.0f) {
        if (!hasBaroRef_) return;
        float altDown = -(altMSL - baroRef_);
        mekf_.updateBaroAltitude(altDown, variance);
    }

    void processAirspeed(const AirspeedData& as) {
        if (!as.valid || calibrating_) return;

        float pressAlt = as.pressureAlt;
        if (std::isnan(pressAlt)) {
            if (hasBaroRef_)
                pressAlt = baroRef_ - mekf_.position().z;
            else
                pressAlt = 0.0f;
        }
        lastTAS_ = MEKF::iasToTas(as.ias, pressAlt);
        hasTAS_  = true;
        mekf_.updateAirspeed(as.ias, as.variance, pressAlt);
    }

    void notifyStationary(float zuptVariance = 0.01f) {
        if (!calibrating_)
            mekf_.updateZupt(zuptVariance);
    }

    // ---- Output ------------------------------------------------------------

    Output output() const {
        Output o;
        o.initialized   = !calibrating_;
        o.attitude      = mekf_.attitude();
        o.euler         = mekf_.euler();
        o.positionNED   = mekf_.position();
        o.velocityNED   = mekf_.velocity();
        o.speed         = mekf_.speed();
        o.course        = mekf_.course();
        o.verticalSpeed = mekf_.verticalSpeed();
        o.teVario       = mekf_.teVario();
        o.loadFactor    = mekf_.loadFactor();

        float yawRateRad = mekf_.yawRate();
        o.turnRate = yawRateRad * (180.0f / 3.14159265358979f);

        // Netto variometer
        if (cfg_.polar.valid() && hasTAS_) {
            float polarSink = cfg_.polar.sinkRate(lastTAS_);
            o.nettoVario = o.teVario + polarSink;
        } else {
            o.nettoVario = o.teVario;
        }

        // Wind
        o.wind      = wind_;
        o.windValid = windValid_;
        if (windValid_) {
            o.windSpeed   = std::sqrt(wind_.x * wind_.x + wind_.y * wind_.y);
            o.windBearing = std::atan2(-wind_.y, -wind_.x);
            if (o.windBearing < 0) o.windBearing += 2.0f * 3.14159265358979f;
        }

        o.gyroBias     = mekf_.gyroBias();
        o.accelBias    = mekf_.accelBias();
        o.gpsAvailable = gpsAvailable_;

        if (hasGpsRef_) {
            double lat, lon;
            float alt;
            nedToGps(mekf_.position(), lat, lon, alt);
            o.latitude  = lat;
            o.longitude = lon;
            o.altitude  = alt;
        } else if (hasBaroRef_) {
            o.altitude = baroRef_ - mekf_.position().z;
        }
        return o;
    }

    const ImuSample& lastImuSample() const { return lastSample_; }
    bool isCalibrating() const { return calibrating_; }

private:
    QMI8658A   imu_;
    MEKF       mekf_;
    Config     cfg_;
    ImuSample  lastSample_;

    bool hasMounting_ = false;

    // Calibration
    bool calibrating_ = true;
    int  calibCount_  = 0;
    Vec3 calibAccel_, calibGyro_;

    // GPS reference (WGS-84)
    double refLat_ = 0, refLon_ = 0;
    float  refAlt_ = 0;
    bool   hasGpsRef_    = false;
    bool   gpsAvailable_ = false;
    double rMeridional_  = 6335439.0;
    double rPrimeVert_   = 6378137.0;

    // Baro reference (independent of GPS)
    float baroRef_    = 0.0f;
    bool  hasBaroRef_ = false;

    // Airspeed
    float lastTAS_ = 0.0f;
    bool  hasTAS_  = false;

    // Wind estimation (simple complementary filter on GPS_vel - TAS*heading)
    Vec3  wind_;
    bool  windValid_ = false;
    static constexpr float WIND_ALPHA = 0.05f;

    // Timestamp handling
    uint32_t lastTs_  = 0;
    bool     firstTs_ = true;

    // ---- Mounting rotation -------------------------------------------------

    static bool isIdentity(const Quaternion& q) {
        return std::fabs(q.w - 1.0f) < 1e-6f &&
               std::fabs(q.x) < 1e-6f &&
               std::fabs(q.y) < 1e-6f &&
               std::fabs(q.z) < 1e-6f;
    }

    void applyMounting(ImuSample& s) const {
        if (!hasMounting_) return;
        s.accel = cfg_.mountingRotation.rotate(s.accel);
        s.gyro  = cfg_.mountingRotation.rotate(s.gyro);
    }

    // ---- Wind estimation ---------------------------------------------------

    void updateWind(const Vec3& gpsVelNED) {
        if (!hasTAS_ || lastTAS_ < 5.0f) return;

        // Airspeed vector in nav frame = TAS along body x rotated to nav
        Vec3 bodyX(1, 0, 0);
        Vec3 airVec = mekf_.attitude().rotate(bodyX) * lastTAS_;

        // wind = ground_velocity - air_velocity
        Vec3 sample = gpsVelNED - airVec;

        if (!windValid_) {
            wind_ = sample;
            windValid_ = true;
        } else {
            wind_ = wind_ * (1.0f - WIND_ALPHA) + sample * WIND_ALPHA;
        }
    }

    // ---- Calibration -------------------------------------------------------

    void finishCalibration() {
        float invN = 1.0f / static_cast<float>(calibCount_);
        Vec3 meanAccel = calibAccel_ * invN;
        Vec3 meanGyro  = calibGyro_  * invN;

        mekf_.initAttitudeFromAccel(meanAccel);
        mekf_.initGyroBias(meanGyro);
        calibrating_ = false;
    }

    // ---- dt from 24-bit sensor timestamp -----------------------------------

    float computeDt(uint32_t ts) {
        if (firstTs_) {
            firstTs_ = false;
            lastTs_  = ts;
            return 1.0f / imu_.odrHz();
        }
        uint32_t diff = (ts >= lastTs_)
                      ? (ts - lastTs_)
                      : (0x1000000u - lastTs_ + ts);
        lastTs_ = ts;
        return static_cast<float>(diff) / imu_.odrHz();
    }

    // ---- WGS-84 <-> local NED ----------------------------------------------

    static constexpr double WGS84_A  = 6378137.0;
    static constexpr double WGS84_E2 = 0.00669437999014;
    static constexpr double DEG2RAD  = 3.14159265358979323846 / 180.0;

    void computeRadii(double latDeg) {
        double sinLat = std::sin(latDeg * DEG2RAD);
        double s2 = sinLat * sinLat;
        double denom = std::sqrt(1.0 - WGS84_E2 * s2);
        rPrimeVert_  = WGS84_A / denom;
        rMeridional_ = WGS84_A * (1.0 - WGS84_E2) / (denom * denom * denom);
    }

    Vec3 gpsToNED(double lat, double lon, float alt) const {
        double dLat = (lat - refLat_) * DEG2RAD;
        double dLon = (lon - refLon_) * DEG2RAD;
        float n = static_cast<float>(dLat * (rMeridional_ + refAlt_));
        float e = static_cast<float>(dLon * (rPrimeVert_  + refAlt_)
                                     * std::cos(refLat_ * DEG2RAD));
        float d = -(alt - refAlt_);
        return {n, e, d};
    }

    void nedToGps(const Vec3& ned, double& lat, double& lon, float& alt) const {
        lat = refLat_ + (ned.x / (rMeridional_ + refAlt_)) / DEG2RAD;
        lon = refLon_ + (ned.y / ((rPrimeVert_ + refAlt_)
                                  * std::cos(refLat_ * DEG2RAD))) / DEG2RAD;
        alt = refAlt_ - ned.z;
    }
};

} // namespace sf
