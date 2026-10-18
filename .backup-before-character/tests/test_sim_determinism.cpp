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
