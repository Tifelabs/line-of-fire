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
