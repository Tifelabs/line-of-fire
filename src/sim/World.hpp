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
