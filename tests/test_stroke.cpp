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

TEST_CASE("pencil_circle stays near its radius and overlaps its start") {
    std::vector<Pt> pts;
    pencil_circle(pts, {0.0f, 0.0f}, 1.0f, 3, 0.0f, 24);
    REQUIRE(pts.size() == 26);
    for (const Pt& p : pts) {
        const float r = std::sqrt(p.x * p.x + p.y * p.y);
        CHECK(r >= 0.999f);
        CHECK(r <= 1.05f);
    }
}

TEST_CASE("hatch_convex fills a square with evenly spaced lines") {
    const std::vector<Pt> square = {{0.0f, 0.0f}, {10.0f, 0.0f}, {10.0f, 10.0f}, {0.0f, 10.0f}};
    std::vector<Seg> lines;
    hatch_convex(lines, square, 0.0f, 2.0f);
    REQUIRE(lines.size() == 5);
    for (const Seg& s : lines) {
        const float len = std::fabs(s.b.x - s.a.x);
        CHECK(len == doctest::Approx(10.0f).epsilon(0.001));
    }
}
