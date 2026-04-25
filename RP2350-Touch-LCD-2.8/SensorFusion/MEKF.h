#pragma once
#include "Vec3.h"
#include "Quaternion.h"
#include "Mat.h"
#include <cmath>
#include <algorithm>

namespace sf {

// 15-state Multiplicative Extended Kalman Filter
//
// Nominal state:  q(4)  p(3)  v(3)  bg(3)  ba(3)
// Error   state:  dθ(3) dp(3) dv(3) dbg(3) dba(3)  = 15
//
// Designed for glider flight: adaptive gravity-update noise scaling,
// centripetal acceleration compensation for coordinated turns,
// airspeed fusion, total-energy variometer, and load-factor output.

class MEKF {
public:
    static constexpr float GRAVITY = 9.80665f;

    struct Config {
        // Sensor noise densities (per sqrt(Hz))
        float gyroNoiseDensity  = 2.27e-4f;   // rad/s/√Hz  (QMI8658A: 13 mdps)
        float accelNoiseDensity = 1.47e-3f;    // m/s²/√Hz   (QMI8658A: 150 µg)

        // Bias random-walk spectral densities
        float gyroBiasWalk  = 3e-5f;           // rad/s²/√Hz  (loosened for thermal turbulence)
        float accelBiasWalk = 3e-4f;           // m/s³/√Hz    (loosened — accel bias shifts in-flight)

        // Adaptive gravity update (replaces hard gate)
        // R_effective = R_base * (1 + adaptiveScale * excess²)
        // where excess = max(0, |a| - g - adaptiveDeadband)
        float adaptiveScale    = 20.0f;        // dimensionless  (how fast R inflates)
        float adaptiveDeadband = 0.3f;         // m/s²  (tolerate small g deviations without inflating)

        // Centripetal compensation: subtract omega × v (body) from accel before
        // gravity comparison.  Disable if the velocity estimate is poor.
        bool  centripetalCompensation = true;

        // Airspeed measurement noise floor (m/s)²
        float airspeedVarianceFloor = 1.0f;

        // Initial 1-sigma uncertainties
        float initialAttStd       = 0.5f;      // rad
        float initialPosStd       = 100.0f;    // m
        float initialVelStd       = 5.0f;      // m/s  (glider may be moving at init)
        float initialGyroBiasStd  = 0.17f;     // rad/s   (~10 dps)
        float initialAccelBiasStd = 0.98f;     // m/s²    (~100 mg)
    };

    MEKF() = default;

    void configure(const Config& cfg, float odrHz) {
        cfg_ = cfg;
        odr_ = odrHz;

        buildProcessNoise(1.0f / odrHz);
        buildAccelR();
        resetCovariance();
    }

    // ---- Nominal-state initialisation --------------------------------------

    void initAttitudeFromAccel(const Vec3& accel) {
        float ax = accel.x, ay = accel.y, az = accel.z;
        float pitch = std::atan2(-ax, std::sqrt(ay * ay + az * az));
        float roll  = std::atan2(ay, az);
        q_ = Quaternion::fromEuler(roll, pitch, 0.0f);
    }

    void initGyroBias(const Vec3& bias) { gyroBias_ = bias; }
    void initPosition(const Vec3& p)    { pos_ = p; }
    void initVelocity(const Vec3& v)    { vel_ = v; }

    // ---- Prediction (call every IMU sample) --------------------------------

