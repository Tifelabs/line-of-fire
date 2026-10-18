#include "render/Palette.hpp"

#include <array>
#include <cmath>

namespace lof::render {
namespace {

const std::array<PlayerStyle, 4> kStyles = {{
    {"Red", {205, 45, 45}, Hat::Triangle},
    {"Blue", {40, 95, 195}, Hat::Rectangle},
    {"Green", {45, 150, 75}, Hat::Semicircle},
    {"Amber", {225, 145, 20}, Hat::Trapezoid},
}};

}  // namespace

const PlayerStyle& player_style(std::size_t index) { return kStyles[index % kStyles.size()]; }

HatShape hat_shape(Hat hat) {
    HatShape s;
    switch (hat) {
        case Hat::Triangle:
            s.crown = {{-0.20f, 0.03f}, {0.20f, 0.03f}, {0.0f, -0.42f}};
            break;
        case Hat::Rectangle:
            s.crown = {{-0.17f, 0.0f}, {0.17f, 0.0f}, {0.17f, -0.36f}, {-0.17f, -0.36f}};
            s.has_brim = true;
            s.brim_a = {-0.30f, 0.0f};
            s.brim_b = {0.30f, 0.0f};
            break;
        case Hat::Semicircle: {
            constexpr int kSteps = 10;
            for (int i = 0; i <= kSteps; ++i) {
                const float a = 3.1415927f + 3.1415927f * static_cast<float>(i) / kSteps;
                s.crown.push_back({std::cos(a) * 0.26f, 0.03f + std::sin(a) * 0.26f});
            }
            break;
        }
        case Hat::Trapezoid:
            s.crown = {{-0.25f, 0.03f}, {0.25f, 0.03f}, {0.16f, -0.23f}, {-0.16f, -0.23f}};
            s.has_brim = true;
            s.brim_a = {-0.32f, 0.03f};
            s.brim_b = {0.32f, 0.03f};
            break;
    }
    return s;
}

}  // namespace lof::render
