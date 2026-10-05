#include "minecraft/java/api/game_freelook.h"
#include "minecraft/java/api/game_chest.h"
#include "minecraft/java/api/game_critical.h"
#include "minecraft/java/api/game_breaker.h"
#include "minecraft/java/api/game_keepsprint.h"
#include "minecraft/modules/player/scaffold/Scaffold.h"
#include "minecraft/java/api/game_backtrack.h"
#include "minecraft/hooks/Hooks.h"
#include "minecraft/java/api/game_freecam.h"
#include "minecraft/compat/Profile.h"
#include "minecraft/proxy/ProxyHooks.h"
#include "minecraft/compat/lunar189/LunarRuntime.h"
#include "minecraft/compat/minecraft263/Minecraft263Game.h"

#include <exception>

#include "minecraft/events/EventBus.h"
#include "minecraft/events/GameEvents.h"
#include "minecraft/java/api/base.h"
#include "minecraft/java/api/game_entity.h"
#include "minecraft/java/api/game_network.h"
#include "minecraft/java/api/game_place.h"
#include "minecraft/java/api/game_slotspoof.h"
#include "minecraft/java/api/jni_lookup.h"
#include "minecraft/utils/rotation/Mth.h"

namespace nova::hook {
namespace {

namespace event = nova::event;

#define NOVA_FORGE1201_HOOKS
#include "minecraft/compat/mappings/forge1201.inc"
#undef NOVA_FORGE1201_HOOKS

struct Game {
    jobject   loader = nullptr;

    jclass    entity = nullptr;
    jclass    vec3   = nullptr;
    jclass    connection = nullptr;
    jclass    input = nullptr;
    jclass    listener = nullptr;
    jclass    minecraft = nullptr;
    jclass    localPlayer = nullptr;

    jmethodID getInstance = nullptr;
    jfieldID  playerField = nullptr;
    jfieldID  playerInput = nullptr;
    jmethodID isMovingSlowly = nullptr;

    jmethodID getYRot   = nullptr;
    jmethodID getXRot   = nullptr;
    jmethodID viewVector = nullptr;
    jmethodID isUsingItem = nullptr;
    jmethodID tick = nullptr;
    jmethodID sendPacket = nullptr;
    jmethodID listenerSend = nullptr;

    jmethodID getX = nullptr;
    jmethodID getY = nullptr;
    jmethodID getZ = nullptr;

    jfieldID  yRotO = nullptr;
    jfieldID  xRotO = nullptr;
    jfieldID  tickCount = nullptr;
    jfieldID  onGround = nullptr;
    jfieldID  inputForward = nullptr;
    jfieldID  inputStrafe = nullptr;
    jfieldID  inputJumping = nullptr;
    jfieldID  inputSneak = nullptr;
    jfieldID  inputUp = nullptr;
    jfieldID  inputDown = nullptr;
    jfieldID  inputLeft = nullptr;
    jfieldID  inputRight = nullptr;

