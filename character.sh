#!/usr/bin/env bash
# Adds the stick-man character (pose, scarf, palette, face, hats) to Line of Fire.
# Run from the project root:   bash add_character.sh
#
# This OVERWRITES the files listed below with their updated versions. Your old
# copies are saved to .backup-before-character/ first. If you changed any of these
# files yourself, commit or merge by hand using the backups.
set -euo pipefail

if [ ! -f CMakeLists.txt ] || [ ! -d src/sim ]; then
    echo "error: run this from the line-of-fire project root" >&2
    exit 1
fi

BACKUP=".backup-before-character"
if [ -e "$BACKUP" ]; then
    echo "error: $BACKUP already exists; move it away first" >&2
    exit 1
fi

FILES=(
    "CMakeLists.txt"
    "src/app/Window.cpp"
    "src/app/Window.hpp"
    "src/app/main.cpp"
    "src/render/Camera.hpp"
    "src/render/Character.cpp"
    "src/render/Character.hpp"
    "src/render/Common.hpp"
    "src/render/Palette.cpp"
    "src/render/Palette.hpp"
    "src/render/Pose.cpp"
    "src/render/Pose.hpp"
    "src/render/Renderer.cpp"
    "src/render/Renderer.hpp"
    "src/render/Scarf.cpp"
    "src/render/Scarf.hpp"
    "src/render/Stroke.cpp"
    "src/render/Stroke.hpp"
    "src/sim/Step.cpp"
    "src/sim/World.cpp"
    "src/sim/World.hpp"
    "tests/CMakeLists.txt"
    "tests/test_pose.cpp"
    "tests/test_sim_determinism.cpp"
    "tests/test_stroke.cpp"
    "tools/preview.cpp"
)

for f in "${FILES[@]}"; do
    if [ -f "$f" ]; then
        mkdir -p "$BACKUP/$(dirname "$f")"
        cp "$f" "$BACKUP/$f"
    fi
done

mkdir -p "$(dirname "CMakeLists.txt")"
cat > "CMakeLists.txt" << '__EOF__'
cmake_minimum_required(VERSION 3.25)
project(LineOfFire VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(LOF_BUILD_APP "Build the game executable (needs SDL3)" ON)
option(LOF_BUILD_TESTS "Build unit tests" ON)
option(LOF_BUILD_TOOLS "Build dev tools (character_preview)" OFF)
option(LOF_SANITIZE "Enable address + undefined behaviour sanitizers" OFF)

find_program(LOF_CCACHE ccache)
if(LOF_CCACHE)
    set(CMAKE_C_COMPILER_LAUNCHER "${LOF_CCACHE}")
    set(CMAKE_CXX_COMPILER_LAUNCHER "${LOF_CCACHE}")
endif()

include(cmake/CompilerFlags.cmake)
include(cmake/Dependencies.cmake)

# Deterministic core: geometry + simulation. No SDL, no rendering, no floats.
add_library(lof_sim STATIC
    src/geometry/Fixed.cpp
    src/geometry/Intersect.cpp
    src/sim/World.cpp
    src/sim/Step.cpp)
target_include_directories(lof_sim PUBLIC src)
target_compile_features(lof_sim PUBLIC cxx_std_20)
target_link_libraries(lof_sim PUBLIC lof_flags)

# Render-side math (strokes, camera). Floats allowed, still no SDL.
add_library(lof_render_core STATIC
    src/render/Stroke.cpp
    src/render/Camera.cpp
    src/render/Palette.cpp
    src/render/Pose.cpp
    src/render/Scarf.cpp)
target_link_libraries(lof_render_core PUBLIC lof_sim)

if(LOF_BUILD_APP)
    # SDL-facing drawing code (renderer + character), shared by the game and tools.
    add_library(lof_render_sdl STATIC
        src/render/Renderer.cpp
        src/render/Character.cpp)
    target_link_libraries(lof_render_sdl PUBLIC lof_render_core ${LOF_SDL_TARGET})

    add_executable(line_of_fire
        src/app/main.cpp
        src/app/Window.cpp
        src/app/Clock.cpp)
    target_link_libraries(line_of_fire PRIVATE lof_render_sdl)

    if(LOF_BUILD_TOOLS)
        add_executable(character_preview tools/preview.cpp)
        target_link_libraries(character_preview PRIVATE lof_render_sdl)
    endif()
endif()

if(LOF_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
__EOF__

mkdir -p "$(dirname "src/app/Window.cpp")"
cat > "src/app/Window.cpp" << '__EOF__'
#include "app/Window.hpp"

#include <stdexcept>

namespace lof::app {

Window::Window(const char* title, int width, int height) {
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        throw std::runtime_error(SDL_GetError());
    }
    if (!SDL_CreateWindowAndRenderer(title, width, height,
                                     SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY, &window_,
                                     &renderer_)) {
        const char* msg = SDL_GetError();
        SDL_Quit();
        throw std::runtime_error(msg);
    }
    SDL_SetRenderVSync(renderer_, 1);
}

Window::~Window() {
    SDL_DestroyRenderer(renderer_);
    SDL_DestroyWindow(window_);
    SDL_Quit();
}

float Window::pixel_scale() const {
    const float d = SDL_GetWindowPixelDensity(window_);
    return d < 1.0f ? 1.0f : d;
}

bool Window::pump_events() {
    bool running = true;
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        if (e.type == SDL_EVENT_QUIT) {
            running = false;
        } else if (e.type == SDL_EVENT_KEY_DOWN && e.key.key == SDLK_ESCAPE) {
            running = false;
        }
    }
    return running;
}

}  // namespace lof::app
__EOF__

mkdir -p "$(dirname "src/app/Window.hpp")"
cat > "src/app/Window.hpp" << '__EOF__'
#pragma once
#include <SDL3/SDL.h>

