#pragma once
#include "geometry/Circle.hpp"
#include "geometry/Segment.hpp"

namespace lof {

// True if the two segments share at least one point (touching counts).
bool segments_intersect(const Segment& s, const Segment& t);

// True if any point of the segment is inside or on the circle.
bool segment_hits_circle(const Segment& s, const Circle& c);

// Mirror direction d about a surface with the given UNIT normal.
Vec2 reflect(Vec2 d, Vec2 unit_normal);

}  // namespace lof
