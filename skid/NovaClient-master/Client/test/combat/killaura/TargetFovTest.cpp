#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <vector>
namespace rotation {
struct Vec3 {};
struct AimFrame {};
struct Rotation { float yaw; };
namespace mth { float WrapDegrees(float f) { return std::remainder(f, 360.0f); } }
void ClearRaycast() {}
}
namespace java {
struct EntityHandle {
    float yaw = 0;
    bool Position(rotation::Vec3&) const { return true; }
    bool Health(float& health, float& absorption) const { health = 20; absorption = 0; return true; }
    bool DistanceTo(const EntityHandle&, double& out) const { out = 2; return true; }
};
std::vector<EntityHandle> world;
bool WorldEntities(const EntityHandle&, int, std::vector<EntityHandle>& out) { out = world; return true; }
}
namespace rotation {
Rotation CalculateEntity(const java::EntityHandle& e) { return {e.yaw}; }
double EntityDistance(Vec3, const java::EntityHandle&) { return 2; }
}
struct Property { int get() const { return 1; } };
struct Fixture {
    struct Candidate { java::EntityHandle entity; double sortKey = 0; };
    Property prop; Property* priority_ = &prop;
    std::vector<Candidate> targets_;
    bool IsTarget(const java::EntityHandle&, const java::EntityHandle&) { return true; }
    bool IsInvisiblePlayer(const java::EntityHandle&) { return false; }
    int SearchArea(rotation::Vec3, double) { return 0; }
    // @PRODUCTION_TARGET_METHODS@
};
int main() {
    Fixture f;
    // The old pitch-vs-yaw comparison lost all targets facing west/south.
    for (float yaw : {-179.f, -150.f, -90.f, 0.f, 90.f, 150.f, 179.f}) {
        java::world = {{yaw}, {yaw + 20}, {yaw - 20}, {yaw + 100}, {yaw - 100}};
        assert(f.CollectTargets({}, {}, yaw, 3, 30, false));
        assert(f.targets_.size() == 3);
        f.SortTargets({}, {}, yaw, 3);
        assert(f.targets_[0].entity.yaw == yaw);
        assert(f.targets_[1].sortKey == 20 && f.targets_[2].sortKey == 20);
    }
    java::world = {{-179}, {170}};
    f.CollectTargets({}, {}, 179, 3, 5, false);
    assert(f.targets_.size() == 1 && f.targets_[0].entity.yaw == -179);
    std::puts("KillAura production FOV filtering/sorting: all headings, symmetric limits and yaw wrap passed");
}
