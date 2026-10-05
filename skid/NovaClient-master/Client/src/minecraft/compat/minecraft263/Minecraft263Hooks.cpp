#include "minecraft/java/api/game_freelook.h"
#include "minecraft/java/api/game_freecam.h"
#include "minecraft/java/api/game_fakeplayer.h"
#include "minecraft/java/api/game_critical.h"
#include "minecraft/java/api/game_breaker.h"
#include "minecraft/java/api/game_keepsprint.h"
#include "minecraft/modules/player/scaffold/Scaffold.h"
#include "minecraft/java/api/game_backtrack.h"
#include "minecraft/compat/minecraft263/Minecraft263Game.h"
#include "minecraft/compat/minecraft263/Minecraft263Access.h"
#include "minecraft/compat/easecation/EaseCation.h"
#include "minecraft/compat/neoforge12110/NeoForge12110.h"
#include "minecraft/compat/lunar12111/Mapping.h"
#include "minecraft/accounts/lunar/AccountHooks.h"
#include "minecraft/events/GameEvents.h"
#include "minecraft/java/api/game_network.h"
#include "minecraft/java/api/game_place.h"
#include "minecraft/java/api/game_slotspoof.h"
#include "minecraft/utils/rotation/RotationManager.h"
#include "minecraft/utils/rotation/PlacementWire.h"
#include <cmath>
#include <exception>

