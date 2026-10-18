#pragma once
#include <cstdint>

#include "geometry/Fixed.hpp"

namespace lof::render {

struct Pt {
    float x;
    float y;
};

struct Color {
    std::uint8_t r;
    std::uint8_t g;
    std::uint8_t b;
};

inline Pt operator+(Pt a, Pt b) { return {a.x + b.x, a.y + b.y}; }
inline Pt operator-(Pt a, Pt b) { return {a.x - b.x, a.y - b.y}; }
inline Pt operator*(Pt a, float s) { return {a.x * s, a.y * s}; }

// Render-only conversion. Floats must never flow back into the simulation.
inline float to_float(Fixed f) {
    return static_cast<float>(f.raw) / static_cast<float>(Fixed::kOne);
}

}  // namespace lof::render
