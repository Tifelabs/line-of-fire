#pragma once
#include <cstdint>

#include "render/Camera.hpp"
#include "render/Palette.hpp"
#include "render/Pose.hpp"
#include "render/Renderer.hpp"
#include "render/Scarf.hpp"

namespace lof::render {

// Draws one stick-man: scarf, limbs with pivot pins, circle head with colored
// hatching, geometric hat, and a face that follows the pose's expression.
void draw_character(Renderer& r, const View& view, const Pose& pose, const PlayerStyle& style,
                    const Scarf& scarf, std::uint32_t seed);

}  // namespace lof::render