    jobject   vec3Zero = nullptr;
};

Game g_game;

struct MotionScratch {
    float yaw = 0.0f;
    float pitch = 0.0f;
    bool  onGround = false;
    bool  ready = false;
};

MotionScratch g_motion;

void ClearGame(JNIEnv* env) {
    if (env) {
        if (g_game.loader)     env->DeleteGlobalRef(g_game.loader);
        if (g_game.entity)     env->DeleteGlobalRef(g_game.entity);
        if (g_game.vec3)       env->DeleteGlobalRef(g_game.vec3);
        if (g_game.connection) env->DeleteGlobalRef(g_game.connection);
        if (g_game.input)      env->DeleteGlobalRef(g_game.input);
        if (g_game.listener)   env->DeleteGlobalRef(g_game.listener);
        if (g_game.minecraft)  env->DeleteGlobalRef(g_game.minecraft);
        if (g_game.localPlayer) env->DeleteGlobalRef(g_game.localPlayer);
        if (g_game.vec3Zero)   env->DeleteGlobalRef(g_game.vec3Zero);
    }
    g_game = Game{};
}

bool Resolve(JNIEnv* env, jobject mcLoader) {
    if (!env || !mcLoader) return false;

    jclass entity = nova_base::loadOrFind(env, mcLoader, kEntityDot, kEntitySig);
    jclass living = nova_base::loadOrFind(env, mcLoader, kLivingDot, kLivingSig);
    jclass vec3   = nova_base::loadOrFind(env, mcLoader, kVec3Dot, kVec3Sig);
    jclass connection = nova_base::loadOrFind(env, mcLoader, kConnectionDot, kConnectionSig);
    jclass input  = nova_base::loadOrFind(env, mcLoader, kInputDot, kInputSig);
    jclass listener = nova_base::loadOrFind(env, mcLoader, kListenerDot, kListenerSig);
    jclass minecraft = nova_base::loadOrFind(env, mcLoader, kMinecraftDot, kMinecraftSig);
    jclass localPlayer = nova_base::loadOrFind(env, mcLoader, kLocalDot, kLocalSig);
    if (!entity || !living || !vec3 || !connection || !input || !listener || !minecraft ||
        !localPlayer) {
        if (entity) env->DeleteLocalRef(entity);
        if (living) env->DeleteLocalRef(living);
        if (vec3)   env->DeleteLocalRef(vec3);
        if (connection) env->DeleteLocalRef(connection);
        if (input)  env->DeleteLocalRef(input);
        if (listener) env->DeleteLocalRef(listener);
        if (minecraft) env->DeleteLocalRef(minecraft);
        if (localPlayer) env->DeleteLocalRef(localPlayer);
        LogTo("Hooks: Entity, LivingEntity, Vec3, Connection, Input, "
              "ClientPacketListener, Minecraft or LocalPlayer is not loadable; no game hooks");
        return false;
    }

    g_game.getYRot = env->GetMethodID(entity, kGetYRot, kMappedNoArgsToFloatDesc);
    g_game.getXRot = env->GetMethodID(entity, kGetXRot, kMappedNoArgsToFloatDesc);
    g_game.viewVector = env->GetMethodID(
        entity, kViewVector, kMappedFloatFloatToVec3Desc);
    g_game.getX = env->GetMethodID(entity, kGetX, kMappedNoArgsToDoubleDesc);
    g_game.getY = env->GetMethodID(entity, kGetY, kMappedNoArgsToDoubleDesc);
    g_game.getZ = env->GetMethodID(entity, kGetZ, kMappedNoArgsToDoubleDesc);
    g_game.yRotO = env->GetFieldID(entity, kMappedPreviousYaw, kMappedPrimitiveFloatSig);
    g_game.xRotO = env->GetFieldID(entity, kMappedPreviousPitch, kMappedPrimitiveFloatSig);
    g_game.tickCount = env->GetFieldID(entity, kMappedTickCount, kMappedPrimitiveIntSig);
    g_game.onGround = env->GetFieldID(entity, kMappedOnGround, kMappedPrimitiveBooleanSig);

    g_game.isUsingItem = env->GetMethodID(living, kIsUsingItem, kMappedNoArgsToBooleanDesc);
    g_game.tick = env->GetMethodID(entity, kMappedTick, kMappedNoArgsToVoidDesc);
    g_game.sendPacket = env->GetMethodID(connection, kSendPacket, kSendPacketDesc);
    g_game.listenerSend = env->GetMethodID(listener, kListenerSend, kListenerSendDesc);

    g_game.getInstance = env->GetStaticMethodID(minecraft, kGetInstance, kGetInstanceDesc);
    g_game.playerField = env->GetFieldID(minecraft, kPlayerField, kLocalSig);
    g_game.playerInput = env->GetFieldID(localPlayer, kPlayerInput, kInputSig);
    g_game.isMovingSlowly = env->GetMethodID(localPlayer, kIsMovingSlowly, kMappedNoArgsToBooleanDesc);
    g_game.minecraft = static_cast<jclass>(env->NewGlobalRef(minecraft));
    g_game.localPlayer = static_cast<jclass>(env->NewGlobalRef(localPlayer));

    g_game.inputForward = env->GetFieldID(input, kMappedInputForward, kMappedPrimitiveFloatSig);
    g_game.inputStrafe = env->GetFieldID(input, kMappedInputStrafe, kMappedPrimitiveFloatSig);
    g_game.inputJumping = env->GetFieldID(input, kMappedInputJumping, kMappedPrimitiveBooleanSig);
    g_game.inputSneak = env->GetFieldID(input, kMappedInputSneak, kMappedPrimitiveBooleanSig);
    g_game.inputUp = env->GetFieldID(input, kMappedInputUp, kMappedPrimitiveBooleanSig);
    g_game.inputDown = env->GetFieldID(input, kMappedInputDown, kMappedPrimitiveBooleanSig);
    g_game.inputLeft = env->GetFieldID(input, kMappedInputLeft, kMappedPrimitiveBooleanSig);
    g_game.inputRight = env->GetFieldID(input, kMappedInputRight, kMappedPrimitiveBooleanSig);

    jfieldID zero = env->GetStaticFieldID(vec3, kMappedVec3Zero, kVec3Sig);
    jobject  zeroObj = zero ? env->GetStaticObjectField(vec3, zero) : nullptr;

    if (env->ExceptionCheck())
        env->ExceptionClear();

    if (!g_game.getYRot || !g_game.getXRot || !g_game.viewVector ||
        !g_game.getX || !g_game.getY || !g_game.getZ ||
        !g_game.isUsingItem || !g_game.tick || !g_game.sendPacket || !g_game.listenerSend ||
        !g_game.yRotO || !g_game.xRotO || !g_game.tickCount || !g_game.onGround ||
        !g_game.inputForward || !g_game.inputStrafe || !g_game.inputJumping ||
        !g_game.inputSneak || !g_game.inputUp || !g_game.inputDown ||
        !g_game.inputLeft || !g_game.inputRight || !zeroObj ||
        !g_game.getInstance || !g_game.playerField || !g_game.playerInput ||
        !g_game.isMovingSlowly || !g_game.minecraft || !g_game.localPlayer) {
        LogTo("Hooks: a game method or field did not resolve (yRot=%d xRot=%d "
              "viewVector=%d xyz=%d usingItem=%d sendPacket=%d listenerSend=%d "
              "rotO=%d tickCount=%d onGround=%d input=%d zero=%d player=%d)",
              g_game.getYRot != nullptr, g_game.getXRot != nullptr,
              g_game.viewVector != nullptr,
              g_game.getX && g_game.getY && g_game.getZ,
              g_game.isUsingItem != nullptr,
              g_game.sendPacket != nullptr, g_game.listenerSend != nullptr,
              g_game.yRotO && g_game.xRotO,
              g_game.tickCount != nullptr, g_game.onGround != nullptr,
              g_game.inputForward && g_game.inputStrafe && g_game.inputJumping &&
                  g_game.inputSneak && g_game.inputUp && g_game.inputDown &&
                  g_game.inputLeft && g_game.inputRight,
              zeroObj != nullptr,
              g_game.getInstance && g_game.playerField && g_game.playerInput &&
                  g_game.isMovingSlowly);
        if (zeroObj) env->DeleteLocalRef(zeroObj);
        env->DeleteLocalRef(entity);
        env->DeleteLocalRef(living);
        env->DeleteLocalRef(vec3);
        env->DeleteLocalRef(connection);
        env->DeleteLocalRef(input);
        env->DeleteLocalRef(listener);
        env->DeleteLocalRef(minecraft);
        env->DeleteLocalRef(localPlayer);
        ClearGame(env);
        return false;
    }

    g_game.loader   = env->NewGlobalRef(mcLoader);
    g_game.entity   = static_cast<jclass>(env->NewGlobalRef(entity));
    g_game.vec3     = static_cast<jclass>(env->NewGlobalRef(vec3));
    g_game.connection = static_cast<jclass>(env->NewGlobalRef(connection));
    g_game.input    = static_cast<jclass>(env->NewGlobalRef(input));
    g_game.listener = static_cast<jclass>(env->NewGlobalRef(listener));
    g_game.vec3Zero = env->NewGlobalRef(zeroObj);

    env->DeleteLocalRef(zeroObj);
    env->DeleteLocalRef(entity);
    env->DeleteLocalRef(living);
    env->DeleteLocalRef(vec3);
    env->DeleteLocalRef(connection);
    env->DeleteLocalRef(input);
    env->DeleteLocalRef(listener);
    env->DeleteLocalRef(minecraft);
    env->DeleteLocalRef(localPlayer);
    return g_game.loader && g_game.entity && g_game.vec3 && g_game.connection &&
           g_game.input && g_game.listener && g_game.vec3Zero && g_game.minecraft &&
           g_game.localPlayer;
}

jfloat CallFloat(JNIEnv* env, jmethodID id, jobject self) {
    jfloat v = env->CallFloatMethod(self, id);
    if (env->ExceptionCheck()) env->ExceptionClear();
    return v;
}

jboolean CallBool(JNIEnv* env, jmethodID id, jobject self) {
    jboolean v = env->CallBooleanMethod(self, id);
    if (env->ExceptionCheck()) env->ExceptionClear();
    return v;
}

jdouble CallDouble(JNIEnv* env, jmethodID id, jobject self) {
    jdouble v = env->CallDoubleMethod(self, id);
    if (env->ExceptionCheck()) env->ExceptionClear();
    return v;
}

jboolean FieldBool(JNIEnv* env, jobject self, jfieldID id) {
    return env->GetBooleanField(self, id);
}

jobject CallViewVector(JNIEnv* env, jobject self, jfloat pitch, jfloat yaw) {
    jobject v = env->CallObjectMethod(self, g_game.viewVector, pitch, yaw);
    if (env->ExceptionCheck()) env->ExceptionClear();
    return v;
}

jfloat FieldFloat(JNIEnv* env, jobject self, jfieldID id) {
    return env->GetFloatField(self, id);
}

// Module handlers run inside this call. A C++ exception escaping a JNI native
// takes the process down: the JVM has no handler for it and the frame is not ours
// to unwind. So the event is posted inside a catch-all, and a handler that throws
// costs its own event rather than the game.
template <class E>
void Announce(E& event) {
    try {
        (void)event::EventBus::Get().Post(event);
    } catch (const std::exception& error) {
        LogTo("Base: an event handler raised '%s'", error.what());
    } catch (...) {
        LogTo("Base: an event handler raised an unknown exception");
    }
}

class Reentry {
public:
    explicit Reentry(bool& busy) : busy_(busy), outer_(!busy) { busy = true; }
    ~Reentry() { if (outer_) busy_ = false; }

