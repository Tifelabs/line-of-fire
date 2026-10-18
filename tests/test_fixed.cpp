#include <doctest/doctest.h>

#include <cstdlib>

#include "geometry/Fixed.hpp"

using lof::Fixed;

TEST_CASE("fixed arithmetic") {
    const Fixed a = Fixed::from_int(3);
    const Fixed b = Fixed::from_int(4);
    CHECK((a + b) == Fixed::from_int(7));
    CHECK((a - b) == Fixed::from_int(-1));
    CHECK((a * b) == Fixed::from_int(12));
    CHECK((Fixed::from_int(12) / b) == a);
    CHECK(Fixed::from_ratio(1, 2).raw == Fixed::kOne / 2);
    CHECK(a < b);
}

TEST_CASE("fixed sqrt") {
    CHECK(lof::sqrt(Fixed::from_int(4)) == Fixed::from_int(2));
    CHECK(lof::sqrt(Fixed::from_int(-1)) == Fixed{});

    const Fixed r = lof::sqrt(Fixed::from_int(2));
    const Fixed err = r * r - Fixed::from_int(2);
    CHECK(std::abs(err.raw) < 16);
}
