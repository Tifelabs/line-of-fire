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
