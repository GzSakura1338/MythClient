#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <string>
#include <vector>

namespace rotation { struct Vec3 { double x = 0, y = 0, z = 0; }; }
namespace java {
std::string PresentationName(std::string_view text){return std::string(text);}
struct Box { double minX, minY, minZ, maxX, maxY, maxZ; };
constexpr unsigned kTypePlayer = 1, kTypeLiving = 2;
struct EntityHandle {
    int id = 0;
    double distance = 0;
    bool player = true, alive = true, spectator = false;
    bool Valid() const { return id != 0; }
    bool IsSame(const EntityHandle& other) const { return id == other.id; }
    bool IsAlive() const { return alive; }
    bool IsPlayer() const { return player; }
    bool IsSpectator() const { return spectator; }
    bool DistanceTo(const EntityHandle&, double& out) const { out = distance; return true; }
    bool BoundingBox(Box& out) const { out = {-.3, 0, distance-.3, .3, 1.8, distance+.3}; return true; }
    bool Position(rotation::Vec3& out) const { out = {0, 0, distance}; return true; }
    bool RenderPositionPair(rotation::Vec3& now, rotation::Vec3& before) const {
        Position(now); before = now; return true;
    }
    bool DisplayName(std::string& out) const { out = std::to_string(id); return true; }
    void Health(float& hp, float& absorption) const { hp = 20; absorption = 0; }
    bool IsAlliedTo(const EntityHandle&, bool& out) const { out = false; return true; }
};
std::vector<EntityHandle> loaded;
bool world = true, queryOk = true;
int queryCount = 0;
EntityHandle PlayerEntity() { return {world ? 1 : 0}; }
bool WorldPlayers(std::vector<EntityHandle>& out) {
    for (auto entity : loaded) if (entity.player) out.push_back(entity);
    return true;
}
bool EntityTypes(const EntityHandle& entity, unsigned& out) {
    out = kTypeLiving | (entity.player ? kTypePlayer : 0); return true;
}
bool RenderDistance(int& out) { out = 16; return true; }
bool WorldEntities(const EntityHandle&, const Box& area, std::vector<EntityHandle>& out) {
    ++queryCount;
    if (!queryOk) return false;
    for (auto entity : loaded)
        if (entity.distance >= area.minZ && entity.distance <= area.maxZ) out.push_back(entity);
    return true;
}
}
struct Setting { double value; double get() const { return value; } };
// @TAG@
struct Fixture {
    Setting limit{0}, range{48}, players{1}, mobs{0}, status{1};
    Setting *limitRange_ = &limit, *range_ = &range, *players_ = &players, *mobs_ = &mobs, *status_ = &status;
    // @METHODS@
    std::vector<Tag> collect() { std::vector<Tag> tags; Gather(tags); return tags; }
};
int main() {
    java::loaded = {{1}, {2, 12}, {3, 48}, {4, 96}, {5, 300},
        {6, 24, true, false}, {7, 24, true, true, true}, {8, 100, false}};
    Fixture module;
    auto tags = module.collect();
    assert(tags.size() == 4); // Old Range=48 cannot suppress loaded distant players by default.
    assert(tags.back().name == "5");
    assert(java::queryCount == 0); // No chunk collision query for players.
    module.limit.value = 1;
    tags = module.collect();
    assert(tags.size() == 2 && tags.back().distance == 48);
    module.range.value = 512;
    assert(module.collect().size() == 4);
    module.limit.value = 0;
    module.mobs.value = 1;
    assert(module.collect().size() == 5); // No duplicate player tags when mobs are enabled.
    java::queryOk = false;
    assert(module.collect().size() == 4); // Mob query failure cannot hide loaded players.
    java::queryOk = true;
    module.players.value = 0;
    tags = module.collect();
    assert(tags.size() == 1 && tags.front().name == "8");
    module.mobs.value = 0;
    assert(module.collect().empty());
    java::world = false;
    module.players.value = 1;
    assert(module.collect().empty());
    std::puts("PASS: distant loaded players, old 48-block setting, optional range boundary, type filters, deduplication, query failure and world exit");
}
