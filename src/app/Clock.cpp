#include "app/Clock.hpp"

#include <SDL3/SDL.h>

#include <algorithm>

namespace lof::app {

Clock::Clock() : last_ns_(SDL_GetTicksNS()) {}

int Clock::advance() {
    const std::uint64_t now = SDL_GetTicksNS();
    // Clamp so a long stall (debugger, window drag) doesn't trigger a huge catch-up.
    const std::uint64_t dt = std::min<std::uint64_t>(now - last_ns_, 250'000'000ULL);
    last_ns_ = now;
    acc_ns_ += dt;

    int ticks = 0;
    while (acc_ns_ >= kTickNs) {
        acc_ns_ -= kTickNs;
        ++ticks;
    }
    return ticks;
}

}  // namespace lof::app
