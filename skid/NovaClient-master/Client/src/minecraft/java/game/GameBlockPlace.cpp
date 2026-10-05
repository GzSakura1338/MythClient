#include "minecraft/java/api/game_blockplace.h"
#include "minecraft/java/api/game_anchor.h"
#include "minecraft/java/api/game_place.h"
#include "minecraft/java/api/game_slotspoof.h"
#include "minecraft/java/api/game_itemuse.h"
#include "minecraft/compat/minecraft263/Minecraft263Access.h"
#include "minecraft/utils/rotation/RotationManager.h"

namespace nova::java {
bool ObsidianContextReady(){
    if(!CrystalContextReady())return false;
    using namespace compat::mc263::access;using namespace compat::mc263::names;
    Frame f;auto player=f.player();
    return player&&!f.call<bool>(player,Entity_isPassenger)&&
        !f.call<bool>(player,{"net.minecraft.world.entity.LivingEntity","isSleeping","()Z"})&&f.check();
}
static bool ReadBlockName(const BlockPos& p,const char* name,bool& out){
    out=false;if(!CrystalSupported())return false;
    using namespace compat::mc263::access;using namespace compat::mc263::names;
    Frame f;auto pos=f.create(BlockPos_Ctor,p.x,p.y,p.z);
    auto state=f.call(f.level(),Level_getBlockState,pos);
    auto block=f.call(state,BlockBehaviour_BlockStateBase_getBlock);
    auto key=f.call(f.stat(BuiltInRegistries_Field_BLOCK),Registry_getKey,block);
    out=f.text(f.call(key,Identifier_getPath))==name;
    return state&&key&&f.check();
}
bool ReadObsidian(const BlockPos& p,bool& out){return ReadBlockName(p,"obsidian",out);}
bool ReadGlowstone(const BlockPos& p,bool& out){return ReadBlockName(p,"glowstone",out);}
bool GlowstoneSupport(const BlockPos& p){
    AnchorState state;return CrystalBaseSupport(p)&&ReadAnchor(p,state)&&!state.anchor;
}
bool ObsidianBuildable(const BlockPos& p){
    // Anchors and obsidian occupy the same full cube. This helper checks only
    // replaceability, world bounds, fluids and entities, never anchor damage.
    return AnchorBuildable(p);
}
static bool PlaceCube(const BlockPos& p,const BlockHitResultInfo& hit,int slot,const void* owner,bool glowstone,bool& entered){
    using namespace compat::mc263::access;
    using namespace compat::mc263::names;
    entered=false;int selected=-1;
    if(!ObsidianContextReady()||!EffectiveSelectedSlot(selected)||selected!=slot||
       !(glowstone?AnchorSlot(slot,AnchorItem::Glowstone):CrystalObsidianSlot(slot))||
       !ObsidianBuildable(p)||hit.kind!=HitKind::Block||!hit.hasBlockPos||hit.direction<0||hit.direction>=6||
       !CrystalBaseSupport(hit.blockPos)||hit.blockPos.x+FacingStepX(hit.direction)!=p.x||
       hit.blockPos.y+FacingStepY(hit.direction)!=p.y||hit.blockPos.z+FacingStepZ(hit.direction)!=p.z)return false;
    // Clicking an anchor with glowstone charges it instead of placing a shield.
    if(glowstone&&!GlowstoneSupport(hit.blockPos))return false;
    Frame f;
    static constexpr Member faces[]={Direction_Field_DOWN,Direction_Field_UP,Direction_Field_NORTH,Direction_Field_SOUTH,Direction_Field_WEST,Direction_Field_EAST};
    auto player=f.player(),mode=f.get(f.client(),Minecraft_Field_gameMode),hand=f.stat(InteractionHand_Field_MAIN_HAND);
    auto pos=f.create(BlockPos_Ctor,hit.blockPos.x,hit.blockPos.y,hit.blockPos.z);
    auto point=f.create(Vec3_Ctor,hit.location.x,hit.location.y,hit.location.z);
    auto result=f.create(BlockHitResult_Ctor,point,f.stat(faces[hit.direction]),pos,JNI_FALSE);
    if(!player||!mode||!hand||!result||!f.check()||!SyncSelectedItem()||!rotation::SubmitItemUse(owner))return false;
    entered=true;auto used=f.call(mode,MultiPlayerGameMode_useItemOn,player,hand,result);
    return used&&f.call<bool>(used,InteractionResult_consumesAction)&&f.check();
}
bool PlaceObsidian(const BlockPos& p,const BlockHitResultInfo& hit,int slot,const void* owner,bool& entered){return PlaceCube(p,hit,slot,owner,false,entered);}
bool PlaceGlowstone(const BlockPos& p,const BlockHitResultInfo& hit,int slot,const void* owner,bool& entered){return PlaceCube(p,hit,slot,owner,true,entered);}
}
