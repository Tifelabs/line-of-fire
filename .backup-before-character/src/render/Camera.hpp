#pragma once
#include "render/Common.hpp"
#include "sim/World.hpp"

namespace lof::render {

// Shared dynamic camera: frames every player, zooms out as they spread,
// with a hard zoom limit so the pencil lines stay crisp.
struct Camera {
    float cx = 15.0f;  // world units
    float cy = 10.0f;
    float zoom = 30.0f;  // pixels per world unit

    void update(const sim::World& world, float view_w, float view_h);
    Pt to_screen(Pt world_pt, float view_w, float view_h) const;
};

}  // namespace lof::render