namespace lof::app {

// RAII wrapper: SDL init, window, renderer, event pump.
class Window {
public:
    Window(const char* title, int width, int height);
    ~Window();
    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;

    SDL_Renderer* renderer() const { return renderer_; }

    // Display pixel density (2.0 on Retina). Never below 1.
    float pixel_scale() const;

    // Returns false once the user asked to quit (window close or Esc).
    bool pump_events();

private:
    SDL_Window* window_ = nullptr;
    SDL_Renderer* renderer_ = nullptr;
};

}  // namespace lof::app
__EOF__

mkdir -p "$(dirname "src/app/main.cpp")"
cat > "src/app/main.cpp" << '__EOF__'
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
__EOF__

mkdir -p "$(dirname "src/render/Camera.hpp")"
cat > "src/render/Camera.hpp" << '__EOF__'
#pragma once
#include "render/Common.hpp"
#include "sim/World.hpp"

namespace lof::render {

// Shared dynamic camera: frames every player, zooms out as they spread,
// with a hard zoom limit so the pencil lines stay crisp.
struct Camera {
    float cx = 15.0f;  // world units
    float cy = 10.0f;
    float zoom = 30.0f;  // pixels per world unit

    void update(const sim::World& world, float view_w, float view_h);
    Pt to_screen(Pt world_pt, float view_w, float view_h) const;
};

// A camera plus the viewport size, so drawing code can project points easily.
struct View {
    Camera cam;
    float w = 1280.0f;
    float h = 720.0f;

    Pt project(Pt world_pt) const { return cam.to_screen(world_pt, w, h); }
};

}  // namespace lof::render
__EOF__

mkdir -p "$(dirname "src/render/Character.cpp")"
cat > "src/render/Character.cpp" << '__EOF__'
#include "render/Character.hpp"

#include <algorithm>
#include <cmath>
#include <vector>

#include "render/Stroke.hpp"

namespace lof::render {

void draw_character(Renderer& r, const View& view, const Pose& pose, const PlayerStyle& style,
                    const Scarf& scarf, std::uint32_t seed) {
    const float f = pose.facing >= 0 ? 1.0f : -1.0f;
    const float zoom = view.cam.zoom;
    auto P = [&](Pt p) { return view.project(p); };

    const Color graphite{45, 45, 50};
    const Pen ink{style.color, 2.2f, 0.9f, 235};
    const Pen sketch{graphite, 1.0f, 2.2f, 90};
    const Pen face_pen{graphite, 1.5f, 0.0f, 235};
    const Pen fill_pen{style.color, 1.0f, 0.0f, 95};
    const Pen scarf_pen{style.color, 2.6f, 0.6f, 235};

    auto limb = [&](Pt a, Pt b, std::uint32_t s) {
        r.stroke(P(a), P(b), seed + s, sketch);
        r.stroke(P(a), P(b), seed + s + 50u, ink);
    };
    auto pin = [&](Pt j) { r.dot(P(j), 0.035f * zoom, graphite, 200); };

    // --- scarf (behind everything) ---
    {
        std::vector<Pt> pts;
        pts.reserve(scarf.p.size());
        for (const Pt& q : scarf.p) {
            pts.push_back(P(q));
        }
        r.path(pts, seed + 400u, scarf_pen);
        r.dot(P(pose.neck), 0.055f * zoom, style.color, 240);
    }

    auto leg = [&](int i, std::uint32_t s) {
        limb(pose.hip, pose.knee[i], s);
        limb(pose.knee[i], pose.foot[i], s + 1u);
        pin(pose.knee[i]);
    };
    auto arm = [&](int i, std::uint32_t s) {
        limb(pose.shoulder, pose.elbow[i], s);
        limb(pose.elbow[i], pose.hand[i], s + 1u);
        pin(pose.elbow[i]);
        r.dot(P(pose.hand[i]), 0.03f * zoom, style.color, 220);
    };

    // --- back limbs, torso, front limbs ---
    arm(1, 10u);
    leg(1, 20u);
    limb(pose.hip, pose.neck, 30u);
    leg(0, 40u);
    arm(0, 50u);
    pin(pose.hip);
    pin(pose.shoulder);

    // --- head: pencil circle + colored hatching ---
    {
        std::vector<Pt> circle;
        pencil_circle(circle, pose.head, pose.head_r, seed + 300u, 0.010f, 26);
        std::vector<Pt> screen;
        for (const Pt& q : circle) screen.push_back(P(q));
        r.polyline(screen, ink);

        std::vector<Pt> circle2;
        pencil_circle(circle2, pose.head, pose.head_r, seed + 301u, 0.022f, 22);
        std::vector<Pt> screen2;
        for (const Pt& q : circle2) screen2.push_back(P(q));
        r.polyline(screen2, sketch);

        std::vector<Pt> poly;
        for (int i = 0; i < 18; ++i) {
            const float a = 6.2831853f * static_cast<float>(i) / 18.0f;
            poly.push_back({pose.head.x + std::cos(a) * pose.head_r * 0.92f,
                            pose.head.y + std::sin(a) * pose.head_r * 0.92f});
        }
        std::vector<Seg> hatch;
        hatch_convex(hatch, poly, -0.8f, 0.06f);
        for (const Seg& s : hatch) {
            r.polyline({P(s.a), P(s.b)}, fill_pen);
        }
    }

    // --- hat (basic geometric shape) ---
    {
        const HatShape hat = hat_shape(style.hat);
        const Pt top{pose.head.x, pose.head.y - pose.head_r};
        std::vector<Pt> crown;
        for (const Pt& q : hat.crown) crown.push_back(top + q);

        std::vector<Seg> hatch;
        hatch_convex(hatch, crown, 0.8f, 0.05f);
        for (const Seg& s : hatch) {
            r.polyline({P(s.a), P(s.b)}, fill_pen);
        }

        std::vector<Pt> loop;
        for (const Pt& q : crown) loop.push_back(P(q));
        loop.push_back(P(crown.front()));
        r.path(loop, seed + 500u, ink);
        if (hat.has_brim) {
            r.stroke(P(top + hat.brim_a), P(top + hat.brim_b), seed + 520u, ink);
        }
    }

    // --- face ---
    {
        const Pt h = pose.head;
        const float ly = 0.02f * pose.look_y;
        const Pt eye0{h.x + f * 0.03f, h.y - 0.02f + ly};
        const Pt eye1{h.x + f * 0.16f, h.y - 0.02f + ly};
        const float eye_px = 0.028f * zoom;
        const Pt eyes[2] = {eye0, eye1};

        for (const Pt& e : eyes) {
            if (pose.expression == Expression::Dead) {
                const float k = 0.035f;
                r.polyline({P({e.x - k, e.y - k}), P({e.x + k, e.y + k})}, face_pen);
                r.polyline({P({e.x - k, e.y + k}), P({e.x + k, e.y - k})}, face_pen);
            } else if (pose.blink) {
                r.polyline({P({e.x - 0.04f, e.y}), P({e.x + 0.04f, e.y})}, face_pen);
            } else {
                r.dot(P(e), eye_px, graphite, 240);
            }
        }

        // brows: back end / front end heights (negative = higher)
        float back = -0.115f;
        float front = -0.115f;
        bool brows = true;
        switch (pose.expression) {
            case Expression::Neutral: break;
            case Expression::Determined: back = -0.14f; front = -0.085f; break;
            case Expression::Hurt: back = -0.085f; front = -0.14f; break;
            case Expression::Victory: back = -0.15f; front = -0.15f; break;
            case Expression::Dead: brows = false; break;
        }
        if (brows) {
            for (const Pt& e : eyes) {
                r.polyline({P({e.x - f * 0.04f, h.y + back + ly}),
                            P({e.x + f * 0.04f, h.y + front + ly})},
                           face_pen);
            }
        }

        // mouth
        const Pt m{h.x + f * 0.12f, h.y + 0.12f};
        std::vector<Pt> mouth;
        switch (pose.expression) {
            case Expression::Neutral:
            case Expression::Dead:
                mouth = {{m.x - f * 0.05f, m.y}, {m.x + f * 0.05f, m.y}};
                break;
            case Expression::Determined:
                mouth = {{m.x - f * 0.05f, m.y + 0.01f}, {m.x + f * 0.05f, m.y - 0.01f}};
                break;
            case Expression::Hurt:
                mouth = {{m.x - f * 0.06f, m.y},
                         {m.x - f * 0.02f, m.y - 0.025f},
                         {m.x + f * 0.02f, m.y + 0.025f},
                         {m.x + f * 0.06f, m.y}};
                break;
            case Expression::Victory:
                for (int i = 0; i <= 6; ++i) {
                    const float u = static_cast<float>(i) / 6.0f * 2.0f - 1.0f;
                    mouth.push_back({m.x + f * 0.075f * u, m.y + 0.05f * (1.0f - u * u)});
                }
                break;
        }
        std::vector<Pt> mouth_screen;
        for (const Pt& q : mouth) mouth_screen.push_back(P(q));
        r.polyline(mouth_screen, face_pen);
    }
}

}  // namespace lof::render
__EOF__

