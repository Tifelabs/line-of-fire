#pragma once
#include <cstdint>

namespace lof::app {

// Fixed 60 Hz timestep accumulator. Call advance() once per frame; it returns
// how many simulation ticks to run. (Render interpolation comes later.)
class Clock {
public:
    static constexpr std::uint64_t kTickNs = 1'000'000'000ULL / 60ULL;

    Clock();
    int advance();

private:
    std::uint64_t last_ns_;
    std::uint64_t acc_ns_ = 0;
};

}  // namespace lof::app
