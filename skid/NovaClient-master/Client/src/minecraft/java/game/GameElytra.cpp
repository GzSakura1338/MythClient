#include "minecraft/java/api/game_elytra.h"
#include "minecraft/java/api/game_inventory.h"
#include "minecraft/compat/minecraft263/Minecraft263Access.h"
#include <algorithm>
#include <cmath>
namespace nova::java {
namespace {
using namespace compat::mc263::access;
struct Mapping { Member flying,velocity,look,id,removed,data,dataGet,target,entities,component,fireworks,duration,x,y,z; };
#define NOVA_ELYTRA_MAPPING
#define NOVA_ELYTRA(key,owner,name,desc) .key={owner,name,desc},
constexpr Mapping modern{
#include "minecraft/compat/mappings/minecraft263.inc"
};
constexpr Mapping forge{
#include "minecraft/compat/mappings/forge1201.inc"
};
#undef NOVA_ELYTRA
#undef NOVA_ELYTRA_MAPPING
const Mapping& Map(){return compat::UsesModernRuntime()?modern:forge;}
bool Vector(Frame& f,jobject object,rotation::Vec3& out){
    const auto& m=Map();
    if(!object)return false;
    out={f.get<double>(object,m.x),f.get<double>(object,m.y),f.get<double>(object,m.z)};
    return f.check()&&std::isfinite(out.x)&&std::isfinite(out.y)&&std::isfinite(out.z);
}
}
bool ReadElytraFlight(ElytraFlight& out){
    out={};if(compat::IsLegacyLunar())return false;
    Frame f;const auto& m=Map();auto player=f.player();
    if(!player||!f)return false;
    out.gliding=f.call<bool>(player,m.flying);
    if(!f.check()||!out.gliding)return f.check();
    if(!Vector(f,f.call(player,m.velocity),out.velocity)||!Vector(f,f.call(player,m.look),out.look))return false;
    const int self=f.call<int>(player,m.id);
    auto target=f.stat(m.target);
    auto level=LevelEntity();
    auto iterable=f.call(static_cast<jobject>(level.raw()),m.entities);
    auto it=f.call(iterable,{"java.lang.Iterable","iterator","()Ljava/util/Iterator;"});
    if(!it||!target||!f.check())return false;
    while(f.call<bool>(it,{"java.util.Iterator","hasNext","()Z"})){
        Frame local(f.env);
        auto entity=local.call(it,{"java.util.Iterator","next","()Ljava/lang/Object;"});
        if(!entity||!local.check())return false;
        if(!local.instance(entity,m.target.owner))continue;
        if(local.call<bool>(entity,m.removed))continue;
        // Synchronized target id works before the client resolves attachedToEntity.
        // Nearby players' rockets and unattached fireworks must not suppress ours.
        auto data=local.call(entity,m.data);
        auto id=local.call(data,m.dataGet,target);
        const bool present=local.call<bool>(id,{"java.util.OptionalInt","isPresent","()Z"});
        const bool mine=present&&local.call<int>(id,{"java.util.OptionalInt","getAsInt","()I"})==self;
        if(!id||!local.check())return false;
        if(mine){out.boosting=true;break;}
    }
    return f.check();
}
bool FireworkFlightDuration(int slot,int& duration){
    duration=0;if(slot<0||slot>8||compat::IsLegacyLunar())return false;
    Frame f;const auto& m=Map();jobject stack=nullptr;ItemFacts facts;
    if(!f||!InventorySlot(slot,stack)||!ReadItemFacts(stack,facts)||facts.empty||facts.item!=KnownItem::FireworkRocket)return false;
    jobject component=nullptr;
    if(compat::UsesModernRuntime()){
        component=f.call(stack,m.component,f.stat(m.fireworks));
        if(component)duration=f.call<int>(component,m.duration);
    }else{
        component=f.call(stack,m.component,f.env->NewStringUTF("Fireworks"));
        if(component)duration=f.call<int>(component,m.duration,f.env->NewStringUTF("Flight"));
    }
    duration=std::clamp(duration,0,255);
    return f.check();
}
}
