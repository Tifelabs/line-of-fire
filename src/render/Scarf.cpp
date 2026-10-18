#include "render/Scarf.hpp"

#include <algorithm>
#include <cmath>

namespace lof::render {

void Scarf::update(Pt anchor, int facing, float dt) {
    dt = std::clamp(dt, 0.001f, 1.0f / 30.0f);
    const float f = facing >= 0 ? 1.0f : -1.0f;

    if (!ready) {
        last_anchor = anchor;
    }

    // How fast the anchor is moving sideways decides the resting shape:
    // still = hangs down behind the neck, running = streams out horizontally.
    const float speed = std::fabs(anchor.x - last_anchor.x) / dt;
    last_anchor = anchor;
    const float k = std::clamp(speed / 12.0f, 0.0f, 1.0f);

    float rx = -f * (0.35f + 0.65f * k);
    float ry = 0.90f - 0.65f * k;
    const float rl = std::sqrt(rx * rx + ry * ry);
    rx /= rl;
    ry /= rl;

    if (!ready) {
        for (int i = 0; i < kPoints; ++i) {
            const Pt q{anchor.x + rx * kSegment * static_cast<float>(i),
                       anchor.y + ry * kSegment * static_cast<float>(i)};
            p[static_cast<std::size_t>(i)] = q;
            prev[static_cast<std::size_t>(i)] = q;
        }
        ready = true;
    }

    p[0] = anchor;
    prev[0] = anchor;

    const float pull = std::clamp(6.0f * dt, 0.0f, 1.0f);
    for (std::size_t i = 1; i < p.size(); ++i) {
        const Pt vel = (p[i] - prev[i]) * 0.96f;
        prev[i] = p[i];
        p[i] = p[i] + vel + Pt{0.0f, 5.0f * dt * dt};
        const Pt target = p[i - 1] + Pt{rx, ry} * kSegment;
        p[i] = p[i] + (target - p[i]) * pull;
    }

    for (int iter = 0; iter < 3; ++iter) {
        for (std::size_t i = 1; i < p.size(); ++i) {
            const Pt d = p[i] - p[i - 1];
            const float len = std::sqrt(d.x * d.x + d.y * d.y);
            if (len > 1e-6f) {
                p[i] = p[i - 1] + d * (kSegment / len);
            }
        }
    }
}

}  // namespace lof::render
