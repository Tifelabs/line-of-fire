#include "sim/World.hpp"

namespace lof::sim {

World make_world() {
    World w;
    for (std::size_t i = 0; i < kMaxPlayers; ++i) {
        w.players[i].pos = {Fixed::from_int(static_cast<std::int32_t>(5 + 5 * i)), kGroundY};
        w.players[i].grounded = true;
        w.players[i].facing = (i < 2) ? 1 : -1;  // two sides facing each other
    }
    return w;
}

namespace {

void mix(std::uint64_t& h, std::uint32_t v) {
    for (int i = 0; i < 4; ++i) {
        h ^= (v & 0xFFu);
        h *= 1099511628211ULL;
        v >>= 8;
    }
}

}  // namespace

std::uint64_t hash(const World& w) {
    std::uint64_t h = 14695981039346656037ULL;
    mix(h, w.tick);
    for (const Player& p : w.players) {
        mix(h, static_cast<std::uint32_t>(p.pos.x.raw));
        mix(h, static_cast<std::uint32_t>(p.pos.y.raw));
        mix(h, static_cast<std::uint32_t>(p.vel.x.raw));
        mix(h, static_cast<std::uint32_t>(p.vel.y.raw));
        mix(h, p.grounded ? 1u : 0u);
        mix(h, static_cast<std::uint32_t>(static_cast<int>(p.facing)));
    }
    return h;
}

}  // namespace lof::sim
