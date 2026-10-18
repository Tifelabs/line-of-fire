#include <doctest/doctest.h>

#include <cmath>
#include <vector>

#include "render/Stroke.hpp"

using namespace lof::render;

TEST_CASE("pencil_line is deterministic per seed") {
    std::vector<Pt> p;
    std::vector<Pt> q;
    std::vector<Pt> r;
    pencil_line(p, {0.0f, 0.0f}, {100.0f, 0.0f}, 7, 2.0f);
    pencil_line(q, {0.0f, 0.0f}, {100.0f, 0.0f}, 7, 2.0f);
    pencil_line(r, {0.0f, 0.0f}, {100.0f, 0.0f}, 8, 2.0f);

    REQUIRE(p.size() >= 2);
    REQUIRE(p.size() == q.size());
    REQUIRE(p.size() == r.size());

    bool differs = false;
    for (std::size_t i = 0; i < p.size(); ++i) {
        CHECK(p[i].x == q[i].x);
        CHECK(p[i].y == q[i].y);
        CHECK(std::fabs(p[i].y) <= 2.001f);
        if (p[i].y != r[i].y) {
            differs = true;
        }
    }
    CHECK(differs);
}
