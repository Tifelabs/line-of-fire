#pragma once
#include <cstdint>
#include <vector>

#include "render/Common.hpp"

namespace lof::render {

struct Seg {
    Pt a;
    Pt b;
};

// Appends a hand-drawn looking polyline from a to b. Same seed = same wobble,
// so the sketch never flickers and looks identical for every networked player.
// wobble is the maximum sideways offset, in the same units as the points.
void pencil_line(std::vector<Pt>& out, Pt a, Pt b, std::uint32_t seed, float wobble);

// Appends a hand-drawn circle: slightly more than one full turn (the ends overlap
// like a real pencil circle) with a small radius drift.
void pencil_circle(std::vector<Pt>& out, Pt center, float radius, std::uint32_t seed, float wobble,
                   int segments = 24);

// Colored-pencil hatching inside a CONVEX polygon. Lines run at angle_rad,
// spaced `gap` apart.
void hatch_convex(std::vector<Seg>& out, const std::vector<Pt>& poly, float angle_rad, float gap);

}  // namespace lof::render
