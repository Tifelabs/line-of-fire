#include "render/Renderer.hpp"

#include <vector>

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

void Renderer::stroke(Pt a, Pt b, std::uint32_t seed) {
    draw_pass(a, b, seed, 1.4f, 215);
    draw_pass(a, b, seed ^ 0x5BD1E995u, 2.6f, 80);
}

void Renderer::draw_pass(Pt a, Pt b, std::uint32_t seed, float wobble_px, std::uint8_t alpha) {
    std::vector<Pt> pts;
    pencil_line(pts, a, b, seed, wobble_px);

    std::vector<SDL_FPoint> sdl_pts;
    sdl_pts.reserve(pts.size());
    for (const Pt& p : pts) {
        sdl_pts.push_back({p.x, p.y});
    }

    SDL_SetRenderDrawColor(r_, 45, 45, 50, alpha);
    SDL_RenderLines(r_, sdl_pts.data(), static_cast<int>(sdl_pts.size()));
}

}  // namespace lof::render
