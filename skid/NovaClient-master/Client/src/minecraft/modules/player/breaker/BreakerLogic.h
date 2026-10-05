#pragma once
#include "minecraft/java/api/game_entity.h"
#include <algorithm>
#include <cmath>
#include <limits>
#include <optional>
#include <vector>

namespace nova::module::breaker {
using Pos = java::BlockPos;
using Vec = rotation::Vec3;
inline bool Same(Pos a, Pos b) { return a.x == b.x && a.y == b.y && a.z == b.z; }
inline Pos Floor(Vec v) { return {int(std::floor(v.x)), int(std::floor(v.y)), int(std::floor(v.z))}; }
inline Vec Center(Pos p) { return {p.x + .5, p.y + .5, p.z + .5}; }
inline Pos Offset(Pos p, int x, int y, int z) { return {p.x + x, p.y + y, p.z + z}; }
inline double DistanceSquared(Vec a, Vec b) {
    return (a.x - b.x) * (a.x - b.x) + (a.y - b.y) * (a.y - b.y) + (a.z - b.z) * (a.z - b.z);
}
inline double DistanceToBlock(Vec v, Pos p) {
    const double x = std::max({p.x - v.x, 0., v.x - p.x - 1}), y = std::max({p.y - v.y, 0., v.y - p.y - 1}),
                 z = std::max({p.z - v.z, 0., v.z - p.z - 1});
    return std::sqrt(x * x + y * y + z * z);
}
struct Block {
    bool valid = false, bed = false, air = false, liquid = false, passable = false;
    float hardness = -1, relative = 0;
};
struct Settings {
    double range = 4.5;
    int mode = 0, layers = 3, movement = 0;
    bool bed = true, keepProgress = true, cancelVelocity = true, delayVelocity = false, through = true,
         empty = false, full = false, stand = false, heypixel = false, groundSpoof = false, attack = true,
         percentage = true, rotate = true, onlyEnds = true, own = true, slowAir = true;
};
struct Candidate {
    Pos bed;
    std::vector<Pos> blocks;
    double cost = 0;
};
inline std::vector<Pos> Trace(Vec from, Vec to) {
    std::vector<Pos> out;
    Pos p = Floor(from), end = Floor(to);
    const double d[] = {to.x - from.x, to.y - from.y, to.z - from.z}, v[] = {from.x, from.y, from.z};
    int step[3];
    double delta[3], next[3];
    const int cell[] = {p.x, p.y, p.z};
    for (int i = 0; i < 3; ++i) {
        step[i] = (d[i] > 0) - (d[i] < 0);
        delta[i] = step[i] ? 1 / std::abs(d[i]) : std::numeric_limits<double>::max();
        next[i] = step[i] ? (step[i] > 0 ? cell[i] + 1 - v[i] : v[i] - cell[i]) / std::abs(d[i])
                          : std::numeric_limits<double>::max();
    }
    out.push_back(p);
    while (!Same(p, end) && out.size() < 64) {
        if (next[0] <= next[1] && next[0] <= next[2]) {
            p.x += step[0];
            next[0] += delta[0];
        } else if (next[1] <= next[2]) {
            p.y += step[1];
            next[1] += delta[1];
        } else {
            p.z += step[2];
            next[2] += delta[2];
        }
        out.push_back(p);
    }
    return out;
}
template <class World> std::vector<Pos> Beds(World &w, Vec player) {
    std::vector<Pos> out;
    auto base = Floor(player);
    for (int x = -5; x <= 5; ++x)
        for (int y = -5; y <= 5; ++y)
            for (int z = -5; z <= 5; ++z) {
                auto p = Offset(base, x, y, z);
                if (w.Read(p).bed)
                    out.push_back(p);
            }
    return out;
}
template <class World>
std::vector<Candidate> Candidates(World &w, Vec eye, const std::vector<Pos> &beds, const Settings &s) {
    std::vector<Candidate> out;
    constexpr double samples[][3] = {{.5, .5, .5}, {.1, .1, .1}, {.9, .1, .9}, {.1, .5, .9}, {.9, .5, .1}};
    for (auto bed : beds)
        for (auto &sample : samples) {
            Candidate c{bed};
            for (auto p : Trace(eye, {bed.x + sample[0], bed.y + sample[1], bed.z + sample[2]})) {
                if (DistanceToBlock(eye, p) > s.range)
                    break;
                if (Same(p, bed)) {
                    c.blocks.push_back(p);
                    out.push_back(std::move(c));
                    break;
                }
                const auto b = w.Read(p);
                if (!b.valid)
                    break;
                if (!b.passable) {
                    if (b.bed || b.hardness < 0 || int(c.blocks.size()) >= s.layers)
                        break;
                    c.blocks.push_back(p);
                    c.cost += 1.f / std::max(b.relative, 1.e-4f);
                }
            }
        }
    return out;
}
inline const Candidate *Best(const std::vector<Candidate> &candidates) {
    const Candidate *result = nullptr;
    for (const auto &c : candidates)
        if (!result || c.cost < result->cost)
            result = &c;
    return result;
}
template <class World>
std::optional<Vec> Find(World &w, Vec player, Vec eye, const Settings &s, std::optional<Vec> home) {
    if (!s.bed || (s.own && home && DistanceSquared(player, *home) < 1225))
        return {};
    const auto beds = Beds(w, player);
    if (s.through && s.empty && s.full) {
        const auto candidates = Candidates(w, eye, beds, s);
        const auto *best = Best(candidates);
        return best ? std::optional<Vec>(Center(best->blocks.front())) : std::nullopt;
    }
    // Rise deliberately ignores the first bed half in scan order.
    for (size_t i = 1; i < beds.size(); ++i) {
        const auto p = beds[i];
        const Vec raw{player.x + p.x - Floor(player).x, player.y + p.y - Floor(player).y,
                      player.z + p.z - Floor(player).z};
        java::BlockHitResultInfo hit;
        if (!w.Ray(eye, raw, s.range + 1, hit))
            continue;
        const Vec below{player.x, player.y - (eye.y - player.y), player.z};
        if (DistanceSquared(hit.location, below) > (s.range + 2) * (s.range + 2))
            continue;
        if (!s.through) {
            if (hit.hasBlockPos && Same(hit.blockPos, p))
                return raw;
            continue;
        }
        if (!s.empty)
            return raw;
        bool exposed = false;
        std::optional<Pos> best;
        double hardness = std::numeric_limits<double>::max();
        const int radius = s.heypixel ? 0 : 4;
        constexpr int sides[][3] = {{0, -1, 0}, {0, 0, -1}, {0, 0, 1}, {-1, 0, 0}, {1, 0, 0}};
        for (int x = -radius; x <= radius && !exposed; ++x)
            for (int y = s.heypixel ? 1 : 0; y <= 1 && !exposed; ++y)
                for (int z = -radius; z <= radius && !exposed; ++z) {
                    auto at = Offset(p, x, y, z);
                    const auto b = w.Read(at);
                    if (!b.valid || b.bed)
                        continue;
                    bool adjacent = false;
                    for (auto &side : sides)
                        adjacent |= w.Read(Offset(at, side[0], side[1], side[2])).bed;
                    if (!adjacent)
                        continue;
                    if (b.air || b.liquid) {
                        exposed = true;
                        break;
                    }
                    const Vec point{raw.x + x, raw.y + y, raw.z + z};
                    if (DistanceSquared(point, eye) > (s.range + 2) * (s.range + 2))
                        continue;
                    if (b.hardness < hardness) {
                        hardness = b.hardness;
                        best = at;
                    }
                }
        if (exposed)
            return raw;
        // The decompiled source compares akax2 to itself. Preserve its intended
        // cheapest adjacent defense selection instead of the unconditional null.
        if (best)
            return Center(*best);
    }
    return {};
}
template <class World> std::optional<Pos> Stand(World &w, Vec player, double height, const Settings &s) {
    auto beds = Beds(w, player);
    if (beds.empty())
        return {};
    const int r = int(std::ceil(s.range)) + 1;
    std::optional<Pos> result;
    double cost = 0, distance = 0;
    for (int x = -r; x <= r; ++x)
        for (int y = -2; y <= 2; ++y)
            for (int z = -r; z <= r; ++z) {
                auto p = Offset(beds.front(), x, y, z);
                Vec eye{p.x + .5, p.y + height, p.z + .5};
                if (std::none_of(beds.begin(), beds.end(),
                                 [&](Pos bed) { return DistanceToBlock(eye, bed) <= s.range; }))
                    continue;
                const auto feet = w.Read(p), head = w.Read(Offset(p, 0, 1, 0)),
                           support = w.Read(Offset(p, 0, -1, 0));
                if (!feet.valid || !head.valid || !support.valid || !feet.passable || !head.passable ||
                    support.passable)
                    continue;
                auto candidates = Candidates(w, eye, beds, s);
                auto *best = Best(candidates);
                if (!best)
                    continue;
                const double dist = DistanceSquared(player, {p.x + .5, double(p.y), p.z + .5});
                if (!result || best->cost < cost - .001 ||
                    (std::abs(best->cost - cost) <= .001 && dist < distance)) {
                    result = p;
                    cost = best->cost;
                    distance = dist;
                }
            }
    return result;
}
} // namespace nova::module::breaker