mkdir -p "$(dirname "src/render/Character.hpp")"
cat > "src/render/Character.hpp" << '__EOF__'
#pragma once
#include <cstdint>

#include "render/Camera.hpp"
#include "render/Palette.hpp"
#include "render/Pose.hpp"
#include "render/Renderer.hpp"
#include "render/Scarf.hpp"

namespace lof::render {

// Draws one stick-man: scarf, limbs with pivot pins, circle head with colored
// hatching, geometric hat, and a face that follows the pose's expression.
void draw_character(Renderer& r, const View& view, const Pose& pose, const PlayerStyle& style,
                    const Scarf& scarf, std::uint32_t seed);

}  // namespace lof::render
__EOF__

mkdir -p "$(dirname "src/render/Common.hpp")"
cat > "src/render/Common.hpp" << '__EOF__'
#pragma once
#include <cstdint>

#include "geometry/Fixed.hpp"

namespace lof::render {

struct Pt {
    float x;
    float y;
};

struct Color {
    std::uint8_t r;
    std::uint8_t g;
    std::uint8_t b;
};

inline Pt operator+(Pt a, Pt b) { return {a.x + b.x, a.y + b.y}; }
inline Pt operator-(Pt a, Pt b) { return {a.x - b.x, a.y - b.y}; }
inline Pt operator*(Pt a, float s) { return {a.x * s, a.y * s}; }

// Render-only conversion. Floats must never flow back into the simulation.
inline float to_float(Fixed f) {
    return static_cast<float>(f.raw) / static_cast<float>(Fixed::kOne);
}

}  // namespace lof::render
__EOF__

mkdir -p "$(dirname "src/render/Palette.cpp")"
cat > "src/render/Palette.cpp" << '__EOF__'
#include "render/Palette.hpp"

#include <array>
#include <cmath>

