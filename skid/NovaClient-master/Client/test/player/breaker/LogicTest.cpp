#include "minecraft/modules/player/breaker/BreakerLogic.h"
#include <cassert>
#include <iostream>
#include <map>
#include <random>
#include <tuple>
using namespace nova::module::breaker;
struct World {
    std::map<std::tuple<int, int, int>, Block> cells;
    bool occluded = false;
    Block Read(Pos p) {
        auto i = cells.find({p.x, p.y, p.z});
        return i == cells.end() ? Block{true, false, true, false, true, 0, 0} : i->second;
    }
    void Put(Pos p, Block b) { cells[{p.x, p.y, p.z}] = b; }
    bool Ray(Vec, Vec target, double, nova::java::BlockHitResultInfo &hit) {
        hit.kind = nova::java::HitKind::Block;
        hit.hasBlockPos = true;
        hit.blockPos = occluded ? Pos{0, 0, 0} : Floor(target);
        hit.location = target;
        return true;
    }
};
int main() {
    const Block bed{true, true, false, false, false, .2f, .1f},
        soft{true, false, false, false, false, 1, .1f}, hard{true, false, false, false, false, 10, .01f};
    const Vec eye{-2.5, .5, .5};
    Settings s;
    s.range = 6;
    s.own = false;
    s.empty = true;
    s.full = true;
    World world;
    world.Put({1, 0, 0}, bed);
    world.Put({2, 0, 0}, bed);
    auto exposed = Candidates(world, eye, {{1, 0, 0}}, s);
    assert(!exposed.empty());
    assert(Best(exposed)->cost == 0 && Same(Best(exposed)->blocks.front(), {1, 0, 0}));
    world.Put({0, 0, 0}, soft);
    auto covered = Candidates(world, eye, {{1, 0, 0}}, s);
    assert(!covered.empty() && std::abs(Best(covered)->cost - 10) < 1.e-5);
    assert(Same(Best(covered)->blocks.front(), {0, 0, 0}));
    world.Put({-1, 0, 0}, hard);
    s.layers = 1;
    assert(Candidates(world, eye, {{1, 0, 0}}, s).empty());
    s.layers = 2;
    auto two = Candidates(world, eye, {{1, 0, 0}}, s);
    assert(!two.empty() && Best(two)->blocks.size() == 3);
    assert(std::abs(Best(two)->cost - 110) < .001);
    auto unbreakable = hard;
    unbreakable.hardness = -1;
    world.Put({-1, 0, 0}, unbreakable);
    assert(Candidates(world, eye, {{1, 0, 0}}, s).empty());
    world.Put({-1, 0, 0}, hard);
    s.range = 1;
    assert(Candidates(world, eye, {{1, 0, 0}}, s).empty());
    s.range = 6;
    s.own = true;
    assert(!Find(world, {-2.5, 0, .5}, eye, s, Vec{-2.5, 0, .5}));
    s.own = false;
    s.bed = false;
    assert(!Find(world, {-2.5, 0, .5}, eye, s, {}));
    s.bed = true;
    s.full = false;
    s.empty = false;
    s.through = false;
    world.occluded = true;
    assert(!Find(world, {0, 0, 0}, {0, 1.62, 0}, s, {}));
    s.through = true;
    assert(Find(world, {0, 0, 0}, {0, 1.62, 0}, s, {}));
    // The fixed Single branch must actually select a covered bed's defense.
    World single;
    single.Put({1, 0, 0}, bed);
    single.Put({2, 0, 0}, bed);
    for (int x = 0; x <= 3; ++x)
        for (int y = 0; y <= 1; ++y)
            for (int z = -1; z <= 1; ++z)
                if (!single.Read({x, y, z}).bed)
                    single.Put({x, y, z}, soft);
    s.empty = true;
    s.heypixel = true;
    auto defense = Find(single, {0, 0, 0}, {0, 1.62, 0}, s, {});
    assert(defense && !single.Read(Floor(*defense)).bed && Floor(*defense).y == 1);
    // Compare DDA visits with an independent slab intersection oracle.
    std::mt19937 rng(489);
    std::uniform_real_distribution<double> random(-5, 5);
    for (int run = 0; run < 10000; ++run) {
        Vec a{random(rng), random(rng), random(rng)}, b{random(rng), random(rng), random(rng)};
        auto trace = Trace(a, b);
        assert(Same(trace.front(), Floor(a)) && Same(trace.back(), Floor(b)) && trace.size() <= 31);
        for (size_t i = 1; i < trace.size(); ++i)
            assert(std::abs(trace[i].x - trace[i - 1].x) + std::abs(trace[i].y - trace[i - 1].y) +
                       std::abs(trace[i].z - trace[i - 1].z) ==
                   1);
        for (auto p : trace) {
            double lo = 0, hi = 1;
            const double start[] = {a.x, a.y, a.z}, delta[] = {b.x - a.x, b.y - a.y, b.z - a.z};
            const int cell[] = {p.x, p.y, p.z};
            for (int axis = 0; axis < 3; ++axis) {
                double near = (cell[axis] - start[axis]) / delta[axis],
                       far = (cell[axis] + 1 - start[axis]) / delta[axis];
                if (near > far)
                    std::swap(near, far);
                lo = std::max(lo, near);
                hi = std::min(hi, far);
            }
            assert(lo <= hi + 1.e-10);
        }
    }
    assert(Trace({-.5, -.5, -.5}, {-.5, -.5, -.5}).size() == 1);
    assert(DistanceToBlock({-.5, .5, .5}, {0, 0, 0}) == .5);
    assert(DistanceToBlock({1, 1, 1}, {0, 0, 0}) == 0);
    // Stand search must reject floating cells and return a supported clear space.
    World stand;
    stand.Put({0, 0, 0}, bed);
    stand.Put({1, 0, 0}, bed);
    for (int x = -7; x <= 7; ++x)
        for (int z = -7; z <= 7; ++z)
            stand.Put({x, -1, z}, soft);
    s.full = true;
    s.heypixel = false;
    s.layers = 3;
    auto p = Stand(stand, {3, 0, 0}, 1.62, s);
    assert(p && stand.Read(*p).passable && !stand.Read(Offset(*p, 0, -1, 0)).passable);
    std::cout << "PASS: 10000 independent ray checks; exposed/covered beds, layer limits, range, unbreakable "
                 "blocks, own-bed exclusion, Single fix and stand search\n";
}
