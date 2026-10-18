#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include <array>
#include <cstdint>
#include <exception>

#include "app/Clock.hpp"
#include "app/Window.hpp"
#include "render/Camera.hpp"
#include "render/Character.hpp"
#include "render/Palette.hpp"
#include "render/Pose.hpp"
#include "render/Renderer.hpp"
#include "render/Scarf.hpp"
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

// Dev: keys 1-5 force player 1's expression, 0 returns to automatic.
int g_forced_expression = -1;

void read_expression_keys() {
    const bool* k = SDL_GetKeyboardState(nullptr);
    if (k[SDL_SCANCODE_0]) g_forced_expression = -1;
    if (k[SDL_SCANCODE_1]) g_forced_expression = 0;
    if (k[SDL_SCANCODE_2]) g_forced_expression = 1;
    if (k[SDL_SCANCODE_3]) g_forced_expression = 2;
    if (k[SDL_SCANCODE_4]) g_forced_expression = 3;
    if (k[SDL_SCANCODE_5]) g_forced_expression = 4;
}

render::Expression expression_for(std::size_t i, const sim::Player& p) {
    if (i == 0 && g_forced_expression >= 0) {
        return static_cast<render::Expression>(g_forced_expression);
    }
    const bool moving = p.vel.x.raw != 0;
    return (moving || !p.grounded) ? render::Expression::Determined : render::Expression::Neutral;
}

render::Pose pose_for(const sim::World& world, std::size_t i) {
    const sim::Player& p = world.players[i];
    render::PoseInput in;
    in.feet = {render::to_float(p.pos.x), render::to_float(p.pos.y)};
    in.vx = render::to_float(p.vel.x) * 60.0f;
    in.vy = render::to_float(p.vel.y) * 60.0f;
    in.grounded = p.grounded;
    in.facing = p.facing;
    in.time_s = static_cast<float>(world.tick) / 60.0f + static_cast<float>(i) * 0.9f;
    in.expression = expression_for(i, p);
    return render::make_pose(in);
}

void draw_world(render::Renderer& r, const render::View& view, const sim::World& world,
                const std::array<render::Scarf, sim::kMaxPlayers>& scarves) {
    auto P = [&](float x, float y) { return view.project({x, y}); };

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

    // Stick men.
    for (std::size_t i = 0; i < world.players.size(); ++i) {
        render::draw_character(r, view, pose_for(world, i), render::player_style(i), scarves[i],
                               1000u + static_cast<std::uint32_t>(i) * 977u);
    }
}

}  // namespace

int main(int, char**) {
    try {
        app::Window window("Line of Fire", 1280, 720);
        render::Renderer renderer(window.renderer());
        app::Clock clock;
        sim::World world = sim::make_world();
        render::View view;
        std::array<render::Scarf, sim::kMaxPlayers> scarves{};
        std::uint64_t last_ns = SDL_GetTicksNS();

        while (window.pump_events()) {
            read_expression_keys();
            const int ticks = clock.advance();
            for (int i = 0; i < ticks; ++i) {
                sim::step(world, read_input());
            }

            const std::uint64_t now = SDL_GetTicksNS();
            const float dt = static_cast<float>(now - last_ns) * 1e-9f;
            last_ns = now;

            renderer.set_pixel_scale(window.pixel_scale());
            renderer.view_size(view.w, view.h);
            view.cam.update(world, view.w, view.h);

            for (std::size_t i = 0; i < scarves.size(); ++i) {
                const render::Pose pose = pose_for(world, i);
                scarves[i].update(pose.neck, pose.facing, dt);
            }

            renderer.begin_frame();
            draw_world(renderer, view, world, scarves);
            renderer.end_frame();
        }
    } catch (const std::exception& e) {
        SDL_Log("Fatal: %s", e.what());
        return 1;
    }
    return 0;
}
