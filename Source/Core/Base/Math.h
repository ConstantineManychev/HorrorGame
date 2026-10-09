#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace hg
{
    inline constexpr float kPi = 3.14159265358979323846f;

    inline float lerp(float aFrom, float aTo, float aT)
    {
        return aFrom + (aTo - aFrom) * aT;
    }

    inline float clamp01(float aValue)
    {
        return std::clamp(aValue, 0.0f, 1.0f);
    }

    inline float degreesToRadians(float aDegrees)
    {
        return aDegrees * kPi / 180.0f;
    }

    inline float radiansToDegrees(float aRadians)
    {
        return aRadians * 180.0f / kPi;
    }

    struct Vec2
    {
        float x = 0.0f;
        float y = 0.0f;

        constexpr Vec2() = default;

        constexpr Vec2(float aX, float aY)
            : x(aX)
            , y(aY)
        {
        }

        Vec2 operator+(const Vec2& aOther) const
        {
            return {x + aOther.x, y + aOther.y};
        }

        Vec2 operator-(const Vec2& aOther) const
        {
            return {x - aOther.x, y - aOther.y};
        }

        Vec2 operator-() const
        {
            return {-x, -y};
        }

        Vec2 operator*(float aScale) const
        {
            return {x * aScale, y * aScale};
        }

        Vec2 operator/(float aScale) const
        {
            return {x / aScale, y / aScale};
        }

        Vec2& operator+=(const Vec2& aOther)
        {
            x += aOther.x;
            y += aOther.y;
            return *this;
        }

        Vec2& operator-=(const Vec2& aOther)
        {
            x -= aOther.x;
            y -= aOther.y;
            return *this;
        }

        Vec2& operator*=(float aScale)
        {
            x *= aScale;
            y *= aScale;
            return *this;
        }

        bool operator==(const Vec2& aOther) const = default;

        float dot(const Vec2& aOther) const
        {
            return x * aOther.x + y * aOther.y;
        }

        float cross(const Vec2& aOther) const
        {
            return x * aOther.y - y * aOther.x;
        }

        float lengthSquared() const
        {
            return x * x + y * y;
        }

        float length() const
        {
            return std::sqrt(lengthSquared());
        }

        float distance(const Vec2& aOther) const
        {
            return (*this - aOther).length();
        }

        Vec2 normalized() const
        {
            const float len = length();
            return len > 0.0f ? Vec2{x / len, y / len} : Vec2{};
        }

        Vec2 rotated(float aRadians) const
        {
            const float c = std::cos(aRadians);
            const float s = std::sin(aRadians);
            return {x * c - y * s, x * s + y * c};
        }

        float angle() const
        {
            return std::atan2(y, x);
        }

        static Vec2 lerp(const Vec2& aFrom, const Vec2& aTo, float aT)
        {
            return {hg::lerp(aFrom.x, aTo.x, aT), hg::lerp(aFrom.y, aTo.y, aT)};
        }
    };

    inline Vec2 operator*(float aScale, const Vec2& aVector)
    {
        return aVector * aScale;
    }

    struct Color
    {
        uint8_t r = 255;
        uint8_t g = 255;
        uint8_t b = 255;
        uint8_t a = 255;

        bool operator==(const Color& aOther) const = default;

        static Color lerp(const Color& aFrom, const Color& aTo, float aT)
        {
            auto mix = [aT](uint8_t aA, uint8_t aB)
            {
                return static_cast<uint8_t>(std::lround(std::clamp(hg::lerp(aA, aB, aT), 0.0f, 255.0f)));
            };
            return {mix(aFrom.r, aTo.r), mix(aFrom.g, aTo.g), mix(aFrom.b, aTo.b), mix(aFrom.a, aTo.a)};
        }
    };

    struct Rect
    {
        Vec2 origin;
        Vec2 size;

        float minX() const
        {
            return origin.x;
        }

        float minY() const
        {
            return origin.y;
        }

        float maxX() const
        {
            return origin.x + size.x;
        }

        float maxY() const
        {
            return origin.y + size.y;
        }

        Vec2 center() const
        {
            return origin + size * 0.5f;
        }

        bool contains(const Vec2& aPoint) const
        {
            return aPoint.x >= minX() && aPoint.x <= maxX() && aPoint.y >= minY() && aPoint.y <= maxY();
        }

        bool intersects(const Rect& aOther) const
        {
            return minX() < aOther.maxX() && maxX() > aOther.minX() && minY() < aOther.maxY() && maxY() > aOther.minY();
        }

        static Rect fromCenter(const Vec2& aCenter, const Vec2& aSize)
        {
            return {aCenter - aSize * 0.5f, aSize};
        }

        static Rect fromCorners(const Vec2& aA, const Vec2& aB)
        {
            const Vec2 low{std::min(aA.x, aB.x), std::min(aA.y, aB.y)};
            const Vec2 high{std::max(aA.x, aB.x), std::max(aA.y, aB.y)};
            return {low, high - low};
        }
    };
}
