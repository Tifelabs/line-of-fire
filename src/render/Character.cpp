#include "render/Character.hpp"

#include <algorithm>
#include <cmath>
#include <vector>

#include "render/Stroke.hpp"

namespace lof::render {

void draw_character(Renderer& r, const View& view, const Pose& pose, const PlayerStyle& style,
                    const Scarf& scarf, std::uint32_t seed) {
    const float f = pose.facing >= 0 ? 1.0f : -1.0f;
    const float zoom = view.cam.zoom;
    auto P = [&](Pt p) { return view.project(p); };

    const Color graphite{45, 45, 50};
    const Pen ink{style.color, 2.2f, 0.9f, 235};
    const Pen sketch{graphite, 1.0f, 2.2f, 90};
    const Pen face_pen{graphite, 1.5f, 0.0f, 235};
    const Pen fill_pen{style.color, 1.0f, 0.0f, 95};
    const Pen scarf_pen{style.color, 2.6f, 0.6f, 235};

    auto limb = [&](Pt a, Pt b, std::uint32_t s) {
        r.stroke(P(a), P(b), seed + s, sketch);
        r.stroke(P(a), P(b), seed + s + 50u, ink);
    };
    auto pin = [&](Pt j) { r.dot(P(j), 0.035f * zoom, graphite, 200); };

    // --- scarf (behind everything) ---
    {
        std::vector<Pt> pts;
        pts.reserve(scarf.p.size());
        for (const Pt& q : scarf.p) {
            pts.push_back(P(q));
        }
        r.path(pts, seed + 400u, scarf_pen);
        r.dot(P(pose.neck), 0.055f * zoom, style.color, 240);
    }

    auto leg = [&](int i, std::uint32_t s) {
        limb(pose.hip, pose.knee[i], s);
        limb(pose.knee[i], pose.foot[i], s + 1u);
        pin(pose.knee[i]);
    };
    auto arm = [&](int i, std::uint32_t s) {
        limb(pose.shoulder, pose.elbow[i], s);
        limb(pose.elbow[i], pose.hand[i], s + 1u);
        pin(pose.elbow[i]);
        r.dot(P(pose.hand[i]), 0.03f * zoom, style.color, 220);
    };

    // --- back limbs, torso, front limbs ---
    arm(1, 10u);
    leg(1, 20u);
    limb(pose.hip, pose.neck, 30u);
    leg(0, 40u);
    arm(0, 50u);
    pin(pose.hip);
    pin(pose.shoulder);

    // --- head: pencil circle + colored hatching ---
    {
        std::vector<Pt> circle;
        pencil_circle(circle, pose.head, pose.head_r, seed + 300u, 0.010f, 26);
        std::vector<Pt> screen;
        for (const Pt& q : circle) screen.push_back(P(q));
        r.polyline(screen, ink);

        std::vector<Pt> circle2;
        pencil_circle(circle2, pose.head, pose.head_r, seed + 301u, 0.022f, 22);
        std::vector<Pt> screen2;
        for (const Pt& q : circle2) screen2.push_back(P(q));
        r.polyline(screen2, sketch);

        std::vector<Pt> poly;
        for (int i = 0; i < 18; ++i) {
            const float a = 6.2831853f * static_cast<float>(i) / 18.0f;
            poly.push_back({pose.head.x + std::cos(a) * pose.head_r * 0.92f,
                            pose.head.y + std::sin(a) * pose.head_r * 0.92f});
        }
        std::vector<Seg> hatch;
        hatch_convex(hatch, poly, -0.8f, 0.06f);
        for (const Seg& s : hatch) {
            r.polyline({P(s.a), P(s.b)}, fill_pen);
        }
    }

    // --- hat (basic geometric shape) ---
    {
        const HatShape hat = hat_shape(style.hat);
        const Pt top{pose.head.x, pose.head.y - pose.head_r};
        std::vector<Pt> crown;
        for (const Pt& q : hat.crown) crown.push_back(top + q);

        std::vector<Seg> hatch;
        hatch_convex(hatch, crown, 0.8f, 0.05f);
        for (const Seg& s : hatch) {
            r.polyline({P(s.a), P(s.b)}, fill_pen);
        }

        std::vector<Pt> loop;
        for (const Pt& q : crown) loop.push_back(P(q));
        loop.push_back(P(crown.front()));
        r.path(loop, seed + 500u, ink);
        if (hat.has_brim) {
            r.stroke(P(top + hat.brim_a), P(top + hat.brim_b), seed + 520u, ink);
        }
    }

    // --- face ---
    {
        const Pt h = pose.head;
        const float ly = 0.02f * pose.look_y;
        const Pt eye0{h.x + f * 0.03f, h.y - 0.02f + ly};
        const Pt eye1{h.x + f * 0.16f, h.y - 0.02f + ly};
        const float eye_px = 0.028f * zoom;
        const Pt eyes[2] = {eye0, eye1};

        for (const Pt& e : eyes) {
            if (pose.expression == Expression::Dead) {
                const float k = 0.035f;
                r.polyline({P({e.x - k, e.y - k}), P({e.x + k, e.y + k})}, face_pen);
                r.polyline({P({e.x - k, e.y + k}), P({e.x + k, e.y - k})}, face_pen);
            } else if (pose.blink) {
                r.polyline({P({e.x - 0.04f, e.y}), P({e.x + 0.04f, e.y})}, face_pen);
            } else {
                r.dot(P(e), eye_px, graphite, 240);
            }
        }

        // brows: back end / front end heights (negative = higher)
        float back = -0.115f;
        float front = -0.115f;
        bool brows = true;
        switch (pose.expression) {
            case Expression::Neutral: break;
            case Expression::Determined: back = -0.14f; front = -0.085f; break;
            case Expression::Hurt: back = -0.085f; front = -0.14f; break;
            case Expression::Victory: back = -0.15f; front = -0.15f; break;
            case Expression::Dead: brows = false; break;
        }
        if (brows) {
            for (const Pt& e : eyes) {
                r.polyline({P({e.x - f * 0.04f, h.y + back + ly}),
                            P({e.x + f * 0.04f, h.y + front + ly})},
                           face_pen);
            }
        }

        // mouth
        const Pt m{h.x + f * 0.12f, h.y + 0.12f};
        std::vector<Pt> mouth;
        switch (pose.expression) {
            case Expression::Neutral:
            case Expression::Dead:
                mouth = {{m.x - f * 0.05f, m.y}, {m.x + f * 0.05f, m.y}};
                break;
            case Expression::Determined:
                mouth = {{m.x - f * 0.05f, m.y + 0.01f}, {m.x + f * 0.05f, m.y - 0.01f}};
                break;
            case Expression::Hurt:
                mouth = {{m.x - f * 0.06f, m.y},
                         {m.x - f * 0.02f, m.y - 0.025f},
                         {m.x + f * 0.02f, m.y + 0.025f},
                         {m.x + f * 0.06f, m.y}};
                break;
            case Expression::Victory:
                for (int i = 0; i <= 6; ++i) {
                    const float u = static_cast<float>(i) / 6.0f * 2.0f - 1.0f;
                    mouth.push_back({m.x + f * 0.075f * u, m.y + 0.05f * (1.0f - u * u)});
                }
                break;
        }
        std::vector<Pt> mouth_screen;
        for (const Pt& q : mouth) mouth_screen.push_back(P(q));
        r.polyline(mouth_screen, face_pen);
    }
}

}  // namespace lof::render
