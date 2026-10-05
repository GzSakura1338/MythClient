#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <limits>
#include <utility>
#include <vector>
#include "minecraft/utils/rotation/Vec3.h"

namespace nova {
namespace rotation {
struct Rotation { float yaw = 0, pitch = 0; };
namespace mth { float WrapDegrees(float v) { return std::remainder(v, 360.f); } }
enum class RotationPriority { Medium, Highest };
struct RotationState {
    Rotation rotations;
    RotationPriority priority = RotationPriority::Medium;
    bool moveInputFix = true, direct = false;
} state;
RotationState& State() { return state; }
Rotation GetRotation() { return state.rotations; }
bool SetDirectRotations(Rotation v) { state.rotations = v; state.direct = true; return true; }
}
namespace java {
using rotation::Vec3;
struct Box { double minX, minY, minZ, maxX, maxY, maxZ; };
struct Entity {
    Box box;
    Vec3 position, eye, previousEye;
    bool spectator = false, pickable = true, riding = false, player = true;
    double border = .1;
};
struct EntityHandle {
    Entity* value = nullptr;
    bool Valid() const { return value; }
    explicit operator bool() const { return Valid(); }
    Entity* raw() const { return value; }
    bool EyePosition(float t, Vec3& out) const {
        out = value->previousEye.Add((value->eye.x-value->previousEye.x)*t,
            (value->eye.y-value->previousEye.y)*t, (value->eye.z-value->previousEye.z)*t);
        return true;
    }
    bool BoundingBox(Box& out) const { out = value->box; return true; }
    bool Position(Vec3& out) const { out = value->position; return true; }
    bool PickRadius(double& out) const { out = value->border; return true; }
    bool IsSpectator() const { return value->spectator; }
    bool IsPickable() const { return value->pickable; }
    bool IsSameRootVehicle(const EntityHandle&) const { return value->riding; }
    bool IsSame(const EntityHandle& other) const { return value == other.value; }
    bool IsPlayer() const { return value->player; }
};
Entity self;
std::vector<EntityHandle> candidates;
EntityHandle PlayerEntity() { return {&self}; }
EntityHandle LevelEntity() { return {&self}; }
EntityHandle AdoptEntity(Entity* v) { return {v}; }
float FrameTime() { return .25f; }
enum class HitKind { Miss, Block, Entity };
struct BlockHitResultInfo { HitKind kind = HitKind::Miss; Vec3 location; };
bool obstructed = false;
double obstacle = 1;
bool WorldClip(const Vec3&, const Vec3& end, const EntityHandle&, BlockHitResultInfo& out) {
    out = {obstructed ? HitKind::Block : HitKind::Miss, obstructed ? Vec3{0,0,obstacle} : end};
    return true;
}
bool Vec3DirectionFromRotation(float pitch, float yaw, Vec3& v) {
    constexpr double deg = 3.141592653589793/180;
    v = {-std::sin(yaw*deg)*std::cos(pitch*deg), -std::sin(pitch*deg), std::cos(yaw*deg)*std::cos(pitch*deg)};
    return true;
}
Box BoxInflate(Box b, double v) { return {b.minX-v,b.minY-v,b.minZ-v,b.maxX+v,b.maxY+v,b.maxZ+v}; }
Box BoxExpandTowards(Box b, const Vec3& v) {
    return {b.minX+std::min(0.,v.x),b.minY+std::min(0.,v.y),b.minZ+std::min(0.,v.z),
        b.maxX+std::max(0.,v.x),b.maxY+std::max(0.,v.y),b.maxZ+std::max(0.,v.z)};
}
bool BoxContains(const Box& b, const Vec3& v) {
    return v.x>b.minX && v.x<b.maxX && v.y>b.minY && v.y<b.maxY && v.z>b.minZ && v.z<b.maxZ;
}
bool BoxClip(const Box& b, const Vec3& from, const Vec3& to, bool& hit, Vec3& out) {
    double enter = -1e100, leave = 1e100;
    auto slab = [&](double lo, double hi, double x, double d) {
        if (std::abs(d)<1e-12) return x>=lo && x<=hi;
        double a=(lo-x)/d, c=(hi-x)/d;
        if (a>c) std::swap(a,c);
        enter=std::max(enter,a); leave=std::min(leave,c);
        return enter<=leave;
    };
    Vec3 d=to.Subtract(from);
    hit=slab(b.minX,b.maxX,from.x,d.x) && slab(b.minY,b.maxY,from.y,d.y) && slab(b.minZ,b.maxZ,from.z,d.z);
    double t=enter>=0 ? enter : leave;
    hit=hit && t>=0 && t<=1;
    if(hit) out=from.Add(d.x*t,d.y*t,d.z*t);
    return true;
}
bool WorldEntities(const EntityHandle&, const Box&, std::vector<EntityHandle>& out) { out=candidates; return true; }
}
namespace rotation {
enum class HitResultType { Miss, Block, Entity };
struct HitResult { HitResultType type=HitResultType::Miss; Vec3 location; java::EntityHandle entity; };
// @RAY_PRODUCTION@
}
namespace lunar {
auto PlayerEntity() { return java::PlayerEntity(); }
bool DirectionFromRotation(float p,float y,rotation::Vec3& out) { return java::Vec3DirectionFromRotation(p,y,out); }
bool BoxClip(const java::Box& b,const rotation::Vec3& a,const rotation::Vec3& c,bool& hit,rotation::Vec3& out) {
    return java::BoxClip(b,a,c,hit,out);
}
rotation::Vec3 interaction, interactionPosition;
bool InteractSwordBlock(const java::EntityHandle&, const rotation::Vec3& hit, const rotation::Vec3* pos) {
    interaction=hit; interactionPosition=*pos; return true;
}
}
namespace compat::lunar {
    bool WorldLoadedEntities(std::vector<java::EntityHandle>& out) { out=java::candidates; return true; }
}
struct Aura {
    bool valid=true;
    bool IsTarget(const java::EntityHandle& e,const java::EntityHandle& self) const {
        return valid && !e.IsSame(self);
    }
    // @BLOCK_TARGET@
};
namespace scaffold { bool IsEnabled() { return false; } }
namespace antibot { bool bot=false; bool IsBot(const java::EntityHandle&) { return bot; } }
struct PredictionRuntime {
    rotation::Rotation reportedRotation, targetRotation, appliedRotation;
    bool ownsRotation=false;
    struct IO {
        PredictionRuntime& r;
        java::EntityHandle ray;
        // @PREDICTION_IO@
    };
};
namespace killaura {
java::Box box;
rotation::Vec3 position;
bool ReadHypixelGeometry(java::Box& b,rotation::Vec3& p) { b=box; p=position; return true; }
}
struct HypixelIO {
    java::EntityHandle target;
    bool automaticAttack=false, geometryValid=false;
    java::Box targetBox;
    rotation::Vec3 targetPosition;
    // @HYPIXEL_GEOMETRY@
};
}

