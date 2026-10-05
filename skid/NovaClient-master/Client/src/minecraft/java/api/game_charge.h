#pragma once
#include "minecraft/java/api/game_entity.h"
#include "minecraft/java/api/game_inventory.h"
namespace nova::java {
bool ChargeSupported();
bool ChargeSlotReady(int slot, KnownItem item);
struct ChargeMovementPacket {
    enum class Kind { Other, Move, EndTick } kind=Kind::Other;
    rotation::Vec3 position;
    bool hasPosition=false,onGround=false;
};
bool ReadChargeMovementPacket(jobject packet,ChargeMovementPacket& out);
bool ChargePearl(const EntityHandle& entity, const EntityHandle& owner, rotation::Vec3& velocity);
bool ChargeWind(const EntityHandle& entity, const EntityHandle& owner, rotation::Vec3& velocity);
bool ChargeEntityHittable(const EntityHandle& entity);
}
