#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include <array>
#include <cstdint>
#include <exception>

#include "app/Clock.hpp"
#include "app/Window.hpp"
#include "render/Camera.hpp"
#include "render/Renderer.hpp"
#include "sim/Step.hpp"
#include "sim/World.hpp"

namespace {

using namespace lof;

constexpr float kSheetW = 29.7f;  // A4 landscape, 1 unit = 1 cm
constexpr float kSheetH = 21.0f;

// Dev controls: P1 = A/D + W/Space, P2 = arrows. Players 3 and 4 idle.
std::array<sim::Input, sim::kMaxPlayers> read_input() {
    const bool* k = SDL_GetKeyboardState(nullptr);
    std::array<sim::Input, sim::kMaxPlayers> in{};

    in[0].move_x = static_cast<std::int8_t>((k[SDL_SCANCODE_D] ? 1 : 0) - (k[SDL_SCANCODE_A] ? 1 : 0));
    in[0].jump = k[SDL_SCANCODE_W] || k[SDL_SCANCODE_SPACE];

    in[1].move_x =
        static_cast<std::int8_t>((k[SDL_SCANCODE_RIGHT] ? 1 : 0) - (k[SDL_SCANCODE_LEFT] ? 1 : 0));
    in[1].jump = k[SDL_SCANCODE_UP];
    return in;
}

void draw_world(render::Renderer& r, const render::Camera& cam, const sim::World& world) {
    float vw = 0.0f;
    float vh = 0.0f;
    r.view_size(vw, vh);
    auto P = [&](float x, float y) { return cam.to_screen({x, y}, vw, vh); };

    // The A4 sheet border.
    const render::Pt c00 = P(0.0f, 0.0f);
    const render::Pt c10 = P(kSheetW, 0.0f);
    const render::Pt c11 = P(kSheetW, kSheetH);
    const render::Pt c01 = P(0.0f, kSheetH);
    r.stroke(c00, c10, 1);
    r.stroke(c10, c11, 2);
    r.stroke(c11, c01, 3);
    r.stroke(c01, c00, 4);

    // Ground line.
    const float gy = render::to_float(sim::kGroundY);
    r.stroke(P(0.0f, gy), P(kSheetW, gy), 5);

    // Players as hand-drawn boxes, 1 x 2 units.
    for (std::size_t i = 0; i < world.players.size(); ++i) {
        const float x = render::to_float(world.players[i].pos.x);
        const float y = render::to_float(world.players[i].pos.y);
        const render::Pt tl = P(x - 0.5f, y - 2.0f);
        const render::Pt tr = P(x + 0.5f, y - 2.0f);
        const render::Pt br = P(x + 0.5f, y);
        const render::Pt bl = P(x - 0.5f, y);
        const std::uint32_t seed = 100 + static_cast<std::uint32_t>(i) * 4;
        r.stroke(tl, tr, seed);
        r.stroke(tr, br, seed + 1);
        r.stroke(br, bl, seed + 2);
        r.stroke(bl, tl, seed + 3);
    }
}

}  // namespace

int main(int, char**) {
    try {
        app::Window window("Line of Fire", 1280, 720);
        render::Renderer renderer(window.renderer());
        app::Clock clock;
        sim::World world = sim::make_world();
        render::Camera camera;

        while (window.pump_events()) {
            const int ticks = clock.advance();
            for (int i = 0; i < ticks; ++i) {
                sim::step(world, read_input());
            }

            float vw = 0.0f;
            float vh = 0.0f;
            renderer.view_size(vw, vh);
            camera.update(world, vw, vh);

            renderer.begin_frame();
            draw_world(renderer, camera, world);
            renderer.end_frame();
        }
    } catch (const std::exception& e) {
        SDL_Log("Fatal: %s", e.what());
        return 1;
    }
    return 0;
}
