#pragma once
#include "geometry/Fixed.hpp"

namespace lof::render {

struct Pt {
    float x;
    float y;
};

// Render-only conversion. Floats must never flow back into the simulation.
inline float to_float(Fixed f) {
    return static_cast<float>(f.raw) / static_cast<float>(Fixed::kOne);
}

}  // namespace lof::render
