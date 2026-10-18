#include "render/Stroke.hpp"

#include <algorithm>
#include <cmath>

namespace lof::render {
namespace {

std::uint32_t hash32(std::uint32_t x) {
    x ^= x >> 16;
    x *= 0x7feb352dU;
    x ^= x >> 15;
    x *= 0x846ca68bU;
    x ^= x >> 16;
    return x;
}

// Deterministic noise in [-1, 1].
float noise(std::uint32_t seed, std::uint32_t i) {
    const std::uint32_t h = hash32(seed * 0x9E3779B9U + i);
    return (static_cast<float>(h & 0xFFFFu) / 32767.5f) - 1.0f;
}

}  // namespace

void pencil_line(std::vector<Pt>& out, Pt a, Pt b, std::uint32_t seed, float wobble_px) {
    const float dx = b.x - a.x;
    const float dy = b.y - a.y;
    const float len = std::sqrt(dx * dx + dy * dy);
    if (len < 1e-3f) {
        out.push_back(a);
        out.push_back(b);
        return;
    }

    const float ux = dx / len;
    const float uy = dy / len;
    const float px = -uy;  // perpendicular
    const float py = ux;

    // Slight overshoot at both ends, like a drafted line.
    const float over = len * 0.03f;
    const Pt s{a.x - ux * over, a.y - uy * over};
    const Pt e{b.x + ux * over, b.y + uy * over};

    const float full = len + 2.0f * over;
    const int n = std::max(2, static_cast<int>(full / 12.0f));
    for (int i = 0; i <= n; ++i) {
        const float t = static_cast<float>(i) / static_cast<float>(n);
        const float off = noise(seed, static_cast<std::uint32_t>(i)) * wobble_px;
        out.push_back({s.x + (e.x - s.x) * t + px * off, s.y + (e.y - s.y) * t + py * off});
    }
}

}  // namespace lof::render
