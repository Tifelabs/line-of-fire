#include <doctest/doctest.h>

#include "geometry/Intersect.hpp"

using namespace lof;

namespace {
Fixed i(int v) { return Fixed::from_int(v); }
}  // namespace

TEST_CASE("segments crossing, parallel, touching") {
    const Segment s1{{i(0), i(0)}, {i(10), i(10)}};
    const Segment s2{{i(0), i(10)}, {i(10), i(0)}};
    const Segment parallel{{i(0), i(1)}, {i(10), i(11)}};
    const Segment touching{{i(10), i(10)}, {i(15), i(5)}};

    CHECK(segments_intersect(s1, s2));
    CHECK_FALSE(segments_intersect(s1, parallel));
    CHECK(segments_intersect(s1, touching));
}

TEST_CASE("reflection off a floor") {
    const Vec2 d{i(1), i(1)};
    const Vec2 floor_normal{i(0), i(-1)};
    const Vec2 out = reflect(d, floor_normal);
    CHECK(out == Vec2{i(1), i(-1)});
}

TEST_CASE("segment vs circle") {
    const Circle c{{i(5), i(5)}, i(2)};
    CHECK(segment_hits_circle({{i(0), i(5)}, {i(10), i(5)}}, c));
    CHECK_FALSE(segment_hits_circle({{i(0), i(0)}, {i(10), i(0)}}, c));
}
