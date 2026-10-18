#pragma once
#include <array>

#include "render/Common.hpp"

namespace lof::render {

// Trailing scarf: a chain of points following the neck. Render-only, so it can
// never affect the simulation or the network.
struct Scarf {
    static constexpr int kPoints = 6;
    static constexpr float kSegment = 0.16f;

    std::array<Pt, kPoints> p{};
    std::array<Pt, kPoints> prev{};
    Pt last_anchor{};
    bool ready = false;

    // anchor = neck position, dt in seconds.
    void update(Pt anchor, int facing, float dt);
};

}  // namespace lof::render