namespace lof::render {
namespace {

const std::array<PlayerStyle, 4> kStyles = {{
    {"Red", {205, 45, 45}, Hat::Triangle},
    {"Blue", {40, 95, 195}, Hat::Rectangle},
    {"Green", {45, 150, 75}, Hat::Semicircle},
    {"Amber", {225, 145, 20}, Hat::Trapezoid},
}};

}  // namespace

const PlayerStyle& player_style(std::size_t index) { return kStyles[index % kStyles.size()]; }

HatShape hat_shape(Hat hat) {
    HatShape s;
    switch (hat) {
        case Hat::Triangle:
            s.crown = {{-0.20f, 0.03f}, {0.20f, 0.03f}, {0.0f, -0.42f}};
            break;
        case Hat::Rectangle:
            s.crown = {{-0.17f, 0.0f}, {0.17f, 0.0f}, {0.17f, -0.36f}, {-0.17f, -0.36f}};
            s.has_brim = true;
            s.brim_a = {-0.30f, 0.0f};
            s.brim_b = {0.30f, 0.0f};
            break;
        case Hat::Semicircle: {
            constexpr int kSteps = 10;
            for (int i = 0; i <= kSteps; ++i) {
                const float a = 3.1415927f + 3.1415927f * static_cast<float>(i) / kSteps;
                s.crown.push_back({std::cos(a) * 0.26f, 0.03f + std::sin(a) * 0.26f});
            }
            break;
        }
        case Hat::Trapezoid:
            s.crown = {{-0.25f, 0.03f}, {0.25f, 0.03f}, {0.16f, -0.23f}, {-0.16f, -0.23f}};
            s.has_brim = true;
            s.brim_a = {-0.32f, 0.03f};
            s.brim_b = {0.32f, 0.03f};
            break;
    }
    return s;
}

}  // namespace lof::render
__EOF__

mkdir -p "$(dirname "src/render/Palette.hpp")"
cat > "src/render/Palette.hpp" << '__EOF__'
#pragma once
#include <cstddef>
#include <vector>

#include "render/Common.hpp"

namespace lof::render {

// Hats are basic geometric shapes so players differ by silhouette, not only colour.
enum class Hat { Triangle, Rectangle, Semicircle, Trapezoid };

struct PlayerStyle {
    const char* name;
    Color color;  // colored pencil
    Hat hat;
};

// Index wraps, so any player index is safe.
const PlayerStyle& player_style(std::size_t index);

struct HatShape {
    std::vector<Pt> crown;  // convex polygon, relative to the top-centre of the head
    bool has_brim = false;
    Pt brim_a{0.0f, 0.0f};
    Pt brim_b{0.0f, 0.0f};
};

// Positive y is DOWN (screen style), so the hat sits at negative y.
HatShape hat_shape(Hat hat);

}  // namespace lof::render
__EOF__

mkdir -p "$(dirname "src/render/Pose.cpp")"
cat > "src/render/Pose.cpp" << '__EOF__'
#include "render/Pose.hpp"

#include <algorithm>
#include <cmath>

namespace lof::render {
namespace {

constexpr float kPi = 3.1415927f;

float clamp01(float v) { return std::clamp(v, 0.0f, 1.0f); }

Pt lerp(Pt a, Pt b, float t) { return {a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t}; }

Pt clamp_reach(Pt root, Pt target, float max_d) {
    const float dx = target.x - root.x;
    const float dy = target.y - root.y;
    const float d = std::sqrt(dx * dx + dy * dy);
    if (d <= max_d || d < 1e-6f) {
        return target;
    }
    const float k = max_d / d;
    return {root.x + dx * k, root.y + dy * k};
}

}  // namespace

Pt solve_two_bone(Pt root, Pt target, float l1, float l2, float bend_dir) {
    float dx = target.x - root.x;
    float dy = target.y - root.y;
    float d = std::sqrt(dx * dx + dy * dy);
    float ux = 0.0f;
    float uy = 1.0f;
    if (d > 1e-5f) {
        ux = dx / d;
        uy = dy / d;
    }
    const float dc = std::clamp(d, std::fabs(l1 - l2) + 0.001f, l1 + l2 - 0.001f);
    const float a = (l1 * l1 - l2 * l2 + dc * dc) / (2.0f * dc);
    const float h = std::sqrt(std::max(0.0f, l1 * l1 - a * a));
    const Pt base{root.x + ux * a, root.y + uy * a};
    const Pt m1{base.x - uy * h, base.y + ux * h};
    const Pt m2{base.x + uy * h, base.y - ux * h};
    return (m1.x * bend_dir >= m2.x * bend_dir) ? m1 : m2;
}

Pose make_pose(const PoseInput& in) {
    Pose p;
    const float f = in.facing >= 0 ? 1.0f : -1.0f;
    p.facing = in.facing >= 0 ? 1 : -1;
    p.expression = in.expression;

    const float speed = std::fabs(in.vx);
    const float run = in.grounded ? clamp01(speed / 12.0f) : 0.0f;
    // Phase follows distance travelled in the facing direction, so feet never slide.
    const float phase = f * in.feet.x * (2.0f * kPi / 3.2f);
    const float t = in.time_s;

    const float breath = (1.0f - run) * 0.012f * std::sin(t * 2.2f);
    const bool dead = in.expression == Expression::Dead;
    const bool hurt = in.expression == Expression::Hurt;
    const bool victory = in.expression == Expression::Victory;

    // Airborne blend: 0 = rising, 1 = falling.
    const float w = clamp01(0.5f + in.vy / 36.0f);

    Pt hip{in.feet.x, in.feet.y - 0.885f - 0.05f * run * std::cos(2.0f * phase)};
    float lean = 0.10f * run;
    if (!in.grounded) {
        hip.y = in.feet.y - 0.90f;
        lean = 0.06f + 0.06f * (1.0f - w);
    }
    if (hurt) lean -= 0.20f;
    if (dead) lean = -0.22f;

    p.hip = hip;
    p.neck = {hip.x + f * lean, hip.y - kTorso + breath};
    p.shoulder = {p.neck.x, p.neck.y + 0.06f};
    p.head_r = kHeadR;
    p.head = {p.neck.x + f * 0.02f * run, p.neck.y - (kHeadR + 0.03f)};
    if (hurt) p.head.x += 0.03f * std::sin(t * 50.0f);
    if (dead) p.head = {p.head.x - f * 0.05f, p.head.y + 0.04f};

    for (int i = 0; i < 2; ++i) {
        const float fi = static_cast<float>(i);
        const float leg_ph = phase + fi * kPi;
        const float arm_ph = phase + (fi + 1.0f) * kPi;
        const float side = (i == 0) ? 1.0f : -1.0f;

        Pt foot_t;
        Pt hand_t;
        if (in.grounded) {
            foot_t = {hip.x + f * std::sin(leg_ph) * 0.5f * run + f * side * 0.08f * (1.0f - run),
                      in.feet.y - 0.30f * run * std::max(0.0f, std::cos(leg_ph))};
            hand_t = {p.shoulder.x + f * (0.08f + 0.45f * run * std::sin(arm_ph)) +
                          f * 0.015f * std::sin(t * 1.7f + fi),
                      p.shoulder.y + 0.62f - 0.20f * run + 0.05f * run * std::cos(arm_ph)};
        } else {
            const Pt rise_foot = (i == 0) ? Pt{hip.x + f * 0.28f, hip.y + 0.62f}
                                          : Pt{hip.x - f * 0.12f, hip.y + 0.80f};
            const Pt fall_foot = (i == 0) ? Pt{hip.x + f * 0.12f, hip.y + 0.86f}
                                          : Pt{hip.x - f * 0.10f, hip.y + 0.88f};
            const Pt rise_hand = (i == 0) ? Pt{p.shoulder.x + f * 0.40f, p.shoulder.y - 0.55f}
                                          : Pt{p.shoulder.x - f * 0.25f, p.shoulder.y - 0.50f};
            const Pt fall_hand = (i == 0) ? Pt{p.shoulder.x + f * 0.62f, p.shoulder.y + 0.10f}
                                          : Pt{p.shoulder.x - f * 0.58f, p.shoulder.y + 0.14f};
            foot_t = lerp(rise_foot, fall_foot, w);
            hand_t = lerp(rise_hand, fall_hand, w);
        }

        if (victory) {
            hand_t = {p.shoulder.x + f * side * 0.60f, p.shoulder.y - 0.38f};  // V shape, clear of the face
        } else if (dead) {
            hand_t = {p.shoulder.x + f * side * 0.30f, p.shoulder.y + 0.62f};
        }

        p.foot[i] = clamp_reach(hip, foot_t, kThigh + kShin - 0.002f);
        p.knee[i] = solve_two_bone(hip, p.foot[i], kThigh, kShin, f);
        p.hand[i] = clamp_reach(p.shoulder, hand_t, kUpperArm + kForearm - 0.002f);
        p.elbow[i] = solve_two_bone(p.shoulder, p.hand[i], kUpperArm, kForearm, -f);
    }

    p.blink = in.grounded && run < 0.1f && std::fmod(std::fabs(t), 3.6f) < 0.12f;
    p.look_y = std::clamp(in.vy / 30.0f, -1.0f, 1.0f);
    return p;
}

}  // namespace lof::render
__EOF__

