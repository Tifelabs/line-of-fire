#pragma once
#include <cstddef>
#include <cstdint>

namespace lof::sim {

inline constexpr std::size_t kMaxPlayers = 4;

// Everything a player can tell the simulation in one tick. Keep it tiny:
// this is what gets sent over the network.
struct Input {
    std::int8_t move_x = 0;  // -1, 0, +1
    bool jump = false;
};

}  // namespace lof::sim
