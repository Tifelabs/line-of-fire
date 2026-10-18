// Dev tool: renders a lineup of every pose and expression to a BMP, using SDL's
// software renderer on an in-memory surface (no window needed).
// Usage: character_preview [out.bmp] [pixel_scale]
#include <SDL3/SDL.h>

#include <cstdio>
#include <cstdlib>
#include <vector>

#include "render/Character.hpp"

using namespace lof::render;

namespace {

struct Figure {
    PoseInput in;
    std::size_t style;
};

// Run the scarf for a couple of seconds of motion so it trails realistically.
Scarf simulate_scarf(PoseInput in) {
    Scarf scarf;
    constexpr int kFrames = 150;
    const float final_x = in.feet.x;
    for (int i = 0; i < kFrames; ++i) {
        PoseInput step = in;
        step.feet.x = final_x - in.vx * static_cast<float>(kFrames - 1 - i) / 60.0f;
        scarf.update(make_pose(step).neck, in.facing, 1.0f / 60.0f);
    }
    return scarf;
}

}  // namespace

int main(int argc, char** argv) {
    const char* out = argc > 1 ? argv[1] : "character_preview.bmp";
    const float scale = argc > 2 ? static_cast<float>(std::atof(argv[2])) : 2.0f;

    const int width = 1800;
    const int height = 620;
    SDL_Init(0);
    SDL_Surface* surface = SDL_CreateSurface(width, height, SDL_PIXELFORMAT_RGBA32);
    SDL_Renderer* sdl = SDL_CreateSoftwareRenderer(surface);
    if (surface == nullptr || sdl == nullptr) {
        std::fprintf(stderr, "failed to create software renderer: %s\n", SDL_GetError());
        return 1;
    }

    const float ground = 20.0f;
    std::vector<Figure> figs;
    auto add = [&](float x, std::size_t style, int facing, float vx, float vy, bool grounded,
                   float lift, Expression ex) {
        Figure f;
        f.in.feet = {x, ground - lift};
        f.in.vx = vx;
        f.in.vy = vy;
        f.in.grounded = grounded;
        f.in.facing = facing;
        f.in.time_s = 1.0f;
        f.in.expression = ex;
        f.style = style;
        figs.push_back(f);
    };
    add(1.5f, 0, 1, 0.0f, 0.0f, true, 0.0f, Expression::Neutral);       // idle
    add(4.0f, 1, 1, 12.0f, 0.0f, true, 0.0f, Expression::Determined);   // run right
    add(6.7f, 2, -1, -12.0f, 0.0f, true, 0.0f, Expression::Determined); // run left
    add(9.2f, 3, 1, 0.0f, -20.0f, false, 2.2f, Expression::Determined); // jump rising
    add(11.7f, 0, 1, 0.0f, 18.0f, false, 1.2f, Expression::Hurt);       // falling, hurt
    add(14.2f, 1, 1, 0.0f, 0.0f, true, 0.0f, Expression::Victory);      // victory
    add(16.7f, 2, -1, 0.0f, 0.0f, true, 0.0f, Expression::Dead);        // dead

    Renderer renderer(sdl);
    renderer.set_pixel_scale(scale);
    View view;
    view.w = static_cast<float>(width);
    view.h = static_cast<float>(height);
    view.cam.cx = 9.1f;
    view.cam.cy = 18.9f;
    view.cam.zoom = static_cast<float>(width) / 19.0f;

    renderer.begin_frame();
    const Pt g0 = view.project({0.0f, ground});
    const Pt g1 = view.project({18.5f, ground});
    renderer.stroke(g0, g1, 5);

    std::uint32_t seed = 1000;
    for (const Figure& fig : figs) {
        const Scarf scarf = simulate_scarf(fig.in);
        draw_character(renderer, view, make_pose(fig.in), player_style(fig.style), scarf, seed);
        seed += 977;
    }

    renderer.end_frame();  // flushes SDL's queued draw commands onto the surface

    if (!SDL_SaveBMP(surface, out)) {
        std::fprintf(stderr, "failed to save %s: %s\n", out, SDL_GetError());
        return 1;
    }
    std::printf("wrote %s\n", out);
    SDL_DestroyRenderer(sdl);
    SDL_DestroySurface(surface);
    SDL_Quit();
    return 0;
}