mkdir -p "$(dirname "src/render/Pose.hpp")"
cat > "src/render/Pose.hpp" << '__EOF__'
#pragma once
#include <array>

#include "render/Common.hpp"

namespace lof::render {

// Body proportions in world units (1 unit = 1 cm of paper). Total height is ~2.
inline constexpr float kThigh = 0.45f;
inline constexpr float kShin = 0.45f;
inline constexpr float kUpperArm = 0.35f;
inline constexpr float kForearm = 0.35f;
inline constexpr float kTorso = 0.60f;
inline constexpr float kHeadR = 0.26f;

enum class Expression { Neutral, Determined, Hurt, Victory, Dead };

struct PoseInput {
    Pt feet{0.0f, 0.0f};  // ground contact point (y grows downward)
    float vx = 0.0f;      // units per second
    float vy = 0.0f;
    bool grounded = true;
    int facing = 1;       // +1 right, -1 left
    float time_s = 0.0f;  // animation clock (use world tick / 60)
    Expression expression = Expression::Neutral;
};

// Index 0 = front limb, 1 = back limb.
struct Pose {
    Pt head{};
    float head_r = kHeadR;
    Pt neck{};
    Pt shoulder{};
    Pt hip{};
    std::array<Pt, 2> elbow{};
    std::array<Pt, 2> hand{};
    std::array<Pt, 2> knee{};
    std::array<Pt, 2> foot{};
    int facing = 1;
    Expression expression = Expression::Neutral;
    bool blink = false;
    float look_y = 0.0f;  // -1 looking up .. +1 looking down
};

// Pure function of its input: same input, same pose. Render-only (uses floats).
Pose make_pose(const PoseInput& in);

// Two-bone IK. Returns the middle joint (knee or elbow). bend_dir (+1/-1) picks
// which horizontal side the joint bends toward.
Pt solve_two_bone(Pt root, Pt target, float l1, float l2, float bend_dir);

}  // namespace lof::render
__EOF__

mkdir -p "$(dirname "src/render/Renderer.cpp")"
cat > "src/render/Renderer.cpp" << '__EOF__'
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
__EOF__

mkdir -p "$(dirname "src/render/Renderer.hpp")"
cat > "src/render/Renderer.hpp" << '__EOF__'
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
__EOF__

mkdir -p "$(dirname "src/render/Scarf.cpp")"
cat > "src/render/Scarf.cpp" << '__EOF__'
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
__EOF__

mkdir -p "$(dirname "src/render/Scarf.hpp")"
cat > "src/render/Scarf.hpp" << '__EOF__'
#pragma once
#include <array>

