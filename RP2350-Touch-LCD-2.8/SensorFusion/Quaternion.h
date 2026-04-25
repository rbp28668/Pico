#pragma once
#include "Vec3.h"
#include <cmath>
#include <algorithm>

namespace sf {

struct Quaternion {
    float w = 1.0f, x = 0.0f, y = 0.0f, z = 0.0f;

    Quaternion() = default;
    Quaternion(float w, float x, float y, float z) : w(w), x(x), y(y), z(z) {}

    static Quaternion identity() { return {1, 0, 0, 0}; }

    static Quaternion fromAxisAngle(const Vec3& axis, float angle) {
        float ha = angle * 0.5f;
        float s = std::sin(ha);
        return {std::cos(ha), axis.x * s, axis.y * s, axis.z * s};
    }

    static Quaternion fromRotationVector(const Vec3& rv) {
        float angle = rv.norm();
        if (angle < 1e-8f)
            return Quaternion(1.0f, rv.x * 0.5f, rv.y * 0.5f, rv.z * 0.5f).normalized();
        Vec3 axis = rv / angle;
        return fromAxisAngle(axis, angle);
    }

    static Quaternion fromAngularVelocity(const Vec3& omega, float dt) {
        float angle = omega.norm() * dt;
        if (angle < 1e-8f) {
            float ht = 0.5f * dt;
            return Quaternion(1.0f, omega.x * ht, omega.y * ht, omega.z * ht).normalized();
        }
        return fromAxisAngle(omega.normalized(), angle);
    }

    // ZYX (yaw-pitch-roll) Euler to quaternion
    static Quaternion fromEuler(float roll, float pitch, float yaw) {
        float cr = std::cos(roll * 0.5f),  sr = std::sin(roll * 0.5f);
        float cp = std::cos(pitch * 0.5f), sp = std::sin(pitch * 0.5f);
        float cy = std::cos(yaw * 0.5f),   sy = std::sin(yaw * 0.5f);
        return {
            cr * cp * cy + sr * sp * sy,
            sr * cp * cy - cr * sp * sy,
            cr * sp * cy + sr * cp * sy,
            cr * cp * sy - sr * sp * cy
        };
    }

    Quaternion operator*(const Quaternion& p) const {
        return {
            w * p.w - x * p.x - y * p.y - z * p.z,
            w * p.x + x * p.w + y * p.z - z * p.y,
            w * p.y - x * p.z + y * p.w + z * p.x,
            w * p.z + x * p.y - y * p.x + z * p.w
        };
    }

    Quaternion conjugate() const { return {w, -x, -y, -z}; }

    float norm() const { return std::sqrt(w * w + x * x + y * y + z * z); }

    Quaternion normalized() const {
        float n = norm();
        if (n < 1e-10f) return identity();
        float inv = 1.0f / n;
        return {w * inv, x * inv, y * inv, z * inv};
    }

    void normalize() {
        float n = norm();
        if (n < 1e-10f) { *this = identity(); return; }
        float inv = 1.0f / n;
        w *= inv; x *= inv; y *= inv; z *= inv;
    }

    // Rotate vector: v' = q * [0,v] * q*
    // Efficient form: t = 2 * cross(qv, v); result = v + w*t + cross(qv, t)
    Vec3 rotate(const Vec3& v) const {
        Vec3 qv(x, y, z);
        Vec3 t = 2.0f * qv.cross(v);
        return v + w * t + qv.cross(t);
    }

    Vec3 rotateInverse(const Vec3& v) const {
        return conjugate().rotate(v);
    }

    // ZYX Euler extraction: returns Vec3(roll, pitch, yaw) in radians
    Vec3 toEuler() const {
        float roll  = std::atan2(2.0f * (w * x + y * z), 1.0f - 2.0f * (x * x + y * y));
        float sinp  = 2.0f * (w * y - z * x);
        float pitch = std::asin(std::clamp(sinp, -1.0f, 1.0f));
        float yaw   = std::atan2(2.0f * (w * z + x * y), 1.0f - 2.0f * (y * y + z * z));
        return {roll, pitch, yaw};
    }

    // Body-to-nav rotation matrix stored row-major in float[9]
    void toRotationMatrix(float R[9]) const {
        float xx = x * x, yy = y * y, zz = z * z;
        float xy = x * y, xz = x * z, yz = y * z;
        float wx = w * x, wy = w * y, wz = w * z;

        R[0] = 1 - 2 * (yy + zz); R[1] = 2 * (xy - wz);     R[2] = 2 * (xz + wy);
        R[3] = 2 * (xy + wz);     R[4] = 1 - 2 * (xx + zz); R[5] = 2 * (yz - wx);
        R[6] = 2 * (xz - wy);     R[7] = 2 * (yz + wx);     R[8] = 1 - 2 * (xx + yy);
    }
};

} // namespace sf
