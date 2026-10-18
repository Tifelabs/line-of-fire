#pragma once
#include "geometry/Fixed.hpp"

namespace lof {

struct Vec2 {
    Fixed x{};
    Fixed y{};
    bool operator==(const Vec2&) const = default;
};

constexpr Vec2 operator+(Vec2 a, Vec2 b) { return {a.x + b.x, a.y + b.y}; }
constexpr Vec2 operator-(Vec2 a, Vec2 b) { return {a.x - b.x, a.y - b.y}; }
constexpr Vec2 operator*(Vec2 v, Fixed s) { return {v.x * s, v.y * s}; }
constexpr Vec2& operator+=(Vec2& a, Vec2 b) {
    a.x += b.x;
    a.y += b.y;
    return a;
}
constexpr Vec2& operator-=(Vec2& a, Vec2 b) {
    a.x -= b.x;
    a.y -= b.y;
    return a;
}

constexpr Fixed dot(Vec2 a, Vec2 b) { return a.x * b.x + a.y * b.y; }
constexpr Fixed cross(Vec2 a, Vec2 b) { return a.x * b.y - a.y * b.x; }
constexpr Fixed length_sq(Vec2 v) { return dot(v, v); }

}  // namespace lof
