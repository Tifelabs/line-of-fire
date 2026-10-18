#pragma once
#include <array>

#include "sim/Input.hpp"
#include "sim/World.hpp"

namespace lof::sim {

// Advance the world by exactly one 60 Hz tick.
void step(World& world, const std::array<Input, kMaxPlayers>& inputs);

}  // namespace lof::sim
