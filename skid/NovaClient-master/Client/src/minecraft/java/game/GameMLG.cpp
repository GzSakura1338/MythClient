#include "minecraft/java/api/game_mlg.h"
#include "minecraft/java/api/game_inventory.h"
#include "minecraft/java/game/MlgWaterState.h"
#include "minecraft/compat/minecraft263/Minecraft263Access.h"
#include <cmath>
namespace nova::java {
namespace {
using compat::mc263::access::Frame;
using compat::mc263::access::Member;
struct Mapping {Member burning,water,lava,passenger,abilities,flying,item,emptyBucket,provider,attribute,evaporates,pos,state;};
#define NOVA_MLG_MAPPING
#define NOVA_MLG(id,owner,name,desc) .id={owner,name,desc},
constexpr Mapping legacy{
#include "minecraft/compat/mappings/Lunar189.inc"
};
constexpr Mapping forge{
#include "minecraft/compat/mappings/forge1201.inc"
};
constexpr Mapping modern{
#include "minecraft/compat/mappings/minecraft263.inc"
};
#define NOVA_MLG_WATER_MAPPING
struct LegacyWaterMapping {Member block,still,flowing,metadata,lava,evaporates;};
constexpr LegacyWaterMapping water17{
#include "minecraft/compat/mappings/Lunar1710.inc"
};
#undef NOVA_MLG_WATER_MAPPING
#undef NOVA_MLG
#undef NOVA_MLG_MAPPING
constexpr Mapping legacy17=[] {auto value=legacy;value.lava=water17.lava;value.evaporates=water17.evaporates;return value;}();
const Mapping& Names(){return compat::IsLunar1710()?legacy17:compat::IsLegacyLunar()?legacy:compat::UsesModernRuntime()?modern:forge;}
}
bool ReadMlgEnvironment(MlgEnvironment& out) {
    Frame f;auto p=f.player();if(!p)return false;const auto& m=Names();
    out.water=f.call<bool>(p,m.water);out.lava=f.call<bool>(p,m.lava);out.passenger=f.call<bool>(p,m.passenger);
    auto abilities=compat::IsLegacyLunar()?f.get(p,m.abilities):f.call(p,m.abilities);
    out.flying=f.get<bool>(abilities,m.flying);
    return abilities && f.check();
}
bool ReadMlgHazards(bool& burning,bool& inWeb,BlockPos& webPos) {
    burning=inWeb=false;webPos={};
    Frame f;auto player=f.player();auto world=LevelEntity();Box box;
    if(!player || !world || !PlayerEntity().BoundingBox(box))return false;
    const auto& m=Names();burning=f.call<bool>(player,m.burning);
    if(!f.check() || !std::isfinite(box.minX+box.minY+box.minZ+box.maxX+box.maxY+box.maxZ) ||
       std::abs(box.minX)>30000001 || std::abs(box.minY)>30000001 || std::abs(box.minZ)>30000001 ||
       box.maxX<=box.minX || box.maxY<=box.minY || box.maxZ<=box.minZ ||
       box.maxX-box.minX>2 || box.maxY-box.minY>4 || box.maxZ-box.minZ>2)return false;
    // Scan the entire body, highest intersecting web first. Water flows down,
    // so upper webs must be cleared before lower ones when both are present.
    for(int y=int(std::floor(box.maxY-.001));y>=int(std::floor(box.minY+.001));--y)
    for(int x=int(std::floor(box.minX+.001));x<=int(std::floor(box.maxX-.001));++x)
    for(int z=int(std::floor(box.minZ+.001));z<=int(std::floor(box.maxZ-.001));++z) {
        if(compat::IsLunar1710()) {
            auto block=f.call(static_cast<jobject>(world.raw()),water17.block,x,y,z);
            if(!block)return false;
            inWeb=f.instance(block,"net.minecraft.block.BlockWeb");
            f.env->DeleteLocalRef(block);
        } else {
            auto pos=f.create(m.pos,x,y,z);
            auto state=f.call(static_cast<jobject>(world.raw()),m.state,pos);
            if(!state)return false;
            auto text=f.call(state,{"java.lang.Object","toString","()Ljava/lang/String;"});
            auto value=f.text(text);
            if(value.starts_with("Block{"))value.erase(0,6);
            const auto id=value.substr(0,value.find_first_of("}["));
            inWeb=id=="minecraft:cobweb" || id=="minecraft:web";
            if(text)f.env->DeleteLocalRef(text);
            f.env->DeleteLocalRef(state);if(pos)f.env->DeleteLocalRef(pos);
        }
        if(!f.check())return false;
        if(inWeb){webPos={x,y,z};return true;}
    }
    return true;
}
MlgBucket MlgBucketAt(int slot) {
    Frame f;jobject stack=nullptr;ItemFacts facts;
    if(!f || slot<0 || slot>8 || !InventorySlot(slot,stack) || !stack || !ReadItemFacts(stack,facts) || facts.empty)return MlgBucket::None;
    if(facts.item==KnownItem::WaterBucket)return MlgBucket::Water;
    const auto& m=Names();auto item=f.call(stack,m.item),empty=f.stat(m.emptyBucket);
    return item && empty && f.check() && f.env->IsSameObject(item,empty)?MlgBucket::Empty:MlgBucket::None;
}
bool MlgAllowsWater(const BlockPos& pos) {
    Frame f;auto world=LevelEntity();if(!f || !world)return false;const auto& m=Names();
    const auto level=static_cast<jobject>(world.raw());
    const auto provider=compat::IsLegacyLunar()?f.get(level,m.provider):f.call(level,m.provider);
    bool evaporates=true;
    if(compat::UsesModernRuntime()) {
        auto at=f.create(m.pos,pos.x,pos.y,pos.z);
        auto value=f.call(provider,m.evaporates,f.stat(m.attribute),at);
        if(!value)return false;
        evaporates=f.call<bool>(value,{"java.lang.Boolean","booleanValue","()Z"});
    } else if(compat::IsLunar1710())evaporates=f.get<bool>(provider,m.evaporates);
    else evaporates=f.call<bool>(provider,m.evaporates);
    return provider && f.check() && !evaporates;
}
bool MlgWaterAt(const BlockPos& pos,bool& water,bool& source) {
    water=source=false;Frame f;auto world=LevelEntity();if(!f || !world)return false;
    if(compat::IsLunar1710()) {
        auto block=f.call(static_cast<jobject>(world.raw()),water17.block,pos.x,pos.y,pos.z);
        auto still=f.stat(water17.still);
        auto flowing=f.stat(water17.flowing);
        if(!block || !still || !flowing || !f.check())return false;
        water=f.env->IsSameObject(block,still)||f.env->IsSameObject(block,flowing);
        source=water && f.call<int>(static_cast<jobject>(world.raw()),water17.metadata,pos.x,pos.y,pos.z)==0;
    } else {
        const auto& m=Names();auto state=f.call(static_cast<jobject>(world.raw()),m.state,f.create(m.pos,pos.x,pos.y,pos.z));
        if(!state)return false;
        // BlockState's canonical registry/property representation is not localized.
        const auto value=f.text(f.call(state,{"java.lang.Object","toString","()Ljava/lang/String;"}));
        water=mlg::IsWaterState(value);
        source=mlg::IsSourceState(value);
    }
    return f.check();
}
}
