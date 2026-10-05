#include "minecraft/java/api/game_anchor.h"
#include "minecraft/java/api/game_inventory.h"
#include "minecraft/java/api/game_itemuse.h"
#include "minecraft/java/api/game_place.h"
#include "minecraft/java/api/game_slotspoof.h"
#include "minecraft/compat/minecraft263/Minecraft263Access.h"
#include "minecraft/utils/rotation/RotationManager.h"
#include "minecraft/modules/combat/legitcrystalaura/ExplosionExposure.h"
#include "core/RuntimeControl.h"
#include <chrono>
#include <cmath>
namespace nova::java {
namespace {
using namespace compat::mc263::access;
using namespace compat::mc263::names;
struct Mapping {Member charge,provider,attribute,value,entities,sneaking,border,within,emptyFluid,minY,height;};
#define NOVA_ANCHOR_MAPPING
#define NOVA_ANCHOR(key,owner,name,desc) .key={owner,name,desc},
constexpr Mapping mapping{
#include "minecraft/compat/mappings/minecraft263.inc"
};
#undef NOVA_ANCHOR
#undef NOVA_ANCHOR_MAPPING
jobject Position(Frame& f,const BlockPos& p){return f.create(BlockPos_Ctor,p.x,p.y,p.z);}
std::string ItemName(Frame& f,jobject stack){
    if(!stack)return {};
    if(f.call<bool>(stack,ItemStack_isEmpty))return "air";
    auto item=f.call(stack,ItemStack_getItem);
    auto key=f.call(f.stat(BuiltInRegistries_Field_ITEM),Registry_getKey,item);
    return f.text(f.call(key,Identifier_getPath));
}
bool Same(const BlockPos& a,const BlockPos& b){return a.x==b.x&&a.y==b.y&&a.z==b.z;}
}
bool ReadAnchor(const BlockPos& p,AnchorState& out){
    out={};if(!CrystalSupported())return false;
    Frame f;auto world=f.level(),pos=Position(f,p),dimension=f.call(world,Level_dimensionType);
    const int min=f.call<int>(dimension,mapping.minY);
    const int height=f.call<int>(dimension,mapping.height);
    if(!dimension||!f.check()||p.y<min||p.y>=min+height||std::abs(double(p.x))>=29999984||std::abs(double(p.z))>=29999984)return false;
    auto border=f.call(world,mapping.border);
    if(!border||!f.call<bool>(border,mapping.within,pos))return false;
    auto state=f.call(world,Level_getBlockState,pos);
    auto block=f.call(state,BlockBehaviour_BlockStateBase_getBlock);
    auto key=f.call(f.stat(BuiltInRegistries_Field_BLOCK),Registry_getKey,block);
    out.anchor=f.text(f.call(key,Identifier_getPath))=="respawn_anchor";
    auto fluid=f.call(state,BlockBehaviour_BlockStateBase_getFluidState);
    out.replaceable=f.call<bool>(state,BlockBehaviour_BlockStateBase_canBeReplaced_3)&&f.call<bool>(fluid,mapping.emptyFluid);
    if(out.anchor){auto value=f.call(state,StateHolder_getValue,f.stat(mapping.charge));
        out.charges=f.call<int>(value,{"java.lang.Integer","intValue","()I"});if(!value)return false;}
    return state&&fluid&&f.check()&&out.charges>=0&&out.charges<=4;
}
bool AnchorExplodes(const BlockPos& p,bool& out){
    out=false;if(!CrystalSupported())return false;
    Frame f;auto reader=f.call(f.level(),mapping.provider);
    auto value=f.call(reader,mapping.value,f.stat(mapping.attribute),Position(f,p));
    const bool works=f.call<bool>(value,{"java.lang.Boolean","booleanValue","()Z"});
    if(!value||!f.check())return false;out=!works;return true;
}
bool AnchorBuildable(const BlockPos& p){
    AnchorState state;if(!ReadAnchor(p,state)||!state.replaceable)return false;
    Frame f;auto box=f.create(AABB_Ctor,double(p.x),double(p.y),double(p.z),double(p.x+1),double(p.y+1),double(p.z+1));
    auto entities=f.call(f.level(),mapping.entities,static_cast<jobject>(nullptr),box);
    if(!entities)return false;
    const int size=f.call<int>(entities,{"java.util.List","size","()I"});
    for(int i=0;i<size;++i){
        auto e=f.call(entities,{"java.util.List","get","(I)Ljava/lang/Object;"},i);
        bool ignored=false;
        for(const char* name:{"net.minecraft.world.entity.item.ItemEntity","net.minecraft.world.entity.ExperienceOrb",
            "net.minecraft.world.entity.decoration.ItemFrame","net.minecraft.world.entity.projectile.arrow.AbstractArrow",
            "net.minecraft.world.entity.projectile.throwableitemprojectile.ThrownExperienceBottle"}){
            if(f.instance(e,name)){ignored=true;break;}
        }
        if(e)f.env->DeleteLocalRef(e);
        if(!ignored||!f.check())return false;
    }
    return f.check();
}
bool AnchorSlot(int slot,AnchorItem kind){
    if(!CrystalSupported()||slot<0||slot>8)return false;
    Frame f;jobject stack=nullptr;if(!InventorySlot(slot,stack)||!stack)return false;
    const auto name=ItemName(f,stack);
    bool matches=kind==AnchorItem::Anchor?name=="respawn_anchor":kind==AnchorItem::Glowstone?name=="glowstone":!name.empty()&&name!="glowstone";
    // Vanilla passes main-hand use to offhand glowstone before useWithoutItem.
    if(kind==AnchorItem::Detonator){auto off=f.call(f.player(),LivingEntity_getOffhandItem);
        matches=matches&&off&&ItemName(f,off)!="glowstone";}
    return matches&&f.check();
}
bool AnchorTargetYaw(const EntityHandle& entity,float& out){
    if(!CrystalSupported()||!entity)return false;
    Frame f;out=f.call<float>(static_cast<jobject>(entity.raw()),Entity_getYRot_2);
    return f.check()&&std::isfinite(out);
}
bool AnchorOffhand(){
    if(!CrystalSupported())return false;
    Frame f;auto stack=f.call(f.player(),LivingEntity_getOffhandItem);
    return stack&&ItemName(f,stack)=="respawn_anchor"&&f.check();
}
bool AnchorExposure(const EntityHandle& entity,const BlockPos& p,const Box& bounds,double& out){
    return AnchorExposureWithShield(entity,p,bounds,nullptr,out);
}
bool AnchorExposureWithShield(const EntityHandle& entity,const BlockPos& p,const Box& bounds,const BlockPos* shield,double& out){
    if(!CrystalSupported()||!entity)return false;
    const rotation::Vec3 center{p.x+.5,p.y+.5,p.z+.5};
    // The anchor is removed before damage. Stop each exposure ray immediately
    // outside its cube, so only this block is ignored without editing the world.
    Box solid{};if(shield)solid={double(shield->x),double(shield->y),double(shield->z),double(shield->x+1),double(shield->y+1),double(shield->z+1)};
    return module::crystal::SampleExposure(bounds,center,shield?&solid:nullptr,[&](const auto& from,const auto&,bool& clear){
        auto delta=from.Subtract(center);
        const double extent=std::max({std::abs(delta.x),std::abs(delta.y),std::abs(delta.z)});
        if(extent<=.50001){clear=true;return true;}
        const auto end=center.Add(delta.x*(.50001/extent),delta.y*(.50001/extent),delta.z*(.50001/extent));
        BlockHitResultInfo hit;
        if(!WorldClip(from,end,entity,hit,ClipMode::Collision))return false;
        clear=hit.kind==HitKind::Miss;return true;
    },out);
}
int AnchorGlowstoneCount(){
    if(!CrystalSupported())return 0;
    Frame f;int count=0;
    for(int slot=0;slot<9;++slot){jobject stack=nullptr;if(!InventorySlot(slot,stack)||!stack)return 0;
        if(ItemName(f,stack)=="glowstone")count+=f.call<int>(stack,ItemStack_getCount);
        f.env->DeleteLocalRef(stack);
    }
    return f.check()?count:0;
}
static bool SubmitAnchor(const BlockPos& p,const BlockHitResultInfo& hit,int slot,AnchorAction action,const void* owner,bool& entered,bool offhand){
    const char* stage="context/slot/state";bool accepted=false;
    struct Report {
        const char*& stage;bool& entered;bool& accepted;const BlockPos& p;int slot;AnchorAction action;
        ~Report(){
            static std::chrono::steady_clock::time_point next;
            const auto now=std::chrono::steady_clock::now();if(now<next)return;next=now+std::chrono::seconds(1);
            LogTo("Anchor native: stage=%s action=%d pos=(%d,%d,%d) slot=%d entered=%d accepted=%d",
                stage,int(action),p.x,p.y,p.z,slot,int(entered),int(accepted));
        }
    } report{stage,entered,accepted,p,slot,action};
    entered=false;int selected=-1;AnchorState state;bool explodes=false;
    if(action==AnchorAction::Shield)return false; // Shield uses the block-placement adapter.
    if(!CrystalContextReady()||!EffectiveSelectedSlot(selected)||selected!=slot||!ReadAnchor(p,state)||
       !AnchorExplodes(p,explodes)||!explodes||hit.kind!=HitKind::Block||!hit.hasBlockPos||hit.direction<0||hit.direction>=6)return false;
    stage="sneaking";Frame f;if(f.call<bool>(f.player(),mapping.sneaking)||!f.check())return false;
    stage="action-recheck";
    if(action==AnchorAction::Place){
        if(!(offhand?AnchorOffhand():AnchorSlot(slot,AnchorItem::Anchor))||!AnchorBuildable(p)||!CrystalBaseSupport(hit.blockPos)||
           hit.blockPos.x+FacingStepX(hit.direction)!=p.x||hit.blockPos.y+FacingStepY(hit.direction)!=p.y||hit.blockPos.z+FacingStepZ(hit.direction)!=p.z)return false;
    }else if(!state.anchor||!Same(hit.blockPos,p)||
        (action==AnchorAction::Charge?(state.charges!=0||!AnchorSlot(slot,AnchorItem::Glowstone)):
        (state.charges==0||!AnchorSlot(slot,AnchorItem::Detonator))))return false;
    static constexpr Member faces[]={Direction_Field_DOWN,Direction_Field_UP,Direction_Field_NORTH,Direction_Field_SOUTH,Direction_Field_WEST,Direction_Field_EAST};
    auto player=f.player(),mode=f.get(f.client(),Minecraft_Field_gameMode),hand=f.stat(offhand?InteractionHand_Field_OFF_HAND:InteractionHand_Field_MAIN_HAND);
    auto point=f.create(Vec3_Ctor,hit.location.x,hit.location.y,hit.location.z);
    auto result=f.create(BlockHitResult_Ctor,point,f.stat(faces[hit.direction]),Position(f,hit.blockPos),JNI_FALSE);
    stage="jni-hit";if(!player||!mode||!result||!hand||!f.check())return false;
    stage="slot-sync";if(!SyncSelectedItem())return false;
    stage="transaction";if(!rotation::SubmitItemUse(owner))return false;
    entered=true;auto used=f.call(mode,MultiPlayerGameMode_useItemOn,player,hand,result);
    accepted=used&&f.call<bool>(used,InteractionResult_consumesAction)&&f.check();
    stage=f.check()?(accepted?"accepted":"vanilla-pass/fail"):"jni-use-failed";return accepted;
}
bool UseAnchor(const BlockPos& p,const BlockHitResultInfo& hit,int slot,AnchorAction action,const void* owner,bool& entered){
    return SubmitAnchor(p,hit,slot,action,owner,entered,false);
}
bool UseOffhandAnchor(const BlockPos& p,const BlockHitResultInfo& hit,int slot,const void* owner,bool& entered){
    return SubmitAnchor(p,hit,slot,AnchorAction::Place,owner,entered,true);
}
}
