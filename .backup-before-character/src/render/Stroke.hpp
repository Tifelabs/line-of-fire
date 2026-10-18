#pragma once
#include <cstdint>
#include <vector>

#include "render/Common.hpp"

namespace lof::render {

// Appends a hand-drawn looking polyline from a to b. Same seed = same wobble,
// so the sketch never flickers and looks identical for every networked player.
// wobble_px is the maximum sideways offset in pixels.
void pencil_line(std::vector<Pt>& out, Pt a, Pt b, std::uint32_t seed, float wobble_px);

}  // namespace lof::render
