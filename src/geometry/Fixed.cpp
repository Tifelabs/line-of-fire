#include "geometry/Fixed.hpp"

namespace lof {

Fixed sqrt(Fixed x) {
    if (x.raw <= 0) {
        return Fixed{};
    }
    // sqrt(raw / 2^12) * 2^12 == sqrt(raw * 2^12)
    std::uint64_t n = static_cast<std::uint64_t>(x.raw) << Fixed::kFrac;
    std::uint64_t r = 0;
    std::uint64_t bit = 1ULL << 62;
    while (bit > n) {
        bit >>= 2;
    }
    while (bit != 0) {
        if (n >= r + bit) {
            n -= r + bit;
            r = (r >> 1) + bit;
        } else {
            r >>= 1;
        }
        bit >>= 2;
    }
    return Fixed::from_raw(static_cast<std::int32_t>(r));
}

}  // namespace lof
