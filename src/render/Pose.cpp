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
