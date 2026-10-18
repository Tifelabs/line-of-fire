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
