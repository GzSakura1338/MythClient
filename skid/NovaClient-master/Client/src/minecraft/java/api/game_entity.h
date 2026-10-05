#pragma once

#include <string>
#include <vector>

#include "minecraft/utils/rotation/Vec3.h"

namespace nova::java {

struct BlockPos {
    int x = 0;
    int y = 0;
    int z = 0;
};

struct Box {
    double minX = 0.0;
    double minY = 0.0;
    double minZ = 0.0;
    double maxX = 0.0;
    double maxY = 0.0;
    double maxZ = 0.0;
};

enum EntityTypeMask : unsigned {
    kTypeLiving   = 1u << 0,
    kTypePlayer   = 1u << 1,
    kTypeVillager = 1u << 2,
    kTypeMonster  = 1u << 3,
    kTypeAnimal   = 1u << 4,
};

class EntityHandle {
public:
    EntityHandle() = default;
    ~EntityHandle();
    EntityHandle(EntityHandle&& other) noexcept;
    EntityHandle& operator=(EntityHandle&& other) noexcept;
    EntityHandle(const EntityHandle&) = delete;
    EntityHandle& operator=(const EntityHandle&) = delete;

    explicit operator bool() const { return obj != nullptr; }
    bool Valid() const { return obj != nullptr; }
    void* raw() const { return obj; }
    void Reset();

    bool IsPlayer() const;
    bool IsSpectator() const;
    bool IsPickable() const;
    bool IsAlive() const;
    bool IsSame(const EntityHandle& other) const;
    bool IsSameRootVehicle(const EntityHandle& other) const;
    bool Position(rotation::Vec3& out) const;

    // The entity's position now and at the end of the previous tick. A frame is
    // drawn between the two, so anything placed over an entity has to interpolate
    // them: the tick moves twenty times a second and the frame sixty, and drawing at
    // the tick position makes the drawing step.
    bool PositionPair(rotation::Vec3& now, rotation::Vec3& before) const;
    bool RenderPositionPair(rotation::Vec3& now, rotation::Vec3& before) const;
    bool EyePosition(float partialTicks, rotation::Vec3& out) const;
    bool EyeHeight(double& out) const;
    bool GetY(double& out) const;
    bool BoundingBox(Box& out) const;
    bool DistanceTo(const EntityHandle& other, double& out) const;
    bool PickRadius(double& out) const;
    bool TickCount(int& out) const;
    bool OnGround(bool& out) const;
    bool IsInvisible(bool& out) const;
    bool IsAlliedTo(const EntityHandle& other, bool& out) const;
    bool DisplayName(std::string& out) const;
    bool Health(float& health, float& absorption) const;
    bool MaxHealth(float& out) const;
    bool SetHealth(float health) const;

    // Player.getScoreboardName -- the plain name the scoreboard knows this player
    // by, with none of the team decoration that DisplayName carries. It is the key
    // a score packet names its owner with.
    bool ScoreboardName(std::string& out) const;
    bool HurtTime(int& out) const;
    bool DeathTime(int& out) const;
    bool DistanceSqTo(const EntityHandle& other, double& out) const;

private:
    friend EntityHandle AdoptEntity(void* localRef);
    friend bool WorldEntities(const EntityHandle& source, const Box& area,
                              std::vector<EntityHandle>& out);
    friend bool WorldPlayers(std::vector<EntityHandle>& out);
    void* obj = nullptr;
};

EntityHandle AdoptEntity(void* localRef);

EntityHandle PlayerEntity();
EntityHandle LevelEntity();

bool EntityTypes(const EntityHandle& entity, unsigned& out);

// Entity.getId() -- the id the motion packet carries for whoever it is about.
bool EntityId(const EntityHandle& entity, int& out);

// Level.getEntity(id) -- the entity the id names, or an invalid handle once the
// level has let it go. Ids are what a module can safely keep: a handle outlives
// the entity it points at, an id does not.
EntityHandle EntityById(int id);

// Entity.fallDistance on the local player.
bool PlayerFallDistance(float& out);

bool WorldPlayers(std::vector<EntityHandle>& out);
bool WorldEntities(const EntityHandle& source, const Box& area, std::vector<EntityHandle>& out);

enum class HitKind { Miss, Block, Entity };

struct BlockHitResultInfo {
    HitKind kind = HitKind::Miss;
    rotation::Vec3 location;
    BlockPos blockPos;
    int direction = -1;
    bool hasBlockPos = false;
};

enum class ClipMode { Outline, Collision, WaterSource };
bool WorldClip(const rotation::Vec3& from, const rotation::Vec3& to, const EntityHandle& source,
               BlockHitResultInfo& out, ClipMode mode=ClipMode::Outline);

EntityHandle MakeEntityHit(const EntityHandle& entity, const rotation::Vec3& location);

bool Vec3DirectionFromRotation(float pitch, float yaw, rotation::Vec3& out);

bool BoxClip(const Box& box, const rotation::Vec3& from, const rotation::Vec3& to, bool& hit,
             rotation::Vec3& out);
bool BoxContains(const Box& box, const rotation::Vec3& point);
Box BoxInflate(const Box& box, double amount);
Box BoxExpandTowards(const Box& box, const rotation::Vec3& delta);

float FrameTime();
bool PlayerOnGround();
int PlayerTickCount();
int PlayerHurtTime();

}
