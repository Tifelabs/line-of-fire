#include "render/Stroke.hpp"

#include <algorithm>
#include <cmath>

namespace lof::render {
namespace {

constexpr float kTwoPi = 6.2831853f;

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

void pencil_line(std::vector<Pt>& out, Pt a, Pt b, std::uint32_t seed, float wobble) {
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
        const float off = noise(seed, static_cast<std::uint32_t>(i)) * wobble;
        out.push_back({s.x + (e.x - s.x) * t + px * off, s.y + (e.y - s.y) * t + py * off});
    }
}

void pencil_circle(std::vector<Pt>& out, Pt center, float radius, std::uint32_t seed, float wobble,
                   int segments) {
    const int n = std::max(8, segments);
    const int total = static_cast<int>(static_cast<float>(n) * 1.08f);
    const float start = noise(seed, 999u) * 3.1415927f;
    for (int i = 0; i <= total; ++i) {
        const float turn = static_cast<float>(i) / static_cast<float>(n);
        const float ang = start + turn * kTwoPi;
        const float r = radius + noise(seed, static_cast<std::uint32_t>(i)) * wobble + radius * 0.04f * turn;
        out.push_back({center.x + std::cos(ang) * r, center.y + std::sin(ang) * r});
    }
}

void hatch_convex(std::vector<Seg>& out, const std::vector<Pt>& poly, float angle_rad, float gap) {
    if (poly.size() < 3 || gap <= 0.0f) {
        return;
    }
    const float ux = std::cos(angle_rad);
    const float uy = std::sin(angle_rad);
    const float vx = -uy;
    const float vy = ux;

    float dmin = 1e30f;
    float dmax = -1e30f;
    for (const Pt& p : poly) {
        const float d = p.x * vx + p.y * vy;
        dmin = std::min(dmin, d);
        dmax = std::max(dmax, d);
    }

    for (float d = dmin + gap * 0.5f; d < dmax; d += gap) {
        float tmin = 1e30f;
        float tmax = -1e30f;
        int hits = 0;
        for (std::size_t i = 0; i < poly.size(); ++i) {
            const Pt a = poly[i];
            const Pt b = poly[(i + 1) % poly.size()];
            const float da = a.x * vx + a.y * vy;
            const float db = b.x * vx + b.y * vy;
            const float denom = db - da;
            if (std::fabs(denom) < 1e-9f) {
                continue;
            }
            const float s = (d - da) / denom;
            if (s < 0.0f || s > 1.0f) {
                continue;
            }
            const float hx = a.x + (b.x - a.x) * s;
            const float hy = a.y + (b.y - a.y) * s;
            const float t = hx * ux + hy * uy;
            tmin = std::min(tmin, t);
            tmax = std::max(tmax, t);
            ++hits;
        }
        if (hits >= 2 && tmax - tmin > 1e-4f) {
            out.push_back({{vx * d + ux * tmin, vy * d + uy * tmin},
                           {vx * d + ux * tmax, vy * d + uy * tmax}});
        }
    }
}

}  // namespace lof::render