    Reentry(const Reentry&) = delete;
    Reentry& operator=(const Reentry&) = delete;

    [[nodiscard]] bool outer() const { return outer_; }

private:
    bool& busy_;
    bool  outer_;
};

JNIEXPORT jobject JNICALL HookViewVector(JNIEnv* env, jclass, jobject entity,
                                        jfloat pitch, jfloat yaw) {
    static bool busy = false;
    Reentry guard(busy);
    if (!guard.outer() || !event::EventBus::Get().HasListeners<event::RayTraceEvent>())
        return CallViewVector(env, entity, pitch, yaw);

    event::RayTraceEvent e(entity, yaw, pitch);
    Announce(e);
    return CallViewVector(env, entity, e.pitch, e.yaw);
}

JNIEXPORT jfloat JNICALL HookStrafeYaw(JNIEnv* env, jclass, jobject entity) {
    static bool busy = false;
    Reentry guard(busy);
    if (!guard.outer() || !event::EventBus::Get().HasListeners<event::StrafeEvent>())
        return CallFloat(env, g_game.getYRot, entity);

    event::StrafeEvent e(CallFloat(env, g_game.getYRot, entity));
    Announce(e);
    return e.yaw;
}

JNIEXPORT jfloat JNICALL HookAttackYaw(JNIEnv* env, jclass, jobject entity) {
    static bool busy = false;
    Reentry guard(busy);
    if (!guard.outer() || !event::EventBus::Get().HasListeners<event::AttackYawEvent>())
        return CallFloat(env, g_game.getYRot, entity);

    event::AttackYawEvent e(CallFloat(env, g_game.getYRot, entity));
    Announce(e);
    return e.yaw;
}

JNIEXPORT jfloat JNICALL HookJumpYaw(JNIEnv* env, jclass, jobject entity) {
    static bool busy = false;
    Reentry guard(busy);
    if (!guard.outer() || !event::EventBus::Get().HasListeners<event::JumpEvent>())
        return CallFloat(env, g_game.getYRot, entity);

    event::JumpEvent e(CallFloat(env, g_game.getYRot, entity));
    Announce(e);
    return e.yaw;
}

JNIEXPORT jfloat JNICALL HookFallFlyingPitch(JNIEnv* env, jclass, jobject entity) {
    static bool busy = false;
    Reentry guard(busy);
    if (!guard.outer() || !event::EventBus::Get().HasListeners<event::FallFlyingEvent>())
        return CallFloat(env, g_game.getXRot, entity);

    event::FallFlyingEvent e(CallFloat(env, g_game.getXRot, entity));
    Announce(e);
    return e.pitch;
}

JNIEXPORT jfloat JNICALL HookBodyYaw(JNIEnv* env, jclass, jobject entity) {
    static bool busy = false;
    Reentry guard(busy);
    if (!guard.outer() || !event::EventBus::Get().HasListeners<event::RotationAnimationEvent>())
        return CallFloat(env, g_game.getYRot, entity);

    event::RotationAnimationEvent e(entity,
                                    CallFloat(env, g_game.getYRot, entity),
                                    FieldFloat(env, entity, g_game.yRotO),
                                    CallFloat(env, g_game.getXRot, entity),
                                    FieldFloat(env, entity, g_game.xRotO));
    Announce(e);
    return e.yaw;
}

struct RenderScratch {
    bool localPlayer = false;
};

RenderScratch g_render;

jfloat RotLerp(jfloat delta, jfloat start, jfloat end) {
    return start + delta * rotation::mth::WrapDegrees(end - start);
}

jfloat PlainLerp(jfloat delta, jfloat start, jfloat end) {
    return start + delta * (end - start);
}

JNIEXPORT void JNICALL HookRenderEntity(JNIEnv* env, jclass, jobject entity) {
    g_render.localPlayer = false;
    if (!entity) return;
    if (!event::EventBus::Get().HasListeners<event::RotationAnimationEvent>()) return;

    nova::java::EntityHandle player = nova::java::PlayerEntity();
    if (!player.Valid()) return;

    g_render.localPlayer =
        env->IsSameObject(entity, static_cast<jobject>(player.raw())) != JNI_FALSE;
}

JNIEXPORT jfloat JNICALL HookRenderYaw(JNIEnv*, jclass,
                                       jfloat delta, jfloat start, jfloat end) {
    static bool busy = false;
    Reentry guard(busy);
    if (!guard.outer() || !g_render.localPlayer ||
        !event::EventBus::Get().HasListeners<event::RotationAnimationEvent>())
        return RotLerp(delta, start, end);

    nova::java::EntityHandle player = nova::java::PlayerEntity();
    if (!player.Valid())
        return RotLerp(delta, start, end);

    event::RotationAnimationEvent e(static_cast<jobject>(player.raw()),
                                    end, start, 0.0f, 0.0f);
    Announce(e);
    return RotLerp(delta, e.lastYaw, e.yaw);
}

JNIEXPORT jfloat JNICALL HookRenderPitch(JNIEnv*, jclass,
                                         jfloat delta, jfloat start, jfloat end) {
    static bool busy = false;
    Reentry guard(busy);
    if (!guard.outer() || !g_render.localPlayer ||
        !event::EventBus::Get().HasListeners<event::RotationAnimationEvent>())
        return PlainLerp(delta, start, end);

    nova::java::EntityHandle player = nova::java::PlayerEntity();
    if (!player.Valid())
        return PlainLerp(delta, start, end);

    event::RotationAnimationEvent e(static_cast<jobject>(player.raw()),
                                    0.0f, 0.0f, end, start);
    Announce(e);
    return PlainLerp(delta, e.lastPitch, e.pitch);
}

JNIEXPORT jboolean JNICALL HookUsingItem(JNIEnv* env, jclass, jobject entity) {
    static bool busy = false;
    Reentry guard(busy);
    if (!guard.outer() || !event::EventBus::Get().HasListeners<event::SlowdownEvent>())
        return CallBool(env, g_game.isUsingItem, entity);

    event::SlowdownEvent e;
    if (event::EventBus::Get().Post(e)) return JNI_FALSE;
    return CallBool(env, g_game.isUsingItem, entity);
}

JNIEXPORT jobject JNICALL HookStuckInBlock(JNIEnv* env, jclass, jobject entity,
                                           jobject state, jobject multiplier) {
    static bool busy = false;
    Reentry guard(busy);
    if (!guard.outer() || !event::EventBus::Get().HasListeners<event::StuckInBlockEvent>())
        return multiplier;

    event::StuckInBlockEvent e(state, multiplier);
    if (event::EventBus::Get().Post(e)) return g_game.vec3Zero;
    return e.stuckSpeedMultiplier ? e.stuckSpeedMultiplier : multiplier;
}

JNIEXPORT jboolean JNICALL HookPlayerTick(JNIEnv*, jclass, jobject player) {
    (void)player;
    if (!event::EventBus::Get().HasListeners<event::PlayerTickEvent>())
        return JNI_FALSE;

    event::PlayerTickEvent e;
    return event::EventBus::Get().Post(e) ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT void JNICALL HookPacketOut(JNIEnv* env, jclass, jobject connection,
                                     jobject packet, jobject listener) {
    if(java::FreeCamBlocksPacket(env,packet))return;
    static bool busy = false;
    Reentry guard(busy);
    if (guard.outer() && !nova::java::ConsumePassthrough(env, packet) &&
        event::EventBus::Get().HasListeners<event::PacketEvent>()) {
        event::PacketEvent e(event::EventType::Outgoing, packet);
        if (event::EventBus::Get().Post(e))
            return;
    }

    env->CallVoidMethod(connection, g_game.sendPacket, packet, listener);
}

// Inbound. Unlike the outbound hook this one runs on the netty I/O thread rather
// than the game thread, so whatever it publishes is observed from there. It stays
// inert until a module declares it wants inbound packets, which keeps the common
// case -- nobody interested -- free of any cross-thread work.
//
// It takes the callee's two arguments and nothing else: the callee is a static
// helper, so the Connection is not on the stack to be forwarded, and the replay
// path reaches the same static helper without needing the instance either.
//
// No re-entry guard is needed: the edit only rewrites the helper call inside
// channelRead0, and ReplayIncoming calls that helper directly, so the hook is not
// on its own path back into the game.
JNIEXPORT void JNICALL HookPacketIn(JNIEnv* env, jclass, jobject packet, jobject listener) {
    if (nova::java::IncomingPacketsWanted() &&
        event::EventBus::Get().HasListeners<event::PacketEvent>()) {
        event::PacketEvent e(event::EventType::Incoming, packet);
        if (event::EventBus::Get().Post(e))
            return;
    }

    nova::java::ReplayIncoming(env, packet, listener);
}

// Runs once per entity per tick, on the game thread. When Scaffold has asked for
// the local player's own tick to be dropped this swallows it; every other entity,
// and every tick with nothing pending, takes the ordinary path. The decision is
// cheap by design -- ShouldSkipPlayerTick touches the JVM only while a skip is
// actually outstanding.
JNIEXPORT void JNICALL HookSkipTick(JNIEnv* env, jclass, jobject entity) {
    if (nova::java::ShouldSkipPlayerTick(entity))
        return;

    env->CallVoidMethod(entity, g_game.tick);
}

jobject PlayerObject(JNIEnv* env) {
    jobject client = env->CallStaticObjectMethod(g_game.minecraft, g_game.getInstance);
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
        return nullptr;
    }
    if (client == nullptr)
        return nullptr;

    jobject player = env->GetObjectField(client, g_game.playerField);
    env->DeleteLocalRef(client);
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
        return nullptr;
    }
    return player;
}

float g_moveInputForwardImpulse = 0.0f;

JNIEXPORT void JNICALL HookMoveInputCapture(JNIEnv*, jclass, jfloat forwardImpulse) {
    g_moveInputForwardImpulse = static_cast<float>(forwardImpulse);
}

JNIEXPORT void JNICALL HookMoveInput(JNIEnv* env, jclass) {
    static bool busy = false;
    Reentry guard(busy);
    if (!guard.outer()) return;

    jobject player = PlayerObject(env);
    if (player == nullptr) return;

    jobject input = env->GetObjectField(player, g_game.playerInput);
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
        env->DeleteLocalRef(player);
        return;
    }
    if (input == nullptr) {
        env->DeleteLocalRef(player);
        return;
    }