    void predict(const Vec3& gyroRaw, const Vec3& accelRaw, float dt) {
        Vec3 omega = gyroRaw - gyroBias_;
        Vec3 ab    = accelRaw - accelBias_;

        q_ = q_ * Quaternion::fromAngularVelocity(omega, dt);
        q_.normalize();

        Vec3 aNav = q_.rotate(ab);
        aNav.z -= GRAVITY;

        pos_ += vel_ * dt + aNav * (0.5f * dt * dt);
        vel_ += aNav * dt;

        // Discrete F = I + Fc·dt
        Mat15 F = Mat15::identity();

        Mat3 skewOm = Mat<3,3>::skew(omega.x, omega.y, omega.z);
        F.setBlock(0, 0, Mat3::identity() - skewOm * dt);
        F.setBlock(0, 9, Mat3::identity() * (-dt));
        F.setBlock(3, 6, Mat3::identity() * dt);

        float Rbuf[9];
        q_.toRotationMatrix(Rbuf);
        Mat3 Rmat;
        for (int i = 0; i < 3; i++)
            for (int j = 0; j < 3; j++)
                Rmat(i, j) = Rbuf[i * 3 + j];

        Mat3 skewAb = Mat<3,3>::skew(ab.x, ab.y, ab.z);
        F.setBlock(6, 0,  (Rmat * skewAb) * (-dt));
        F.setBlock(6, 12, Rmat * (-dt));

        P_ = F * P_ * F.transpose() + Q_;
        P_.symmetrize();

        lastOmega_ = omega;
        lastAccelRaw_ = accelRaw;
    }

    // ---- Accelerometer gravity-reference update (adaptive R) ---------------

    void updateGravity(const Vec3& accelRaw) {
        Vec3 gNav(0.0f, 0.0f, GRAVITY);
        Vec3 gExpected = q_.rotateInverse(gNav);

        Vec3 accelCorrected = accelRaw;
        if (cfg_.centripetalCompensation) {
            // Centripetal accel in body frame: omega × v_body
            Vec3 vBody = q_.rotateInverse(vel_);
            Vec3 centripetal = lastOmega_.cross(vBody);
            accelCorrected = accelCorrected - centripetal;
        }

        Vec3 innov = accelCorrected - gExpected - accelBias_;

        // Adaptive R: inflate noise based on corrected (not raw) magnitude.
        // After centripetal subtraction a coordinated turn reads ~1g, so R
        // stays at baseline and the gravity update retains full authority.
        // Only genuinely unexplained dynamics (turbulence, abrupt inputs)
        // inflate R.
        float aMag = accelCorrected.norm();
        float excess = std::max(0.0f, std::fabs(aMag - GRAVITY) - cfg_.adaptiveDeadband);
        float scale = 1.0f + cfg_.adaptiveScale * excess * excess;
        Mat3 R = Raccel_ * scale;

        Mat<3, 15> H;
        H.setBlock(0, 0, Mat<3,3>::skew(gExpected.x, gExpected.y, gExpected.z));
        H(0, 12) = 1.0f; H(1, 13) = 1.0f; H(2, 14) = 1.0f;

        kalmanUpdate3(H, innov, R);
    }

    // Prediction + adaptive gravity update in one call.
    void processImu(const Vec3& gyroRaw, const Vec3& accelRaw, float dt) {
        predict(gyroRaw, accelRaw, dt);
        updateGravity(accelRaw);
    }

    // ---- GPS position+velocity update --------------------------------------

    void updateGps(const Vec3& posNED, const Vec3& velNED,
                   const Vec3& posVariance, const Vec3& velVariance) {
        Mat<6, 15> H;
        H(0, 3) = 1; H(1, 4) = 1; H(2, 5) = 1;
        H(3, 6) = 1; H(4, 7) = 1; H(5, 8) = 1;

        Mat<6, 1> z;
        Vec3 dP = posNED - pos_;
        Vec3 dV = velNED - vel_;
        z(0, 0) = dP.x; z(1, 0) = dP.y; z(2, 0) = dP.z;
        z(3, 0) = dV.x; z(4, 0) = dV.y; z(5, 0) = dV.z;

        Mat6 R;
        R(0, 0) = posVariance.x; R(1, 1) = posVariance.y; R(2, 2) = posVariance.z;
        R(3, 3) = velVariance.x; R(4, 4) = velVariance.y; R(5, 5) = velVariance.z;

        Mat<6, 6> S = H * P_ * H.transpose() + R;
        Mat<15, 6> K = P_ * H.transpose() * S.inverse();
        Mat<15, 1> dx = K * z;

        applyCorrection(dx);
        P_ = (Mat15::identity() - K * H) * P_;
        P_.symmetrize();
    }

