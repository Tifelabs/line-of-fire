#pragma once
#include <SDL3/SDL.h>

#include <cstdint>
#include <vector>

#include "render/Common.hpp"

namespace lof::render {

// How a stroke looks. Sizes are in "1x display pixels" and get multiplied by the
// renderer's pixel scale, so lines stay the same visual weight on Retina screens.
struct Pen {
    Color color{45, 45, 50};
    float weight = 1.2f;
    float wobble = 1.4f;
    std::uint8_t alpha = 215;
};

// Milestone renderer: SDL's simple 2D renderer. Swap for the SDL GPU API
// (Metal on macOS) when the paper and pencil shaders arrive.
class Renderer {
public:
    explicit Renderer(SDL_Renderer* renderer);

    void set_pixel_scale(float s) { scale_ = s < 1.0f ? 1.0f : s; }

    void begin_frame();  // clears to paper colour
    void end_frame();
    void view_size(float& w, float& h) const;  // in pixels

    // Two-pass pencil stroke (dark core + soft halo) between two points.
    void stroke(Pt a, Pt b, std::uint32_t seed);
    void stroke(Pt a, Pt b, std::uint32_t seed, const Pen& pen);

    // Connected strokes: each segment is wobbled separately.
    void path(const std::vector<Pt>& pts, std::uint32_t seed, const Pen& pen);

    // Draws points exactly as given (use for already-wobbly shapes like circles).
    void polyline(const std::vector<Pt>& pts, const Pen& pen);

    // Small filled dot (eyes, joint pins).
    void dot(Pt center, float radius_px, Color color, std::uint8_t alpha = 235);

private:
    void draw_points(const std::vector<Pt>& pts, Color c, float weight_px, std::uint8_t alpha);
    void draw_pass(Pt a, Pt b, std::uint32_t seed, Color c, float weight_px, float wobble_px,
                   std::uint8_t alpha);

    SDL_Renderer* r_;
    float scale_ = 1.0f;
};

}  // namespace lof::render