namespace nova::compat::mc263 {
using namespace names;
using access::Frame;
namespace {
template<class E> bool Dispatch(E& e) noexcept {
    if (IsCircuitOpen()) return false;
    try { return event::EventBus::Get().Post(e); }
    catch(const std::exception& ex){LogTo("26.3: event exception: %s",ex.what());}
    catch(...){LogTo("26.3: event exception");}
    return false;
}
bool Local(Frame& f,jobject obj){auto p=f.player();return p&&obj&&f.env->IsSameObject(p,obj);}
struct Motion { float yaw{},pitch{};bool ground{},active{}; };
// sendPosition is synchronous on the client thread. The head always refreshes state.
Motion motion;
DWORD networkReentrySlot=TLS_OUT_OF_INDEXES;
struct NetworkScope {
    bool owner=false;
    NetworkScope(){if(networkReentrySlot!=TLS_OUT_OF_INDEXES&&!TlsGetValue(networkReentrySlot))owner=TlsSetValue(networkReentrySlot,reinterpret_cast<void*>(1))!=FALSE;}
    ~NetworkScope(){if(owner)TlsSetValue(networkReentrySlot,nullptr);}
};
void JNICALL GameTickHook(JNIEnv*,jclass){java::TickFakePlayer();rotation::BeginInteractionTick();event::GameTickEvent e(event::EventType::Pre);Dispatch(e);}
void JNICALL PostGameTickHook(JNIEnv*,jclass){event::GameTickEvent e(event::EventType::Post);Dispatch(e);}
void JNICALL InteractHook(JNIEnv*,jclass){event::PrePlayerInteractEvent e;Dispatch(e);java::SuppressFreeCamKeys();}
jboolean JNICALL AttackGuardHook(JNIEnv* env,jclass type,jobject,jobject,jobject target){
    if(IsCircuitOpen())return JNI_FALSE;
    if(!rotation::EnterNativeAttack())return JNI_TRUE;
    if(java::AttackFakePlayer(env,target))return JNI_TRUE;
    java::BacktrackAttackHook(env,type,target);
    return JNI_FALSE;
}
void JNICALL CursorPickEndHook(JNIEnv*, jclass, jfloat partialTicks) {
    event::CursorPickEvent e(event::EventType::Post, partialTicks); Dispatch(e);
}
jboolean JNICALL ClickHook(JNIEnv*,jclass,jobject){
    if(java::FreeCamBlocksMouse(true))return JNI_TRUE;
    if(IsCircuitOpen())return JNI_FALSE;
    if(rotation::BlocksManualItemUse())return JNI_TRUE;
    event::ClickEvent e;return Dispatch(e)?JNI_TRUE:JNI_FALSE;
}
void JNICALL RenderHook(JNIEnv*,jclass){event::RenderFrameEvent e;Dispatch(e);}
jboolean JNICALL PlayerTickHook(JNIEnv* env,jclass,jobject player){
    if(IsCircuitOpen())return JNI_FALSE;
    if(java::ShouldSkipPlayerTick(player))return JNI_TRUE;
    Frame f(env);if(f.get<int>(player,Entity_Field_tickCount)<=1){rotation::ResetPlacementWire();event::RespawnEvent e;Dispatch(e);}
    event::PlayerTickEvent e;return Dispatch(e)?JNI_TRUE:JNI_FALSE;
}
void JNICALL InputHook(JNIEnv* env,jclass,jobject input){
    if(IsCircuitOpen())return;Frame f(env);
    auto keys=f.get(input,ClientInput_Field_keyPresses);if(!keys)return;
    float forward=static_cast<float>(f.call<bool>(keys,Input_forward))-static_cast<float>(f.call<bool>(keys,Input_backward));
    float strafe=static_cast<float>(f.call<bool>(keys,Input_left))-static_cast<float>(f.call<bool>(keys,Input_right));
    bool jump=f.call<bool>(keys,Input_jump),sneak=f.call<bool>(keys,Input_shift),sprint=f.call<bool>(keys,Input_sprint);
    if(!f.check())return;
    event::MoveInputEvent e(forward,strafe,jump,sneak,0.3);Dispatch(e);
    sprint=(sprint||e.requestSprint)&&!e.suppressSprint;
    if(java::FreeCamActive()){e.forward=e.strafe=0;e.jump=e.sneak=false;sprint=false;}
    // Preserve the record and vector invariant expected by LocalPlayer and input packets.
    auto next=f.create(Input_Ctor,e.forward>0?JNI_TRUE:JNI_FALSE,e.forward<0?JNI_TRUE:JNI_FALSE,
        e.strafe>0?JNI_TRUE:JNI_FALSE,e.strafe<0?JNI_TRUE:JNI_FALSE,e.jump?JNI_TRUE:JNI_FALSE,e.sneak?JNI_TRUE:JNI_FALSE,sprint?JNI_TRUE:JNI_FALSE);
    const float len=std::sqrt(e.strafe*e.strafe+e.forward*e.forward);
    const float divisor=std::max(1.0f,len);
    auto vector=f.create(Vec2_Ctor,e.strafe/divisor,e.forward/divisor);
    if(next&&vector){f.set(input,ClientInput_Field_keyPresses,next);f.set(input,ClientInput_Field_moveVector,vector);}
}
void JNICALL BeginMotionHook(JNIEnv* env,jclass,jobject player){
    motion={};if(IsCircuitOpen())return;Frame f(env);
    const float yaw=f.get<float>(player,Entity_Field_yRot),pitch=f.get<float>(player,Entity_Field_xRot);
    const bool ground=f.call<bool>(player,Entity_onGround);
    event::MotionEvent e(event::EventType::Pre,f.call<double>(player,Entity_getX),f.call<double>(player,Entity_getY),f.call<double>(player,Entity_getZ),yaw,pitch,ground);
    if(!f.check())return;Dispatch(e);
    motion=java::FreeCamActive()?Motion{yaw,pitch,ground,true}:Motion{e.yaw,e.pitch,e.onGround,true};
    if(!java::FreeCamActive())rotation::PublishInteractionMotion(motion.yaw,motion.pitch);
}
void JNICALL EndMotionHook(JNIEnv*,jclass,jobject){
    if(!motion.active)return;
    auto saved=motion;motion.active=false;
    event::MotionEvent e(event::EventType::Post,saved.yaw,saved.pitch);Dispatch(e);
}
jfloat JNICALL MotionYawHook(JNIEnv* env,jclass,jobject p){Frame f(env);return motion.active&&!IsCircuitOpen()?motion.yaw:f.call<float>(p,Entity_getYRot_2);}
jfloat JNICALL MotionPitchHook(JNIEnv* env,jclass,jobject p){Frame f(env);return motion.active&&!IsCircuitOpen()?motion.pitch:f.call<float>(p,Entity_getXRot_2);}
jboolean JNICALL MotionGroundHook(JNIEnv* env,jclass,jobject p){Frame f(env);return (motion.active&&!IsCircuitOpen()?motion.ground:f.call<bool>(p,Entity_onGround))?JNI_TRUE:JNI_FALSE;}
jfloat JNICALL StrafeHook(JNIEnv* env,jclass,jobject p){Frame f(env);float yaw=f.call<float>(p,Entity_getYRot_2);if(!Local(f,p))return yaw;event::StrafeEvent e(yaw);Dispatch(e);return e.yaw;}
jfloat JNICALL JumpHook(JNIEnv* env,jclass,jobject p){Frame f(env);float yaw=f.call<float>(p,Entity_getYRot_2);if(!Local(f,p))return yaw;event::JumpEvent e(yaw);Dispatch(e);return e.yaw;}
jfloat JNICALL AttackYawHook(JNIEnv* env,jclass,jobject p){Frame f(env);float yaw=f.call<float>(p,Entity_getYRot_2);if(!Local(f,p))return yaw;event::AttackYawEvent e(yaw);Dispatch(e);return e.yaw;}
jfloat JNICALL FallFlyingHook(JNIEnv* env,jclass,jobject p){Frame f(env);float pitch=f.call<float>(p,Entity_getXRot_2);if(!Local(f,p))return pitch;event::FallFlyingEvent e(pitch);Dispatch(e);return e.pitch;}
jfloat ItemRotation(JNIEnv* env,jobject p,bool pitch){
    Frame f(env);float y=f.call<float>(p,Entity_getYRot_2),x=f.call<float>(p,Entity_getXRot_2);
    if(!Local(f,p))return pitch?x:y;
    event::UseItemRayTraceEvent e(y,x);Dispatch(e);return pitch?e.pitch:e.yaw;
}
jfloat JNICALL ItemYawHook(JNIEnv* env,jclass,jobject p){return ItemRotation(env,p,false);}
jfloat JNICALL ItemPitchHook(JNIEnv* env,jclass,jobject p){return ItemRotation(env,p,true);}
jfloat JNICALL UseYawHook(JNIEnv* env,jclass,jobject p){
    Frame f(env);if(IsCircuitOpen()||!Local(f,p))return f.call<float>(p,Entity_getYRot_2);
    return rotation::CommitInteractionRotation().yaw;
}
jfloat JNICALL UsePitchHook(JNIEnv* env,jclass,jobject p){
    Frame f(env);if(IsCircuitOpen()||!Local(f,p))return f.call<float>(p,Entity_getXRot_2);
    return rotation::CommitInteractionRotation().pitch;
}
void JNICALL AnimationHook(JNIEnv* env,jclass,jobject p,jobject state,jfloat partial){
    Frame f(env);if(IsCircuitOpen()||!Local(f,p))return;
    const float yaw=f.get<float>(p,LivingEntity_Field_yHeadRot),lastYaw=f.get<float>(p,LivingEntity_Field_yHeadRotO);
    const float pitch=f.get<float>(p,Entity_Field_xRot),lastPitch=f.get<float>(p,Entity_Field_xRotO);
    if(!f.check())return;
    event::RotationAnimationEvent e(p,yaw,lastYaw,pitch,lastPitch);Dispatch(e);
    if(e.yaw==yaw&&e.lastYaw==lastYaw&&e.pitch==pitch&&e.lastPitch==lastPitch)return;
    auto wrap=[](float angle){return angle-360.0f*std::floor((angle+180.0f)/360.0f);};
    const float original=lastYaw+partial*wrap(yaw-lastYaw);
    const float adjusted=e.lastYaw+partial*wrap(e.yaw-e.lastYaw);
    const float body=f.get<float>(state,LivingEntityRenderState_Field_bodyRot)+wrap(adjusted-original);
    const float sign=f.get<bool>(state,LivingEntityRenderState_Field_isUpsideDown)?-1.0f:1.0f;
    f.set(state,LivingEntityRenderState_Field_bodyRot,body);
    f.set(state,LivingEntityRenderState_Field_yRot,sign*wrap(adjusted-body));
    f.set(state,LivingEntityRenderState_Field_xRot,sign*(e.lastPitch+partial*(e.pitch-e.lastPitch)));
}
jfloat ViewRotation(JNIEnv* env,jobject p,float partial,bool pitch){
    Frame f(env);auto y=f.call<float>(p,Entity_getViewYRot,partial),x=f.call<float>(p,Entity_getViewXRot,partial);
    event::RayTraceEvent e(p,y,x);Dispatch(e);return pitch?e.pitch:e.yaw;
}
jfloat JNICALL ViewYawHook(JNIEnv* env,jclass,jobject p,jfloat partial){return ViewRotation(env,p,partial,false);}
jfloat JNICALL ViewPitchHook(JNIEnv* env,jclass,jobject p,jfloat partial){return ViewRotation(env,p,partial,true);}
void ObservePlacementWire(Frame& f,jobject packet) {
    if(f.instance(packet,ServerboundMovePlayerPacket)) {
        const bool look=f.get<bool>(packet,ServerboundMovePlayerPacket_Field_hasRot);
        const float yaw=look?f.get<float>(packet,ServerboundMovePlayerPacket_Field_yRot):0;
        const float pitch=look?f.get<float>(packet,ServerboundMovePlayerPacket_Field_xRot):0;
        if(f.check())rotation::ObservePlacementMovement(look,yaw,pitch);
    } else if(f.instance(packet,ServerboundUseItemOnPacket)&&f.check())rotation::ObservePlacementPacket();
    else if(f.instance(packet,ServerboundUseItemPacket)&&f.check())rotation::ObserveUnclassifiedItemUse();
}
jboolean JNICALL PacketOutHook(JNIEnv* env,jclass,jobject connection,jobject packet){
    if(java::BlocksFakePlayerPacket(env,packet))return JNI_TRUE;
    if(java::FreeCamBlocksPacket(env,packet))return JNI_TRUE;
    if(IsCircuitOpen())return JNI_FALSE;
    Frame f(env);auto listener=f.call(connection,Connection_getPacketListener);
    if(!f.instance(listener,ClientPacketListener))return JNI_FALSE;
    if(java::ConsumePassthrough(env,packet)){ObservePlacementWire(f,packet);return JNI_FALSE;}
    NetworkScope scope;
    if(scope.owner) {
        event::PacketEvent e(event::EventType::Outgoing,packet);
        if(Dispatch(e))return JNI_TRUE;
    }
    // Observe the final uncancelled packet, including replay/no-event sends.
    ObservePlacementWire(f,packet);return JNI_FALSE;
}
void JNICALL PacketInHook(JNIEnv* env,jclass,jobject packet,jobject listener){
    {
        Frame f(env);
        if(!IsCircuitOpen()&&f.instance(listener,ClientPacketListener)){
            if(java::IncomingPacketsWanted()) {
                event::PacketEvent e(event::EventType::Incoming,packet);if(Dispatch(e))return;
            }
            java::ObserveFakePlayerExplosion(env,packet);
        }
    }
    // Leave vanilla's thread handoff exception for Connection's original catch block.
    mc263::ReplayIncoming(env,packet,listener);
}
void JNICALL VelocityHook(JNIEnv* env,jclass,jobject entity,jobject vector){
    auto method=access::Method(env,Entity_lerpMotion);if(!method)return;
    env->CallVoidMethod(entity,method,vector);if(env->ExceptionCheck())return;
    Frame f(env);if(!Local(f,entity))return;
    event::AppliedVelocityEvent e(f.get<double>(vector,Vec3_Field_x),f.get<double>(vector,Vec3_Field_y),f.get<double>(vector,Vec3_Field_z));
    if(f.check())Dispatch(e);
}
#define NOVA_MC263_BINDINGS
#include "minecraft/compat/mappings/minecraft263.inc"
#undef NOVA_MC263_BINDINGS
void JNICALL VelocityLegacyHook(JNIEnv* env,jclass,jobject entity,jdouble x,jdouble y,jdouble z){
    auto method=access::Method(env,easecation::names::LerpMotion);if(!method)return;
    env->CallVoidMethod(entity,method,x,y,z);if(env->ExceptionCheck())return;
    Frame f(env);if(!Local(f,entity))return;
    event::AppliedVelocityEvent e(x,y,z);if(f.check())Dispatch(e);
}
float JNICALL TimerMillisHook(JNIEnv*,jclass,jfloat vanilla){
    return IsCircuitOpen() || !compat::ValidatePlayer() ? vanilla :
        IsNeoForge12110()?neoforge12110::TimerMillis(vanilla):easecation::TimerMillis(vanilla);
}
// Namespace initialization avoids MSVC's local-static guard TLS in the reflective DLL.
const auto glfwModernHooks=[] {
    std::vector<bridge::HookDef> result(std::begin(mc263Hooks),std::end(mc263Hooks));
    result.push_back({"velocityLegacy","(Lnet/minecraft/world/entity/Entity;DDD)V",reinterpret_cast<void*>(&VelocityLegacyHook)});
    result.push_back({"timerMillis","(F)F",reinterpret_cast<void*>(&TimerMillisHook)});
    result.push_back({"worldCamera","(Ljava/lang/Object;Ljava/lang/Object;Ljava/lang/Object;)V",reinterpret_cast<void*>(&WorldCameraHook)});
    result.push_back({"resetWorldCamera","()V",reinterpret_cast<void*>(&java::ResetWorldCameraHook)});
    return result;
}();
std::vector<bridge::HookDef> lunar12111Hooks;
}
const bridge::HookDef* Hooks(int& count){
    if(IsLunar12111()){
        if(lunar12111Hooks.empty()) {
            for(auto hook:glfwModernHooks){hook.descriptor=lunar12111::Descriptor(hook.descriptor);lunar12111Hooks.push_back(hook);}
            lunar12111Hooks.push_back({"accountBrowserAdd","(Ljava/lang/Object;)V",reinterpret_cast<void*>(&minecraft::accounts::BrowserAddAccount)});
            lunar12111Hooks.push_back({"accountBrowserSelect","(Ljava/lang/String;)Ljava/util/Optional;",reinterpret_cast<void*>(&minecraft::accounts::BrowserSelectAccount)});
            lunar12111Hooks.push_back({"accountScreenRequiresLogin","(Ljava/lang/Object;)Z",reinterpret_cast<void*>(&minecraft::accounts::ScreenRequiresLogin)});
            lunar12111Hooks.push_back({"accountCanOpenMultiplayer","(Ljava/lang/Object;)Z",reinterpret_cast<void*>(&minecraft::accounts::CanOpenMultiplayer)});
            lunar12111Hooks.push_back({"accountBrowserRefresh","(Ljava/lang/Object;Ljava/lang/Object;)V",reinterpret_cast<void*>(&minecraft::accounts::BrowserRefreshAccount)});
        }
        count=static_cast<int>(lunar12111Hooks.size());return lunar12111Hooks.data();
    }
    if(IsEaseCation() || IsNeoForge12110()){
        count=static_cast<int>(glfwModernHooks.size());return glfwModernHooks.data();
    }
    count=static_cast<int>(sizeof(mc263Hooks)/sizeof(mc263Hooks[0]));return mc263Hooks;
}
bool PrepareHooks(JNIEnv* env){
    if(networkReentrySlot==TLS_OUT_OF_INDEXES)networkReentrySlot=TlsAlloc();
    if(networkReentrySlot==TLS_OUT_OF_INDEXES)return false;
    // Preload each edited class so installation can fail atomically before modules run.
    const char* classes[]={Minecraft,LocalPlayer,KeyboardInput,Entity,LivingEntity,Player,Item,MultiPlayerGameMode,LivingEntityRenderer,Connection,ClientPacketListener};
    for(auto c:classes)if(!access::Class(env,c))return false;
    const names::Member methods[]={Entity_getYRot_2,Entity_getXRot_2,Entity_onGround,Entity_getViewYRot,Entity_getViewXRot,
        IsEaseCation()?easecation::names::LerpMotion:Entity_lerpMotion,Connection_getPacketListener,Packet_handle,Input_Ctor,Input_forward,Input_backward,Input_left,Input_right,Input_jump,Input_shift,Input_sprint,Vec2_Ctor};
    for(auto m:methods)if(!access::Method(env,m))return false;
    return access::Field(env,ClientInput_Field_keyPresses)&&access::Field(env,ClientInput_Field_moveVector);
}
void ReleaseGame(JNIEnv* env){
    motion={};rotation::ResetPlacementWire();access::Release(env);
    if(networkReentrySlot!=TLS_OUT_OF_INDEXES){TlsFree(networkReentrySlot);networkReentrySlot=TLS_OUT_OF_INDEXES;}
}
}
