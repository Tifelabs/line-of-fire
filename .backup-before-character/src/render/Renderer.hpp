#pragma once
#include <SDL3/SDL.h>

#include <cstdint>

#include "render/Common.hpp"

namespace lof::render {

// Milestone 1 renderer: SDL's simple 2D renderer. Swap for the SDL GPU API
// (Metal on macOS) when the paper and pencil shaders arrive.
class Renderer {
public:
    explicit Renderer(SDL_Renderer* renderer);

    void begin_frame();  // clears to paper colour
    void end_frame();
    void view_size(float& w, float& h) const;  // in pixels

    // Draws a pencil stroke (two passes: dark core + soft halo).
    void stroke(Pt a, Pt b, std::uint32_t seed);

private:
    void draw_pass(Pt a, Pt b, std::uint32_t seed, float wobble_px, std::uint8_t alpha);

    SDL_Renderer* r_;
};

}  // namespace lof::render
