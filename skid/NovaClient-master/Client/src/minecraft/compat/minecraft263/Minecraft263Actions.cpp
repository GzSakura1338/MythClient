#include "minecraft/compat/minecraft263/Minecraft263Game.h"
#include "minecraft/compat/minecraft263/Minecraft263Access.h"
#include "minecraft/compat/easecation/EaseCation.h"
#include "minecraft/java/api/game_network.h"
#include "minecraft/utils/rotation/RotationManager.h"
#include "minecraft/java/api/game_place.h"
#include <cmath>
#include <algorithm>
#include <limits>

namespace nova::compat::mc263 {
using namespace names;
using access::Frame;
namespace {
jobject Position(Frame& f,const java::BlockPos& p){return f.create(BlockPos_Ctor,p.x,p.y,p.z);}
jobject Vector(Frame& f,const rotation::Vec3& v){return f.create(Vec3_Ctor,v.x,v.y,v.z);}
jobject Key(Frame& f,names::Member key){return f.get(f.get(f.client(),Minecraft_Field_options),key);}
bool SetKey(names::Member key,bool down){Frame f;return f.invoke(Key(f,key),KeyMapping_setDown,down?JNI_TRUE:JNI_FALSE);}
bool ReadPos(Frame& f,jobject pos,java::BlockPos& out){if(!pos)return false;out={f.call<int>(pos,Vec3i_getX),f.call<int>(pos,Vec3i_getY),f.call<int>(pos,Vec3i_getZ)};return f.check();}
names::Member Facing(int facing){static constexpr names::Member values[]={Direction_Field_DOWN,Direction_Field_UP,Direction_Field_NORTH,Direction_Field_SOUTH,Direction_Field_WEST,Direction_Field_EAST};return values[std::clamp(facing,0,5)];}

bool ChestOutline(Frame& f, jobject level, jobject state, jobject position, java::ChestBox& out) {
    java::BlockPos pos;
    if (!ReadPos(f, position, pos)) return false;
    auto shape = f.call(state, BlockBehaviour_BlockStateBase_getShape, level, position);
    if (!shape || f.call<bool>(shape, VoxelShape_isEmpty)) return false;
    auto bounds = f.call(shape, VoxelShape_bounds);
    if (!bounds) return false;
    out.x=pos.x; out.y=pos.y; out.z=pos.z;
    out.minX=pos.x+f.get<double>(bounds,AABB_Field_minX);
    out.minY=pos.y+f.get<double>(bounds,AABB_Field_minY);
    out.minZ=pos.z+f.get<double>(bounds,AABB_Field_minZ);
    out.maxX=pos.x+f.get<double>(bounds,AABB_Field_maxX);
    out.maxY=pos.y+f.get<double>(bounds,AABB_Field_maxY);
    out.maxZ=pos.z+f.get<double>(bounds,AABB_Field_maxZ);
    return f.check();
}

bool ChestBoxAt(Frame& f, jobject level, jobject entity, java::ChestBox& out) {
    auto state=f.call(entity,BlockEntity_getBlockState);
    auto property=f.stat(ChestBlock_Field_TYPE);
    if (!f.call<bool>(state,StateHolder_hasProperty,property)) return false;
    auto type=f.call(state,StateHolder_getValue,property);
    const int ordinal=f.call<int>(type,{"java.lang.Enum","ordinal","()I"});
    if (!f.check() || ordinal==1) return false; // LEFT; RIGHT owns the union.
    auto position=f.call(entity,BlockEntity_getBlockPos);
    if (!ChestOutline(f,level,state,position,out)) return false;
    if (ordinal!=0) {
        auto direction=f.staticCall(ChestBlock_getConnectedDirection,state);
        auto neighbour=f.call(position,BlockPos_relative,direction);
        auto neighbourState=f.call(level,BlockGetter_getBlockState,neighbour);
        java::ChestBox second;
        if (ChestOutline(f,level,neighbourState,neighbour,second)) {
            out.minX=std::min(out.minX,second.minX); out.maxX=std::max(out.maxX,second.maxX);
            out.minY=std::min(out.minY,second.minY); out.maxY=std::max(out.maxY,second.maxY);
            out.minZ=std::min(out.minZ,second.minZ); out.maxZ=std::max(out.maxZ,second.maxZ);
        }
    }
    return f.check();
}
}
bool MouseGrabbed(){Frame f;return f.call<bool>(f.get(f.client(),Minecraft_Field_mouseHandler),MouseHandler_isMouseGrabbed);}
bool ClickAttack(){Frame f;auto mc=f.client();if(compat::ScreenOpen()||!f.player())return false;f.call<bool>(mc,Minecraft_startAttack);return f.check();}
bool SetAttackDown(bool down){return SetKey(Options_Field_keyAttack,down);}
bool ClearMissTime(){Frame f;return f.set(f.client(),Minecraft_Field_missTime,0);}
bool AttackTargetIsBlock(){Frame f;auto hit=f.get(f.client(),Minecraft_Field_hitResult);return f.instance(hit,BlockHitResult)&&!f.get<bool>(hit,BlockHitResult_Field_miss);}
bool HitBlockPos(java::BlockPos& out){Frame f;auto hit=f.get(f.client(),Minecraft_Field_hitResult);return f.instance(hit,BlockHitResult)&&!f.get<bool>(hit,BlockHitResult_Field_miss)&&ReadPos(f,f.call(hit,BlockHitResult_getBlockPos),out);}
bool TargetedBlockBreakable(){Frame f;java::BlockPos pos;if(!HitBlockPos(pos))return false;auto level=f.level(),p=Position(f,pos);auto state=f.call(level,Level_getBlockState,p);return state&&!f.call<bool>(state,BlockBehaviour_BlockStateBase_isAir)&&f.call<float>(state,BlockBehaviour_BlockStateBase_getDestroySpeed,level,p)>=0&&f.check();}
bool AttackTargetEntity(java::EntityHandle& out){Frame f;out={};auto hit=f.get(f.client(),Minecraft_Field_hitResult);if(!f.instance(hit,EntityHitResult))return false;out=java::AdoptEntity(f.call(hit,EntityHitResult_getEntity));return out.Valid()&&f.check();}
bool AttackEntity(const java::EntityHandle& target){Frame f;if(!target)return false;auto p=f.player();return p&&f.invoke(f.get(f.client(),Minecraft_Field_gameMode),MultiPlayerGameMode_attack,p,static_cast<jobject>(target.raw()));}
bool SwingHand(){Frame f;auto p=f.player();if(UsesGlfwModernRuntime())return f.invoke(p,easecation::names::Swing,f.stat(InteractionHand_Field_MAIN_HAND));auto stack=f.call(p,LivingEntity_getMainHandItem);auto animation=f.call(stack,ItemStack_getAttackAnimation);f.call<bool>(p,LivingEntity_swing,f.stat(InteractionHand_Field_MAIN_HAND),animation,JNI_FALSE);return p&&f.check()&&mc263::SendPacket(f.stat(ServerboundPunchPacket_Field_INSTANCE));}
bool SwingHand(bool offhand,bool animate) {
    if(!offhand)return animate?mc263::SwingHand():mc263::SendSwingPacket();
    Frame f;auto p=f.player(),hand=f.stat(InteractionHand_Field_OFF_HAND);
    if(!p||!hand)return false;
    if(UsesGlfwModernRuntime()) {
        if(animate)return f.invoke(p,easecation::names::Swing,hand);
        return mc263::SendPacket(f.create(easecation::names::SwingPacket,hand));
    }
    if(animate) {
        auto stack=f.call(p,LivingEntity_getItemBySlot,f.stat(EquipmentSlot_Field_OFFHAND));
        auto animation=f.call(stack,ItemStack_getAttackAnimation);
        if(!stack||!animation)return false;
        f.call<bool>(p,LivingEntity_swing,hand,animation,JNI_FALSE);
    }
    return f.check()&&mc263::SendPacket(f.stat(ServerboundPunchPacket_Field_INSTANCE));
}
// The 26.3 main-hand swing is transmitted as Punch.
bool SendSwingPacket(){Frame f;return mc263::SendPacket(UsesGlfwModernRuntime()?f.create(easecation::names::SwingPacket,f.stat(InteractionHand_Field_MAIN_HAND)):f.stat(ServerboundPunchPacket_Field_INSTANCE));}
bool RightClickDelay(int& out){Frame f;auto mc=f.client();out=f.get<int>(mc,Minecraft_Field_rightClickDelay);return mc&&f.check();}
bool SetRightClickDelay(int value){Frame f;return f.set(f.client(),Minecraft_Field_rightClickDelay,value);}
bool KeyDown(InputKey key){Frame f;return f.call<bool>(Key(f,key==InputKey::Use?Options_Field_keyUse:key==InputKey::Jump?Options_Field_keyJump:Options_Field_keyShift),KeyMapping_isDown);}
bool SetKeyShiftDown(bool down){return SetKey(Options_Field_keyShift,down);}
bool ReadBlockProperty(const java::BlockPos& pos,BlockProperty property){
    Frame f;auto level=f.level();auto p=Position(f,pos);auto state=f.call(level,Level_getBlockState,p);if(!state)return false;
    if(property==BlockProperty::ScaffoldExcluded)return java::ScaffoldExcludedObject(f.env,state);
    if(property==BlockProperty::Replaceable)return f.call<bool>(state,BlockBehaviour_BlockStateBase_canBeReplaced_3)&&f.check();
    if(property==BlockProperty::OpensMenu)return f.call(state,BlockBehaviour_BlockStateBase_getMenuProvider,level,p)!=nullptr&&f.check();
    auto shape=f.call(state,BlockBehaviour_BlockStateBase_getCollisionShape,level,p);
    return f.call<bool>(shape,VoxelShape_isEmpty)&&f.check();
}
int FacingStep(int facing,int axis){static const int steps[6][3]={{0,-1,0},{0,1,0},{0,0,-1},{0,0,1},{-1,0,0},{1,0,0}};return facing>=0&&facing<6&&axis>=0&&axis<3?steps[facing][axis]:0;}
int FacingOpposite(int facing){return facing>=0&&facing<6?facing^1:facing;}
bool UseItemOnBlock(const java::BlockPos& pos,int facing,const rotation::Vec3& hit){
    if(facing<0||facing>5)return false;Frame f;auto player=f.player();if(!player)return false;
    auto result=f.create(BlockHitResult_Ctor,Vector(f,hit),f.stat(Facing(facing)),Position(f,pos),JNI_FALSE);
    // Block placement has no embedded yaw/pitch and may legitimately run after
    // motion. Only defer it when another automated item already used this tick.
    if(!result || !f.check() || rotation::AutomatedItemUseSubmitted())return false;
    auto action=f.call(f.get(f.client(),Minecraft_Field_gameMode),MultiPlayerGameMode_useItemOn,player,f.stat(InteractionHand_Field_MAIN_HAND),result);
    return action&&f.call<bool>(action,InteractionResult_consumesAction)&&f.check();
}
bool GameModeDestroying(){Frame f;return f.call<bool>(f.get(f.client(),Minecraft_Field_gameMode),MultiPlayerGameMode_isDestroying);}
bool BestToolSlotFor(const java::BlockPos& pos,int& out){
    Frame f;auto inv=f.call(f.player(),Player_getInventory);auto state=f.call(f.level(),Level_getBlockState,Position(f,pos));if(!inv||!state)return false;
    out=f.call<int>(inv,Inventory_getSelectedSlot);float best=1.0f;
    for(int slot=0;slot<9;++slot){auto stack=f.call(inv,Inventory_getItem,slot);float speed=f.call<float>(stack,ItemStack_getDestroySpeed,state);
        if(speed>1){java::ItemFacts facts;if(mc263::ReadItemFacts(stack,facts)&&facts.efficiency>0)speed+=static_cast<float>(facts.efficiency*facts.efficiency+1);}
        if(speed>best){best=speed;out=slot;}if(stack)f.env->DeleteLocalRef(stack);
    }return f.check();
}
bool WorldSpawn(int& x,int& z){
    if(IsEaseCation()){Frame f;java::BlockPos pos;if(!ReadPos(f,f.call(f.level(),easecation::names::Spawn),pos))return false;x=pos.x;z=pos.z;return f.check();}
    Frame f;auto level=f.level();auto data=f.call(level,Level_getRespawnData);if(!data)return false;
    auto dim=f.call(level,Level_dimension),spawnDim=f.call(data,RespawnData_dimension);
    if(!f.call<bool>(dim,{"java.lang.Object","equals","(Ljava/lang/Object;)Z"},spawnDim))return false;
    java::BlockPos pos;if(!ReadPos(f,f.call(data,RespawnData_pos),pos))return false;x=pos.x;z=pos.z;return true;
}
bool LoadedChestBoxes(std::vector<java::ChestBox>& out){
    out.clear();Frame f;auto level=f.level();auto cache=f.call(level,ClientLevel_getChunkSource);
    auto chunks=f.get(f.get(cache,ClientChunkCache_Field_storage),ClientChunkCache_Storage_Field_chunks);if(!chunks)return false;
    int count=f.call<int>(chunks,{"java.util.concurrent.atomic.AtomicReferenceArray","length","()I"});
    for(int i=0;i<count&&f;++i){
        Frame local(f.env);auto chunk=local.call(chunks,{"java.util.concurrent.atomic.AtomicReferenceArray","get","(I)Ljava/lang/Object;"},i);if(!chunk)continue;
        auto values=local.call(local.call(chunk,LevelChunk_getBlockEntities),{"java.util.Map","values","()Ljava/util/Collection;"});
        auto it=local.call(values,{"java.lang.Iterable","iterator","()Ljava/util/Iterator;"});
        while(local.call<bool>(it,{"java.util.Iterator","hasNext","()Z"})){
            Frame entry(f.env);
            auto be=entry.call(it,{"java.util.Iterator","next","()Ljava/lang/Object;"});
            if(entry.instance(be,ChestBlockEntity)){
                java::ChestBox box;
                if(ChestBoxAt(entry,level,be,box))out.push_back(box);
            }
            if(!entry.check()){out.clear();return false;}
        }if(!local.check())return false;
    }return f.check();
}
bool ReadChestOpenEvent(jobject packet,int& x,int& y,int& z){
    Frame f;if(!f.instance(packet,ClientboundBlockEventPacket))return false;
    if(f.call<int>(packet,ClientboundBlockEventPacket_getB0)!=1||f.call<int>(packet,ClientboundBlockEventPacket_getB1)!=1)return false;
    if(!f.instance(f.call(packet,ClientboundBlockEventPacket_getBlock),ChestBlock))return false;
    java::BlockPos pos;if(!ReadPos(f,f.call(packet,ClientboundBlockEventPacket_getPos),pos))return false;x=pos.x;y=pos.y;z=pos.z;return true;
}
bool IsPacket(jobject packet,PacketKind kind){
    Frame f;if(!packet)return false;auto is=[&](const char* c){return f.instance(packet,c);};
    switch(kind){
    case PacketKind::UseItem:return is(ServerboundUseItemPacket)||is(ServerboundUseItemOnPacket);
    case PacketKind::UseItemOnly:return is(ServerboundUseItemPacket);
    case PacketKind::VelocityDelay:return is(ClientboundSetEntityMotionPacket)||is(ClientboundPingPacket)||is(ClientboundExplodePacket);
    case PacketKind::ReduceDelay:return is(ClientboundSetEntityMotionPacket)||is(ClientboundMoveEntityPacket)||
        is(ClientboundTeleportEntityPacket)||is(ClientboundPingPacket)||is(ClientboundPlayerLookAtPacket)||
        is(ClientboundPlayerPositionPacket);
    case PacketKind::Motion:return is(ClientboundSetEntityMotionPacket);
    case PacketKind::PositionOrExplode:return is(ClientboundPlayerPositionPacket)||is(ClientboundExplodePacket);
    case PacketKind::Ping:return is(ClientboundPingPacket);
    case PacketKind::Explode:return is(ClientboundExplodePacket);
    case PacketKind::Position:return is(ClientboundPlayerPositionPacket);
    case PacketKind::Disconnect:return is(ClientboundDisconnectPacket);
    case PacketKind::Respawn:return is(ClientboundRespawnPacket);
    case PacketKind::ContainerClose:return is(ServerboundContainerClosePacket);
    case PacketKind::MovePlayer:return is(ServerboundMovePlayerPacket);
    case PacketKind::UseItemOn:return is(ServerboundUseItemOnPacket);
    case PacketKind::Interact:return is(ServerboundInteractPacket)||(IsMinecraft263()&&is(ServerboundAttackPacket));
    case PacketKind::PlayerAction:return is(ServerboundPlayerActionPacket);
    }return false;
}
bool SendPacket(jobject packet){Frame f;return packet&&f.invoke(f.call(f.client(),Minecraft_getConnection),ClientCommonPacketListenerImpl_send,packet);}
bool SendStatusOnlyPacket(bool ground){Frame f;return mc263::SendPacket(f.create(ServerboundMovePlayerPacket_StatusOnly_Ctor,ground?JNI_TRUE:JNI_FALSE,f.get<bool>(f.player(),Entity_Field_horizontalCollision)?JNI_TRUE:JNI_FALSE));}
bool SendMoveRotation(float yaw,float pitch,bool ground){Frame f;return mc263::SendPacket(f.create(ServerboundMovePlayerPacket_Rot_Ctor,yaw,pitch,ground?JNI_TRUE:JNI_FALSE,f.get<bool>(f.player(),Entity_Field_horizontalCollision)?JNI_TRUE:JNI_FALSE));}
bool ReplayIncoming(JNIEnv* env,jobject packet,jobject listener){
    if(!env||!packet||!listener)return false;
    auto method=access::Method(env,Packet_handle);if(!method)return false;
    env->CallVoidMethod(packet,method,listener);
    // Preserve exceptions here: Connection handles the game's thread handoff exception.
    return !env->ExceptionCheck();
}
bool HandleIncomingPacket(jobject packet){
    Frame f;auto listener=f.call(f.client(),Minecraft_getConnection);
    if(!packet||!listener||!f.check())return false;
    if(mc263::ReplayIncoming(f.env,packet,listener))return true;
    // An off-thread replay has already queued the packet when vanilla throws this sentinel.
    auto exception=f.env->ExceptionOccurred();if(!exception)return false;
    f.env->ExceptionClear();
    return f.instance(exception,RunningOnDifferentThreadException);
}
bool ReadMotionPacket(jobject packet,int& id,int& x,int& y,int& z){
    Frame f;if(!f.instance(packet,ClientboundSetEntityMotionPacket))return false;
    if(IsEaseCation()){
        id=f.call<int>(packet,ClientboundSetEntityMotionPacket_id);
        x=static_cast<int>(std::round(f.call<double>(packet,easecation::names::MotionX)*8000.0));
        y=static_cast<int>(std::round(f.call<double>(packet,easecation::names::MotionY)*8000.0));
        z=static_cast<int>(std::round(f.call<double>(packet,easecation::names::MotionZ)*8000.0));
        return f.check();
    }
    id=f.call<int>(packet,ClientboundSetEntityMotionPacket_id);auto movement=f.call(packet,ClientboundSetEntityMotionPacket_movement);if(!movement)return false;
    // The common module API uses fixed-point units. Only this adapter converts the new Vec3 protocol.
    const double vx=f.get<double>(movement,Vec3_Field_x),vy=f.get<double>(movement,Vec3_Field_y),vz=f.get<double>(movement,Vec3_Field_z);
    if(!f.check()||!std::isfinite(vx)||!std::isfinite(vy)||!std::isfinite(vz))return false;
    auto fixed=[](double v){return static_cast<int>(std::clamp(std::round(v*8000.0),static_cast<double>(INT_MIN),static_cast<double>(INT_MAX)));};
    x=fixed(vx);y=fixed(vy);z=fixed(vz);return true;
}
bool IsPosRot(void* packet){Frame f;return f.instance(static_cast<jobject>(packet),ServerboundMovePlayerPacket_PosRot);}
bool ReadPosRot(void* packet,java::PosRot& out){
    Frame f;auto p=static_cast<jobject>(packet);if(!f.instance(p,ServerboundMovePlayerPacket_PosRot))return false;
    out.x=f.call<double>(p,ServerboundMovePlayerPacket_getX,0.0);out.y=f.call<double>(p,ServerboundMovePlayerPacket_getY,0.0);out.z=f.call<double>(p,ServerboundMovePlayerPacket_getZ,0.0);
    out.yaw=f.call<float>(p,ServerboundMovePlayerPacket_getYRot,0.0f);out.pitch=f.call<float>(p,ServerboundMovePlayerPacket_getXRot,0.0f);out.onGround=f.call<bool>(p,ServerboundMovePlayerPacket_isOnGround);
    out.horizontalCollision=f.call<bool>(p,ServerboundMovePlayerPacket_horizontalCollision);return f.check();
}
void* MakePosRot(const java::PosRot& v){Frame f;return f.keep(f.create(ServerboundMovePlayerPacket_PosRot_Ctor_2,v.x,v.y,v.z,v.yaw,v.pitch,v.onGround?JNI_TRUE:JNI_FALSE,v.horizontalCollision?JNI_TRUE:JNI_FALSE));}
bool ReadHealthScore(jobject packet,std::string& owner,int& score){
    owner.clear();score=0;
    Frame f;if(!f.instance(packet,ClientboundSetScorePacket))return false;
    const auto objective=f.text(f.call(packet,ClientboundSetScorePacket_objectiveName));
    if(!f || (objective!="health" && objective!="belowHealth"))return false;
    auto name=f.text(f.call(packet,ClientboundSetScorePacket_owner));
    const int value=f.call<int>(packet,ClientboundSetScorePacket_score);
    if(!f.check() || name.empty())return false;
    owner=std::move(name);score=value;return true;
}
bool ReadStoredHealthScores(std::unordered_map<std::string,int>& scores,std::string* objectives){
    scores.clear();if(objectives)objectives->clear();
    if(!IsLunar12111())return false;
    Frame f;
    auto board=f.call(f.level(),{"net.minecraft.client.multiplayer.ClientLevel","getScoreboard","()Lnet/minecraft/world/scores/Scoreboard;"});
    if(!board || !f.check())return false;
    constexpr Member iterator{"java.util.Collection","iterator","()Ljava/util/Iterator;"};
    constexpr Member hasNext{"java.util.Iterator","hasNext","()Z"};
    constexpr Member next{"java.util.Iterator","next","()Ljava/lang/Object;"};
    if(objectives){
        auto names=f.call(board,{"net.minecraft.world.scores.Scoreboard","getObjectiveNames","()Ljava/util/Collection;"});
        auto it=f.call(names,iterator);
        for(int i=0;i<32 && f.call<bool>(it,hasNext);++i){
            auto name=f.call(it,next);
            auto text=f.text(name);
            for(auto& c:text)if(static_cast<unsigned char>(c)<32)c='?';
            if(!objectives->empty())*objectives+=",";
            *objectives+=text.substr(0,80);
            if(name)f.env->DeleteLocalRef(name);
        }
    }
    // Read only existing entries: never create a missing score with a fake zero.
    // belowHealth takes priority when both accepted objectives are present.
    for(const char* name:{"health","belowHealth"}){
        auto key=f.env->NewStringUTF(name);
        auto objective=f.call(board,{"net.minecraft.world.scores.Scoreboard","getObjective","(Ljava/lang/String;)Lnet/minecraft/world/scores/Objective;"},key);
        if(key)f.env->DeleteLocalRef(key);
        if(!objective)continue;
        auto entries=f.call(board,{"net.minecraft.world.scores.Scoreboard","listPlayerScores","(Lnet/minecraft/world/scores/Objective;)Ljava/util/Collection;"},objective);
        auto it=f.call(entries,iterator);
        for(int i=0;i<4096 && f.call<bool>(it,hasNext);++i){
            auto entry=f.call(it,next);
            auto owner=f.call(entry,{"net.minecraft.world.scores.PlayerScoreEntry","owner","()Ljava/lang/String;"});
            auto text=f.text(owner);
            const auto score=f.call<int>(entry,{"net.minecraft.world.scores.PlayerScoreEntry","value","()I"});
            if(f.check() && !text.empty())scores.insert_or_assign(std::move(text),score);
            if(owner)f.env->DeleteLocalRef(owner);
            if(entry)f.env->DeleteLocalRef(entry);
        }
    }
    if(!f.check()){scores.clear();return false;}
    return true;
}
}