    // GPS position-only update (no velocity available)
    void updateGpsPosition(const Vec3& posNED, const Vec3& posVariance) {
        Mat<3, 15> H;
        H(0, 3) = 1; H(1, 4) = 1; H(2, 5) = 1;

        Vec3 innov = posNED - pos_;
        Mat3 R;
        R(0, 0) = posVariance.x; R(1, 1) = posVariance.y; R(2, 2) = posVariance.z;

        kalmanUpdate3(H, innov, R);
    }

    // ---- Airspeed update ---------------------------------------------------
    // IAS is converted to TAS, then compared against the body-frame x component
    // of the nav-frame velocity.  H is a single row mapping to dv states
    // through the rotation matrix.

    void updateAirspeed(float ias, float variance, float pressureAltM) {
        float tas = iasToTas(ias, pressureAltM);
        if (tas < 5.0f) return;  // too slow for a meaningful heading constraint

        // Predicted TAS: body-x component of velocity = first row of R^T * v
        Vec3 vBody = q_.rotateInverse(vel_);
        float predicted = vBody.x;
        float innov = tas - predicted;

        // H = dh/dx.  h = e_x^T · R(q)^T · v
        // dh/dv = e_x^T · R^T  (row 0 of R^T = column 0 of R)
        float Rbuf[9];
        q_.toRotationMatrix(Rbuf);

        // dh/dθ via perturbation:  -e_x^T · R^T · [v×]
        //   = -[R^T·v]× row x  but simpler to express as cross product
        // d(R^T·v)/dθ = -[R^T·v]×  so dh/dθ = -e_x^T · [R^T·v]×
        //   = -( (R^T·v) × e_x )^T  ... expand:
        //   dh/dθ = [0, vBody.z, -vBody.y]
        Mat<1, 15> H;
        H(0, 0) =  0.0f;
        H(0, 1) =  vBody.z;
        H(0, 2) = -vBody.y;
        H(0, 6) = Rbuf[0];   // R(0,0)
        H(0, 7) = Rbuf[3];   // R(1,0)
        H(0, 8) = Rbuf[6];   // R(2,0)

        float R_as = std::max(variance, cfg_.airspeedVarianceFloor);

        // Scalar Kalman update
        float S = 0.0f;
        for (int j = 0; j < 15; j++) {
            float hpj = 0.0f;
            for (int k = 0; k < 15; k++)
                hpj += H(0, k) * P_(k, j);
            S += hpj * H(0, j);
        }
        S += R_as;
        if (std::fabs(S) < 1e-12f) return;
        float Sinv = 1.0f / S;

        // K = P · H^T / S
        Mat<15, 1> K;
        for (int i = 0; i < 15; i++) {
            float ph = 0.0f;
            for (int k = 0; k < 15; k++)
                ph += P_(i, k) * H(0, k);
            K(i, 0) = ph * Sinv;
        }

        Mat<15, 1> dx;
        for (int i = 0; i < 15; i++)
            dx(i, 0) = K(i, 0) * innov;

        applyCorrection(dx);

        // P = (I - K·H)·P
        // cache H·P row
        float hp[15];
        for (int j = 0; j < 15; j++) {
            hp[j] = 0.0f;
            for (int k = 0; k < 15; k++)
                hp[j] += H(0, k) * P_(k, j);
        }
        for (int i = 0; i < 15; i++)
            for (int j = 0; j < 15; j++)
                P_(i, j) -= K(i, 0) * hp[j];

        P_.symmetrize();
    }

