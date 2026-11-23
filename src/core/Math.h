// Math.h - tiny fixed-size vector helpers used by the simulation.
#pragma once

#include <cmath>

namespace aa {

struct Vec2 {
    float x = 0.0f;
    float y = 0.0f;

    Vec2() = default;
    Vec2(float ix, float iy) : x(ix), y(iy) {}

    Vec2 operator+(const Vec2& o) const { return {x + o.x, y + o.y}; }
    Vec2 operator-(const Vec2& o) const { return {x - o.x, y - o.y}; }
    Vec2 operator*(float s) const { return {x * s, y * s}; }
    Vec2& operator+=(const Vec2& o) { x += o.x; y += o.y; return *this; }
    Vec2& operator-=(const Vec2& o) { x -= o.x; y -= o.y; return *this; }

    float lengthSq() const { return x * x + y * y; }
    float length() const { return std::sqrt(lengthSq()); }

    Vec2 normalized() const {
        float len = length();
        if (len < 1e-5f) return {0.0f, 0.0f};
        return {x / len, y / len};
    }
};

inline float distance(const Vec2& a, const Vec2& b) { return (a - b).length(); }
inline float distanceSq(const Vec2& a, const Vec2& b) { return (a - b).lengthSq(); }
inline float clampf(float v, float lo, float hi) { return v < lo ? lo : (v > hi ? hi : v); }

}  // namespace aa
