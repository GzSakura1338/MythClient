#include "minecraft/java/api/game_mace.h"
#include "minecraft/java/api/game_inventory.h"
#include "minecraft/java/api/game_mlg.h"
#include "minecraft/compat/minecraft263/Minecraft263Access.h"
#include <cmath>

namespace nova::java {
namespace {
using namespace compat::mc263::access;
using namespace compat::mc263::names;
struct Mapping { Member component, type, fallback, maximum, within, usingItem, destroying, blockingWith, axes, isTag; };
#define NOVA_MACE_MAPPING
#define NOVA_MACE(key,owner,name,desc) .key={owner,name,desc},
constexpr Mapping mapping{
#include "minecraft/compat/mappings/minecraft263.inc"
};
#undef NOVA_MACE
#undef NOVA_MACE_MAPPING
jobject Range(Frame& f, int slot) {
    jobject stack=nullptr;
    if(!InventorySlot(slot,stack)||!stack)return nullptr;
    auto range=f.call(stack,mapping.component,f.stat(mapping.type));
    return range?range:f.staticCall(mapping.fallback,f.player());
}
}
bool MaceSupported(){return compat::IsLunar12111()||compat::IsMinecraft263();}
bool ReadMaceState(MaceState& out) {
    out={};if(!MaceSupported())return false;
    Frame f;auto p=f.player();if(!f||!p)return false;
    out.fall=f.get<double>(p,Entity_Field_fallDistance);
    out.ground=f.call<bool>(p,Entity_onGround);
    out.gliding=f.call<bool>(p,LivingEntity_isFallFlying);
    auto velocity=f.call(p,Entity_getDeltaMovement);
    out.verticalSpeed=f.get<double>(velocity,Vec3_Field_y);
    out.strength=f.call<float>(p,Player_getAttackStrengthScale,.5f);
    out.obstructed=f.call<bool>(p,LivingEntity_onClimbable);
    out.interacting=f.call<bool>(p,mapping.usingItem)||
        f.call<bool>(f.get(f.client(),Minecraft_Field_gameMode),mapping.destroying);
    MlgEnvironment env;
    if(!ReadMlgEnvironment(env))return false;
    out.obstructed=out.obstructed||env.water||env.lava||env.passenger||env.flying;
    return f.check()&&velocity&&std::isfinite(out.fall)&&std::isfinite(out.verticalSpeed)&&std::isfinite(out.strength);
}
bool MaceSlotAvailable(int slot) {
    if(!MaceSupported()||slot<0||slot>8)return false;
    Frame f;jobject stack=nullptr;
    // Selection must not depend on unrelated inventory scoring bindings.
    if(!f||!InventorySlot(slot,stack)||!stack||f.call<bool>(stack,ItemStack_isEmpty))return false;
    return f.instance(f.call(stack,ItemStack_getItem),"net.minecraft.world.item.MaceItem")&&f.check();
}
bool MaceAxeSlotAvailable(int slot) {
    if(!MaceSupported()||slot<0||slot>8)return false;
    Frame f;jobject stack=nullptr;
    if(!f||!InventorySlot(slot,stack)||!stack||f.call<bool>(stack,ItemStack_isEmpty))return false;
    // 26.3 uses components/tags rather than the removed AxeItem subclass.
    auto tag=f.stat(mapping.axes);
    return tag&&f.call<bool>(stack,mapping.isTag,tag)&&f.check();
}
bool MaceTargetBlockingShield(const EntityHandle& target) {
    if(!MaceSupported()||!target)return false;
    Frame f;if(!f)return false;
    // Native getter includes active use and the shield's initial blocking delay.
    auto stack=f.call(static_cast<jobject>(target.raw()),mapping.blockingWith);
    return stack&&!f.call<bool>(stack,ItemStack_isEmpty)&&f.check();
}
EntityHandle MaceSlotIdentity(int slot) {
    if(!MaceSupported()||slot<0||slot>8)return {};
    Frame f;jobject stack=nullptr;
    return f&&InventorySlot(slot,stack)&&stack?AdoptEntity(stack):EntityHandle{};
}
bool MaceSlotMatches(int slot,const EntityHandle& identity) {
    auto current=MaceSlotIdentity(slot);return current&&identity&&current.IsSame(identity);
}
bool MaceReach(int slot,double& out) {
    out=0;if(!MaceSupported())return false;
    Frame f;auto range=Range(f,slot);
    out=f.call<float>(range,mapping.maximum,f.player());
    return range&&f.check()&&std::isfinite(out)&&out>0&&out<=64;
}
bool MaceCanReach(int slot,const EntityHandle& target) {
    if(!MaceSupported()||!target)return false;
    Frame f;auto range=Range(f,slot);
    auto box=f.call(static_cast<jobject>(target.raw()),Entity_getBoundingBox);
    return range&&box&&f.call<bool>(range,mapping.within,f.player(),box,0.)&&f.check();
}
}
