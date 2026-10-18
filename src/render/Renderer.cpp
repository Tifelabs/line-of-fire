#include "render/Renderer.hpp"

#include <algorithm>
#include <cmath>

#include "render/Stroke.hpp"

namespace lof::render {

Renderer::Renderer(SDL_Renderer* renderer) : r_(renderer) {
    SDL_SetRenderDrawBlendMode(r_, SDL_BLENDMODE_BLEND);
}

void Renderer::begin_frame() {
    SDL_SetRenderDrawColor(r_, 245, 240, 225, 255);
    SDL_RenderClear(r_);
}

void Renderer::end_frame() { SDL_RenderPresent(r_); }

void Renderer::view_size(float& w, float& h) const {
    int iw = 0;
    int ih = 0;
    SDL_GetRenderOutputSize(r_, &iw, &ih);
    w = static_cast<float>(iw);
    h = static_cast<float>(ih);
}

void Renderer::stroke(Pt a, Pt b, std::uint32_t seed) { stroke(a, b, seed, Pen{}); }

void Renderer::stroke(Pt a, Pt b, std::uint32_t seed, const Pen& pen) {
    draw_pass(a, b, seed, pen.color, pen.weight * scale_, pen.wobble * scale_, pen.alpha);
    draw_pass(a, b, seed ^ 0x5BD1E995u, pen.color, pen.weight * 0.6f * scale_,
              pen.wobble * 1.9f * scale_, static_cast<std::uint8_t>(pen.alpha * 0.4f));
}

void Renderer::path(const std::vector<Pt>& pts, std::uint32_t seed, const Pen& pen) {
    for (std::size_t i = 1; i < pts.size(); ++i) {
        stroke(pts[i - 1], pts[i], seed + static_cast<std::uint32_t>(i) * 7u, pen);
    }
}

void Renderer::polyline(const std::vector<Pt>& pts, const Pen& pen) {
    draw_points(pts, pen.color, pen.weight * scale_, pen.alpha);
}

void Renderer::dot(Pt c, float radius_px, Color color, std::uint8_t alpha) {
    const float rad = std::max(radius_px, 1.2f * scale_);
    SDL_SetRenderDrawColor(r_, color.r, color.g, color.b, alpha);
    const int ir = static_cast<int>(std::ceil(rad));
    for (int dy = -ir; dy <= ir; ++dy) {
        const float fy = static_cast<float>(dy);
        const float w = std::sqrt(std::max(0.0f, rad * rad - fy * fy));
        SDL_RenderLine(r_, c.x - w, c.y + fy, c.x + w, c.y + fy);
    }
}

void Renderer::draw_pass(Pt a, Pt b, std::uint32_t seed, Color c, float weight_px, float wobble_px,
                         std::uint8_t alpha) {
    std::vector<Pt> pts;
    pencil_line(pts, a, b, seed, wobble_px);
    draw_points(pts, c, weight_px, alpha);
}

void Renderer::draw_points(const std::vector<Pt>& pts, Color c, float weight_px,
                           std::uint8_t alpha) {
    if (pts.size() < 2) {
        return;
    }
    auto submit = [&](float ox, float oy, std::uint8_t a) {
        std::vector<SDL_FPoint> sdl_pts;
        sdl_pts.reserve(pts.size());
        for (const Pt& p : pts) {
            sdl_pts.push_back({p.x + ox, p.y + oy});
        }
        SDL_SetRenderDrawColor(r_, c.r, c.g, c.b, a);
        SDL_RenderLines(r_, sdl_pts.data(), static_cast<int>(sdl_pts.size()));
    };

    if (weight_px < 1.5f) {
        submit(0.0f, 0.0f, alpha);
        return;
    }
    // SDL has no line width: fake it with a few offset passes (looks like soft graphite).
    const float o = weight_px * 0.3f;
    const auto a = static_cast<std::uint8_t>(alpha * 0.7f);
    submit(0.0f, 0.0f, a);
    submit(o, 0.0f, a);
    submit(-o, 0.0f, a);
    submit(0.0f, o, a);
    submit(0.0f, -o, a);
}

}  // namespace lof::render