#include "render/Common.hpp"

namespace lof::render {

// Trailing scarf: a chain of points following the neck. Render-only, so it can
// never affect the simulation or the network.
struct Scarf {
    static constexpr int kPoints = 6;
    static constexpr float kSegment = 0.16f;

    std::array<Pt, kPoints> p{};
    std::array<Pt, kPoints> prev{};
    Pt last_anchor{};
    bool ready = false;

    // anchor = neck position, dt in seconds.
    void update(Pt anchor, int facing, float dt);
};

}  // namespace lof::render
__EOF__

mkdir -p "$(dirname "src/render/Stroke.cpp")"
cat > "src/render/Stroke.cpp" << '__EOF__'
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
__EOF__

mkdir -p "$(dirname "src/render/Stroke.hpp")"
cat > "src/render/Stroke.hpp" << '__EOF__'
#pragma once
#include <cstdint>
#include <vector>

#include "render/Common.hpp"

namespace lof::render {

struct Seg {
    Pt a;
    Pt b;
};

// Appends a hand-drawn looking polyline from a to b. Same seed = same wobble,
// so the sketch never flickers and looks identical for every networked player.
// wobble is the maximum sideways offset, in the same units as the points.
void pencil_line(std::vector<Pt>& out, Pt a, Pt b, std::uint32_t seed, float wobble);

// Appends a hand-drawn circle: slightly more than one full turn (the ends overlap
// like a real pencil circle) with a small radius drift.
void pencil_circle(std::vector<Pt>& out, Pt center, float radius, std::uint32_t seed, float wobble,
                   int segments = 24);

// Colored-pencil hatching inside a CONVEX polygon. Lines run at angle_rad,
// spaced `gap` apart.
void hatch_convex(std::vector<Seg>& out, const std::vector<Pt>& poly, float angle_rad, float gap);

}  // namespace lof::render
__EOF__

mkdir -p "$(dirname "src/sim/Step.cpp")"
cat > "src/sim/Step.cpp" << '__EOF__'
#include "sim/Step.hpp"

namespace lof::sim {
namespace {

// Placeholder movement numbers (units per tick at 60 Hz). Tune later.
constexpr Fixed kRunSpeed = Fixed::from_ratio(1, 5);     // 12 units/s
constexpr Fixed kJumpSpeed = Fixed::from_ratio(9, 20);   // about 5 units high
constexpr Fixed kGravity = Fixed::from_ratio(1, 50);

}  // namespace

void step(World& world, const std::array<Input, kMaxPlayers>& inputs) {
    for (std::size_t i = 0; i < kMaxPlayers; ++i) {
        Player& p = world.players[i];
        const Input& in = inputs[i];

        p.vel.x = Fixed::from_int(in.move_x) * kRunSpeed;
        if (in.move_x != 0) {
            p.facing = in.move_x > 0 ? 1 : -1;
        }
        if (in.jump && p.grounded) {
            p.vel.y = -kJumpSpeed;
            p.grounded = false;
        }
        p.vel.y += kGravity;

        p.pos += p.vel;

        if (p.pos.y >= kGroundY) {
            p.pos.y = kGroundY;
            p.vel.y = Fixed{};
            p.grounded = true;
        }
    }
    ++world.tick;
}

}  // namespace lof::sim
__EOF__

mkdir -p "$(dirname "src/sim/World.cpp")"
cat > "src/sim/World.cpp" << '__EOF__'
#include "sim/World.hpp"

namespace lof::sim {

World make_world() {
    World w;
    for (std::size_t i = 0; i < kMaxPlayers; ++i) {
        w.players[i].pos = {Fixed::from_int(static_cast<std::int32_t>(5 + 5 * i)), kGroundY};
        w.players[i].grounded = true;
        w.players[i].facing = (i < 2) ? 1 : -1;  // two sides facing each other
    }
    return w;
}

namespace {

void mix(std::uint64_t& h, std::uint32_t v) {
    for (int i = 0; i < 4; ++i) {
        h ^= (v & 0xFFu);
        h *= 1099511628211ULL;
        v >>= 8;
    }
}

}  // namespace

std::uint64_t hash(const World& w) {
    std::uint64_t h = 14695981039346656037ULL;
    mix(h, w.tick);
    for (const Player& p : w.players) {
        mix(h, static_cast<std::uint32_t>(p.pos.x.raw));
        mix(h, static_cast<std::uint32_t>(p.pos.y.raw));
        mix(h, static_cast<std::uint32_t>(p.vel.x.raw));
        mix(h, static_cast<std::uint32_t>(p.vel.y.raw));
        mix(h, p.grounded ? 1u : 0u);
        mix(h, static_cast<std::uint32_t>(static_cast<int>(p.facing)));
    }
    return h;
}

}  // namespace lof::sim
__EOF__

mkdir -p "$(dirname "src/sim/World.hpp")"
cat > "src/sim/World.hpp" << '__EOF__'
#pragma once
#include <array>
#include <cstdint>

#include "geometry/Vec2.hpp"
#include "sim/Input.hpp"

namespace lof::sim {

// World units: 1 unit = 1 cm of paper. An A4 sheet is 29.7 x 21.0 units.
inline constexpr Fixed kGroundY = Fixed::from_int(20);

struct Player {
    Vec2 pos;  // feet position (y grows downward)
    Vec2 vel;  // units per tick
    bool grounded = false;
    std::int8_t facing = 1;  // +1 right, -1 left
};

struct World {
    std::uint32_t tick = 0;
    std::array<Player, kMaxPlayers> players{};
};

World make_world();

// FNV-1a over the full state. Used by tests now, desync detection later.
std::uint64_t hash(const World& w);

}  // namespace lof::sim
__EOF__