    const jboolean up = FieldBool(env, input, g_game.inputUp);
    const jboolean down = FieldBool(env, input, g_game.inputDown);
    const jboolean left = FieldBool(env, input, g_game.inputLeft);
    const jboolean right = FieldBool(env, input, g_game.inputRight);

    float forward = (up == down) ? 0.0f : (up != JNI_FALSE ? 1.0f : -1.0f);
    float strafe = (left == right) ? 0.0f : (left != JNI_FALSE ? 1.0f : -1.0f);
    bool jump = FieldBool(env, input, g_game.inputJumping) != JNI_FALSE;
    bool sneak = FieldBool(env, input, g_game.inputSneak) != JNI_FALSE;
    double sneakMultiplier = static_cast<double>(g_moveInputForwardImpulse);

    if (event::EventBus::Get().HasListeners<event::MoveInputEvent>()) {
        event::MoveInputEvent e(forward, strafe, jump, sneak, sneakMultiplier);
        Announce(e);
        forward = e.forward;
        strafe = e.strafe;
        jump = e.jump;
        sneak = e.sneak;
        sneakMultiplier = e.sneakSlowDownMultiplier;
    }

    if (CallBool(env, g_game.isMovingSlowly, player) != JNI_FALSE) {
        forward = static_cast<float>(static_cast<double>(forward) * sneakMultiplier);
        strafe = static_cast<float>(static_cast<double>(strafe) * sneakMultiplier);
    }

