#pragma once
#include <cmath>

namespace sf {

struct Vec3 {
    float x = 0.0f, y = 0.0f, z = 0.0f;

    Vec3() = default;
    Vec3(float x, float y, float z) : x(x), y(y), z(z) {}

    Vec3 operator+(const Vec3& b) const { return {x + b.x, y + b.y, z + b.z}; }
    Vec3 operator-(const Vec3& b) const { return {x - b.x, y - b.y, z - b.z}; }
    Vec3 operator*(float s) const { return {x * s, y * s, z * s}; }
    Vec3 operator/(float s) const { float inv = 1.0f / s; return {x * inv, y * inv, z * inv}; }
    Vec3 operator-() const { return {-x, -y, -z}; }

    Vec3& operator+=(const Vec3& b) { x += b.x; y += b.y; z += b.z; return *this; }
    Vec3& operator-=(const Vec3& b) { x -= b.x; y -= b.y; z -= b.z; return *this; }
    Vec3& operator*=(float s) { x *= s; y *= s; z *= s; return *this; }

    float dot(const Vec3& b) const { return x * b.x + y * b.y + z * b.z; }

    Vec3 cross(const Vec3& b) const {
        return {y * b.z - z * b.y,
                z * b.x - x * b.z,
                x * b.y - y * b.x};
    }

    float norm() const { return std::sqrt(x * x + y * y + z * z); }
    float normSq() const { return x * x + y * y + z * z; }

    Vec3 normalized() const {
        float n = norm();
        return n > 1e-10f ? *this / n : Vec3{};
    }
};

inline Vec3 operator*(float s, const Vec3& v) { return v * s; }

} // namespace sf
