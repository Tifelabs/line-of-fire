#pragma once
#include <cstddef>
#include <vector>

#include "render/Common.hpp"

namespace lof::render {

// Hats are basic geometric shapes so players differ by silhouette, not only colour.
enum class Hat { Triangle, Rectangle, Semicircle, Trapezoid };

struct PlayerStyle {
    const char* name;
    Color color;  // colored pencil
    Hat hat;
};

// Index wraps, so any player index is safe.
const PlayerStyle& player_style(std::size_t index);

struct HatShape {
    std::vector<Pt> crown;  // convex polygon, relative to the top-centre of the head
    bool has_brim = false;
    Pt brim_a{0.0f, 0.0f};
    Pt brim_b{0.0f, 0.0f};
};

// Positive y is DOWN (screen style), so the hat sits at negative y.
HatShape hat_shape(Hat hat);

}  // namespace lof::render