    // ---- Barometric altitude update (scalar, maps to pos_z) ----------------

    void updateBaroAltitude(float altDown, float variance) {
        float innov = altDown - pos_.z;
        float S = P_(5, 5) + variance;
        if (S < 1e-12f) return;

        float Sinv = 1.0f / S;
        Mat<15, 1> K;
        for (int i = 0; i < 15; i++)
            K(i, 0) = P_(i, 5) * Sinv;

        Mat<15, 1> dx;
        for (int i = 0; i < 15; i++)
            dx(i, 0) = K(i, 0) * innov;

        applyCorrection(dx);

        float col5[15];
        for (int i = 0; i < 15; i++) col5[i] = P_(5, i);
        for (int i = 0; i < 15; i++)
            for (int j = 0; j < 15; j++)
                P_(i, j) -= K(i, 0) * col5[j];

        P_.symmetrize();
    }

    // ---- Zero-velocity update (stationary detection) -----------------------

    void updateZupt(float variance = 0.01f) {
        Mat<3, 15> H;
        H(0, 6) = 1; H(1, 7) = 1; H(2, 8) = 1;

        Vec3 innov = -vel_;
        Mat3 R;
        R(0, 0) = R(1, 1) = R(2, 2) = variance;

        kalmanUpdate3(H, innov, R);
    }

    // ---- Accessors ---------------------------------------------------------

    const Quaternion& attitude() const { return q_; }
    Vec3 euler() const { return q_.toEuler(); }
    const Vec3& position() const { return pos_; }
    const Vec3& velocity() const { return vel_; }
    const Vec3& gyroBias() const { return gyroBias_; }
    const Vec3& accelBias() const { return accelBias_; }
    const Mat15& covariance() const { return P_; }

    float speed() const { return std::sqrt(vel_.x * vel_.x + vel_.y * vel_.y); }
    float course() const { return std::atan2(vel_.y, vel_.x); }
    float verticalSpeed() const { return -vel_.z; }

    float loadFactor() const { return lastAccelRaw_.norm() / GRAVITY; }

    // Bias-corrected yaw rate projected onto the nav-frame down axis (rad/s).
    // Positive = turning right (clockwise seen from above).
    float yawRate() const {
        Vec3 omegaNav = q_.rotate(lastOmega_);
        return omegaNav.z;
    }

    // Total-energy compensated variometer (m/s, positive = up).
    // TE_vario = climb_rate + V·V_dot / g
    // V_dot is estimated from the body-x acceleration minus gravity component.
    float teVario() const {
        float spd = std::sqrt(vel_.x * vel_.x + vel_.y * vel_.y + vel_.z * vel_.z);
        if (spd < 2.0f) return verticalSpeed();
        Vec3 vUnit = vel_ / spd;
        Vec3 aNav = q_.rotate(lastAccelRaw_ - accelBias_);
        aNav.z -= GRAVITY;
        float vDot = aNav.dot(vUnit);
        return verticalSpeed() + (spd * vDot) / GRAVITY;
    }

private:
    Quaternion q_;
    Vec3 pos_, vel_;
    Vec3 gyroBias_, accelBias_;

    Mat15 P_, Q_;
    Mat3  Raccel_;
    Config cfg_;
    float odr_ = 448.4f;

    Vec3 lastOmega_;
    Vec3 lastAccelRaw_;

    // ---- Internal helpers --------------------------------------------------

    void buildProcessNoise(float dt) {
        float sg = cfg_.gyroNoiseDensity  * std::sqrt(odr_);
        float sa = cfg_.accelNoiseDensity * std::sqrt(odr_);

        float qAtt = sg * sg * dt;
        float qVel = sa * sa * dt;
        float qBg  = cfg_.gyroBiasWalk  * cfg_.gyroBiasWalk  * dt;
        float qBa  = cfg_.accelBiasWalk * cfg_.accelBiasWalk * dt;

        Q_ = Mat15::zeros();
        for (int i = 0; i < 3; i++) {
            Q_(i,      i)      = qAtt;
            Q_(3 + i,  3 + i)  = 1e-8f;
            Q_(6 + i,  6 + i)  = qVel;
            Q_(9 + i,  9 + i)  = qBg;
            Q_(12 + i, 12 + i) = qBa;
        }
    }

