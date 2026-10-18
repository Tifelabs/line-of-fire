#pragma once
#include <compare>
#include <cstdint>

namespace lof {

// Q20.12 fixed-point number. Deterministic on every platform (integer math only).
// Range is about +-524288; squares overflow past ~724, so keep distances small.
struct Fixed {
    static constexpr int kFrac = 12;
    static constexpr std::int32_t kOne = 1 << kFrac;

    std::int32_t raw = 0;

    constexpr Fixed() = default;

    static constexpr Fixed from_raw(std::int32_t r) {
        Fixed f;
        f.raw = r;
        return f;
    }
    static constexpr Fixed from_int(std::int32_t v) { return from_raw(v * kOne); }
    static constexpr Fixed from_ratio(std::int32_t num, std::int32_t den) {
        return from_raw(static_cast<std::int32_t>((static_cast<std::int64_t>(num) * kOne) / den));
    }

    constexpr std::int32_t to_int() const { return raw >> kFrac; }  // floor

    constexpr Fixed operator-() const { return from_raw(-raw); }
    constexpr Fixed& operator+=(Fixed o) {
        raw += o.raw;
        return *this;
    }
    constexpr Fixed& operator-=(Fixed o) {
        raw -= o.raw;
        return *this;
    }

    friend constexpr Fixed operator+(Fixed a, Fixed b) { return from_raw(a.raw + b.raw); }
    friend constexpr Fixed operator-(Fixed a, Fixed b) { return from_raw(a.raw - b.raw); }
    friend constexpr Fixed operator*(Fixed a, Fixed b) {
        return from_raw(
            static_cast<std::int32_t>((static_cast<std::int64_t>(a.raw) * b.raw) >> kFrac));
    }
    friend constexpr Fixed operator/(Fixed a, Fixed b) {
        return from_raw(
            static_cast<std::int32_t>((static_cast<std::int64_t>(a.raw) * kOne) / b.raw));
    }

    auto operator<=>(const Fixed&) const = default;
};

constexpr Fixed abs(Fixed v) { return v.raw < 0 ? -v : v; }

// Integer square root in fixed point. Returns 0 for x <= 0.
Fixed sqrt(Fixed x);

}  // namespace lof