    if(java::FreeCamActive()){forward=strafe=0;jump=sneak=false;}
    env->SetFloatField(input, g_game.inputForward, static_cast<jfloat>(forward));
    env->SetFloatField(input, g_game.inputStrafe, static_cast<jfloat>(strafe));
    env->SetBooleanField(input, g_game.inputJumping, jump ? JNI_TRUE : JNI_FALSE);
    env->SetBooleanField(input, g_game.inputSneak, sneak ? JNI_TRUE : JNI_FALSE);

    env->DeleteLocalRef(player);
    env->DeleteLocalRef(input);
}

JNIEXPORT void JNICALL HookGameTick(JNIEnv*, jclass) {
    static bool busy = false;
    Reentry guard(busy);
    if (!guard.outer() || !event::EventBus::Get().HasListeners<event::GameTickEvent>())
        return;

    event::GameTickEvent e(event::EventType::Pre);
    Announce(e);
}

JNIEXPORT void JNICALL HookPostGameTick(JNIEnv*,jclass){event::GameTickEvent e(event::EventType::Post);Announce(e);}

JNIEXPORT jboolean JNICALL HookClick(JNIEnv*, jclass, jobject) {
    if(java::FreeCamBlocksMouse(true))return JNI_TRUE;
    static bool busy = false;
    Reentry guard(busy);
    if (!guard.outer()) return JNI_FALSE;
    event::ClickEvent e;
    Announce(e);
    return e.IsCancelled() ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT void JNICALL HookPreInteract(JNIEnv*,jclass) {
    static bool busy=false;Reentry guard(busy);
    if(!guard.outer())return;
    event::PrePlayerInteractEvent e;Announce(e);java::SuppressFreeCamKeys();
}

JNIEXPORT void JNICALL HookCursorPickEnd(JNIEnv*, jclass, jfloat partialTicks) {
    event::CursorPickEvent e(event::EventType::Post, partialTicks);
    Announce(e);
}

JNIEXPORT void JNICALL HookBeginMotion(JNIEnv* env, jclass, jobject player) {
    static bool busy = false;
    Reentry guard(busy);
    if (!guard.outer()) return;

    g_motion.ready = false;
    g_motion.yaw = CallFloat(env, g_game.getYRot, player);
    g_motion.pitch = CallFloat(env, g_game.getXRot, player);
    g_motion.onGround = FieldBool(env, player, g_game.onGround) != JNI_FALSE;

    const jint tickCount = env->GetIntField(player, g_game.tickCount);
    if (tickCount <= 1 && event::EventBus::Get().HasListeners<event::RespawnEvent>()) {
        event::RespawnEvent respawn;
        Announce(respawn);
    }

    if (!event::EventBus::Get().HasListeners<event::MotionEvent>())
        return;

    event::MotionEvent e(event::EventType::Pre,
                         CallDouble(env, g_game.getX, player),
                         CallDouble(env, g_game.getY, player),
                         CallDouble(env, g_game.getZ, player),
                         g_motion.yaw, g_motion.pitch, g_motion.onGround);
    Announce(e);

    g_motion.yaw = e.yaw;
    g_motion.pitch = e.pitch;
    g_motion.onGround = e.onGround;
    g_motion.ready = true;
}

JNIEXPORT void JNICALL HookEndMotion(JNIEnv* env, jclass, jobject player) {
    if (!event::EventBus::Get().HasListeners<event::MotionEvent>())
        return;

    const float yaw = g_motion.ready ? g_motion.yaw : CallFloat(env, g_game.getYRot, player);
    const float pitch =
        g_motion.ready ? g_motion.pitch : CallFloat(env, g_game.getXRot, player);

    event::MotionEvent e(event::EventType::Post, yaw, pitch);
    Announce(e);
}

JNIEXPORT jfloat JNICALL HookMotionYaw(JNIEnv* env, jclass, jobject entity) {
    if (java::FreeCamActive()) return CallFloat(env, g_game.getYRot, entity);
    if (!g_motion.ready) return CallFloat(env, g_game.getYRot, entity);
    return g_motion.yaw;
}

JNIEXPORT jfloat JNICALL HookMotionPitch(JNIEnv* env, jclass, jobject entity) {
    if (java::FreeCamActive()) return CallFloat(env, g_game.getXRot, entity);
    if (!g_motion.ready) return CallFloat(env, g_game.getXRot, entity);
    return g_motion.pitch;
}

JNIEXPORT jboolean JNICALL HookMotionOnGround(JNIEnv* env, jclass, jobject entity) {
    if (java::FreeCamActive()) return FieldBool(env, entity, g_game.onGround);
    if (!g_motion.ready)
        return FieldBool(env, entity, g_game.onGround);
    return g_motion.onGround ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT jfloat JNICALL HookUseItemRayTraceYaw(JNIEnv* env, jclass, jobject player) {
    static bool busy = false;
    Reentry guard(busy);
    if (!guard.outer() || !event::EventBus::Get().HasListeners<event::UseItemRayTraceEvent>())
        return CallFloat(env, g_game.getYRot, player);

    event::UseItemRayTraceEvent e(CallFloat(env, g_game.getYRot, player),
                                  CallFloat(env, g_game.getXRot, player));
    Announce(e);
    return e.yaw;
}

JNIEXPORT jfloat JNICALL HookUseItemRayTracePitch(JNIEnv* env, jclass, jobject player) {
    static bool busy = false;
    Reentry guard(busy);
    if (!guard.outer() || !event::EventBus::Get().HasListeners<event::UseItemRayTraceEvent>())
        return CallFloat(env, g_game.getXRot, player);

    event::UseItemRayTraceEvent e(CallFloat(env, g_game.getYRot, player),
                                  CallFloat(env, g_game.getXRot, player));
    Announce(e);
    return e.pitch;
}

JNIEXPORT void JNICALL HookPositionItem(JNIEnv* env, jclass, jobject listener,
                                        jobject packet) {
    static bool busy = false;
    Reentry guard(busy);
    if (guard.outer() && event::EventBus::Get().HasListeners<event::PositionItemEvent>()) {
        event::PositionItemEvent e(packet);
        if (event::EventBus::Get().Post(e))
            return;
        env->CallVoidMethod(listener, g_game.listenerSend,
                            e.packet ? e.packet : packet);
        return;
    }

    env->CallVoidMethod(listener, g_game.listenerSend, packet);
}

#define NOVA_FORGE1201_HOOK_BINDINGS
#include "minecraft/compat/mappings/forge1201.inc"
#undef NOVA_FORGE1201_HOOK_BINDINGS

}

bool Prepare(JNIEnv* env, jobject mcLoader) {
    minecraft::proxyadapter::Initialize();
    if (compat::UsesModernRuntime()) return compat::mc263::PrepareHooks(env);
    if (compat::IsLegacyLunar()) return env && mcLoader;
    ClearGame(env);
    return Resolve(env, mcLoader);
}

void Release(JNIEnv* env) { if (compat::UsesModernRuntime()) compat::mc263::ReleaseGame(env); else ClearGame(env); }

}

namespace nova::bridge {

const HookDef* ProxyHooks(int& count) {
    const HookDef* base=nullptr;
    if (compat::UsesModernRuntime()) base=compat::mc263::Hooks(count);
    else if (compat::IsLegacyLunar()) base=compat::lunar::Hooks(count);
    else {base=nova::hook::kHooks;count=nova::hook::kHookCount;}
    return minecraft::proxyadapter::ExtendHooks(base,count);
}
const HookDef* Hooks() {int count=0;return ProxyHooks(count);}

int HookCount() {
    int count=0;ProxyHooks(count);return count;
}

}