mkdir -p "$(dirname "tests/CMakeLists.txt")"
cat > "tests/CMakeLists.txt" << '__EOF__'
add_executable(lof_tests
    test_main.cpp
    test_fixed.cpp
    test_intersect.cpp
    test_sim_determinism.cpp
    test_stroke.cpp
    test_pose.cpp)
target_link_libraries(lof_tests PRIVATE lof_render_core doctest::doctest)
add_test(NAME lof_tests COMMAND lof_tests)
__EOF__

mkdir -p "$(dirname "tests/test_pose.cpp")"
cat > "tests/test_pose.cpp" << '__EOF__'
#include <doctest/doctest.h>

#include <cmath>

#include "render/Palette.hpp"
#include "render/Pose.hpp"
#include "render/Scarf.hpp"
#include "render/Stroke.hpp"

using namespace lof::render;

namespace {
float dist(Pt a, Pt b) { return std::sqrt((a.x - b.x) * (a.x - b.x) + (a.y - b.y) * (a.y - b.y)); }
}  // namespace

TEST_CASE("two-bone IK keeps bone lengths") {
    const Pt root{0.0f, 0.0f};
    const Pt target{0.3f, 0.7f};
    const Pt knee = solve_two_bone(root, target, 0.45f, 0.45f, 1.0f);
    CHECK(dist(root, knee) == doctest::Approx(0.45f).epsilon(0.01));
    CHECK(dist(knee, target) == doctest::Approx(0.45f).epsilon(0.01));
    CHECK(knee.x > 0.0f);  // bends toward +x

    const Pt knee_left = solve_two_bone(root, target, 0.45f, 0.45f, -1.0f);
    CHECK(knee_left.x < knee.x);
}

TEST_CASE("idle pose is upright and anatomically consistent") {
    PoseInput in;
    in.feet = {10.0f, 20.0f};
    const Pose p = make_pose(in);

    CHECK(p.head.y < p.neck.y);
    CHECK(p.neck.y < p.hip.y);
    CHECK(p.hip.y < 20.0f);
    for (int i = 0; i < 2; ++i) {
        CHECK(dist(p.hip, p.knee[i]) == doctest::Approx(kThigh).epsilon(0.01));
        CHECK(dist(p.knee[i], p.foot[i]) == doctest::Approx(kShin).epsilon(0.01));
        CHECK(dist(p.shoulder, p.elbow[i]) == doctest::Approx(kUpperArm).epsilon(0.01));
        CHECK(dist(p.elbow[i], p.hand[i]) == doctest::Approx(kForearm).epsilon(0.01));
    }
}

TEST_CASE("facing mirrors the pose") {
    PoseInput right;
    right.feet = {10.0f, 20.0f};
    right.facing = 1;
    PoseInput left = right;
    left.facing = -1;

    const Pose a = make_pose(right);
    const Pose b = make_pose(left);
    CHECK((a.head.x - 10.0f) == doctest::Approx(-(b.head.x - 10.0f)).epsilon(0.01));
    CHECK((a.knee[0].x - 10.0f) == doctest::Approx(-(b.knee[0].x - 10.0f)).epsilon(0.01));
    CHECK(a.head.y == doctest::Approx(b.head.y));
}

TEST_CASE("running lifts the swinging foot, feet never go below the ground") {
    PoseInput in;
    in.feet = {0.0f, 20.0f};  // phase 0: leg 0 lifted
    in.vx = 12.0f;
    const Pose p = make_pose(in);
    CHECK(p.foot[0].y < p.foot[1].y);
    CHECK(p.foot[0].y <= 20.001f);
    CHECK(p.foot[1].y <= 20.001f);
}

TEST_CASE("airborne poses differ between rising and falling") {
    PoseInput rise;
    rise.feet = {10.0f, 18.0f};
    rise.grounded = false;
    rise.vy = -25.0f;
    PoseInput fall = rise;
    fall.vy = 25.0f;
    const Pose a = make_pose(rise);
    const Pose b = make_pose(fall);
    CHECK(a.hand[0].y < a.shoulder.y);  // hands up when rising
    CHECK(b.hand[0].y > b.shoulder.y - 0.01f);
}

TEST_CASE("pose is deterministic") {
    PoseInput in;
    in.feet = {3.0f, 20.0f};
    in.vx = 7.0f;
    in.time_s = 2.5f;
    const Pose a = make_pose(in);
    const Pose b = make_pose(in);
    CHECK(a.head.x == b.head.x);
    CHECK(a.knee[1].y == b.knee[1].y);
}

TEST_CASE("every player gets a distinct hat and colour") {
    for (std::size_t i = 0; i < 4; ++i) {
        for (std::size_t j = i + 1; j < 4; ++j) {
            CHECK(player_style(i).hat != player_style(j).hat);
            const Color a = player_style(i).color;
            const Color b = player_style(j).color;
            CHECK((a.r != b.r || a.g != b.g || a.b != b.b));
        }
    }
    for (std::size_t i = 0; i < 4; ++i) {
        CHECK(hat_shape(player_style(i).hat).crown.size() >= 3);
    }
}

TEST_CASE("scarf keeps segment length and lags behind a moving anchor") {
    Scarf scarf;
    Pt anchor{0.0f, 0.0f};
    for (int i = 0; i < 90; ++i) {
        anchor.x += 0.2f;  // 12 units/s at 60 fps
        scarf.update(anchor, 1, 1.0f / 60.0f);
    }
    for (std::size_t i = 1; i < scarf.p.size(); ++i) {
        CHECK(dist(scarf.p[i], scarf.p[i - 1]) == doctest::Approx(Scarf::kSegment).epsilon(0.1));
    }
    CHECK(scarf.p.back().x < anchor.x - 0.5f);  // trailing behind
}

