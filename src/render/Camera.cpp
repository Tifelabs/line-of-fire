#include "render/Camera.hpp"

#include <algorithm>

namespace lof::render {

void Camera::update(const sim::World& world, float view_w, float view_h) {
    float min_x = 1e9f;
    float max_x = -1e9f;
    float min_y = 1e9f;
    float max_y = -1e9f;
    for (const sim::Player& p : world.players) {
        const float x = to_float(p.pos.x);
        const float y = to_float(p.pos.y) - 1.0f;  // roughly body centre
        min_x = std::min(min_x, x);
        max_x = std::max(max_x, x);
        min_y = std::min(min_y, y);
        max_y = std::max(max_y, y);
    }

    constexpr float kMargin = 6.0f;  // world units of breathing room
    const float need_w = (max_x - min_x) + kMargin * 2.0f;
    const float need_h = (max_y - min_y) + kMargin * 2.0f;

    // Resolution independent zoom limits: between 10 and 40 units visible vertically.
    const float zoom_max = view_h / 10.0f;
    const float zoom_min = view_h / 40.0f;
    const float target_zoom = std::clamp(std::min(view_w / need_w, view_h / need_h), zoom_min, zoom_max);

    const float target_x = (min_x + max_x) * 0.5f;
    const float target_y = (min_y + max_y) * 0.5f;

    constexpr float kSmooth = 0.1f;
    cx += (target_x - cx) * kSmooth;
    cy += (target_y - cy) * kSmooth;
    zoom += (target_zoom - zoom) * kSmooth;
}

Pt Camera::to_screen(Pt p, float view_w, float view_h) const {
    return {(p.x - cx) * zoom + view_w * 0.5f, (p.y - cy) * zoom + view_h * 0.5f};
}

}  // namespace lof::render
