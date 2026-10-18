#include "geometry/Intersect.hpp"

#include <algorithm>

namespace lof {
namespace {

int sign(Fixed v) { return (v.raw > 0) - (v.raw < 0); }

int orientation(Vec2 a, Vec2 b, Vec2 c) { return sign(cross(b - a, c - a)); }

// Assumes p is collinear with a-b.
bool within_box(Vec2 a, Vec2 b, Vec2 p) {
    return std::min(a.x, b.x) <= p.x && p.x <= std::max(a.x, b.x) && std::min(a.y, b.y) <= p.y &&
           p.y <= std::max(a.y, b.y);
}

}  // namespace

bool segments_intersect(const Segment& s, const Segment& t) {
    const int o1 = orientation(s.a, s.b, t.a);
    const int o2 = orientation(s.a, s.b, t.b);
    const int o3 = orientation(t.a, t.b, s.a);
    const int o4 = orientation(t.a, t.b, s.b);

    if (o1 != o2 && o3 != o4) {
        return true;
    }
    if (o1 == 0 && within_box(s.a, s.b, t.a)) return true;
    if (o2 == 0 && within_box(s.a, s.b, t.b)) return true;
    if (o3 == 0 && within_box(t.a, t.b, s.a)) return true;
    if (o4 == 0 && within_box(t.a, t.b, s.b)) return true;
    return false;
}

bool segment_hits_circle(const Segment& s, const Circle& c) {
    const Vec2 d = s.b - s.a;
    const Fixed len2 = length_sq(d);
    Vec2 closest = s.a;
    if (len2.raw > 0) {
        Fixed t = dot(c.center - s.a, d) / len2;
        t = std::clamp(t, Fixed{}, Fixed::from_int(1));
        closest = s.a + d * t;
    }
    return length_sq(c.center - closest) <= c.radius * c.radius;
}

Vec2 reflect(Vec2 d, Vec2 unit_normal) {
    const Fixed k = dot(d, unit_normal) * Fixed::from_int(2);
    return d - unit_normal * k;
}

}  // namespace lof
