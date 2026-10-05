#include "minecraft/java/api/game_charge.h"
#include "minecraft/compat/minecraft263/Minecraft263Access.h"
#include <cmath>
namespace nova::java {
namespace {
using compat::mc263::access::Frame;
using compat::mc263::access::Member;
struct Mapping {const char* pearl; const char* wind; Member owner,cooldowns,onCooldown,movement,hittable;};
#define NOVA_AUTOCHARGE_MAPPING
#define NOVA_CHARGE_CLASS(name) .pearl=name,
#define NOVA_CHARGE_WIND_CLASS(name) .wind=name,
#define NOVA_CHARGE(key,owner,name,desc) .key={owner,name,desc},
constexpr Mapping mapping{
#include "minecraft/compat/mappings/minecraft263.inc"
};
#undef NOVA_CHARGE
#undef NOVA_CHARGE_CLASS
#undef NOVA_CHARGE_WIND_CLASS
#undef NOVA_AUTOCHARGE_MAPPING
bool Vector(Frame& f,jobject vec,rotation::Vec3& out){
    using namespace compat::mc263::names;
    if(!vec)return false;
    out={f.get<double>(vec,Vec3_Field_x),f.get<double>(vec,Vec3_Field_y),f.get<double>(vec,Vec3_Field_z)};
    return f.check()&&std::isfinite(out.x)&&std::isfinite(out.y)&&std::isfinite(out.z);
}
}
bool ChargeSupported(){return compat::IsLunar12111()||compat::IsMinecraft263();}
bool ReadChargeMovementPacket(jobject packet,ChargeMovementPacket& out){
    out={};if(!ChargeSupported()||!packet)return false;
    Frame f;using namespace compat::mc263::names;
    if(f.instance(packet,"net.minecraft.network.protocol.game.ServerboundClientTickEndPacket")){
        out.kind=ChargeMovementPacket::Kind::EndTick;return f.check();
    }
    if(!f.instance(packet,ServerboundMovePlayerPacket))return false;
    out.kind=ChargeMovementPacket::Kind::Move;
    out.hasPosition=f.call<bool>(packet,ServerboundMovePlayerPacket_hasPosition);
    out.onGround=f.call<bool>(packet,ServerboundMovePlayerPacket_isOnGround);
    if(out.hasPosition)out.position={f.call<double>(packet,ServerboundMovePlayerPacket_getX,0.),
        f.call<double>(packet,ServerboundMovePlayerPacket_getY,0.),f.call<double>(packet,ServerboundMovePlayerPacket_getZ,0.)};
    return f.check();
}
bool ChargeSlotReady(int slot,KnownItem expected){
    if(!ChargeSupported()||slot<0||slot>8)return false;
    Frame f;jobject stack=nullptr;ItemFacts facts;
    if(!f||!InventorySlot(slot,stack)||!stack||!ReadItemFacts(stack,facts)||
       facts.empty||facts.count<=0||facts.item!=expected)return false;
    auto cooldowns=f.call(f.player(),mapping.cooldowns);
    const bool blocked=f.call<bool>(cooldowns,mapping.onCooldown,stack);
    return cooldowns&&f.check()&&!blocked;
}
bool ChargePearl(const EntityHandle& entity,const EntityHandle& owner,rotation::Vec3& velocity){
    if(!ChargeSupported()||!entity||!owner)return false;
    Frame f;auto object=static_cast<jobject>(entity.raw());
    if(!f.instance(object,mapping.pearl))return false;
    auto source=f.call(object,mapping.owner);
    if(!source||!f.env->IsSameObject(source,static_cast<jobject>(owner.raw())))return false;
    // Only the verified in-air model is used; fluids/bubble columns need another solver.
    if(f.call<bool>(object,{"net.minecraft.world.entity.Entity","isInWater","()Z"}))return false;
    return Vector(f,f.call(object,compat::mc263::names::Entity_getDeltaMovement),velocity);
}
bool ChargeWind(const EntityHandle& entity,const EntityHandle& owner,rotation::Vec3& velocity){
    if(!ChargeSupported()||!entity||!owner)return false;
    Frame f;auto object=static_cast<jobject>(entity.raw());
    if(!f.instance(object,mapping.wind))return false;
    auto source=f.call(object,mapping.owner);
    if(!source||!f.env->IsSameObject(source,static_cast<jobject>(owner.raw())))return false;
    return Vector(f,f.call(object,compat::mc263::names::Entity_getDeltaMovement),velocity);
}
bool ChargeEntityHittable(const EntityHandle& entity){
    if(!ChargeSupported())return false;
    Frame f;return entity&&f.call<bool>(static_cast<jobject>(entity.raw()),mapping.hittable)&&f.check();
}
}
