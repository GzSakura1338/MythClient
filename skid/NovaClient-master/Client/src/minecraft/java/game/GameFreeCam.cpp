#include "minecraft/java/api/game_freecam.h"
#include "minecraft/java/api/game_freelook.h"
#include "minecraft/java/api/game_entity.h"
#include "minecraft/java/api/game_rotation.h"
#include "minecraft/java/api/game_click.h"
#include "minecraft/java/api/game_network.h"
#include "minecraft/java/api/game_options.h"
#include "minecraft/compat/minecraft263/Minecraft263Access.h"
#include "minecraft/compat/lunar1710/Access.h"
#include "minecraft/modules/render/freecam/FreeCamLogic.h"
#include "ui/clickgui/ClickGui.h"
#include <atomic>
#include <limits>
#include <mutex>

namespace nova::java {
namespace {
using compat::mc263::access::Frame;
using compat::mc263::access::Member;
struct Mapping {
    Member attack,controlled,ctor,profile,view,setView,move,yaw,pitch,oldYaw,oldPitch,xo,yo,zo,oldX,oldY,oldZ;
    Member headYaw,invisible,velocity,sprint,sneak,forward,strafe,add,remove,refresh,arm,oldArm,minSection,sectionCount,setId,setUuid,turn;
};
#define NOVA_FREECAM_MAPPING
#define NOVA_FREECAM(id,owner,name,desc) .id={owner,name,desc},
constexpr Mapping legacy{
#include "minecraft/compat/mappings/Lunar189.inc"
};
constexpr Mapping forge{
#include "minecraft/compat/mappings/forge1201.inc"
};
constexpr Mapping modern{
#include "minecraft/compat/mappings/minecraft263.inc"
};
#undef NOVA_FREECAM
#undef NOVA_FREECAM_MAPPING
constexpr bool Complete(const Mapping& m){
    for(Member entry:{m.attack,m.ctor,m.profile,m.move,m.yaw,m.pitch,m.oldYaw,m.oldPitch,
            m.xo,m.yo,m.zo,m.oldX,m.oldY,m.oldZ,m.headYaw,m.invisible,m.velocity,
            m.sprint,m.sneak,m.forward,m.strafe,m.add,m.remove,m.refresh,m.arm,m.oldArm,m.turn})
        if(!compat::mc263::access::ValidMember(entry))return false;
    return true;
}
static_assert(Complete(legacy)&&Complete(forge)&&Complete(modern),"FreeCam lifecycle mapping is incomplete");
const Mapping& Names(){return compat::IsLegacyLunar()?legacy:compat::UsesModernRuntime()?modern:forge;}
std::atomic<bool> active{false};
std::recursive_mutex mutex;
FreeCamSettings settings;
EntityHandle camera,body,world;
bool inserted=false,armHidden=false;
int lastChunkX=std::numeric_limits<int>::max(),lastChunkZ=0;
constexpr int cameraId=-8008;
bool Focused(){DWORD p=0;GetWindowThreadProcessId(GetForegroundWindow(),&p);return p==GetCurrentProcessId();}
jobject View(Frame& f){return compat::IsLunar1710()?compat::lunar1710::View(f):f.call(f.client(),Names().view);}
bool SetView(Frame& f,jobject entity){
    return compat::IsLunar1710()?f.set(f.client(),compat::lunar1710::names::View,entity):f.invoke(f.client(),Names().setView,entity);
}
void RefreshChunk(Frame& f,jobject level,int x,int z){
    const auto& m=Names();
    if(compat::IsLegacyLunar())f.invoke(level,m.refresh,x*16,0,z*16,x*16+15,256,z*16+15);
    else {
        const int first=f.call<int>(level,m.minSection),count=f.call<int>(level,m.sectionCount);
        for(int y=first;f&&y<first+count;++y)f.invoke(level,m.refresh,x,y,z);
    }
}
void RestoreArm(Frame& f,jobject player){
    if(!armHidden||!player)return;
    const auto& m=Names();const float pitch=f.get<float>(player,m.pitch);
    f.set(player,m.arm,pitch);f.set(player,m.oldArm,pitch);armHidden=false;
}
void StopLocked(){
    const bool wasActive=active.exchange(false);
    if(camera){
        // Independent frames: failure to restore a stale player cannot prevent removal.
        {
            Frame f;auto local=PlayerEntity(),level=LevelEntity();
            if(local&&level&&local.IsSame(body)&&level.IsSame(world)){
                auto p=static_cast<jobject>(local.raw());
                SetView(f,p);
                // Mouse never changed the body. Keep any server-applied correction.
                RestoreArm(f,p);
            }
        }
        if(inserted){
            Frame f;
            if(compat::IsLegacyLunar())f.invoke(static_cast<jobject>(world.raw()),Names().remove,static_cast<jobject>(camera.raw()));
            else f.invoke(static_cast<jobject>(camera.raw()),Names().remove);
        }
        {
            Frame f;auto level=LevelEntity(),local=PlayerEntity();rotation::Vec3 p;
            if(level&&local&&level.IsSame(world)&&local.Position(p)){
                const int x=static_cast<int>(std::floor(p.x/16)),z=static_cast<int>(std::floor(p.z/16));
                for(int dx=-1;dx<=1;++dx)for(int dz=-1;dz<=1;++dz)RefreshChunk(f,static_cast<jobject>(level.raw()),x+dx,z+dz);
            }
        }
    }
    camera.Reset();body.Reset();world.Reset();inserted=false;armHidden=false;
    lastChunkX=std::numeric_limits<int>::max();lastChunkZ=0;
    if(wasActive&&Focused()&&!clickgui::IsOpen())RestoreMovementKeys();
}
bool Start(Frame& f,EntityHandle local,EntityHandle level){
    bool ground=false;if(!local.OnGround(ground)||!ground)return false;
    const auto& m=Names();auto player=static_cast<jobject>(local.raw()),w=static_cast<jobject>(level.raw());
    rotation::Vec3 p;float yaw,pitch;
    if(!local.Position(p)||!PlayerRotation(yaw,pitch))return false;
    StopFreeLook();
    auto profile=f.call(player,m.profile);if(!profile)return false;
    camera=AdoptEntity(f.create(m.ctor,w,profile));if(!camera)return false;
    body=std::move(local);world=std::move(level);
    auto c=static_cast<jobject>(camera.raw());
    if(!f.invoke(c,m.move,p.x,p.y,p.z)||!f.set(c,m.yaw,yaw)||!f.set(c,m.pitch,pitch)||!f.set(c,m.headYaw,yaw)||
       !f.set(c,m.oldYaw,yaw)||!f.set(c,m.oldPitch,pitch)||
       !f.set(c,m.xo,p.x)||!f.set(c,m.yo,p.y)||!f.set(c,m.zo,p.z)||
       !f.set(c,m.oldX,p.x)||!f.set(c,m.oldY,p.y)||!f.set(c,m.oldZ,p.z)||
       !f.invoke(c,m.velocity,0.,0.,0.)||!f.invoke(c,m.invisible,JNI_TRUE))return false;
    // Avoid replacing another module's entity if Raven's reserved id is occupied.
    if(EntityById(cameraId))return false;
    if(!compat::IsLegacyLunar()){
        // Modern entity storage indexes UUID as well as id. Keep the original
        // GameProfile for appearance, but never alias the body's storage entry.
        auto uuid=f.staticCall({"java.util.UUID","randomUUID","()Ljava/util/UUID;"});
        if(!uuid||!f.invoke(c,m.setUuid,uuid))return false;
    }
    if(compat::UsesModernRuntime()){
        if(!f.invoke(c,m.setId,cameraId))return false;
        inserted=true;if(!f.invoke(w,m.add,c))return false;
    }else {inserted=true;if(!f.invoke(w,m.add,cameraId,c))return false;}
    if(!SetView(f,c))return false;
    active=true;LogTo("FreeCam: Raven camera active; vanilla body updates retained, mouse isolated");return true;
}
}
void ConfigureFreeCam(const FreeCamSettings& value){std::lock_guard guard(mutex);settings=value;}
bool FreeCamActive(){return active.load(std::memory_order_acquire);}
bool FreeCamCamera(void* entity){
    if(!entity||!active)return false;
    std::lock_guard guard(mutex);Frame f;
    return f&&camera&&f.env->IsSameObject(static_cast<jobject>(entity),static_cast<jobject>(camera.raw()));
}
bool FreeCamTurn(double dx,double dy){
    if(!active)return false;
    std::lock_guard guard(mutex);
    if(!active||!camera)return false;
    if(!Focused()||compat::ScreenOpen()||clickgui::IsOpen()||!std::isfinite(dx+dy))return true;
    Frame f;auto c=static_cast<jobject>(camera.raw());
    // Use the game's turn implementation, including its previous-angle updates.
    // LWJGL2 setAngles uses upward-positive Y; modern turn uses downward-positive Y.
    if(compat::IsLegacyLunar())f.invoke(c,Names().turn,static_cast<float>(dx),static_cast<float>(-dy));
    else f.invoke(c,Names().turn,dx,dy);
    f.set(c,Names().headYaw,f.get<float>(c,Names().yaw));
    return true;
}
void JNICALL FreeCamAnglesHook(JNIEnv* env,jclass,jobject player,jfloat dx,jfloat dy){
    if(FreeCamTurn(dx,-dy))return;
    Frame f(env);f.invoke(player,Names().turn,dx,dy);
}
bool FreeCamBlocks(FreeCamAction action){
    if(!active)return false;
    std::lock_guard guard(mutex);
    switch(action){
    case FreeCamAction::Dig:return !settings.allowDigging;
    case FreeCamAction::Interact:return !settings.allowInteracting;
    case FreeCamAction::Place:return !settings.allowPlacing;
    }
    return false;
}
bool FreeCamBlocksMouse(bool right){
    if(!active)return false;
    if(AttackTargetIsBlock())return FreeCamBlocks(right?FreeCamAction::Place:FreeCamAction::Dig);
    EntityHandle entity;
    return AttackTargetEntity(entity)&&FreeCamBlocks(FreeCamAction::Interact);
}
jboolean JNICALL FreeCamAttackHook(JNIEnv* env,jclass,jobject client){
    if(FreeCamBlocksMouse(false))return JNI_FALSE;
    Frame f(env);return f.call<bool>(client,Names().attack)?JNI_TRUE:JNI_FALSE;
}
void JNICALL FreeCamClickHook(JNIEnv* env,jclass,jobject client){
    if(FreeCamBlocksMouse(false))return;
    Frame f(env);f.invoke(client,Names().attack);
}
jboolean JNICALL FreeCamControlledHook(JNIEnv* env,jclass,jobject player){
    // Redirect only the send-position gate, never the physics/render camera checks.
    // Vanilla still owns position deltas, ground state, corrections and cadence.
    if(active){
        std::lock_guard guard(mutex);
        if(active&&body&&player&&env->IsSameObject(player,static_cast<jobject>(body.raw())))return JNI_TRUE;
    }
    Frame f(env);return f.call<bool>(player,Names().controlled)?JNI_TRUE:JNI_FALSE;
}
void StopFreeCam(){std::lock_guard guard(mutex);settings.enabled=false;StopLocked();}
bool UpdateFreeCam(){
    std::lock_guard guard(mutex);
    if(!settings.enabled||IsCircuitOpen()){StopLocked();return false;}
    Frame f;auto local=PlayerEntity(),level=LevelEntity();
    if(!f||!local||!level||!local.IsAlive()||(active&&(!local.IsSame(body)||!level.IsSame(world)))){StopLocked();return false;}
    if(!active&&!Start(f,std::move(local),std::move(level))){StopLocked();return false;}
    if(settings.disableOnDamage&&PlayerHurtTime()!=0){StopLocked();return false;}
    auto c=static_cast<jobject>(camera.raw()),p=static_cast<jobject>(body.raw());
    auto current=View(f);
    if(!current||!f.env->IsSameObject(current,c)){StopLocked();return false;}
    const auto& m=Names();
    f.invoke(p,m.sprint,JNI_FALSE);f.set(p,m.forward,0.f);f.set(p,m.strafe,0.f);f.invoke(p,m.sneak,JNI_FALSE);
    const float yaw=f.get<float>(c,m.yaw);
    rotation::Vec3 pos;FreeCamKeys keys;
    if(!camera.Position(pos)){StopLocked();return false;}
    if(Focused()&&!compat::ScreenOpen()&&!clickgui::IsOpen()&&ReadFreeCamKeys(keys))
        pos=module::freecam::Step(pos,yaw,keys.forward,keys.strafe,keys.up,keys.down,settings.speed);
    f.invoke(c,m.move,pos.x,pos.y,pos.z);
    const int x=static_cast<int>(std::floor(pos.x/16)),z=static_cast<int>(std::floor(pos.z/16));
    if(lastChunkX!=std::numeric_limits<int>::max()&&(x!=lastChunkX||z!=lastChunkZ))RefreshChunk(f,static_cast<jobject>(world.raw()),x,z);
    lastChunkX=x;lastChunkZ=z;
    if(!f.check()){StopLocked();return false;}
    return true;
}
void RenderFreeCam(){
    if(!active)return;
    std::lock_guard guard(mutex);Frame f;auto local=PlayerEntity();
    if(!f||!local||!local.IsSame(body))return;
    auto p=static_cast<jobject>(local.raw());
    if(settings.showArm)RestoreArm(f,p);
    else {f.set(p,Names().arm,700.f);f.set(p,Names().oldArm,700.f);armHidden=true;}
}
bool FreeCamBlocksPacket(JNIEnv* env,jobject packet){
    if(!active||!packet)return false;
    Frame f(env);
    const bool legacy=compat::IsLegacyLunar();
    // Movement and teleport acknowledgements always follow the original game path.
    if(FreeCamBlocks(FreeCamAction::Interact)&&f.instance(packet,legacy?"net.minecraft.network.play.client.C02PacketUseEntity":"net.minecraft.network.protocol.game.ServerboundInteractPacket"))return true;
    if(FreeCamBlocks(FreeCamAction::Dig)&&f.instance(packet,legacy?"net.minecraft.network.play.client.C07PacketPlayerDigging":"net.minecraft.network.protocol.game.ServerboundPlayerActionPacket"))return true;
    if(FreeCamBlocks(FreeCamAction::Place)){
        if(legacy)return f.instance(packet,"net.minecraft.network.play.client.C08PacketPlayerBlockPlacement");
        return f.instance(packet,"net.minecraft.network.protocol.game.ServerboundUseItemPacket")||f.instance(packet,"net.minecraft.network.protocol.game.ServerboundUseItemOnPacket");
    }
    return false;
}
}
