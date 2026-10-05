#include "minecraft/java/api/game_freelook.h"
#include "minecraft/java/api/game_freecam.h"
#include "bind/PhysicalKey.h"
#include "minecraft/java/api/game_entity.h"
#include "minecraft/java/api/game_rotation.h"
#include "minecraft/utils/rotation/LinearAim.h"
#include "minecraft/compat/minecraft263/Minecraft263Access.h"
#include "minecraft/compat/lunar1710/Access.h"
#include "ui/clickgui/ClickGui.h"
#include <mutex>

namespace nova::java {
namespace {
using compat::mc263::access::Frame;
using compat::mc263::access::Member;
struct Mapping {
    Member options{}, view{}, perspective{}, setPerspective{}, cameraValues{}, fov{}, optionValue{};
    Member invertX{}, invertY{};
    Member yaw{}, pitch{}, previousYaw{}, previousPitch{}, turn{}, viewYaw{}, viewPitch{};
    Member focus{}, helper{}, mouseUpdate{}, deltaX{}, deltaY{}, sensitivity{};
    Member renderer{}, loadShader{}, renderGlobal{}, dirty{}, viewX{}, viewY{};
    Member rotationX{}, rotationZ{}, rotationYZ{}, rotationXY{}, rotationXZ{};
    Member armYaw{}, previousArmYaw{};
};
#define NOVA_FREELOOK_MAPPING
#define NOVA_FREELOOK(key, owner, name, desc) .key = {owner, name, desc},
constexpr Mapping lunar{
#include "minecraft/compat/mappings/Lunar189.inc"
};
constexpr Mapping forge{
#include "minecraft/compat/mappings/forge1201.inc"
};
constexpr Mapping modern{
#include "minecraft/compat/mappings/minecraft263.inc"
};
#undef NOVA_FREELOOK
#undef NOVA_FREELOOK_MAPPING
const Mapping& Names() { return compat::IsLegacyLunar() ? lunar : compat::UsesModernRuntime() ? modern : forge; }
constexpr Member ordinal{"java.lang.Enum", "ordinal", "()I"};
constexpr Member intValue{"java.lang.Integer", "intValue", "()I"};
constexpr Member integer{"java.lang.Integer", "valueOf", "(I)Ljava/lang/Integer;"};
constexpr Member booleanValue{"java.lang.Boolean", "booleanValue", "()Z"};
std::mutex stateLock;
module::freelook::Settings settings;
module::freelook::State state;
EntityHandle trackedPlayer, trackedWorld;

bool PhysicalKey(int key) {
    return bind::IsPhysicalBindingDown(key);
}
bool Focused() {
    DWORD process=0;
    GetWindowThreadProcessId(GetForegroundWindow(), &process);
    return process == GetCurrentProcessId();
}
struct IO {
    Frame f;
    const Mapping& n = Names();
    bool legacy = compat::IsLegacyLunar();
    jobject client{}, player{}, options{};
    explicit IO(JNIEnv* env=nullptr) : f(env) {
        if (!f) return;
        client=f.client();player=f.player();options=f.get(client,n.options);
    }
    bool Capture(int& view, float& fov, float& yaw, float& pitch) {
        if (!player || !options || !f.check()) return false;
        if (legacy) {
            view=f.get<int>(options,n.perspective);fov=f.get<float>(options,n.fov);
            yaw=f.get<float>(player,n.yaw);pitch=f.get<float>(player,n.pitch);
        } else {
            view=f.call<int>(f.call(options,n.perspective),ordinal);
            auto option=f.call(options,n.fov);
            fov=static_cast<float>(f.call<int>(f.get(option,n.optionValue),intValue));
            yaw=f.call<float>(player,n.yaw);pitch=f.call<float>(player,n.pitch);
        }
        return f.check();
    }
    bool SetView(int value) {
        value=std::clamp(value,0,2);
        // Restoration must still run even if the preceding JNI operation failed.
        Frame restore(f.env);
        if (legacy) {
            if (!restore.set(options,n.perspective,value)) return false;
            // 1.7 reads thirdPersonView each frame; it has neither 1.8 callback.
            if (compat::IsLunar1710()) return true;
            auto renderer=restore.get(client,n.renderer);
            if (renderer && value != 2) {
                auto entity=value==0 ? restore.call(client,n.view) : nullptr;
                restore.invoke(renderer,n.loadShader,entity);
            }
            auto worldRenderer=restore.get(client,n.renderGlobal);
            if (worldRenderer) restore.invoke(worldRenderer,n.dirty);
            // The perspective field was set successfully. Keep ownership even
            // if an optional shader/dirty callback failed, so exit restores it.
            return true;
        }
        auto array=static_cast<jobjectArray>(restore.staticCall(n.cameraValues));
        if (!array || !restore.check() || restore.env->GetArrayLength(array)<=value) return false;
        auto type=restore.env->GetObjectArrayElement(array,value);
        return restore.invoke(options,n.setPerspective,type);
    }
    bool SetFov(float value) {
        Frame restore(f.env);
        if (legacy) return restore.set(options,n.fov,value);
        auto option=restore.call(options,n.fov);
        auto boxed=restore.staticCall(integer,static_cast<jint>(value));
        // Temporary value, restored on exit. OptionInstance.set clamps to the
        // vanilla slider's smaller range, preventing Raven's 10..150 FOV range.
        return boxed && restore.set(option,n.optionValue,boxed);
    }
};
void UpdateLocked(IO& io) {
    if(FreeCamActive()){state.Stop(io);return;}
    if (!settings.enabled && !state.active) return;
    if (IsCircuitOpen()) { state.Stop(io); return; }
    auto player=PlayerEntity();auto world=LevelEntity();
    bool same=player && world && trackedPlayer.IsSame(player) && trackedWorld.IsSame(world);
    if (!same) {
        state.Stop(io);
        const bool replacing=static_cast<bool>(trackedPlayer) || static_cast<bool>(trackedWorld);
        trackedPlayer=std::move(player);trackedWorld=std::move(world);
        if (replacing) state.lastKey=PhysicalKey(settings.key);
    }
    auto view=compat::IsLunar1710() ? compat::lunar1710::View(io.f) : io.f.call(io.client,io.n.view);
    const bool ready=trackedPlayer && trackedWorld && io.player && view && io.f.check() &&
        io.f.env->IsSameObject(view,io.player);
    const bool screen=compat::ScreenOpen() || clickgui::IsOpen();
    const bool focus=Focused() && (!io.legacy || io.f.get<bool>(io.client,io.n.focus));
    state.Tick(settings,{PhysicalKey(settings.key),ready,focus,screen},io);
}
bool OwnsEntity(Frame& f, jobject entity) {
    return state.active && trackedPlayer && entity && f.check() &&
        f.env->IsSameObject(entity,static_cast<jobject>(trackedPlayer.raw()));
}
float Angle(JNIEnv* env, jobject entity, bool pitch, bool previous, float partial=0, bool interpolated=false) {
    std::lock_guard lock(stateLock);
    Frame f(env);
    if (OwnsEntity(f,entity)) return pitch?state.pitch:state.yaw;
    const auto& n=Names();
    if (interpolated) return f.call<float>(entity,pitch?n.viewPitch:n.viewYaw,partial);
    return f.get<float>(entity,pitch?(previous?n.previousPitch:n.pitch):(previous?n.previousYaw:n.yaw));
}
}

void ConfigureFreeLook(const module::freelook::Settings& value) {
    std::lock_guard lock(stateLock);settings=value;
}
bool CaptureCameraView(int& view) {IO io;float fov=0,yaw=0,pitch=0;return io.Capture(view,fov,yaw,pitch);}
bool SetCameraView(int view) {
    IO io;
    const int current=io.legacy?io.f.get<int>(io.options,io.n.perspective):io.f.call<int>(io.f.call(io.options,io.n.perspective),ordinal);
    // In legacy clients setting the perspective also reloads shaders and chunks.
    return io.f.check() && (current==view || io.SetView(view));
}
bool SetPlayerCameraRotation(float yaw, float pitch, double smoothAmount) {
    // Share camera mappings with FreeLook, but do not acquire its state lock:
    // this writes player angles and does not modify the FreeLook camera state.
    Frame f;
    if (!f) return false;
    const auto& n = Names();
    auto player = f.player();
    if (!player || !f.check()) return false;
    rotation::Rotation current;
    current.yaw = compat::IsLegacyLunar() ? f.get<float>(player, n.yaw) : f.call<float>(player, n.yaw);
    current.pitch = compat::IsLegacyLunar() ? f.get<float>(player, n.pitch) : f.call<float>(player, n.pitch);
    const rotation::Rotation previous{f.get<float>(player, n.previousYaw), f.get<float>(player, n.previousPitch)};
    if (!f.check() || !std::isfinite(yaw) || !std::isfinite(pitch) ||
        !std::isfinite(current.yaw) || !std::isfinite(current.pitch) ||
        !std::isfinite(previous.yaw) || !std::isfinite(previous.pitch)) return false;
    const auto adjusted = rotation::CameraPreviousAfterTurn(previous, current, {yaw, pitch}, smoothAmount);
    // Silent-rotation release may rebase the camera by whole turns. Vanilla
    // renders hand sway from camera yaw minus these smoothed yaw fields. Leaving
    // them on the old revolution creates a large transient hand rotation.
    const float rebase = rotation::CameraYawRebase(current, {yaw, pitch});
    float armYaw = 0, previousArmYaw = 0;
    if (rebase != 0) {
        armYaw = f.get<float>(player, n.armYaw);
        previousArmYaw = f.get<float>(player, n.previousArmYaw);
        if (!f.check() || !std::isfinite(armYaw) || !std::isfinite(previousArmYaw)) return false;
    }
    if (!SetPlayerRotation(yaw, pitch)) return false;
    if (rebase != 0 && (!f.set(player, n.armYaw, armYaw + rebase) ||
        !f.set(player, n.previousArmYaw, previousArmYaw + rebase))) return false;
    return f.set(player, n.previousYaw, adjusted.yaw) && f.set(player, n.previousPitch, adjusted.pitch);
}
void UpdateFreeLook() {
    std::lock_guard lock(stateLock);
    if (!settings.enabled && !state.active) return;
    IO io;UpdateLocked(io);
}
void StopFreeLook() {
    std::lock_guard lock(stateLock);
    IO io;state.Stop(io);state.lastKey=false;
    trackedPlayer.Reset();trackedWorld.Reset();
    settings.enabled=false;
}
bool FreeLookAngles(float& yaw, float& pitch) {
    std::lock_guard lock(stateLock);
    if (!state.active) return false;
    yaw=state.yaw;pitch=state.pitch;return true;
}
jboolean JNICALL FreeLookMouseHook(JNIEnv* env,jclass,jobject client) {
    std::lock_guard lock(stateLock);
    IO io(env);UpdateLocked(io);
    const bool focused=io.f.get<bool>(client,io.n.focus);
    if (!focused || !state.active) return focused?JNI_TRUE:JNI_FALSE;
    if (compat::ScreenOpen() || clickgui::IsOpen() || !Focused()) return JNI_FALSE;
    auto helper=io.f.get(client,io.n.helper);
    if (!io.f.invoke(helper,io.n.mouseUpdate)) return JNI_FALSE;
    const float sensitivity=io.f.get<float>(io.options,io.n.sensitivity)*.6f+.2f;
    const float multiplier=sensitivity*sensitivity*sensitivity*8.f;
    const float dx=static_cast<float>(io.f.get<int>(helper,io.n.deltaX))*multiplier;
    const float dy=static_cast<float>(io.f.get<int>(helper,io.n.deltaY))*multiplier;
    if (io.f.check()) state.Turn(settings,dx,dy);
    return JNI_FALSE; // consume mouse once; skip vanilla player rotation
}
jfloat JNICALL FreeLookYawHook(JNIEnv* e,jclass,jobject o) { return Angle(e,o,false,false); }
jfloat JNICALL FreeLookPitchHook(JNIEnv* e,jclass,jobject o) { return Angle(e,o,true,false); }
jfloat JNICALL FreeLookPreviousYawHook(JNIEnv* e,jclass,jobject o) { return Angle(e,o,false,true); }
jfloat JNICALL FreeLookPreviousPitchHook(JNIEnv* e,jclass,jobject o) { return Angle(e,o,true,true); }
jfloat JNICALL FreeLookViewYawHook(JNIEnv* e,jclass,jobject o,jfloat p) { return Angle(e,o,false,false,p,true); }
jfloat JNICALL FreeLookViewPitchHook(JNIEnv* e,jclass,jobject o,jfloat p) { return Angle(e,o,true,false,p,true); }
void JNICALL FreeLookTurnHook(JNIEnv* env,jclass,jobject player,jdouble dx,jdouble dy) {
    if(FreeCamTurn(dx,dy))return;
    std::lock_guard lock(stateLock);
    IO io(env);UpdateLocked(io);
    if (OwnsEntity(io.f,player)) {
        if (!compat::ScreenOpen() && !clickgui::IsOpen() && Focused()) {
            // Undo the game's invert options: Raven's independent Invert Pitch
            // setting acts on raw mouse direction. Modern Y is opposite LWJGL2.
            auto inverted=[&](Member m) {
                return m.owner && io.f.call<bool>(io.f.get(io.f.call(io.options,m),io.n.optionValue),booleanValue);
            };
            if (!compat::IsEaseCation() && inverted(io.n.invertX)) dx=-dx;
            if (inverted(io.n.invertY)) dy=-dy;
            if (io.f.check()) state.Turn(settings,static_cast<float>(dx),static_cast<float>(-dy));
        }
    } else io.f.invoke(player,io.n.turn,dx,dy);
}
void JNICALL FreeLookBillboardHook(JNIEnv* env,jclass,jobject manager) {
    std::lock_guard lock(stateLock);
    if (!state.active) return;
    Frame f(env);f.set(manager,Names().viewX,state.pitch);f.set(manager,Names().viewY,state.yaw);
}
void JNICALL FreeLookParticlesHook(JNIEnv* env,jclass,jboolean front) {
    std::lock_guard lock(stateLock);
    if (!state.active) return;
    Frame f(env);
    const auto& n=Names();
    const float y=state.yaw*3.1415927f/180.f,p=state.pitch*3.1415927f/180.f;
    const float sign=front?-1.f:1.f;
    const float x=std::cos(y)*sign,z=std::sin(y)*sign;
    auto set=[&](Member m,float v) {
        auto field=compat::mc263::access::Field(env,m,true);
        auto cls=compat::mc263::access::Class(env,m.owner);
        if(field && cls && f.check())env->SetStaticFloatField(cls,field,v);
    };
    set(n.rotationX,x);set(n.rotationZ,z);set(n.rotationYZ,-z*std::sin(p)*sign);
    set(n.rotationXY,x*std::sin(p)*sign);set(n.rotationXZ,std::cos(p));
}
}