    void buildAccelR() {
        float sa = cfg_.accelNoiseDensity * std::sqrt(odr_);
        float r  = sa * sa;
        Raccel_ = Mat3::zeros();
        Raccel_(0, 0) = r; Raccel_(1, 1) = r; Raccel_(2, 2) = r;
    }

    void resetCovariance() {
        P_ = Mat15::zeros();
        auto sq = [](float v) { return v * v; };
        for (int i = 0; i < 3; i++) {
            P_(i,      i)      = sq(cfg_.initialAttStd);
            P_(3 + i,  3 + i)  = sq(cfg_.initialPosStd);
            P_(6 + i,  6 + i)  = sq(cfg_.initialVelStd);
            P_(9 + i,  9 + i)  = sq(cfg_.initialGyroBiasStd);
            P_(12 + i, 12 + i) = sq(cfg_.initialAccelBiasStd);
        }
    }

    void applyCorrection(const Mat<15, 1>& dx) {
        Vec3 dtheta(dx(0, 0), dx(1, 0), dx(2, 0));
        q_ = q_ * Quaternion::fromRotationVector(dtheta);
        q_.normalize();

        pos_.x += dx(3, 0);  pos_.y += dx(4, 0);  pos_.z += dx(5, 0);
        vel_.x += dx(6, 0);  vel_.y += dx(7, 0);  vel_.z += dx(8, 0);

        gyroBias_.x  += dx(9, 0);   gyroBias_.y  += dx(10, 0); gyroBias_.z  += dx(11, 0);
        accelBias_.x += dx(12, 0);  accelBias_.y += dx(13, 0); accelBias_.z += dx(14, 0);

        clampBiases();
    }

    void clampBiases() {
        constexpr float MAX_GB = 0.35f;   // ~20 dps
        constexpr float MAX_AB = 2.0f;    // m/s²
        auto cl = [](float v, float lim) { return std::clamp(v, -lim, lim); };
        gyroBias_.x  = cl(gyroBias_.x,  MAX_GB);
        gyroBias_.y  = cl(gyroBias_.y,  MAX_GB);
        gyroBias_.z  = cl(gyroBias_.z,  MAX_GB);
        accelBias_.x = cl(accelBias_.x, MAX_AB);
        accelBias_.y = cl(accelBias_.y, MAX_AB);
        accelBias_.z = cl(accelBias_.z, MAX_AB);
    }

    void kalmanUpdate3(const Mat<3, 15>& H, const Vec3& innov, const Mat3& R) {
        Mat3 S = H * P_ * H.transpose() + R;
        Mat<15, 3> K = P_ * H.transpose() * S.inverse();

        Mat<3, 1> z;
        z(0, 0) = innov.x; z(1, 0) = innov.y; z(2, 0) = innov.z;
        Mat<15, 1> dx = K * z;

        applyCorrection(dx);
        P_ = (Mat15::identity() - K * H) * P_;
        P_.symmetrize();
    }

public:
    static float iasToTas(float ias, float pressureAltM) {
        constexpr float T0 = 288.15f;
        constexpr float L  = 0.0065f;
        constexpr float RHO0 = 1.225f;
        float T = T0 - L * pressureAltM;
        if (T < 100.0f) T = 100.0f;
        float rhoRatio = std::pow(T / T0, 4.2559f);
        float rho = RHO0 * rhoRatio;
        if (rho < 0.05f) rho = 0.05f;
        return ias * std::sqrt(RHO0 / rho);
    }
};

} // namespace sf
