#pragma once

#include <cmath>

namespace Breakout {

struct Vec2 {
    float x = 0.0f;
    float y = 0.0f;

    constexpr Vec2() = default;
    constexpr Vec2(float px, float py) : x(px), y(py) {}

    constexpr Vec2 operator+(const Vec2& rhs) const { return {x + rhs.x, y + rhs.y}; }
    constexpr Vec2 operator-(const Vec2& rhs) const { return {x - rhs.x, y - rhs.y}; }
    constexpr Vec2 operator*(float scalar) const { return {x * scalar, y * scalar}; }
    constexpr Vec2 operator/(float scalar) const { return {x / scalar, y / scalar}; }

    Vec2& operator+=(const Vec2& rhs) {
        x += rhs.x;
        y += rhs.y;
        return *this;
    }

    Vec2& operator-=(const Vec2& rhs) {
        x -= rhs.x;
        y -= rhs.y;
        return *this;
    }

    Vec2& operator*=(float scalar) {
        x *= scalar;
        y *= scalar;
        return *this;
    }

    Vec2& operator/=(float scalar) {
        x /= scalar;
        y /= scalar;
        return *this;
    }

    [[nodiscard]] float lengthSquared() const { return x * x + y * y; }

    [[nodiscard]] float length() const { return std::sqrt(lengthSquared()); }

    [[nodiscard]] Vec2 normalized() const {
        float len = length();
        if (len > 0.00001f) {
            return {x / len, y / len};
        }
        return {0.0f, 0.0f};
    }

    [[nodiscard]] float dot(const Vec2& rhs) const { return x * rhs.x + y * rhs.y; }

    [[nodiscard]] float distanceTo(const Vec2& rhs) const { return (*this - rhs).length(); }

    [[nodiscard]] Vec2 reflected(const Vec2& normal) const {
        return *this - normal * (2.0f * this->dot(normal));
    }
};

inline Vec2 operator*(float scalar, const Vec2& v) {
    return {v.x * scalar, v.y * scalar};
}

} // namespace Breakout
