#include "minecraft/java/api/game_crystal.h"
#include "minecraft/java/api/game_anchor.h"
#include "minecraft/java/api/game_inventory.h"
#include "minecraft/java/api/game_itemuse.h"
#include "minecraft/java/api/game_place.h"
#include "minecraft/java/api/game_slotspoof.h"
#include "minecraft/compat/minecraft263/Minecraft263Access.h"
#include "minecraft/compat/minecraft263/Minecraft263Game.h"
#include "minecraft/utils/rotation/RotationManager.h"
#include "minecraft/modules/combat/legitcrystalaura/ExplosionExposure.h"
#include "minecraft/modules/combat/autocrystal/ResistanceTrace.h"
#include <cmath>

namespace nova::java {
namespace {
using namespace compat::mc263::access;
using namespace compat::mc263::names;
struct Mapping { Member armor,effect,resistance,difficulty,difficultyId,exposure,reach,entities,usingItem,destroying,collisions,blastResistance; };
#define NOVA_CRYSTAL_MAPPING
#define NOVA_CRYSTAL(key,owner,name,desc) .key={owner,name,desc},
constexpr Mapping mapping{
#include "minecraft/compat/mappings/minecraft263.inc"
};
#undef NOVA_CRYSTAL
#undef NOVA_CRYSTAL_MAPPING
jobject Position(Frame& f,const BlockPos& p){return f.create(BlockPos_Ctor,p.x,p.y,p.z);}
}
bool CrystalSupported(){return compat::IsLunar12111()||compat::IsMinecraft263();}
bool CrystalContextReady() {
    if(!CrystalSupported())return false;
    Frame f;auto player=f.player(),level=f.level(),mode=f.get(f.client(),Minecraft_Field_gameMode);
    return player&&level&&mode&&!f.call<bool>(player,mapping.usingItem)&&
        !f.call<bool>(mode,mapping.destroying)&&f.check();
}
bool IsEndCrystal(const EntityHandle& entity) {
    if(!CrystalSupported()||!entity)return false;
    Frame f;return f.instance(static_cast<jobject>(entity.raw()),"net.minecraft.world.entity.boss.enderdragon.EndCrystal")&&f.check();
}
bool CrystalHotbarSlot(int slot) {
    if(!CrystalSupported()||slot<0||slot>8)return false;
    Frame f;jobject stack=nullptr;
    if(!InventorySlot(slot,stack)||!stack||f.call<bool>(stack,ItemStack_isEmpty))return false;
    auto item=f.call(stack,ItemStack_getItem),crystal=f.stat(Items_Field_END_CRYSTAL);
    return item&&crystal&&f.env->IsSameObject(item,crystal)&&f.check();
}
bool CrystalObsidianSlot(int slot) {
    if(!CrystalSupported()||slot<0||slot>8)return false;
    Frame f;jobject stack=nullptr;
    if(!InventorySlot(slot,stack)||!stack||f.call<bool>(stack,ItemStack_isEmpty))return false;
    auto item=f.call(stack,ItemStack_getItem);
    auto key=f.call(f.stat(BuiltInRegistries_Field_ITEM),Registry_getKey,item);
    const auto name=f.text(f.call(key,Identifier_getPath));
    return f.check()&&name=="obsidian";
}
bool CrystalCreative(const EntityHandle& entity,bool& out) {
    out=false;if(!CrystalSupported()||!entity||!entity.IsPlayer())return false;
    Frame f;auto abilities=f.call(static_cast<jobject>(entity.raw()),Player_getAbilities);
    out=f.get<bool>(abilities,Abilities_Field_instabuild);return abilities&&f.check();
}
bool CrystalOffhand() {
    if(!CrystalSupported())return false;
    Frame f;auto stack=f.call(f.player(),LivingEntity_getItemBySlot,f.stat(EquipmentSlot_Field_OFFHAND));
    if(!stack||f.call<bool>(stack,ItemStack_isEmpty))return false;
    auto item=f.call(stack,ItemStack_getItem),crystal=f.stat(Items_Field_END_CRYSTAL);
    return item&&crystal&&f.env->IsSameObject(item,crystal)&&f.check();
}
bool SwingCrystalHand(bool offhand,bool animate) {
    return CrystalSupported()&&compat::mc263::SwingHand(offhand,animate);
}
bool CrystalBaseBuildable(const BlockPos& pos) {
    if(!CrystalSupported())return false;
    Frame f;auto world=f.level();
    auto state=f.call(world,Level_getBlockState,Position(f,pos));
    auto above=f.call(world,Level_getBlockState,Position(f,{pos.x,pos.y+1,pos.z}));
    if(!state||!above||!f.call<bool>(state,BlockBehaviour_BlockStateBase_isAir)||
       !f.call<bool>(above,BlockBehaviour_BlockStateBase_isAir))return false;
    const auto volume=CrystalPlacementVolume(pos,true);
    auto box=f.create(AABB_Ctor,volume.minX,volume.minY,volume.minZ,volume.maxX,volume.maxY,volume.maxZ);
    auto entities=f.call(world,mapping.entities,static_cast<jobject>(nullptr),box);
    return entities&&f.call<bool>(entities,{"java.util.List","isEmpty","()Z"})&&f.check();
}
bool CrystalBaseSupport(const BlockPos& pos) {
    return CrystalSupported()&&!BlockStateReplaceable(pos)&&!BlockStateCollisionEmpty(pos)&&!BlockStateOpensMenu(pos);
}
bool CrystalMotionClear(const EntityHandle& entity,const Box& box) {
    if(!CrystalSupported()||!entity)return false;
    Frame f;
    auto bounds=f.create(AABB_Ctor,box.minX,box.minY,box.minZ,box.maxX,box.maxY,box.maxZ);
    auto collisions=f.call(f.level(),mapping.collisions,static_cast<jobject>(entity.raw()),bounds);
    auto iterator=f.call(collisions,{"java.lang.Iterable","iterator","()Ljava/util/Iterator;"});
    return iterator&&!f.call<bool>(iterator,{"java.util.Iterator","hasNext","()Z"})&&f.check();
}
bool CrystalBlockReach(double& out) {
    out=0;if(!CrystalSupported())return false;
    Frame f;out=f.call<double>(f.player(),mapping.reach);
    return f.check()&&std::isfinite(out)&&out>0&&out<=64;
}
bool CrystalBase(const BlockPos& pos) {
    if(!CrystalSupported())return false;
    Frame f;auto state=f.call(f.level(),Level_getBlockState,Position(f,pos));
    auto block=f.call(state,BlockBehaviour_BlockStateBase_getBlock);
    auto key=f.call(f.stat(BuiltInRegistries_Field_BLOCK),Registry_getKey,block);
    const auto name=f.text(f.call(key,Identifier_getPath));
    return f.check()&&(name=="obsidian"||name=="bedrock");
}
bool CrystalPlaceable(const BlockPos& pos) {
    return CrystalBase(pos)&&CrystalSpaceClear(pos);
}
bool CrystalSpaceClear(const BlockPos& pos) {
    if(!CrystalSupported())return false;
    Frame f;auto world=f.level();
    auto above=f.call(world,Level_getBlockState,Position(f,{pos.x,pos.y+1,pos.z}));
    if(!above||!f.call<bool>(above,BlockBehaviour_BlockStateBase_isAir))return false;
    const auto volume=CrystalPlacementVolume(pos,false);
    auto box=f.create(AABB_Ctor,volume.minX,volume.minY,volume.minZ,volume.maxX,volume.maxY,volume.maxZ);
    // Null source is intentional: the local player also obstructs placement.
    auto entities=f.call(world,mapping.entities,static_cast<jobject>(nullptr),box);
    return entities&&f.call<bool>(entities,{"java.util.List","isEmpty","()Z"})&&f.check();
}
bool ReadCrystalDefense(const EntityHandle& entity,CrystalDefense& out) {
    out={};if(!CrystalSupported()||!entity)return false;
    Frame f;auto e=static_cast<jobject>(entity.raw());float hp=0,absorption=0;
    if(!entity.Health(hp,absorption))return false;
    out.health=hp;out.absorption=absorption;
    out.armor=f.call<int>(e,mapping.armor);
    out.toughness=f.call<double>(e,LivingEntity_getAttributeValue,f.stat(Attributes_Field_ARMOR_TOUGHNESS));
    auto effect=f.call(e,mapping.effect,f.stat(mapping.resistance));
    if(effect)out.resistance=f.call<int>(effect,MobEffectInstance_getAmplifier);
    auto data=f.call(f.level(),Level_getLevelData);
    auto difficulty=f.call(data,mapping.difficulty);
    out.difficulty=f.call<int>(difficulty,mapping.difficultyId);
    auto protection=f.stat(Enchantments_Field_PROTECTION),blast=f.stat(Enchantments_Field_BLAST_PROTECTION);
    if(!data||!difficulty||!protection||!blast)return false;
    for(const auto slot:{EquipmentSlot_Field_HEAD,EquipmentSlot_Field_CHEST,EquipmentSlot_Field_LEGS,EquipmentSlot_Field_FEET}) {
        auto stack=f.call(e,LivingEntity_getItemBySlot,f.stat(slot));
        if(!stack)return false;
        if(f.call<bool>(stack,ItemStack_isEmpty))continue;
        auto enchants=f.call(stack,DataComponentHolder_get,f.stat(DataComponents_Field_ENCHANTMENTS));
        if(!enchants)return false;
        auto keys=f.call(enchants,ItemEnchantments_keySet);
        auto it=f.call(keys,{"java.lang.Iterable","iterator","()Ljava/util/Iterator;"});
        if(!it)return false;
        while(f.call<bool>(it,{"java.util.Iterator","hasNext","()Z"})) {
            auto holder=f.call(it,{"java.util.Iterator","next","()Ljava/lang/Object;"});
            const int level=f.call<int>(enchants,ItemEnchantments_getLevel,holder);
            if(f.call<bool>(holder,Holder_is_2,protection))out.protection+=level;
            if(f.call<bool>(holder,Holder_is_2,blast))out.protection+=2*level;
            if(holder)f.env->DeleteLocalRef(holder);
        }
    }
    return f.check()&&out.armor>=0&&out.armor<=30&&std::isfinite(out.toughness)&&out.toughness>=0&&
        std::isfinite(out.health)&&std::isfinite(out.absorption)&&out.difficulty>=0&&out.difficulty<=3;
}
bool CrystalExposure(const EntityHandle& entity,const rotation::Vec3& explosion,double& out) {
    out=0;if(!CrystalSupported()||!entity)return false;
    Frame f;if(!f)return false;
    auto point=f.create(Vec3_Ctor,explosion.x,explosion.y,explosion.z);
    auto method=Method(f.env,mapping.exposure,true);auto owner=Class(f.env,mapping.exposure.owner);
    if(!point||!method||!owner||!f.check())return false;
    out=f.env->CallStaticFloatMethod(owner,method,point,static_cast<jobject>(entity.raw()));
    return f.check()&&std::isfinite(out)&&out>=0&&out<=1;
}
bool CrystalExposureAt(const EntityHandle& entity,const rotation::Vec3& explosion,
                       const Box& bounds,const BlockPos* virtualBase,double& out) {
    if(!CrystalSupported()||!entity)return false;
    Frame f;auto world=f.level();
    auto end=f.create(Vec3_Ctor,explosion.x,explosion.y,explosion.z);
    auto collider=f.stat(ClipContext_Block_Field_COLLIDER),fluid=f.stat(ClipContext_Fluid_Field_NONE);
    if(!world||!end||!collider||!fluid||!f.check())return false;
    Box solid{};
    if(virtualBase)solid=CrystalPlacementVolume(*virtualBase,true);
    return module::crystal::SampleExposure(bounds,explosion,virtualBase?&solid:nullptr,
        [&](const rotation::Vec3& from,const rotation::Vec3&,bool& clear){
            // Reuse the JNI frame, level and endpoint across all exposure rays.
            auto start=f.create(Vec3_Ctor,from.x,from.y,from.z);
            auto context=f.create(ClipContext_Ctor,start,end,collider,fluid,static_cast<jobject>(entity.raw()));
            auto hit=f.call(world,BlockGetter_clip,context);
            auto type=f.call(hit,HitResult_getType);
            clear=f.call<int>(type,{"java.lang.Enum","ordinal","()I"})==0;
            const bool valid=start&&context&&hit&&type&&f.check();
            for(auto local:{start,context,hit,type})if(local)f.env->DeleteLocalRef(local);
            return valid;
        },out);
}
static bool MioExposureAt(const EntityHandle& entity,const rotation::Vec3& explosion,
                         const Box& bounds,const BlockPos* virtualBase,const BlockPos* ignored,double& out) {
    if(!CrystalSupported()||!entity)return false;
    Frame f;auto world=f.level();if(!world||!f.check())return false;
    Box solid{};if(virtualBase)solid=CrystalPlacementVolume(*virtualBase,true);
    return module::crystal::SampleExposure(bounds,explosion,virtualBase?&solid:nullptr,
        [&](const rotation::Vec3& from,const rotation::Vec3& to,bool& clear){
            return module::autocrystal::ResistantRay(from,to,
                [&](const rotation::Vec3& a,const rotation::Vec3& b,BlockHitResultInfo& hit){return WorldClip(a,b,entity,hit,ClipMode::Collision);},
                [&](const BlockPos& pos,double& resistance){
                    if(ignored&&pos.x==ignored->x&&pos.y==ignored->y&&pos.z==ignored->z){resistance=0;return true;}
                    auto p=Position(f,pos),state=f.call(world,Level_getBlockState,p);
                    auto block=f.call(state,BlockBehaviour_BlockStateBase_getBlock);
                    resistance=f.call<float>(block,mapping.blastResistance);
                    const bool ok=p&&state&&block&&f.check();
                    for(auto local:{p,state,block})if(local)f.env->DeleteLocalRef(local);
                    return ok;
                },clear);
        },out);
}
bool MioCrystalExposureAt(const EntityHandle& entity,const rotation::Vec3& explosion,
                         const Box& bounds,const BlockPos* virtualBase,double& out) {
    return MioExposureAt(entity,explosion,bounds,virtualBase,nullptr,out);
}
bool MioAnchorExposure(const EntityHandle& entity,const BlockPos& pos,const Box& bounds,double& out) {
    return MioExposureAt(entity,{pos.x+.5,pos.y+.5,pos.z+.5},bounds,nullptr,&pos,out);
}
namespace {
bool SubmitCrystalUse(const BlockHitResultInfo& hit,const void* owner,bool& entered,bool offhand=false) {
    Frame f;static constexpr Member faces[]={Direction_Field_DOWN,Direction_Field_UP,Direction_Field_NORTH,Direction_Field_SOUTH,Direction_Field_WEST,Direction_Field_EAST};
    auto p=f.player(),mode=f.get(f.client(),Minecraft_Field_gameMode),hand=f.stat(offhand?InteractionHand_Field_OFF_HAND:InteractionHand_Field_MAIN_HAND);
    auto point=f.create(Vec3_Ctor,hit.location.x,hit.location.y,hit.location.z);
    auto result=f.create(BlockHitResult_Ctor,point,f.stat(faces[hit.direction]),Position(f,hit.blockPos),JNI_FALSE);
    if(!p||!mode||!hand||!result||!f.check()||!SyncSelectedItem()||!rotation::SubmitItemUse(owner))return false;
    entered=true;
    auto action=f.call(mode,MultiPlayerGameMode_useItemOn,p,hand,result);
    return action&&f.call<bool>(action,InteractionResult_consumesAction)&&f.check();
}
bool ValidUse(const BlockHitResultInfo& hit,int expectedSlot) {
    int selected=-1;
    return CrystalContextReady()&&hit.kind==HitKind::Block&&hit.hasBlockPos&&hit.direction>=0&&hit.direction<6&&
        EffectiveSelectedSlot(selected)&&selected==expectedSlot;
}
}
bool PlaceEndCrystal(const BlockHitResultInfo& hit,int expectedSlot,const void* owner,bool& entered) {
    entered=false;
    return ValidUse(hit,expectedSlot)&&CrystalHotbarSlot(expectedSlot)&&CrystalPlaceable(hit.blockPos)&&SubmitCrystalUse(hit,owner,entered);
}
bool PlaceOffhandEndCrystal(const BlockHitResultInfo& hit,const void* owner,bool& entered) {
    entered=false;int slot=-1;
    return EffectiveSelectedSlot(slot)&&ValidUse(hit,slot)&&CrystalOffhand()&&
        CrystalPlaceable(hit.blockPos)&&SubmitCrystalUse(hit,owner,entered,true);
}
bool PlaceCrystalBase(const BlockPos& base,const BlockHitResultInfo& hit,int expectedSlot,const void* owner,bool& entered) {
    entered=false;
    if(!ValidUse(hit,expectedSlot)||!CrystalObsidianSlot(expectedSlot)||!CrystalBaseBuildable(base)||
       !CrystalSpaceClear(base)||!CrystalBaseSupport(hit.blockPos))return false;
    if(hit.blockPos.x+FacingStepX(hit.direction)!=base.x||hit.blockPos.y+FacingStepY(hit.direction)!=base.y||
       hit.blockPos.z+FacingStepZ(hit.direction)!=base.z)return false;
    return SubmitCrystalUse(hit,owner,entered);
}
}