int main() {
    using namespace nova;
    java::self.box={-.3,-1,-.3,.3,1,.3};
    java::self.previousEye={-4,0,0}; java::self.eye={0,0,0};
    java::Entity enemy;
    enemy.box={-.3,-1,1.8,.3,1,2.2}; enemy.position={0,-1,2};
    java::candidates={{&enemy}};
    Aura aura;
    assert(aura.HasBlockTarget(2) && !aura.HasBlockTarget(1));
    aura.valid=false; assert(!aura.HasBlockTarget(6)); aura.valid=true;
    enemy.box={-.3,-1,-2.2,.3,1,-1.8};
    assert(aura.HasBlockTarget(2)); // behind the camera, no attack/FOV selection needed
    enemy.box={-.3,-1,1.8,.3,1,2.2};
    auto hit=rotation::LeaderRayCast({},3);
    assert(hit.type==rotation::HitResultType::Entity && hit.entity.raw()==&enemy);
    assert(rotation::RayCast({},3).type==rotation::HitResultType::Miss); // interpolation unchanged globally
    java::self.previousEye=java::self.eye;
    enemy.riding=true; enemy.spectator=true;
    assert(rotation::LeaderRayCast({},3).type==rotation::HitResultType::Entity);
    assert(rotation::RayCast({},3).type==rotation::HitResultType::Miss);
    assert(rotation::RiseRayCast({},3,0.1f).type==rotation::HitResultType::Miss);
    enemy.spectator=false;
    assert(rotation::RayCast({},3).type==rotation::HitResultType::Miss); // shared vehicle only
    assert(rotation::RiseRayCast({},3,0.1f).type==rotation::HitResultType::Entity); // Rise includes shared vehicles.
    enemy.riding=false; enemy.pickable=false;
    assert(rotation::LeaderRayCast({},3).type==rotation::HitResultType::Miss);
    enemy.pickable=true; java::obstructed=true;
    assert(rotation::LeaderRayCast({},3).type==rotation::HitResultType::Block);
    java::obstacle=2.5;
    assert(rotation::LeaderRayCast({},3).type==rotation::HitResultType::Entity);
    java::obstructed=false;
    PredictionRuntime runtime;
    PredictionRuntime::IO io{runtime};
    runtime.reportedRotation={0,0}; rotation::state.rotations={-90,25};
    io.ComputeRotation(-1,0); io.Rotate();
    assert(std::abs(runtime.targetRotation.yaw-90)<.01f && runtime.targetRotation.pitch==0);
    assert(runtime.ownsRotation && !rotation::state.moveInputFix);
    assert(io.RayPlayer()); // still uses original reported direction after Rotate
    antibot::bot=true; assert(!io.RayPlayer()); antibot::bot=false;
    killaura::box=java::BoxInflate(enemy.box,.1); killaura::position=enemy.position;
    HypixelIO ab; ab.target={&enemy}; ab.automaticAttack=true; ab.CaptureGeometry();
    enemy.box={9,9,9,10,10,10}; enemy.position={10,10,10};
    rotation::state.rotations={};
    rotation::Vec3 point;
    assert(ab.Ray(3,point) && std::abs(point.z-1.7)<1e-6 && ab.InSwingRange());
    ab.InteractBlock();
    assert(lunar::interactionPosition.z==2 && std::abs(lunar::interaction.z-1.7)<1e-6);
    ab.targetBox={-1,-1,-1,1,1,1};
    assert(ab.Ray(8,point) && point.z==1); // inside returns exit face, not eye
    assert(!ab.Ray(.5,point)); // ray entirely inside has no face intersection
    puts("PASS: production Leader ray rules, Prediction rotation ownership/snapshot, Hypixel geometry and interaction");
}