TEST_CASE("scarf hangs near its rest shape when the anchor is still") {
    Scarf scarf;
    for (int i = 0; i < 240; ++i) {
        scarf.update({5.0f, 5.0f}, 1, 1.0f / 60.0f);
    }
    CHECK(scarf.p.back().x < 5.0f);  // facing right, scarf trails to the left
}
__EOF__

mkdir -p "$(dirname "tests/test_sim_determinism.cpp")"
cat > "tests/test_sim_determinism.cpp" << '__EOF__'
#include <doctest/doctest.h>

#include <array>
#include <cstdint>

#include "sim/Step.hpp"
#include "sim/World.hpp"

using namespace lof;
using namespace lof::sim;

namespace {

std::array<Input, kMaxPlayers> scripted(std::uint32_t tick) {
    std::array<Input, kMaxPlayers> in{};
    in[0].move_x = ((tick / 60) % 2) != 0 ? -1 : 1;
    in[0].jump = (tick % 90) == 0;
    in[1].move_x = static_cast<std::int8_t>(static_cast<int>((tick / 45) % 3) - 1);
    in[1].jump = (tick % 70) == 0;
    return in;
}

}  // namespace

TEST_CASE("same inputs give identical state") {
    World a = make_world();
    World b = make_world();
    for (std::uint32_t t = 0; t < 600; ++t) {
        const auto in = scripted(t);
        step(a, in);
        step(b, in);
    }
    CHECK(a.tick == 600);
    CHECK(hash(a) == hash(b));
}

TEST_CASE("different inputs diverge") {
    World a = make_world();
    World b = make_world();
    for (std::uint32_t t = 0; t < 120; ++t) {
        step(a, scripted(t));
        step(b, std::array<Input, kMaxPlayers>{});
    }
    CHECK(hash(a) != hash(b));
}

TEST_CASE("players settle on the ground") {
    World w = make_world();
    std::array<Input, kMaxPlayers> jump{};
    jump[0].jump = true;
    step(w, jump);
    CHECK_FALSE(w.players[0].grounded);

    for (int t = 0; t < 200; ++t) {
        step(w, std::array<Input, kMaxPlayers>{});
    }
    for (const Player& p : w.players) {
        CHECK(p.grounded);
        CHECK(p.pos.y == kGroundY);
    }
}

TEST_CASE("facing follows the last horizontal input") {
    World w = make_world();
    CHECK(w.players[0].facing == 1);
    std::array<Input, kMaxPlayers> in{};
    in[0].move_x = -1;
    step(w, in);
    CHECK(w.players[0].facing == -1);
    in[0].move_x = 0;
    step(w, in);
    CHECK(w.players[0].facing == -1);  // standing still keeps the last direction
    in[0].move_x = 1;
    step(w, in);
    CHECK(w.players[0].facing == 1);
}
__EOF__

mkdir -p "$(dirname "tests/test_stroke.cpp")"
cat > "tests/test_stroke.cpp" << '__EOF__'
#include <doctest/doctest.h>

#include <cmath>
#include <vector>

#include "render/Stroke.hpp"

using namespace lof::render;

TEST_CASE("pencil_line is deterministic per seed") {
    std::vector<Pt> p;
    std::vector<Pt> q;
    std::vector<Pt> r;
    pencil_line(p, {0.0f, 0.0f}, {100.0f, 0.0f}, 7, 2.0f);
    pencil_line(q, {0.0f, 0.0f}, {100.0f, 0.0f}, 7, 2.0f);
    pencil_line(r, {0.0f, 0.0f}, {100.0f, 0.0f}, 8, 2.0f);

    REQUIRE(p.size() >= 2);
    REQUIRE(p.size() == q.size());
    REQUIRE(p.size() == r.size());

    bool differs = false;
    for (std::size_t i = 0; i < p.size(); ++i) {
        CHECK(p[i].x == q[i].x);
        CHECK(p[i].y == q[i].y);
        CHECK(std::fabs(p[i].y) <= 2.001f);
        if (p[i].y != r[i].y) {
            differs = true;
        }
    }
    CHECK(differs);
}

TEST_CASE("pencil_circle stays near its radius and overlaps its start") {
    std::vector<Pt> pts;
    pencil_circle(pts, {0.0f, 0.0f}, 1.0f, 3, 0.0f, 24);
    REQUIRE(pts.size() == 26);
    for (const Pt& p : pts) {
        const float r = std::sqrt(p.x * p.x + p.y * p.y);
        CHECK(r >= 0.999f);
        CHECK(r <= 1.05f);
    }
}

TEST_CASE("hatch_convex fills a square with evenly spaced lines") {
    const std::vector<Pt> square = {{0.0f, 0.0f}, {10.0f, 0.0f}, {10.0f, 10.0f}, {0.0f, 10.0f}};
    std::vector<Seg> lines;
    hatch_convex(lines, square, 0.0f, 2.0f);
    REQUIRE(lines.size() == 5);
    for (const Seg& s : lines) {
        const float len = std::fabs(s.b.x - s.a.x);
        CHECK(len == doctest::Approx(10.0f).epsilon(0.001));
    }
}
__EOF__

mkdir -p "$(dirname "tools/preview.cpp")"
cat > "tools/preview.cpp" << '__EOF__'
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
__EOF__

echo "Updated ${#FILES[@]} files (originals saved in $BACKUP/)."
echo
echo "Rebuild (CMake files changed, so re-run configure):"
echo "  cmake --preset debug && cmake --build --preset debug && ctest --preset debug"
echo "  ./build/debug/line_of_fire"
echo
echo "Optional preview image of every pose (no window needed):"
echo "  cmake --preset debug -DLOF_BUILD_TOOLS=ON && cmake --build --preset debug"
echo "  ./build/debug/character_preview preview.bmp"