#include "minecraft/java/api/game_freelook.h"
#include "minecraft/modules/player/breaker/Breaker.h"
#include "minecraft/modules/player/blockin/BlockIn.h"
#include "minecraft/java/api/game_keepsprint.h"
#include "minecraft/modules/player/scaffold/Scaffold.h"
#include "minecraft/modules/movement/invmove/InventoryMove.h"
#include "minecraft/java/api/game_backtrack.h"
#include "minecraft/java/api/game_click.h"
#include "minecraft/compat/lunar189/LunarRuntime.h"
#include "minecraft/accounts/lunar/AccountHooks.h"
#include "minecraft/java/api/game_freecam.h"
#include "minecraft/compat/lunar189/IncomingTasks.h"
#include "minecraft/compat/Profile.h"
#include "minecraft/compat/lunar1710/Mapping.h"
#include "minecraft/compat/minecraft263/Minecraft263Access.h"
#include "minecraft/events/GameEvents.h"
#include "minecraft/java/api/game_network.h"
#include "minecraft/java/api/game_place.h"
#include "minecraft/java/api/game_slotspoof.h"
#include "minecraft/java/api/game_skin.h"
#include "minecraft/utils/inventory/InventoryUtils.h"
#include "minecraft/modules/combat/autoblock/AutoBlockStats.h"
#include "minecraft/modules/combat/autoblock/AutoBlockPacketSync.h"
#include "minecraft/modules/combat/antikb/AntiKB.h"
#include "minecraft/modules/combat/displace/DisplacePackets.h"

#include <cmath>
#include <utility>
#include <exception>
#include <atomic>
#include <algorithm>

namespace nova::compat::lunar {
namespace {
struct Handles {
#define NOVA_CLASS(id, name) jclass id = nullptr;
#define NOVA_FIELD(id, owner, name, desc) jfieldID id = nullptr;
#define NOVA_STATIC_FIELD NOVA_FIELD
#define NOVA_METHOD(id, owner, name, desc) jmethodID id = nullptr;
#define NOVA_STATIC_METHOD NOVA_METHOD
#include "minecraft/compat/mappings/Lunar189.inc"
#define NOVA_LUNAR1710_EXTRA
#include "minecraft/compat/mappings/Lunar1710.inc"
#undef NOVA_LUNAR1710_EXTRA
#undef NOVA_CLASS
#undef NOVA_FIELD
#undef NOVA_STATIC_FIELD
#undef NOVA_METHOD
#undef NOVA_STATIC_METHOD
    jint nightVisionId = 0;
    bool ready = false;
};
Handles h;
IncomingTasks<jobject> incomingTasks;
bool worldValidated = false;
DWORD networkReentrySlot = TLS_OUT_OF_INDEXES;
std::atomic<bool> autoBlockManaged{false};
std::atomic<bool> autoBlockManualSuppressed{false};
std::atomic<bool> autoBlockEnabled{false};
std::atomic<bool> autoBlockAnimation{false};
std::atomic<bool> blockLagActive{false};
std::atomic<std::uint64_t> movementPacketSequence{0};
std::atomic<bool> heldSlotOrderReady{true};
std::atomic<std::uint64_t> grimResetSequence{0};
bool grimBlockLag = false;
bool hypixelBlockLag = false; // protected by blockLagLock
bool slinkyBlockLag = false; // shares the queue, never a second packet owner
ULONGLONG slinkyDeadline = 0;
int slinkyMaxPackets = 3;
bool slinkyPreventAttacks = true;
std::atomic<bool> slinkyAttackReleased{false};
std::atomic<bool> predictionBlockLag{false};
std::atomic<bool> hypixelBlockManaged{false};
std::atomic<bool> hypixelBlockingState{false};
std::atomic<bool> hypixelDigging{false}, hypixelPlacing{false};
// Counts accepted local movement before output buffering, never its replay.
std::atomic<std::uint64_t> blockInputMovementSequence{0};
std::atomic<bool> blockInputAttacked{false};
bool grimReblockPending = false;
ULONGLONG grimBlockDeadline = 0;
SRWLOCK blockLagLock = SRWLOCK_INIT;
struct BlockPacket { jobject manager; jobject packet; jobject callbacks; };
std::vector<BlockPacket> blockPackets;

struct BlockLagLock {
    BlockLagLock() { AcquireSRWLockExclusive(&blockLagLock); }
    ~BlockLagLock() { ReleaseSRWLockExclusive(&blockLagLock); }
};

// Watchdog's synchronous receive runs either before physics (attack) or after
// physics (walking PRE). Keep actions together until C03, placing acknowledgements
// on the side of that movement which actually uses the newly applied velocity.
std::vector<BlockPacket> watchdogActions, watchdogConfirmations;
std::atomic<bool> watchdogPhasePending{false};
std::atomic<bool> watchdogBeforeMovement{false};
jobject watchdogPhasePlayer = nullptr, watchdogPhaseWorld = nullptr;

bool Good(JNIEnv* env) {
    if (!env->ExceptionCheck()) return true;
    env->ExceptionClear();
    return false;
}

bool BlockReleasePacket(JNIEnv* env, jobject packet) {
    if (!env->IsInstanceOf(packet, h.ActionPacket)) return false;
    if (IsLunar1710()) { const int action = env->CallIntMethod(packet, h.DigAction); return Good(env) && action == 5; }
    jobject action = env->CallObjectMethod(packet, h.DigAction);
    if (!Good(env) || !action) return false;
    const int ordinal = env->CallIntMethod(action, h.Ordinal);
    env->DeleteLocalRef(action);
    return Good(env) && ordinal == 5;
}

bool BlockControlPacket(JNIEnv* env, jobject packet) {
    return env->IsInstanceOf(packet, h.KeepAliveReply) ||
        env->IsInstanceOf(packet, h.TransactionReply) || BlockReleasePacket(env, packet);
}

bool SlotOrderAction(JNIEnv* env, jobject packet) {
    // Conservatively treat gameplay packets as actions; keep-alives, slot
    // synchronization and arm animation do not start a conflicting action.
    return !env->IsInstanceOf(packet, h.MovePacket) &&
        !env->IsInstanceOf(packet, h.HeldSlotPacket) &&
        !env->IsInstanceOf(packet, h.SwingPacket) &&
        !env->IsInstanceOf(packet, h.KeepAliveReply) &&
        !env->IsInstanceOf(packet, h.TransactionReply);
}

void RecordBlockPacket(JNIEnv* env, jobject packet) {
    if (!IsLunar1710()) {
        if (env->IsInstanceOf(packet, h.MovePacket)) {
            const bool look = env->IsInstanceOf(packet, h.RotationPacket) ||
                              env->IsInstanceOf(packet, h.PosRotPacket);
            const float yaw = look ? env->GetFloatField(packet, h.PacketYaw) : 0.f;
            const float pitch = look ? env->GetFloatField(packet, h.PacketPitch) : 0.f;
            if (Good(env)) module::scaffold::ObserveLegacyScaffoldPacket(true, look, yaw, pitch);
        } else if (env->IsInstanceOf(packet, h.UsePacket)) {
            const int face = env->CallIntMethod(packet, h.UseDirection);
            if (Good(env) && face >= 0 && face <= 5)
                module::scaffold::ObserveLegacyScaffoldPacket(false, false, 0, 0);
        }
    }
    if (env->IsInstanceOf(packet, h.MovePacket)) heldSlotOrderReady.store(true);
    else if (SlotOrderAction(env, packet)) heldSlotOrderReady.store(false);
    if (env->IsInstanceOf(packet, h.HeldSlotPacket) || BlockReleasePacket(env, packet)) {
        module::autoblock::RecordBlockState(false);
        return;
    }
    if (!env->IsInstanceOf(packet, h.UsePacket)) return;
    const int direction = env->CallIntMethod(packet, h.UseDirection);
    if (!Good(env) || direction != 255) return;
    jobject stack = env->CallObjectMethod(packet, h.UseStack);
    if (!Good(env) || !stack) return;
    jobject item = env->CallObjectMethod(stack, h.GetItem);
    if (Good(env) && item)
        module::autoblock::RecordBlockState(env->IsInstanceOf(item, h.Sword) == JNI_TRUE);
    if (item) env->DeleteLocalRef(item);
    env->DeleteLocalRef(stack);
}

bool DispatchClientOutput(JNIEnv* env, jobject manager, jobject packet, jobject callbacks) {
    if (module::displace::HoldOutput(env, manager, packet, callbacks)) return true;
    return SendDisplacePacket(env, manager, packet, callbacks);
}

void DrainBlockPackets(JNIEnv* env, bool send, bool controlsOnly = false) {
    const bool prediction = predictionBlockLag.exchange(false);
    blockLagActive.store(false);
    grimBlockLag = false;
    hypixelBlockLag = false;
    slinkyBlockLag = false;
    slinkyDeadline = 0;
    grimReblockPending = false;
    grimBlockDeadline = 0;
    for (const auto& entry : blockPackets) {
        if (send && (!controlsOnly || BlockControlPacket(env, entry.packet))) {
            // Leader compensates every C02, including INTERACT/INTERACT_AT.
            if (prediction && env->IsInstanceOf(entry.packet, h.InteractPacket)) {
                jobject player = compat::LocalPlayer(env);
                if (player) {
                    const double x = env->GetDoubleField(player, h.MotionX);
                    const double z = env->GetDoubleField(player, h.MotionZ);
                    if (Good(env)) {
                        env->SetDoubleField(player, h.MotionX, x * .6);
                        env->SetDoubleField(player, h.MotionZ, z * .6);
                    }
                    env->DeleteLocalRef(player);
                    Good(env);
                }
            }
            DispatchClientOutput(env, entry.manager, entry.packet, entry.callbacks);
        }
        env->DeleteGlobalRef(entry.manager);
        env->DeleteGlobalRef(entry.packet);
        if (entry.callbacks) env->DeleteGlobalRef(entry.callbacks);
    }
    blockPackets.clear();
}

void SendOldestBlockPacket(JNIEnv* env) {
    if (blockPackets.empty()) return;
    const auto entry = blockPackets.front();
    blockPackets.erase(blockPackets.begin());
    DispatchClientOutput(env, entry.manager, entry.packet, entry.callbacks);
    env->DeleteGlobalRef(entry.manager);
    env->DeleteGlobalRef(entry.packet);
    if (entry.callbacks) env->DeleteGlobalRef(entry.callbacks);
}

bool SlinkyPacketFlush(JNIEnv* env, jobject packet, bool& attack) {
    attack = false;
    if (env->IsInstanceOf(packet, h.WindowClickPacket) || env->IsInstanceOf(packet, h.ClosePacket)) return true;
    if (!env->IsInstanceOf(packet, h.InteractPacket)) return false;
    jobject action = env->CallObjectMethod(packet, h.InteractAction);
    if (!Good(env) || !action) return true;
    const int ordinal = env->CallIntMethod(action, h.Ordinal);
    env->DeleteLocalRef(action);
    if (!Good(env)) return true;
    attack = ordinal == 1;
    return !attack || slinkyPreventAttacks;
}

bool HoldBlockPacket(JNIEnv* env, jobject manager, jobject packet, jobject callbacks, bool passthrough) {
    BlockLagLock lock;
    if (passthrough && !grimBlockLag) return false;
    if (!blockPackets.empty() && !env->IsSameObject(manager, blockPackets.front().manager)) {
        if (grimBlockLag || hypixelBlockLag || slinkyBlockLag) grimResetSequence.fetch_add(1);
        DrainBlockPackets(env, false);
        return false;
    }
    if (grimBlockLag && (GetTickCount64() >= grimBlockDeadline + 100 || blockPackets.size() >= 512))
        DrainBlockPackets(env, true);
    if (!blockLagActive.load()) {
        DrainBlockPackets(env, true);
        return false;
    }
    if ((slinkyBlockLag || hypixelBlockLag) &&
        (module::antikb::BlocksAutoBlockBlink() || watchdogPhasePending.load())) {
        DrainBlockPackets(env, true);
        return false;
    }
    if (slinkyBlockLag) {
        if (GetTickCount64() >= slinkyDeadline) {
            DrainBlockPackets(env, true);
            return false;
        }
        if (env->IsInstanceOf(packet, h.LoginStartPacket) || env->IsInstanceOf(packet, h.HandshakePacket)) return false;
        bool attack = false;
        if (SlinkyPacketFlush(env, packet, attack)) {
            DrainBlockPackets(env, true);
            if (attack) slinkyAttackReleased.store(true);
            return false;
        }
        if (blockPackets.size() >= static_cast<std::size_t>(slinkyMaxPackets)) SendOldestBlockPacket(env);
    }
    // Leader BlinkManager.offerPacket: keepalive and chat always pass; C0F
    // passes only while the queue is empty. C07 releases must be buffered.
    if ((hypixelBlockLag || predictionBlockLag.load()) && (env->IsInstanceOf(packet, h.KeepAliveReply) ||
        env->IsInstanceOf(packet, h.ChatPacket) ||
        (blockPackets.empty() && env->IsInstanceOf(packet, h.TransactionReply)))) return false;
    BlockPacket entry{env->NewGlobalRef(manager), env->NewGlobalRef(packet),
                      callbacks ? env->NewGlobalRef(callbacks) : nullptr};
    if (!Good(env) || !entry.manager || !entry.packet || (callbacks && !entry.callbacks)) {
        if (entry.manager) env->DeleteGlobalRef(entry.manager);
        if (entry.packet) env->DeleteGlobalRef(entry.packet);
        if (entry.callbacks) env->DeleteGlobalRef(entry.callbacks);
        DrainBlockPackets(env, true);
        return false;
    }
    blockPackets.push_back(entry);
    return true;
}

bool SendWatchdogPacket(JNIEnv* env, const BlockPacket& entry) {
    return DispatchClientOutput(env, entry.manager, entry.packet, entry.callbacks);
}

// Caller owns blockLagLock; raw sends cannot re-enter module packet listeners.
void DrainWatchdogPackets(JNIEnv* env, std::vector<BlockPacket>& packets, bool send) {
    for (const auto& entry : packets) {
        if (send) SendWatchdogPacket(env, entry);
        env->DeleteGlobalRef(entry.manager);
        env->DeleteGlobalRef(entry.packet);
        if (entry.callbacks) env->DeleteGlobalRef(entry.callbacks);
    }
    packets.clear();
}

void ClearWatchdogPhase(JNIEnv* env) {
    if (watchdogPhasePlayer) env->DeleteGlobalRef(watchdogPhasePlayer);
    if (watchdogPhaseWorld) env->DeleteGlobalRef(watchdogPhaseWorld);
    watchdogPhasePlayer = watchdogPhaseWorld = nullptr;
    watchdogPhasePending.store(false);
}

// At tick HEAD, a skipped walking update has not sent any affected movement.
// Acknowledge before the upcoming physics, retaining the action group for C03.
// World loss/unload discards both; correction preserves valid acknowledgements
// and discards stale actions.
void MaintainWatchdogPhase(JNIEnv* env, bool discard, bool correction = false) {
    if (!watchdogPhasePending.load()) return;
    const auto player = PlayerEntity(), world = LevelEntity();
    BlockLagLock lock;
    const bool same = player && world &&
        env->IsSameObject(static_cast<jobject>(player.raw()), watchdogPhasePlayer) &&
        env->IsSameObject(static_cast<jobject>(world.raw()), watchdogPhaseWorld);
    DrainWatchdogPackets(env, watchdogConfirmations, !discard && same);
    if (discard || !same || correction) DrainWatchdogPackets(env, watchdogActions, false);
    if (watchdogActions.empty()) ClearWatchdogPhase(env);
}

bool StageWatchdogPacket(JNIEnv* env, jobject manager, jobject packet, jobject callbacks, bool passthrough) {
    const bool holding = module::antikb::WatchdogHoldingPackets();
    const bool replaying = module::antikb::WatchdogReplayingPackets();
    if (!watchdogPhasePending.load() && !holding && !replaying) return false;
    if (env->IsInstanceOf(packet, h.KeepAliveReply) || env->IsInstanceOf(packet, h.ChatPacket) ||
        env->IsInstanceOf(packet, h.LoginStartPacket) || env->IsInstanceOf(packet, h.HandshakePacket)) return false;
    if (passthrough && !replaying) return false;
    BlockLagLock lock;
    auto* first = !watchdogActions.empty() ? &watchdogActions.front() :
        !watchdogConfirmations.empty() ? &watchdogConfirmations.front() : nullptr;
    if (first && !env->IsSameObject(manager, first->manager)) {
        DrainWatchdogPackets(env, watchdogActions, false);
        DrainWatchdogPackets(env, watchdogConfirmations, false);
        ClearWatchdogPhase(env);
        return false;
    }
    if (slinkyBlockLag || hypixelBlockLag) DrainBlockPackets(env, true);
    // Do not take a Prediction/Grim/ordinary Lag owner's packets.
    if (blockLagActive.load()) return false;
    if (env->IsInstanceOf(packet, h.MovePacket)) {
        if (!watchdogPhasePending.load()) return false;
        DrainWatchdogPackets(env, watchdogActions, true);
        const bool moved = SendWatchdogPacket(env, {manager, packet, callbacks});
        watchdogBeforeMovement.store(false);
        DrainWatchdogPackets(env, watchdogConfirmations, moved);
        ClearWatchdogPhase(env);
        return true;
    }
    const bool confirmation = env->IsInstanceOf(packet, h.TransactionReply);
    if (confirmation && !(watchdogBeforeMovement.load() && replaying) && watchdogConfirmations.empty())
        return false; // pre-physics C0F must precede the staged swing/attack
    if (!confirmation && !holding && !watchdogPhasePending.load()) return false;
    if (!watchdogPhasePending.load()) {
        const auto player = PlayerEntity(), world = LevelEntity();
        if (!player || !world) return false;
        watchdogPhasePlayer = env->NewGlobalRef(static_cast<jobject>(player.raw()));
        watchdogPhaseWorld = env->NewGlobalRef(static_cast<jobject>(world.raw()));
    }
    BlockPacket entry{env->NewGlobalRef(manager), env->NewGlobalRef(packet),
                      callbacks ? env->NewGlobalRef(callbacks) : nullptr};
    if (!Good(env) || !watchdogPhasePlayer || !watchdogPhaseWorld || !entry.manager ||
        !entry.packet || (callbacks && !entry.callbacks) ||
        watchdogActions.size() + watchdogConfirmations.size() >= 4096) {
        if (entry.manager) env->DeleteGlobalRef(entry.manager);
        if (entry.packet) env->DeleteGlobalRef(entry.packet);
        if (entry.callbacks) env->DeleteGlobalRef(entry.callbacks);
        DrainWatchdogPackets(env, watchdogConfirmations, true);
        DrainWatchdogPackets(env, watchdogActions, true);
        ClearWatchdogPhase(env);
        LogTo("AutoBlock: Watchdog packet phase exhausted JNI ownership/capacity; passing output");
        return false;
    }
    (confirmation ? watchdogConfirmations : watchdogActions).push_back(entry);
    watchdogPhasePending.store(true);
    return true;
}

bool Binding(JNIEnv* env, const void* value, const char* name) {
    const bool clean = Good(env);
    if (clean && value) return true;
    LogTo("Lunar: required binding unavailable: %s", name);
    return false;
}

struct Frame {
    JniAttach attach;
    JNIEnv* env = nullptr;
    jobject mc = nullptr;
    Frame() {
        if (!attach || !h.ready) return;
        if (attach->PushLocalFrame(32) != JNI_OK) { Good(attach.env); return; }
        env = attach.env;
        mc = Minecraft(env);
    }
    ~Frame() { if (env) env->PopLocalFrame(nullptr); }
    explicit operator bool() const { return env && mc; }
    jobject player() { return *this ? LocalPlayer(env) : nullptr; }
    jobject options() {
        if (!*this) return nullptr;
        jobject value = env->GetObjectField(mc, h.Options);
        return Good(env) ? value : nullptr;
    }
    jobject key(jfieldID field) {
        jobject settings = options();
        if (!settings) return nullptr;
        jobject value = env->GetObjectField(settings, field);
        return Good(env) ? value : nullptr;
    }
};

jobject UsePacket(JNIEnv* env, jobject stack) {
    return IsLunar1710() ? env->NewObject(h.UsePacket, h.UsePacketCtor, -1, -1, -1, 255, stack, 0.0f, 0.0f, 0.0f)
                        : env->NewObject(h.UsePacket, h.UsePacketCtor, stack);
}

bool HitCoordinates17(JNIEnv* env, jobject hit, java::BlockPos& pos) {
    if (!hit) return false;
    pos = {env->GetIntField(hit, h.HitX17), env->GetIntField(hit, h.HitY17), env->GetIntField(hit, h.HitZ17)};
    return Good(env);
}

bool SetKey(jfieldID field, bool down) {
    Frame frame;
    jobject key = frame.key(field);
    if (!key) return false;
    frame.env->SetBooleanField(key, h.KeyPressed, down ? JNI_TRUE : JNI_FALSE);
    return Good(frame.env);
}

bool Ability(jfieldID field) {
    Frame frame;
    jobject player = frame.player();
    if (!player) return false;
    jobject abilities = frame.env->GetObjectField(player, h.Capabilities);
    if (!Good(frame.env) || !abilities) return false;
    const bool value = frame.env->GetBooleanField(abilities, field) == JNI_TRUE;
    return Good(frame.env) && value;
}

jobject BlockTarget(Frame& frame) {
    if (!frame) return nullptr;
    jobject hit = frame.env->GetObjectField(frame.mc, h.HitResult);
    if (!Good(frame.env) || !hit) return nullptr;
    jobject type = frame.env->GetObjectField(hit, h.HitKind);
    if (!Good(frame.env) || !type) return nullptr;
    jobject blockType = frame.env->GetStaticObjectField(h.HitType, h.BlockType);
    if (!Good(frame.env) || !blockType) return nullptr;
    return frame.env->IsSameObject(type, blockType) ? hit : nullptr;
}

jobject ContainerScreen(Frame& frame) {
    if (!frame) return nullptr;
    jobject screen = frame.env->GetObjectField(frame.mc, h.Screen);
    if (!Good(frame.env) || !screen) return nullptr;
    return frame.env->IsInstanceOf(screen, h.ContainerScreen) ? screen : nullptr;
}
}

bool SendDisplacePacket(JNIEnv* env, jobject manager, jobject packet, jobject callbacks) {
    env->CallNonvirtualVoidMethod(manager, h.NetworkManager, h.DispatchPacket, packet, callbacks);
    if (!Good(env)) return false;
    if (env->IsInstanceOf(packet, h.MovePacket)) movementPacketSequence.fetch_add(1);
    RecordBlockPacket(env, packet);
    return true;
}

bool Initialize(JNIEnv* env) {
    jobject loader = ClassLoader(env);
    if (!loader) return false;
    bool ok = true;
#define NOVA_CLASS(id, name) \
    if (ok) { \
        jclass local = LoadClassInLoader(env, loader, name); \
        ok = Binding(env, local, name); \
        if (ok) { h.id = static_cast<jclass>(env->NewGlobalRef(local)); ok = Binding(env, h.id, name); } \
        if (local) env->DeleteLocalRef(local); \
    }
#define NOVA_FIELD(id, owner, name, desc) \
    if (ok) { h.id = env->GetFieldID(h.owner, name, desc); ok = Binding(env, h.id, name desc); }
#define NOVA_STATIC_FIELD(id, owner, name, desc) \
    if (ok) { h.id = env->GetStaticFieldID(h.owner, name, desc); ok = Binding(env, h.id, name desc); }
#define NOVA_METHOD(id, owner, name, desc) \
    if (ok) { h.id = env->GetMethodID(h.owner, name, desc); ok = Binding(env, h.id, name desc); }
#define NOVA_STATIC_METHOD(id, owner, name, desc) \
    if (ok) { h.id = env->GetStaticMethodID(h.owner, name, desc); ok = Binding(env, h.id, name desc); }
    if (IsLunar1710()) {
#define NOVA_LUNAR1710_RUNTIME
#include "minecraft/compat/mappings/Lunar1710.inc"
#undef NOVA_LUNAR1710_RUNTIME
    } else {
#include "minecraft/compat/mappings/Lunar189.inc"
    }
#undef NOVA_CLASS
#undef NOVA_FIELD
#undef NOVA_STATIC_FIELD
#undef NOVA_METHOD
#undef NOVA_STATIC_METHOD
    env->DeleteGlobalRef(loader);
    if (ok) {
        jobject effect = env->GetStaticObjectField(h.Potion, h.NightVision);
        ok = Binding(env, effect, "Potion.nightVision");
        if (ok) { h.nightVisionId = env->GetIntField(effect, h.PotionId); ok = Good(env); }
        if (effect) env->DeleteLocalRef(effect);
    }
    if (ok) {
        networkReentrySlot = TlsAlloc();
        ok = networkReentrySlot != TLS_OUT_OF_INDEXES;
        if (!ok) LogTo("Lunar: network reentry slot unavailable");
    }
    h.ready = ok;
    if (ok) LogTo("Lunar: required runtime mappings validated");
    else Shutdown(env);
    return ok;
}

void Shutdown(JNIEnv* env) {
    // Hooks have been restored and callbacks quiesced. Scheduled tasks remain
    // vanilla Runnable objects, so Java can finish them after DLL unload.
    incomingTasks.Clear(env);
    module::displace::EndBlink(false);
    MaintainWatchdogPhase(env, true);
    module::autoblock::SynchronizePacketDelays(true);
    module::autoblock::ResetCoverage();
    autoBlockEnabled.store(false);
    autoBlockManualSuppressed.store(false);
    ResetHypixelPacketState();
    movementPacketSequence.store(0);
    heldSlotOrderReady.store(true);
    grimResetSequence.store(0);
    SetAutoBlockState(false, false);
    {
        BlockLagLock lock;
        DrainBlockPackets(env, false);
    }
    worldValidated = false;
    if (networkReentrySlot != TLS_OUT_OF_INDEXES) {
        TlsFree(networkReentrySlot);
        networkReentrySlot = TLS_OUT_OF_INDEXES;
    }
#define NOVA_CLASS(id, name) if (h.id) env->DeleteGlobalRef(h.id);
#define NOVA_FIELD(id, owner, name, desc)
#define NOVA_STATIC_FIELD NOVA_FIELD
#define NOVA_METHOD NOVA_FIELD
#define NOVA_STATIC_METHOD NOVA_FIELD
#include "minecraft/compat/mappings/Lunar189.inc"
#define NOVA_LUNAR1710_EXTRA
#include "minecraft/compat/mappings/Lunar1710.inc"
#undef NOVA_LUNAR1710_EXTRA
#undef NOVA_CLASS
#undef NOVA_FIELD
#undef NOVA_STATIC_FIELD
#undef NOVA_METHOD
#undef NOVA_STATIC_METHOD
    h = {};
}

bool SetSprintKeyDown(bool down) { return SetKey(h.SprintKey, down); }
bool SetAttackDown(bool down) { return SetKey(h.AttackKey, down); }
bool SetUseKeyDown(bool down) {
    Frame frame;
    jobject key = frame.key(h.UseKey);
    if (!key) return false;
    const jint code = frame.env->CallIntMethod(key, h.KeyCode);
    if (!Good(frame.env)) return false;
    frame.env->CallStaticVoidMethod(h.KeyBinding, h.KeyState, code, down ? JNI_TRUE : JNI_FALSE);
    return Good(frame.env);
}
bool PlayerInCreative() { return Ability(h.Creative); }
bool PlayerCanEdit() { return Ability(h.AllowEdit); }

bool GetGamma(double* out) {
    if (!out) return false;
    Frame frame;
    jobject options = frame.options();
    if (!options) return false;
    const float value = frame.env->GetFloatField(options, h.Gamma);
    if (!Good(frame.env)) return false;
    *out = value;
    return true;
}

bool SetGamma(double value) {
    if (!std::isfinite(value)) return false;
    Frame frame;
    jobject options = frame.options();
    if (!options) return false;
    frame.env->SetFloatField(options, h.Gamma, static_cast<jfloat>(value));
    return Good(frame.env);
}

bool PlayerOnGround() {
    Frame frame;
    jobject player = frame.player();
    if (!player) return false;
    const bool result = frame.env->GetBooleanField(player, h.OnGround) == JNI_TRUE;
    return Good(frame.env) && result;
}

bool PlayerUsingItem() {
    Frame frame;
    jobject player = frame.player();
    if (!player) return false;
    const bool result = frame.env->CallBooleanMethod(player, h.UsingItem) == JNI_TRUE;
    return Good(frame.env) && result;
}

bool JumpResetReady(int& playerTicks) {
    Frame frame;
    jobject player = frame.player();
    if (!player) return false;
    auto* env = frame.env;
    playerTicks = env->GetIntField(player, h.Ticks);
    const bool grounded = env->GetBooleanField(player, h.OnGround);
    const bool dead = env->GetBooleanField(player, h.EntityDead);
    const int deathTime = env->GetIntField(player, h.DeathTime);
    const int jumpCooldown = env->GetIntField(player, h.JumpCooldown);
    if (!Good(env) || !grounded || dead || deathTime != 0 || jumpCooldown > 1) return false;
    jobject riding = env->GetObjectField(player, h.Riding);
    if (!Good(env) || riding) return false;
    jobject abilities = env->GetObjectField(player, h.Capabilities);
    if (!Good(env) || !abilities) return false;
    const bool flying = env->GetBooleanField(abilities, h.Flying) || env->GetBooleanField(abilities, h.Creative);
    if (!Good(env) || flying) return false;
    const bool water = env->CallBooleanMethod(player, h.InWater);
    if (!Good(env) || water) return false;
    const bool lava = env->CallBooleanMethod(player, h.InLava);
    if (!Good(env) || lava) return false;
    const bool ladder = env->CallBooleanMethod(player, h.OnLadder);
    return Good(env) && !ladder;
}

bool HoldingWeapon() {
    Frame frame;
    jobject player = frame.player();
    if (!player) return false;
    jobject stack = frame.env->CallObjectMethod(player, h.HeldItem);
    if (!Good(frame.env) || !stack) return false;
    jobject item = frame.env->CallObjectMethod(stack, h.GetItem);
    if (!Good(frame.env) || !item) return false;
    return frame.env->IsInstanceOf(item, h.Sword) || frame.env->IsInstanceOf(item, h.Axe);
}

bool HoldingSword() {
    Frame frame;
    jobject player = frame.player();
    if (!player) return false;
    jobject stack = frame.env->CallObjectMethod(player, h.HeldItem);
    if (!Good(frame.env) || !stack) return false;
    jobject item = frame.env->CallObjectMethod(stack, h.GetItem);
    return Good(frame.env) && item && frame.env->IsInstanceOf(item, h.Sword);
}

bool StartSwordBlock() {
    Frame frame;
    if (!frame || !HoldingSword()) return false;
    jobject key = frame.key(h.UseKey);
    if (!key) return false;
    const jint code = frame.env->CallIntMethod(key, h.KeyCode);
    if (!Good(frame.env)) return false;
    frame.env->CallStaticVoidMethod(h.KeyBinding, h.KeyState, code, JNI_TRUE);
    frame.env->CallStaticVoidMethod(h.KeyBinding, h.KeyTick, code);
    return Good(frame.env);
}

bool EntityDead(void* entity) {
    Frame frame;
    if (!frame || !entity) return true;
    const bool dead = frame.env->GetBooleanField(static_cast<jobject>(entity), h.EntityDead);
    return !Good(frame.env) || dead;
}

bool StopSwordBlock() {
    return SetUseKeyDown(false);
}

bool StartSwordBlockDirect() {
    Frame f;
    jobject player = f.player();
    if (!player || !HoldingSword()) return false;
    jobject controller = f.env->GetObjectField(f.mc, h.GameController);
    if (!Good(f.env) || !controller) return false;
    f.env->CallVoidMethod(controller, h.SyncHeldItem);
    if (!Good(f.env)) return false;
    jobject stack = f.env->CallObjectMethod(player, h.HeldItem);
    if (!Good(f.env) || !stack) return false;
    jobject packet = UsePacket(f.env, stack);
    if (!Good(f.env) || !packet || !SendPacket(packet)) return false;
    const int duration = f.env->CallIntMethod(stack, h.MaxItemUseDuration);
    if (!Good(f.env)) return false;
    f.env->CallVoidMethod(player, h.SetItemInUse, stack, duration);
    if (!Good(f.env)) return false;
    hypixelBlockingState.store(true);
    return true;
}

bool StopSwordBlockDirect() {
    Frame f;
    jobject player = f.player();
    if (!player) return false;
    if (IsLunar1710()) {
        auto packet = f.env->NewObject(h.ActionPacket, h.DigPacketCtor, 5, 0, 0, 0, 0);
        if (!Good(f.env) || !packet || !SendPacket(packet)) return false;
        f.env->CallVoidMethod(player, h.StopItemInUse);
        hypixelBlockingState.store(false);
        return Good(f.env);
    }
    jobject action = f.env->GetStaticObjectField(h.DigActionEnum, h.ReleaseItemAction);
    jobject down = f.env->GetStaticObjectField(h.Facing, h.FacingDown);
    jobject origin = f.env->NewObject(h.BlockPos, h.BlockPosCtor, 0, 0, 0);
    if (!Good(f.env) || !action || !down || !origin) return false;
    jobject packet = f.env->NewObject(h.ActionPacket, h.DigPacketCtor, action, origin, down);
    if (!Good(f.env) || !packet || !SendPacket(packet)) return false;
    f.env->CallVoidMethod(player, h.StopItemInUse);
    hypixelBlockingState.store(false);
    return Good(f.env);
}

bool SendHeldSlot(int slot) {
    Frame f;
    if (!f || slot < 0 || slot > 8) return false;
    jobject packet = f.env->NewObject(h.HeldSlotPacket, h.HeldSlotCtor, slot);
    return Good(f.env) && packet && SendPacket(packet);
}

bool InteractSwordBlock(const java::EntityHandle& target, const rotation::Vec3& hit,
                        const rotation::Vec3* snapshotPosition) {
    Frame f;
    jobject player = f.player();
    if (!player || !target || !HoldingSword()) return false;
    rotation::Vec3 pos;
    if (snapshotPosition) pos = *snapshotPosition;
    else if (!target.Position(pos)) return false;
    jobject controller = f.env->GetObjectField(f.mc, h.GameController);
    if (!Good(f.env) || !controller) return false;
    f.env->CallVoidMethod(controller, h.SyncHeldItem);
    if (!Good(f.env)) return false;
    jobject vec = f.env->NewObject(h.Vec, h.VecCtor, hit.x-pos.x, hit.y-pos.y, hit.z-pos.z);
    jobject interact = f.env->GetStaticObjectField(h.InteractActionEnum, h.InteractEntityAction);
    if (!Good(f.env) || !vec || !interact) return false;
    if (!IsLunar1710()) {
        jobject at = f.env->NewObject(h.InteractPacket, h.InteractAtCtor, static_cast<jobject>(target.raw()), vec);
        if (!Good(f.env) || !at || !SendPacket(at)) return false;
    }
    jobject packet = f.env->NewObject(h.InteractPacket, h.InteractEntityCtor, static_cast<jobject>(target.raw()), interact);
    if (!Good(f.env) || !packet || !SendPacket(packet)) return false;
    // Sync already ran before INTERACT_AT; do not insert another C09 here.
    jobject stack = f.env->CallObjectMethod(player, h.HeldItem);
    if (!Good(f.env) || !stack) return false;
    jobject use = UsePacket(f.env, stack);
    if (!Good(f.env) || !use || !SendPacket(use)) return false;
    const int duration = f.env->CallIntMethod(stack, h.MaxItemUseDuration);
    if (!Good(f.env)) return false;
    f.env->CallVoidMethod(player, h.SetItemInUse, stack, duration);
    if (!Good(f.env)) return false;
    hypixelBlockingState.store(true);
    return true;
}

bool HypixelPlayerBlocking() {
    return HoldingSword() && (PlayerUsingItem() || hypixelBlockingState.load());
}
bool HypixelPlayerBusy() { return hypixelDigging.load() || hypixelPlacing.load(); }
void SetHypixelBlockManaged(bool managed) { hypixelBlockManaged.store(managed); }
void ResetHypixelPacketState() {
    hypixelBlockManaged.store(false);
    hypixelBlockingState.store(false);
    hypixelDigging.store(false);
    hypixelPlacing.store(false);
}
bool TimerAtLeastNormal() {
    Frame f;
    if (!f) return false;
    jobject timer = f.env->GetObjectField(f.mc, h.GameTimer);
    if (!Good(f.env) || !timer) return false;
    const float speed = f.env->GetFloatField(timer, h.TimerSpeed);
    return Good(f.env) && speed >= 1.0f;
}

bool SetTimerSpeed(float speed) {
    if (!std::isfinite(speed) || speed <= 0.f) return false;
    Frame f;
    if (!f) return false;
    jobject timer = f.env->GetObjectField(f.mc, h.GameTimer);
    if (!Good(f.env) || !timer) return false;
    f.env->SetFloatField(timer, h.TimerSpeed, speed);
    return Good(f.env);
}

void SetAutoBlockState(bool managed, bool animation) {
    autoBlockManaged.store(managed);
    autoBlockAnimation.store(animation);
}

void SetAutoBlockEnabled(bool enabled) { autoBlockEnabled.store(enabled); }

bool PhysicalMouseDown(int button) {
    Frame frame;
    if (!frame) return false;
    const bool down = frame.env->CallStaticBooleanMethod(h.Mouse, h.MouseButton, static_cast<jint>(button));
    return Good(frame.env) && down;
}

bool PhysicalUseDown() {
    Frame f;
    if (!f || java::ScreenOpen()) return false;
    jobject key = f.key(h.UseKey);
    if (!key) return false;
    const int code = f.env->CallIntMethod(key, h.KeyCode);
    if (!Good(f.env)) return false;
    const bool down = code < 0
        ? f.env->CallStaticBooleanMethod(h.Mouse, h.MouseButton, code + 100)
        : f.env->CallStaticBooleanMethod(h.Keyboard, h.KeyboardKey, code);
    return Good(f.env) && down;
}

bool IsAttackPacket(jobject packet) {
    Frame frame;
    if (!frame || !packet || !frame.env->IsInstanceOf(packet, h.InteractPacket)) return false;
    jobject action = frame.env->CallObjectMethod(packet, h.InteractAction);
    if (!Good(frame.env) || !action) return false;
    const int ordinal = frame.env->CallIntMethod(action, h.Ordinal);
    return Good(frame.env) && ordinal == 1;
}

void BeginBlockLag() {
    JniAttach attach;
    if (!attach) return;
    BlockLagLock lock;
    if (blockLagActive.load() || watchdogPhasePending.load()) return;
    DrainBlockPackets(attach.env, true);
    blockLagActive.store(true);
}

void BeginHypixelBlockLag() {
    JniAttach attach;
    if (!attach || !h.ready) return;
    BlockLagLock lock;
    if (blockLagActive.load() || module::antikb::BlocksAutoBlockBlink() || watchdogPhasePending.load()) return;
    DrainBlockPackets(attach.env, true);
    hypixelBlockLag = true;
    blockLagActive.store(true);
}

void EndHypixelBlockLag(bool send) {
    JniAttach attach;
    if (!attach) return;
    BlockLagLock lock;
    if (hypixelBlockLag && !predictionBlockLag.load()) DrainBlockPackets(attach.env, send && h.ready);
}

bool HypixelBlockLagging() { BlockLagLock lock; return hypixelBlockLag; }

bool BeginPredictionBlink() {
    JniAttach attach;
    if (!attach || !h.ready) return false;
    BlockLagLock lock;
    if (watchdogPhasePending.load()) return false;
    // Transfer the existing queue; do not flush AutoBlock's accumulated packets.
    grimBlockLag = hypixelBlockLag = false;
    slinkyBlockLag = false;
    slinkyDeadline = 0;
    slinkyAttackReleased.store(false);
    grimReblockPending = false;
    grimBlockDeadline = 0;
    predictionBlockLag.store(true);
    blockLagActive.store(true);
    return true;
}

void EndPredictionBlink(bool send) {
    JniAttach attach;
    if (!attach) return;
    BlockLagLock lock;
    if (predictionBlockLag.load()) DrainBlockPackets(attach.env, send && h.ready);
}
bool PredictionBlinking() { return predictionBlockLag.load(); }

bool BeginGrimBlockLag(int maxMillis) {
    JniAttach attach;
    if (!attach || !h.ready) return false;
    BlockLagLock lock;
    if (blockLagActive.load() || watchdogPhasePending.load()) return false;
    DrainBlockPackets(attach.env, true);
    grimBlockLag = true;
    grimBlockDeadline = GetTickCount64() + static_cast<ULONGLONG>(std::clamp(maxMillis, 50, 300));
    blockLagActive.store(true);
    return true;
}

std::uint64_t GrimResetSequence() { return grimResetSequence.load(); }

bool GrimBlockLagExpired() {
    BlockLagLock lock;
    return !grimBlockLag || !blockLagActive.load() || GetTickCount64() >= grimBlockDeadline;
}

bool PrepareGrimBlockLagRelease(std::uint64_t& movementBoundary) {
    JniAttach attach;
    if (!attach || !h.ready) return false;
    BlockLagLock lock;
    if (!grimBlockLag || !blockLagActive.load()) return false;
    bool released = false, movedAfterRelease = false;
    std::uint64_t queuedMovement = 0;
    for (const auto& entry : blockPackets) {
        if (BlockReleasePacket(attach.env, entry.packet)) {
            released = true;
            movedAfterRelease = false;
        }
        if (attach.env->IsInstanceOf(entry.packet, h.MovePacket)) {
            ++queuedMovement;
            if (released) movedAfterRelease = true;
        }
    }
    if (!movedAfterRelease) return false;
    movementBoundary = movementPacketSequence.load() + queuedMovement;
    grimReblockPending = true;
    return true;
}

void ReleaseBlockLag() {
    BlockLagLock lock;
    if (!predictionBlockLag.load()) blockLagActive.store(false);
}

void EndBlockLag(bool send) {
    JniAttach attach;
    if (!attach) return;
    BlockLagLock lock;
    if (predictionBlockLag.load()) return; // AutoBlock cannot release Velocity's owner.
    DrainBlockPackets(attach.env, send && h.ready);
}

bool BlockLagging() { return blockLagActive.load(); }

bool BeginSlinkyBlockLag(int maxMillis, int maxPackets, bool preventAttacks) {
    JniAttach attach;
    if (!attach || !h.ready) return false;
    BlockLagLock lock;
    if (blockLagActive.load() || module::antikb::BlocksAutoBlockBlink() || watchdogPhasePending.load()) return false;
    DrainBlockPackets(attach.env, true);
    slinkyBlockLag = true;
    slinkyDeadline = GetTickCount64() + static_cast<ULONGLONG>(std::clamp(maxMillis, 0, 500));
    slinkyMaxPackets = std::clamp(maxPackets, 1, 10);
    slinkyPreventAttacks = preventAttacks;
    slinkyAttackReleased.store(false);
    blockLagActive.store(true);
    return true;
}

void EndSlinkyBlockLag(bool send) {
    JniAttach attach;
    if (!attach) return;
    BlockLagLock lock;
    if (slinkyBlockLag) DrainBlockPackets(attach.env, send && h.ready);
    slinkyAttackReleased.store(false);
}

void DrainSlinkyBlockLag() {
    JniAttach attach;
    if (!attach || !h.ready) return;
    BlockLagLock lock;
    // One expired packet per rendered frame; tick/outgoing expiration drains all.
    if (slinkyBlockLag && GetTickCount64() >= slinkyDeadline) SendOldestBlockPacket(attach.env);
}

bool SlinkyBlockLagging() {
    BlockLagLock lock;
    return slinkyBlockLag;
}
bool ConsumeSlinkyAttackRelease() { return slinkyAttackReleased.exchange(false); }

bool PredictionLiquid() {
    Frame f;
    jobject p = f.player();
    if (!p) return true;
    const bool water = f.env->CallBooleanMethod(p, h.InWater);
    const bool lava = f.env->CallBooleanMethod(p, h.InLava);
    return !Good(f.env) || water || lava;
}
bool PredictionLiquidWeb() {
    Frame f;
    jobject p = f.player();
    if (!p) return true;
    const bool web = f.env->GetBooleanField(p, h.InWeb);
    return !Good(f.env) || web || PredictionLiquid();
}
bool PredictionJumpBoost() {
    Frame f;
    jobject p = f.player();
    if (!p) return true;
    jobject potion = f.env->GetStaticObjectField(h.Potion, h.JumpPotion);
    if (!Good(f.env) || !potion) return true;
    const bool active = f.env->CallBooleanMethod(p, h.PotionActive, potion);
    return !Good(f.env) || active;
}
bool PredictionForwardPressed() {
    Frame f;
    if (!f) return false;
    auto down = [&](jfieldID field) {
        jobject key = f.key(field);
        const bool pressed = key && f.env->CallBooleanMethod(key, h.KeyDown);
        return Good(f.env) && pressed;
    };
    // The reference helper's name is misleading: either movement axis counts.
    return (down(h.ForwardKey) != down(h.BackKey)) || (down(h.LeftKey) != down(h.RightKey));
}
void PredictionJump() {
    Frame f;
    jobject p = f.player();
    if (!p) return;
    jobject input = f.env->GetObjectField(p, h.PredictionInput);
    if (Good(f.env) && input) f.env->SetBooleanField(input, h.MoveJump, JNI_TRUE);
    Good(f.env);
}
int PredictionIncomingKind(jobject packet) {
    Frame f;
    if (!f || !packet) return 0;
    if (f.env->IsInstanceOf(packet, h.DisconnectPacket)) return 5;
    if (f.env->IsInstanceOf(packet, h.JoinPacket) || f.env->IsInstanceOf(packet, h.RespawnPacket)) return 2;
    if (f.env->IsInstanceOf(packet, h.KeepAlivePacket)) return 1;
    if (f.env->IsInstanceOf(packet, h.PositionPacket)) return 6;
    if (f.env->IsInstanceOf(packet, h.PingPacket)) return 7;
    if (!f.env->IsInstanceOf(packet, h.EntityStatusPacket)) return 0;
    jobject p = f.player();
    if (!p) return 0;
    const int id = f.env->GetIntField(packet, h.StatusEntityId);
    const int opcode = f.env->CallByteMethod(packet, h.StatusOpcode);
    const int self = f.env->CallIntMethod(p, h.EntityId);
    if (!Good(f.env) || !EntityById(id)) return 0;
    return id == self && opcode == 2 ? 3 : 4;
}

bool SendAttackPacket(const java::EntityHandle& target) {
    Frame f;
    if (!f || !target) return false;
    jobject action = f.env->GetStaticObjectField(h.InteractActionEnum, h.AttackEntityAction);
    if (!Good(f.env) || !action) return false;
    jobject packet = f.env->NewObject(h.InteractPacket, h.InteractEntityCtor,
                                     static_cast<jobject>(target.raw()), action);
    return Good(f.env) && packet && SendPacket(packet);
}

bool AddNightVision() {
    Frame frame;
    jobject player = frame.player();
    if (!player) return false;
    jobject effect = IsLunar1710()
        ? frame.env->NewObject(h.Effect, h.EffectCtor, h.nightVisionId, 1337, 0, JNI_FALSE)
        : frame.env->NewObject(h.Effect, h.EffectCtor, h.nightVisionId, 1337, 0, JNI_FALSE, JNI_FALSE);
    if (!Good(frame.env) || !effect) return false;
    frame.env->CallVoidMethod(player, h.AddEffect, effect);
    return Good(frame.env);
}

bool RemoveNightVision() {
    Frame frame;
    jobject player = frame.player();
    if (!player) return false;
    frame.env->CallVoidMethod(player, h.RemoveEffect, h.nightVisionId);
    return Good(frame.env);
}

bool MouseGrabbed() {
    Frame frame;
    if (!frame) return false;
    const bool result = frame.env->CallStaticBooleanMethod(h.Mouse, h.MouseGrabbed) == JNI_TRUE;
    return Good(frame.env) && result;
}

bool ClickAttack() {
    Frame frame;
    jobject key = frame.key(h.AttackKey);
    if (!key) return false;
    const jint code = frame.env->CallIntMethod(key, h.KeyCode);
    if (!Good(frame.env)) return false;
    frame.env->CallStaticVoidMethod(h.KeyBinding, h.KeyTick, code);
    return Good(frame.env);
}

bool PendingAttackClick() {
    Frame frame;
    jobject attack = frame.key(h.AttackKey);
    if (!attack) return false;
    const jint presses = frame.env->GetIntField(attack, h.KeyPressTime);
    return Good(frame.env) && presses > 0;
}

bool ClearCombatClicks() {
    Frame frame;
    jobject attack = frame.key(h.AttackKey), use = frame.key(h.UseKey);
    if (!attack || !use) return false;
    frame.env->SetIntField(attack, h.KeyPressTime, 0);
    frame.env->SetIntField(use, h.KeyPressTime, 0);
    return Good(frame.env);
}

std::uint64_t MovementPacketSequence() { return movementPacketSequence.load(); }
std::uint64_t BlockInputMovementSequence() { return blockInputMovementSequence.load(); }
bool BlockInputAttacked() { return blockInputAttacked.load(); }
bool HeldSlotChangeReady() { return heldSlotOrderReady.load(); }
void SetAutoBlockManualSuppressed(bool suppressed) { autoBlockManualSuppressed.store(suppressed); }

bool ClearMissTime() {
    Frame frame;
    if (!frame) return false;
    frame.env->SetIntField(frame.mc, h.MissTime, 0);
    return Good(frame.env);
}

bool AttackTargetIsBlock() {
    Frame frame;
    return BlockTarget(frame) != nullptr;
}

bool TargetedBlockBreakable() {
    Frame frame;
    jobject hit = BlockTarget(frame);
    if (!hit) return false;
    jobject level = frame.env->GetObjectField(frame.mc, h.Level);
    if (!Good(frame.env) || !level) return false;
    if (IsLunar1710()) {
        java::BlockPos p;
        if (!HitCoordinates17(frame.env, hit, p)) return false;
        auto block = frame.env->CallObjectMethod(level, h.GetBlockState, p.x, p.y, p.z);
        return Good(frame.env) && block && !frame.env->IsInstanceOf(block, h.Air) && !frame.env->IsInstanceOf(block, h.Liquid);
    }
    jobject pos = frame.env->CallObjectMethod(hit, h.HitBlockPos);
    if (!Good(frame.env) || !pos) return false;
    jobject state = frame.env->CallObjectMethod(level, h.GetBlockState, pos);
    if (!Good(frame.env) || !state) return false;
    jobject block = frame.env->CallObjectMethod(state, h.GetBlock);
    if (!Good(frame.env) || !block) return false;
    return !frame.env->IsInstanceOf(block, h.Air) && !frame.env->IsInstanceOf(block, h.Liquid);
}

bool InventoryScreenOpen() {
    Frame frame;
    return ContainerScreen(frame) != nullptr;
}

bool ClickHoveredSlot(bool quickMove) {
    Frame frame;
    jobject screen = ContainerScreen(frame);
    if (!screen || !frame.player()) return false;
    jobject slot = frame.env->GetObjectField(screen, h.HoveredSlot);
    if (!Good(frame.env) || !slot) return false;
    const jint id = frame.env->GetIntField(slot, h.SlotIndex);
    if (!Good(frame.env) || id < 0) return false;
    frame.env->CallVoidMethod(screen, h.ClickSlot, slot, id, static_cast<jint>(0),
        static_cast<jint>(quickMove ? 1 : 0));
    return Good(frame.env);
}

bool PlayerReportedRotation(float& yaw, float& pitch) {
    Frame f;
    jobject player = f.player();
    if (!player) return false;
    const float y = f.env->GetFloatField(player, h.ReportedYaw);
    const float p = f.env->GetFloatField(player, h.ReportedPitch);
    if (!Good(f.env)) return false;
    yaw = y; pitch = p;
    return true;
}

bool PlayerRotation(float& yaw, float& pitch, bool previous) {
    Frame frame;
    jobject player = frame.player();
    if (!player) return false;
    const float y = frame.env->GetFloatField(player, previous ? h.PreviousYaw : h.Yaw);
    const float p = frame.env->GetFloatField(player, previous ? h.PreviousPitch : h.Pitch);
    if (!Good(frame.env)) return false;
    yaw = y; pitch = p;
    return true;
}

bool SetPlayerRotation(float yaw, float pitch) {
    if (!std::isfinite(yaw) || !std::isfinite(pitch)) return false;
    Frame frame;
    jobject player = frame.player();
    if (!player) return false;
    frame.env->SetFloatField(player, h.Yaw, yaw);
    frame.env->SetFloatField(player, h.Pitch, pitch);
    return Good(frame.env);
}

bool PlayerEyePosition(double& x, double& y, double& z) {
    Frame frame;
    jobject player = frame.player();
    if (!player) return false;
    if (IsLunar1710()) {
        rotation::Vec3 eye;
        if (!EntityEyePosition(player,1.f,eye)) return false;
        x=eye.x;y=eye.y;z=eye.z;return true;
    }
    const double px = frame.env->GetDoubleField(player, h.X);
    const double py = frame.env->GetDoubleField(player, h.Y);
    const double pz = frame.env->GetDoubleField(player, h.Z);
    const float height = frame.env->CallFloatMethod(player, h.EyeHeight);
    if (!Good(frame.env)) return false;
    x = px; y = py + height; z = pz;
    return true;
}

bool PlayerSensitivity(double& out) {
    Frame frame;
    jobject options = frame.options();
    if (!options) return false;
    const float value = frame.env->GetFloatField(options, h.Sensitivity);
    if (!Good(frame.env)) return false;
    out = value;
    return true;
}

bool RenderDistance(int& out) {
    Frame frame;
    jobject options = frame.options();
    if (!options) return false;
    const jint value = frame.env->GetIntField(options, h.RenderChunks);
    if (!Good(frame.env)) return false;
    out = value;
    return true;
}

bool Fov(int& out) {
    Frame frame;
    jobject options = frame.options();
    if (!options) return false;
    const float value = frame.env->GetFloatField(options, h.Fov);
    if (!Good(frame.env) || !std::isfinite(value)) return false;
    out = static_cast<int>(value);
    return true;
}

int ClientFps() {
    Frame frame;
    if (!frame) return 0;
    const jint value = (IsLunar1710() ? frame.env->GetStaticIntField(h.Minecraft, h.Fps17) : frame.env->CallStaticIntMethod(h.Minecraft, h.DebugFps));
    return Good(frame.env) ? value : 0;
}

void PickCursor(float partialTicks) {
    Frame frame;
    if (!frame) return;
    jobject renderer = frame.env->GetObjectField(frame.mc, h.GameRenderer);
    if (!Good(frame.env) || !renderer) return;
    frame.env->CallVoidMethod(renderer, h.Pick, partialTicks);
    Good(frame.env);
}

bool IsPosRot(void* packet) {
    Frame frame;
    return frame && packet && frame.env->IsInstanceOf(static_cast<jobject>(packet), h.PosRotPacket);
}

bool ReadPosRot(void* packet, java::PosRot& out) {
    Frame frame;
    auto object = static_cast<jobject>(packet);
    if (!frame || !object || !frame.env->IsInstanceOf(object, h.PosRotPacket)) return false;
    java::PosRot value;
    value.x = frame.env->GetDoubleField(object, h.PacketX);
    value.y = frame.env->GetDoubleField(object, h.PacketY);
    value.z = frame.env->GetDoubleField(object, h.PacketZ);
    value.yaw = frame.env->GetFloatField(object, h.PacketYaw);
    value.pitch = frame.env->GetFloatField(object, h.PacketPitch);
    value.onGround = frame.env->GetBooleanField(object, h.PacketOnGround) == JNI_TRUE;
    if (!Good(frame.env)) return false;
    out = value;
    return true;
}

void* MakePosRot(const java::PosRot& value) {
    Frame frame;
    if (!frame) return nullptr;
    jobject packet = IsLunar1710() ? frame.env->NewObject(h.PosRotPacket, h.PosRotCtor,
        value.x, value.y, value.y + 1.62, value.z, value.yaw, value.pitch, value.onGround ? JNI_TRUE : JNI_FALSE)
        : frame.env->NewObject(h.PosRotPacket, h.PosRotCtor,
        static_cast<jdouble>(value.x), static_cast<jdouble>(value.y), static_cast<jdouble>(value.z),
        static_cast<jfloat>(value.yaw), static_cast<jfloat>(value.pitch), value.onGround ? JNI_TRUE : JNI_FALSE);
    if (!Good(frame.env)) return nullptr;
    jobject result = frame.env->PopLocalFrame(packet);
    frame.env = nullptr;
    return result;
}

bool ReadEntityBool(void* object, EntityBool property, bool& out) {
    Frame frame;
    auto entity = static_cast<jobject>(object);
    if (!frame || !entity) return false;
    bool value = false;
    switch (property) {
    case EntityBool::Player: value = frame.env->IsInstanceOf(entity, h.Player); break;
    case EntityBool::Spectator:
        value = !IsLunar1710() && frame.env->IsInstanceOf(entity, h.Player) && frame.env->CallBooleanMethod(entity, h.IsSpectator);
        break;
    case EntityBool::Pickable: value = frame.env->CallBooleanMethod(entity, h.IsPickable); break;
    case EntityBool::Alive: value = frame.env->CallBooleanMethod(entity, h.IsAlive); break;
    case EntityBool::Invisible: value = frame.env->CallBooleanMethod(entity, h.IsInvisible); break;
    case EntityBool::OnGround: value = frame.env->GetBooleanField(entity, h.OnGround); break;
    }
    if (!Good(frame.env)) return false;
    out = value;
    return true;
}

bool ReadEntityInt(void* object, EntityInt property, int& out) {
    Frame frame;
    auto entity = static_cast<jobject>(object);
    if (!frame || !entity) return false;
    if ((property == EntityInt::Hurt || property == EntityInt::Death) &&
        !frame.env->IsInstanceOf(entity, h.Living)) return false;
    jint value = 0;
    switch (property) {
    case EntityInt::Id: value = frame.env->CallIntMethod(entity, h.EntityId); break;
    case EntityInt::Ticks: value = frame.env->GetIntField(entity, h.Ticks); break;
    case EntityInt::Hurt: value = frame.env->GetIntField(entity, h.HurtTime); break;
    case EntityInt::Death: value = frame.env->GetIntField(entity, h.DeathTime); break;
    }
    if (!Good(frame.env)) return false;
    out = value;
    return true;
}

bool ReadEntityFloat(void* object, EntityFloat property, float& out) {
    Frame frame;
    auto entity = static_cast<jobject>(object);
    if (!frame || !entity) return false;
    if ((property == EntityFloat::Health || property == EntityFloat::Absorption ||
         property == EntityFloat::MaxHealth) && !frame.env->IsInstanceOf(entity, h.Living)) return false;
    jfloat value = 0;
    switch (property) {
    case EntityFloat::EyeHeight:
        value = IsLunar1710() && frame.env->IsInstanceOf(entity,h.Player) ? 1.62f : frame.env->CallFloatMethod(entity, h.EyeHeight);
        break;
    case EntityFloat::PickRadius: value = frame.env->CallFloatMethod(entity, h.PickRadius); break;
    case EntityFloat::Health: value = frame.env->CallFloatMethod(entity, h.GetHealth); break;
    case EntityFloat::Absorption: value = frame.env->CallFloatMethod(entity, h.GetAbsorption); break;
    case EntityFloat::MaxHealth: value = frame.env->CallFloatMethod(entity, h.GetMaxHealth); break;
    case EntityFloat::FallDistance: value = frame.env->GetFloatField(entity, h.FallDistance); break;
    }
    if (!Good(frame.env)) return false;
    out = value;
    return true;
}

bool EntityPosition(void* object, rotation::Vec3& out, bool previous) {
    Frame frame;
    auto entity = static_cast<jobject>(object);
    if (!frame || !entity) return false;
    rotation::Vec3 value;
    value.x = frame.env->GetDoubleField(entity, previous ? h.PreviousX : h.X);
    value.y = frame.env->GetDoubleField(entity, previous ? h.PreviousY : h.Y);
    value.z = frame.env->GetDoubleField(entity, previous ? h.PreviousZ : h.Z);
    if (IsLunar1710()) value.y -= frame.env->GetFloatField(entity,h.YOffset17);
    if (!Good(frame.env)) return false;
    out = value;
    return true;
}

bool EntityRenderPositionPair(void* object, rotation::Vec3& now, rotation::Vec3& before) {
    Frame frame;
    auto entity = static_cast<jobject>(object);
    if (!frame || !entity) return false;
    rotation::Vec3 current, previous;
    current.x = frame.env->GetDoubleField(entity, h.X);
    current.y = frame.env->GetDoubleField(entity, h.Y);
    current.z = frame.env->GetDoubleField(entity, h.Z);
    previous.x = frame.env->GetDoubleField(entity, h.RenderPreviousX);
    previous.y = frame.env->GetDoubleField(entity, h.RenderPreviousY);
    previous.z = frame.env->GetDoubleField(entity, h.RenderPreviousZ);
    if (IsLunar1710()) {
        const float offset=frame.env->GetFloatField(entity,h.YOffset17);
        current.y-=offset;previous.y-=offset;
    }
    const int ticks = frame.env->GetIntField(entity, h.Ticks);
    if (!Good(frame.env)) return false;
    now = current;
    before = ticks == 0 ? current : previous;
    return true;
}

bool EntityEyePosition(void* object, float partialTicks, rotation::Vec3& out) {
    Frame frame;
    auto entity = static_cast<jobject>(object);
    if (!frame || !entity) return false;
    if (IsLunar1710()) {
        rotation::Vec3 now, before;
        float eye = 0;
        if (!EntityPosition(object, now) || !EntityPosition(object, before, true) || !ReadEntityFloat(object, EntityFloat::EyeHeight, eye)) return false;
        out = {before.x + (now.x-before.x)*partialTicks, before.y+(now.y-before.y)*partialTicks+eye, before.z+(now.z-before.z)*partialTicks};
        return true;
    }
    jobject vec = frame.env->CallObjectMethod(entity, h.EyePosition, partialTicks);
    if (!Good(frame.env) || !vec) return false;
    rotation::Vec3 value;
    value.x = frame.env->GetDoubleField(vec, h.VecX);
    value.y = frame.env->GetDoubleField(vec, h.VecY);
    value.z = frame.env->GetDoubleField(vec, h.VecZ);
    if (!Good(frame.env)) return false;
    out = value;
    return true;
}

bool EntityBox(void* object, java::Box& out) {
    Frame frame;
    auto entity = static_cast<jobject>(object);
    if (!frame || !entity) return false;
    jobject box = IsLunar1710() ? frame.env->GetObjectField(entity, h.Box17) : frame.env->CallObjectMethod(entity, h.BoundingBox);
    if (!Good(frame.env) || !box) return false;
    java::Box value;
    value.minX = frame.env->GetDoubleField(box, h.MinX);
    value.minY = frame.env->GetDoubleField(box, h.MinY);
    value.minZ = frame.env->GetDoubleField(box, h.MinZ);
    value.maxX = frame.env->GetDoubleField(box, h.MaxX);
    value.maxY = frame.env->GetDoubleField(box, h.MaxY);
    value.maxZ = frame.env->GetDoubleField(box, h.MaxZ);
    if (!Good(frame.env)) return false;
    out = value;
    return true;
}

bool EntityName(void* object, std::string& out, bool formatted) {
    Frame frame;
    auto entity = static_cast<jobject>(object);
    if (!frame || !entity) return false;
    jstring value = nullptr;
    if (formatted) {
        jobject chat = frame.env->CallObjectMethod(entity, h.DisplayName);
        if (!Good(frame.env) || !chat) return false;
        value = static_cast<jstring>(frame.env->CallObjectMethod(chat, h.FormattedText));
    } else value = static_cast<jstring>(frame.env->CallObjectMethod(entity, h.GetName));
    if (!Good(frame.env) || !value) return false;
    const char* text = frame.env->GetStringUTFChars(value, nullptr);
    if (!Good(frame.env) || !text) return false;
    try { out = text; }
    catch (...) { frame.env->ReleaseStringUTFChars(value, text); throw; }
    frame.env->ReleaseStringUTFChars(value, text);
    return true;
}

bool EntitySameTeam(void* object, void* other, bool& out) {
    Frame frame;
    auto a = static_cast<jobject>(object), b = static_cast<jobject>(other);
    if (!frame || !a || !b) return false;
    if (!frame.env->IsInstanceOf(a, h.Living) || !frame.env->IsInstanceOf(b, h.Living)) {
        out = false;
        return true;
    }
    const bool value = frame.env->CallBooleanMethod(a, h.SameTeam, b);
    if (!Good(frame.env)) return false;
    out = value;
    return true;
}

bool EntitySameVehicle(void* object, void* other) {
    Frame frame;
    if (!frame || !object || !other) return false;
    auto root = [&](void* raw) -> jobject {
        jobject current = frame.env->NewLocalRef(static_cast<jobject>(raw));
        for (int i = 0; current && i < 256; ++i) {
            jobject next = frame.env->GetObjectField(current, h.Riding);
            if (!Good(frame.env)) return nullptr;
            if (!next) return current;
            if (frame.env->IsSameObject(next, current)) return nullptr;
            frame.env->DeleteLocalRef(current);
            current = next;
        }
        return nullptr;
    };
    jobject a = root(object), b = root(other);
    return a && b && frame.env->IsSameObject(a, b);
}

bool EntitySetHealth(void* object, float value) {
    Frame frame;
    auto entity = static_cast<jobject>(object);
    if (!frame || !entity || !std::isfinite(value) || !frame.env->IsInstanceOf(entity, h.Living)) return false;
    frame.env->CallVoidMethod(entity, h.SetHealth, value);
    return Good(frame.env);
}

java::EntityHandle PlayerEntity() {
    Frame frame;
    return java::AdoptEntity(frame.player());
}

java::EntityHandle LevelEntity() {
    Frame frame;
    if (!frame) return {};
    jobject level = frame.env->GetObjectField(frame.mc, h.Level);
    return Good(frame.env) ? java::AdoptEntity(level) : java::EntityHandle{};
}

java::EntityHandle EntityById(int id) {
    Frame frame;
    if (!frame) return {};
    jobject level = frame.env->GetObjectField(frame.mc, h.Level);
    if (!Good(frame.env) || !level) return {};
    jobject entity = frame.env->CallObjectMethod(level, h.EntityById, static_cast<jint>(id));
    return Good(frame.env) ? java::AdoptEntity(entity) : java::EntityHandle{};
}

namespace {
bool ReadEntities(Frame& frame, jobject list, std::vector<java::EntityHandle>& out) {
    out.clear();
    if (!list) return false;
    const jint count = frame.env->CallIntMethod(list, h.ListSize);
    if (!Good(frame.env) || count < 0) return false;
    for (jint i = 0; i < count; ++i) {
        jobject entity = frame.env->CallObjectMethod(list, h.ListGet, i);
        if (!Good(frame.env)) { out.clear(); return false; }
        if (!entity) continue;
        auto handle = java::AdoptEntity(entity);
        frame.env->DeleteLocalRef(entity);
        if (!handle) { out.clear(); return false; }
        out.push_back(std::move(handle));
    }
    return true;
}
}

bool WorldLoadedEntities(std::vector<java::EntityHandle>& out) {
    out.clear();
    Frame frame;
    if (!frame) return false;
    jobject level = frame.env->GetObjectField(frame.mc, h.Level);
    if (!Good(frame.env) || !level) return false;
    jobject entities = frame.env->GetObjectField(level, h.LoadedEntities);
    return Good(frame.env) && ReadEntities(frame, entities, out);
}

bool WorldPlayers(std::vector<java::EntityHandle>& out) {
    out.clear();
    Frame frame;
    if (!frame) return false;
    jobject level = frame.env->GetObjectField(frame.mc, h.Level);
    if (!Good(frame.env) || !level) return false;
    jobject players = frame.env->GetObjectField(level, h.Players);
    return Good(frame.env) && ReadEntities(frame, players, out);
}

bool WorldEntities(const java::EntityHandle& source, const java::Box& area,
                   std::vector<java::EntityHandle>& out) {
    out.clear();
    Frame frame;
    if (!frame) return false;
    jobject level = frame.env->GetObjectField(frame.mc, h.Level);
    if (!Good(frame.env) || !level) return false;
    jobject box = frame.env->NewObject(h.Box, h.BoxCtor, area.minX, area.minY, area.minZ,
                                      area.maxX, area.maxY, area.maxZ);
    if (!Good(frame.env) || !box) return false;
    jobject entities = frame.env->CallObjectMethod(level, h.GetEntities, static_cast<jobject>(source.raw()), box);
    return Good(frame.env) && ReadEntities(frame, entities, out);
}

bool AttackTargetEntity(java::EntityHandle& out) {
    out.Reset();
    Frame frame;
    if (!frame) return false;
    jobject hit = frame.env->GetObjectField(frame.mc, h.HitResult);
    if (!Good(frame.env) || !hit) return false;
    jobject type = frame.env->GetObjectField(hit, h.HitKind);
    jobject expected = frame.env->GetStaticObjectField(h.HitType, h.EntityType);
    if (!Good(frame.env) || !type || !expected || !frame.env->IsSameObject(type, expected)) return false;
    jobject entity = frame.env->GetObjectField(hit, h.HitEntity);
    if (!Good(frame.env) || !entity) return false;
    out = java::AdoptEntity(entity);
    return out.Valid();
}

bool AttackEntity(const java::EntityHandle& target) {
    Frame frame;
    jobject player = frame.player();
    if (!player || !target) return false;
    jobject controller = frame.env->GetObjectField(frame.mc, h.GameController);
    if (!Good(frame.env) || !controller) return false;
    frame.env->CallVoidMethod(controller, h.Attack, player, static_cast<jobject>(target.raw()));
    return Good(frame.env);
}

bool SwingHand() {
    Frame frame;
    jobject player = frame.player();
    if (!player) return false;
    frame.env->CallVoidMethod(player, h.Swing);
    return Good(frame.env);
}

bool RightClickDelay(int& out) {
    Frame frame;
    if (!frame) return false;
    const int value = frame.env->GetIntField(frame.mc, h.PlaceDelay);
    if (!Good(frame.env)) return false;
    out = value;
    return true;
}

bool SetRightClickDelay(int value) {
    Frame frame;
    if (!frame) return false;
    frame.env->SetIntField(frame.mc, h.PlaceDelay, value);
    return Good(frame.env);
}

bool HoldingBlockItem() {
    Frame frame;
    jobject player = frame.player();
    if (!player) return false;
    jobject stack = frame.env->CallObjectMethod(player, h.HeldItem);
    if (!Good(frame.env) || !stack) return false;
    jobject item = frame.env->CallObjectMethod(stack, h.GetItem);
    return Good(frame.env) && item && frame.env->IsInstanceOf(item, h.BlockItem);
}

bool KeyDown(InputKey key) {
    Frame frame;
    const jfieldID field = key == InputKey::Use ? h.UseKey : key == InputKey::Jump ? h.JumpKey : h.SneakKey;
    jobject binding = frame.key(field);
    if (!binding) return false;
    const bool down = frame.env->CallBooleanMethod(binding, h.KeyDown);
    return Good(frame.env) && down;
}

bool SetKeyShiftDown(bool down) { return SetKey(h.SneakKey, down); }

bool PlayerDeltaMovement(rotation::Vec3& out) {
    Frame frame;
    jobject player = frame.player();
    if (!player) return false;
    rotation::Vec3 value;
    value.x = frame.env->GetDoubleField(player, h.MotionX);
    value.y = frame.env->GetDoubleField(player, h.MotionY);
    value.z = frame.env->GetDoubleField(player, h.MotionZ);
    if (!Good(frame.env)) return false;
    out = value;
    return true;
}

bool SetPlayerDeltaMovement(double x, double y, double z) {
    Frame frame;
    jobject player = frame.player();
    if (!player || !std::isfinite(x) || !std::isfinite(y) || !std::isfinite(z)) return false;
    frame.env->SetDoubleField(player, h.MotionX, x);
    frame.env->SetDoubleField(player, h.MotionY, y);
    frame.env->SetDoubleField(player, h.MotionZ, z);
    return Good(frame.env);
}

bool SetPlayerSprinting(bool value) {
    Frame frame;
    jobject player = frame.player();
    if (!player) return false;
    frame.env->CallVoidMethod(player, h.SetSprinting, value ? JNI_TRUE : JNI_FALSE);
    return Good(frame.env);
}

bool PlayerSprinting(bool& out) {
    Frame frame;
    jobject player = frame.player();
    if (!player) return false;
    const bool value = frame.env->CallBooleanMethod(player, h.IsSprinting);
    if (!Good(frame.env)) return false;
    out = value;
    return true;
}

bool PlayerSpeed(float& out) {
    Frame frame;
    jobject player = frame.player();
    if (!player) return false;
    const float value = frame.env->CallFloatMethod(player, h.MoveSpeed);
    if (!Good(frame.env)) return false;
    out = value;
    return true;
}

namespace {
jobject Escape(Frame& frame, jobject value) {
    if (!Good(frame.env)) return nullptr;
    jobject result = frame.env->PopLocalFrame(value);
    frame.env = nullptr;
    return result;
}

jobjectArray InventoryArray(Frame& frame, bool armor) {
    jobject player = frame.player();
    if (!player) return nullptr;
    jobject inventory = frame.env->GetObjectField(player, h.PlayerInventory);
    if (!Good(frame.env) || !inventory) return nullptr;
    auto result = static_cast<jobjectArray>(frame.env->GetObjectField(
        inventory, armor ? h.InventoryArmor : h.InventoryItems));
    return Good(frame.env) ? result : nullptr;
}

bool StringText(JNIEnv* env, jstring value, std::string& out) {
    out.clear();
    if (!Good(env) || !value) return false;
    const char* chars = env->GetStringUTFChars(value, nullptr);
    if (!Good(env) || !chars) return false;
    try { out.assign(chars); }
    catch (...) { env->ReleaseStringUTFChars(value, chars); throw; }
    env->ReleaseStringUTFChars(value, chars);
    return Good(env);
}
}


namespace {
bool SameStatic(JNIEnv* env, jobject object, jclass owner, jfieldID field) {
    jobject target = env->GetStaticObjectField(owner, field);
    if (!Good(env) || !target) return false;
    const bool same = env->IsSameObject(object, target) == JNI_TRUE;
    env->DeleteLocalRef(target);
    return same;
}

java::KnownItem ItemIdentity(JNIEnv* env, jobject item, int damage) {
    struct Identity { jfieldID field; java::KnownItem value; };
    const Identity identities[] = {
        {h.ItemGoldenAxe, java::KnownItem::GoldenAxe},
        {h.ItemSlimeBall, java::KnownItem::SlimeBall},
        {h.ItemStick, java::KnownItem::Stick},
        {h.ItemGoldenApple, java::KnownItem::GoldenApple},
        {h.ItemCompass, java::KnownItem::Compass},
        {h.ItemWaterBucket, java::KnownItem::WaterBucket},
        {h.ItemLavaBucket, java::KnownItem::LavaBucket},
        {h.ItemArrow, java::KnownItem::Arrow},
        {h.ItemSnowball, java::KnownItem::Snowball},
        {h.ItemEgg, java::KnownItem::Egg},
        {h.ItemFishingRod, java::KnownItem::FishingRod},
        {h.ItemWoodenAxe, java::KnownItem::WoodenAxe},
        {h.ItemStoneAxe, java::KnownItem::StoneAxe},
        {h.ItemIronAxe, java::KnownItem::IronAxe},
        {h.ItemDiamondAxe, java::KnownItem::DiamondAxe},
        {h.ItemWheatSeeds, java::KnownItem::WheatSeeds},
        {h.ItemMelonSeeds, java::KnownItem::MelonSeeds},
        {h.ItemPumpkinSeeds, java::KnownItem::PumpkinSeeds},
        {h.ItemFlintAndSteel, java::KnownItem::FlintAndSteel},
        {h.ItemBow, java::KnownItem::Bow},
        {h.ItemEnderPearl, java::KnownItem::EnderPearl},
        {h.ItemFireCharge, java::KnownItem::FireCharge},
    };
    for (const auto& identity : identities) {
        if (SameStatic(env, item, h.Items, identity.field)) {
            if (identity.value == java::KnownItem::GoldenApple && damage == 1)
                return java::KnownItem::EnchantedGoldenApple;
            return identity.value;
        }
    }
    return java::KnownItem::Nothing;
}
}

bool ReadItemFacts(jobject stack, java::ItemFacts& out) {
    out = {};
    Frame frame;
    if (!frame) return false;
    if (!stack) return true;
    if (!frame.env->IsInstanceOf(stack, h.Stack)) return false;
    auto* env = frame.env;
    java::ItemFacts facts;
    facts.count = env->GetIntField(stack, h.StackCount);
    if (!Good(env)) return false;
    if (facts.count <= 0) return true;
    jobject item = env->CallObjectMethod(stack, h.GetItem);
    if (!Good(env)) return false;
    if (!item) return true;
    facts.empty = false;
    facts.damageValue = env->CallIntMethod(stack, h.StackDamage);
    facts.maxDamage = env->CallIntMethod(stack, h.StackMaxDamage);
    facts.maxStack = env->CallIntMethod(stack, h.StackMaxCount);
    if (!Good(env)) return false;
    const struct { jclass type; unsigned flag; } kinds[] = {
        {h.BlockItem, java::kItemBlock}, {h.Sword, java::kItemSword}, {h.Axe, java::kItemAxe},
        {h.Pickaxe, java::kItemPickaxe}, {h.Shovel, java::kItemShovel}, {h.Bow, java::kItemBow},
        {h.FishingRod, java::kItemFishingRod}, {h.Armor, java::kItemArmor},
        {h.Seeds, java::kItemNameBlock}, {h.SeedFood, java::kItemNameBlock},
        {h.Book, java::kItemBook}, {h.ExpBottle, java::kItemBottle}, {h.Firework, java::kItemFirework}
    };
    for (const auto& kind : kinds) if (env->IsInstanceOf(item, kind.type)) facts.kinds |= kind.flag;
    facts.item = ItemIdentity(env, item, facts.damageValue);
    if (facts.item == java::KnownItem::Arrow) facts.kinds |= java::kItemArrow;
    if (env->IsInstanceOf(item, h.SkullItem)) {
        facts.kinds |= java::kItemBlock | java::kItemSkullBlock;
        if (facts.damageValue == 3) facts.kinds |= java::kItemHead;
    }
    jobject block = nullptr;
    if (env->IsInstanceOf(item, h.BlockItem)) {
        block = (IsLunar1710() ? env->GetObjectField(item, h.ItemBlock17) : env->CallObjectMethod(item, h.ItemBlock));
        if (!Good(env)) return false;
    }
    if (block) {
        const struct { jfieldID field; java::KnownItem value; } identities[] = {
        {h.Block_enchanting_table, java::KnownItem::EnchantingTableBlock},
        {h.Block_web, java::KnownItem::CobwebBlock},
        {h.Block_stone, java::KnownItem::StoneBlock},
        {h.Block_log, java::KnownItem::OakLogBlock},
        {h.Block_dirt, java::KnownItem::DirtBlock},
        };
        for (const auto& identity : identities) {
            if (SameStatic(env, block, h.Blocks, identity.field)) { facts.item = identity.value; break; }
        }
        const jfieldID blacklist[] = {h.Block_air, h.Block_water, h.Block_lava, h.Block_enchanting_table, h.Block_glass_pane, h.Block_iron_bars, h.Block_snow_layer, h.Block_coal_ore, h.Block_diamond_ore, h.Block_emerald_ore, h.Block_chest, h.Block_trapped_chest, h.Block_torch, h.Block_anvil, h.Block_noteblock, h.Block_jukebox, h.Block_tnt, h.Block_gold_ore, h.Block_iron_ore, h.Block_lapis_ore, h.Block_stone_pressure_plate, h.Block_light_weighted_pressure_plate, h.Block_heavy_weighted_pressure_plate, h.Block_stone_button, h.Block_lever, h.Block_tallgrass, h.Block_tripwire, h.Block_tripwire_hook, h.Block_rail, h.Block_red_mushroom, h.Block_brown_mushroom, h.Block_vine, h.Block_double_plant, h.Block_ladder, h.Block_furnace, h.Block_sand, h.Block_cactus, h.Block_dispenser, h.Block_dropper, h.Block_crafting_table, h.Block_web, h.Block_pumpkin, h.Block_cobblestone_wall, h.Block_oak_fence, h.Block_redstone_torch, h.Block_flower_pot, h.Block_flowing_water, h.Block_flowing_lava, h.Block_lit_furnace, h.Block_unlit_redstone_torch};
        for (jfieldID field : blacklist) {
            if (SameStatic(env, block, h.Blocks, field)) { facts.blacklistedBlock = true; break; }
        }
        const struct { jclass type; unsigned flag; } blockKinds[] = {
            {h.Flower, java::kBlockFlower}, {h.Bush, java::kBlockBush},
            {h.Mushroom, java::kBlockFungus}, {h.Crop, java::kBlockCrop}, {h.Slab, java::kBlockSlab}
        };
        for (const auto& kind : blockKinds)
            if (env->IsInstanceOf(block, kind.type)) facts.blockKinds |= kind.flag;
    }
    const bool enchantedKind = (facts.kinds & (java::kItemSword | java::kItemAxe | java::kItemPickaxe |
        java::kItemShovel | java::kItemBow | java::kItemArmor)) ||
        facts.item == java::KnownItem::Stick || facts.item == java::KnownItem::SlimeBall;
    if (enchantedKind) {
        const struct { jfieldID field; int java::ItemFacts::* value; } enchants[] = {
            {h.Enchant_sharpness, &java::ItemFacts::sharpness},
            {h.Enchant_knockback, &java::ItemFacts::knockback},
            {h.Enchant_punch, &java::ItemFacts::punch},
            {h.Enchant_power, &java::ItemFacts::power},
            {h.Enchant_infinity, &java::ItemFacts::infinity},
            {h.Enchant_flame, &java::ItemFacts::flame},
            {h.Enchant_efficiency, &java::ItemFacts::efficiency},
            {h.Enchant_protection, &java::ItemFacts::protection},
        };
        for (const auto& enchant : enchants) {
            jobject type = env->GetStaticObjectField(h.Enchantment, enchant.field);
            if (!Good(env) || !type) return false;
            const int id = env->GetIntField(type, h.EnchantId);
            if (!Good(env)) return false;
            facts.*enchant.value = env->CallStaticIntMethod(h.EnchantHelper, h.EnchantmentLevel, static_cast<jint>(id), stack);
            env->DeleteLocalRef(type);
            if (!Good(env)) return false;
        }
    }
    if (facts.sharpness > 0) {
        jobject enchant = env->GetStaticObjectField(h.Enchantment, h.Enchant_sharpness);
        jobject creature = env->GetStaticObjectField(h.CreatureAttribute, h.CreatureUndefined);
        if (!Good(env) || !enchant || !creature) return false;
        facts.sharpnessBonus = env->CallFloatMethod(enchant, h.SharpnessDamage, static_cast<jint>(facts.sharpness), creature);
        if (!Good(env)) return false;
    }
    if (facts.kinds & java::kItemSword) {
        facts.swordDamage = env->GetFloatField(item, h.SwordAttackDamage);
        if (!Good(env)) return false;
    }
    if (facts.kinds & (java::kItemPickaxe | java::kItemAxe | java::kItemShovel)) {
        const jfieldID field = (facts.kinds & java::kItemPickaxe) ? h.Block_stone :
            ((facts.kinds & java::kItemAxe) ? h.Block_log : h.Block_dirt);
        jobject target = env->GetStaticObjectField(h.Blocks, field);
        if (!Good(env) || !target) return false;
        facts.destroySpeed = env->CallFloatMethod(stack, h.StackSpeed, target);
        if (!Good(env)) return false;
    }
    if (facts.kinds & java::kItemArmor) {
        facts.armorSlot = env->GetIntField(item, h.ArmorSlot);
        jobject material = env->CallObjectMethod(item, h.GetArmorMaterial);
        if (!Good(env) || !material) return false;
        const struct { jfieldID field; java::KnownMaterial value; } materials[] = {
            {h.Material_LEATHER, java::KnownMaterial::Leather},
            {h.Material_CHAIN, java::KnownMaterial::Chain},
            {h.Material_IRON, java::KnownMaterial::Iron},
            {h.Material_GOLD, java::KnownMaterial::Gold},
            {h.Material_DIAMOND, java::KnownMaterial::Diamond},
        };
        for (const auto& entry : materials)
            if (SameStatic(env, material, h.ArmorMaterial, entry.field)) { facts.material = entry.value; break; }
    }
    if (!Good(env)) return false;
    out = facts;
    return true;
}

bool ItemDisplayName(jobject stack, std::string& out) {
    out.clear();
    Frame frame;
    if (!frame || !stack || !frame.env->IsInstanceOf(stack, h.Stack)) return false;
    auto value = static_cast<jstring>(frame.env->CallObjectMethod(stack, h.StackName));
    return StringText(frame.env, value, out);
}

bool InventoryHasItem(java::KnownItem wanted) {
    if (wanted == java::KnownItem::Nothing) return false;
    Frame frame;
    if (!frame) return false;
    for (bool armor : {false, true}) {
        jobjectArray items = InventoryArray(frame, armor);
        if (!items) return false;
        const int count = frame.env->GetArrayLength(items);
        for (int i = 0; i < count; ++i) {
            jobject stack = frame.env->GetObjectArrayElement(items, i);
            if (!Good(frame.env)) return false;
            if (!stack) continue;
            const int size = frame.env->GetIntField(stack, h.StackCount);
            const int damage = frame.env->CallIntMethod(stack, h.StackDamage);
            jobject item = frame.env->CallObjectMethod(stack, h.GetItem);
            if (!Good(frame.env)) return false;
            const bool found = size > 0 && item && ItemIdentity(frame.env, item, damage) == wanted;
            if (item) frame.env->DeleteLocalRef(item);
            frame.env->DeleteLocalRef(stack);
            if (found) return true;
        }
        frame.env->DeleteLocalRef(items);
    }
    return false;
}

bool InventoryStack(int index, bool armor, jobject& out) {
    out = nullptr;
    Frame frame;
    jobjectArray items = InventoryArray(frame, armor);
    if (!items) return false;
    const int count = frame.env->GetArrayLength(items);
    if (!Good(frame.env) || index < 0 || index >= count) return false;
    jobject stack = frame.env->GetObjectArrayElement(items, index);
    if (!Good(frame.env)) return false;
    out = Escape(frame, stack);
    return true;
}

int InventoryCount(bool armor) {
    Frame frame;
    jobjectArray items = InventoryArray(frame, armor);
    if (!items) return 0;
    const int count = frame.env->GetArrayLength(items);
    return Good(frame.env) ? count : 0;
}

jobject CurrentScreen() {
    Frame frame;
    if (!frame) return nullptr;
    return Escape(frame, frame.env->GetObjectField(frame.mc, h.Screen));
}

bool ScreenIsContainerScreen(jobject screen) {
    Frame frame;
    return frame && screen && frame.env->IsInstanceOf(screen, h.ContainerScreen) == JNI_TRUE;
}

bool ScreenIsInventoryScreen(jobject screen) {
    Frame frame;
    return frame && screen && frame.env->IsInstanceOf(screen, h.InventoryScreen) == JNI_TRUE;
}

bool ScreenMenu(jobject screen, jobject& out) {
    out = nullptr;
    Frame frame;
    if (!frame || !screen || !frame.env->IsInstanceOf(screen, h.ContainerScreen)) return false;
    out = Escape(frame, frame.env->GetObjectField(screen, h.ScreenMenu));
    return out != nullptr;
}

bool MenuContainerId(jobject menu, int& out) {
    out = -1;
    Frame frame;
    if (!frame || !menu || !frame.env->IsInstanceOf(menu, h.Container)) return false;
    const int id = frame.env->GetIntField(menu, h.MenuId);
    if (!Good(frame.env)) return false;
    out = id;
    return true;
}

bool PlayerMenu(jobject& out) {
    out = nullptr;
    Frame frame;
    jobject player = frame.player();
    if (!player) return false;
    out = Escape(frame, frame.env->GetObjectField(player, h.PlayerMenu));
    return out != nullptr;
}

bool MenuIsPlayerInventory(jobject menu) {
    Frame frame;
    jobject player = frame.player();
    if (!player || !menu) return false;
    jobject own = frame.env->GetObjectField(player, h.PlayerMenu);
    return Good(frame.env) && own && frame.env->IsSameObject(menu, own) == JNI_TRUE;
}

bool MenuIsChest(jobject menu) {
    Frame frame;
    return frame && menu && frame.env->IsInstanceOf(menu, h.ChestContainer) == JNI_TRUE;
}

bool MenuRowCount(jobject menu, int& out) {
    out = 0;
    Frame frame;
    if (!frame || !menu || !frame.env->IsInstanceOf(menu, h.ChestContainer)) return false;
    const int rows = frame.env->GetIntField(menu, h.ChestRows);
    if (!Good(frame.env)) return false;
    out = rows;
    return true;
}

bool MenuSlotStack(jobject menu, int slot, jobject& out) {
    out = nullptr;
    Frame frame;
    if (!frame || !menu || !frame.env->IsInstanceOf(menu, h.Container) || slot < 0) return false;
    jobject slots = frame.env->GetObjectField(menu, h.MenuSlots);
    if (!Good(frame.env) || !slots) return false;
    const int count = frame.env->CallIntMethod(slots, h.ListSize);
    if (!Good(frame.env) || slot >= count) return false;
    jobject entry = frame.env->CallObjectMethod(menu, h.MenuGetSlot, static_cast<jint>(slot));
    if (!Good(frame.env) || !entry) return false;
    jobject stack = frame.env->CallObjectMethod(entry, h.SlotStack);
    if (!Good(frame.env)) return false;
    out = Escape(frame, stack);
    return true;
}

bool TranslatedText(const char* key, std::string& out) {
    out.clear();
    Frame frame;
    if (!frame || !key) return false;
    jstring name = frame.env->NewStringUTF(key);
    if (!Good(frame.env) || !name) return false;
    jobjectArray args = frame.env->NewObjectArray(0, h.Object, nullptr);
    if (!Good(frame.env) || !args) return false;
    auto value = static_cast<jstring>(frame.env->CallStaticObjectMethod(h.I18n, h.Translate, name, args));
    return StringText(frame.env, value, out);
}

bool ScreenTitle(jobject screen, std::string& out) {
    out.clear();
    Frame frame;
    if (!frame || !screen || !frame.env->IsInstanceOf(screen, h.ContainerScreen)) return false;
    jobject menu = frame.env->GetObjectField(screen, h.ScreenMenu);
    if (!Good(frame.env) || !menu) return false;
    if (!frame.env->IsInstanceOf(menu, h.ChestContainer)) {
        return frame.env->IsInstanceOf(screen, h.InventoryScreen) && TranslatedText("container.crafting", out);
    }
    jobject inventory = frame.env->GetObjectField(menu, h.ChestInventory);
    if (!Good(frame.env) || !inventory) return false;
    jobject title = frame.env->CallObjectMethod(inventory, h.InventoryTitle);
    if (IsLunar1710()) return Good(frame.env) && StringText(frame.env, static_cast<jstring>(title), out);
    if (!Good(frame.env) || !title) return false;
    auto value = static_cast<jstring>(frame.env->CallObjectMethod(title, h.UnformattedText));
    return StringText(frame.env, value, out);
}

bool ClickMenuSlot(int containerId, int slot, int button, java::ClickKind click) {
    int mode = 0;
    switch (click) {
    case java::ClickKind::Pickup: mode = 0; break;
    case java::ClickKind::QuickMove: mode = 1; break;
    case java::ClickKind::Swap: mode = 2; break;
    case java::ClickKind::Throw: mode = 4; break;
    default: return false;
    }
    Frame frame;
    jobject player = frame.player();
    if (!player) return false;
    jobject menu = frame.env->GetObjectField(player, h.OpenMenu);
    if (!Good(frame.env) || !menu) return false;
    const int currentId = frame.env->GetIntField(menu, h.MenuId);
    if (!Good(frame.env) || currentId != containerId) return false;
    jobject controller = frame.env->GetObjectField(frame.mc, h.GameController);
    if (!Good(frame.env) || !controller) return false;
    frame.env->CallObjectMethod(controller, h.WindowClick, static_cast<jint>(containerId),
        static_cast<jint>(slot), static_cast<jint>(button), static_cast<jint>(mode), player);
    return Good(frame.env);
}

bool CloseContainer() {
    Frame frame;
    jobject player = frame.player();
    if (!player) return false;
    frame.env->CallVoidMethod(player, h.CloseScreen);
    return Good(frame.env);
}

bool SendContainerClose(int containerId) {
    Frame frame;
    if (!frame) return false;
    jobject packet = frame.env->NewObject(h.ClosePacket, h.ClosePacketCtor, static_cast<jint>(containerId));
    return Good(frame.env) && packet && java::SendPacket(packet);
}

namespace {
jobject InventoryObject(Frame& frame) {
    jobject player = frame.player();
    if (!player) return nullptr;
    jobject value = frame.env->GetObjectField(player, h.PlayerInventory);
    return Good(frame.env) ? value : nullptr;
}

jobject FacingObject(Frame& frame, int facing) {
    if (!frame || facing < 0 || facing > 5) return nullptr;
    const jfieldID fields[] = {h.FacingDown, h.FacingUp, h.FacingNorth, h.FacingSouth, h.FacingWest, h.FacingEast};
    jobject value = frame.env->GetStaticObjectField(h.Facing, fields[facing]);
    return Good(frame.env) ? value : nullptr;
}

struct BlockRead {
    java::BlockPos coordinates{};
    jobject world = nullptr;
    jobject position = nullptr;
    jobject state = nullptr;
    jobject block = nullptr;
    bool read(Frame& frame, const java::BlockPos& pos) {
        if (!frame) return false;
        world = frame.env->GetObjectField(frame.mc, h.Level);
        if (!Good(frame.env) || !world) return false;
        coordinates = pos;
        if (IsLunar1710()) {
            block = frame.env->CallObjectMethod(world, h.GetBlockState, pos.x, pos.y, pos.z);
            return Good(frame.env) && block;
        }
        position = frame.env->NewObject(h.BlockPos, h.BlockPosCtor,
            static_cast<jint>(pos.x), static_cast<jint>(pos.y), static_cast<jint>(pos.z));
        if (!Good(frame.env) || !position) return false;
        state = frame.env->CallObjectMethod(world, h.GetBlockState, position);
        if (!Good(frame.env) || !state) return false;
        block = frame.env->CallObjectMethod(state, h.GetBlock);
        return Good(frame.env) && block;
    }
};
}

bool ReadBlockProperty(const java::BlockPos& pos, BlockProperty property) {
    Frame frame;
    BlockRead read;
    if (!read.read(frame, pos)) return false;
    auto* env = frame.env;
    switch (property) {
    case BlockProperty::ScaffoldExcluded:
        if (IsLunar1710()) {
            // 1.7 Block inherits Object.toString(): parsing "Block{...}" would
            // reject every solid origin and leave Scaffold without any target.
            jobject registry = env->GetStaticObjectField(h.Block, h.BlockRegistry);
            if (!Good(env) || !registry) return true;
            auto name = static_cast<jstring>(env->CallObjectMethod(registry, h.RegistryName, read.block));
            std::string id;
            return !Good(env) || !StringText(env, name, id) || java::ScaffoldExcludedRegistryName(id);
        }
        return java::ScaffoldExcludedObject(env, read.block);
    case BlockProperty::CollisionEmpty: {
        jobject box = IsLunar1710() ? env->CallObjectMethod(read.block, h.CollisionBox, read.world, pos.x, pos.y, pos.z)
            : env->CallObjectMethod(read.block, h.CollisionBox, read.world, read.position, read.state);
        return Good(env) && !box;
    }
    case BlockProperty::Replaceable: {
        if (IsLunar1710()) {
            mc263::access::Frame f(env);
            return f.call<bool>(f.call(read.block, lunar1710::names::Material), lunar1710::names::Replaceable) && f.check();
        }
        const bool result = (IsLunar1710() ? env->CallBooleanMethod(read.block, h.Replaceable, read.world, pos.x, pos.y, pos.z)
            : env->CallBooleanMethod(read.block, h.Replaceable, read.world, read.position)) == JNI_TRUE;
        return Good(env) && result;
    }
    case BlockProperty::OpensMenu: {
        if (env->IsInstanceOf(read.block, h.Workbench) || env->IsInstanceOf(read.block, h.EnchantingTable) ||
            env->IsInstanceOf(read.block, h.Anvil) || env->IsInstanceOf(read.block, h.EnderChest)) return true;
        jobject tile = IsLunar1710() ? env->CallObjectMethod(read.world, h.GetTileEntity, pos.x, pos.y, pos.z)
            : env->CallObjectMethod(read.world, h.GetTileEntity, read.position);
        return Good(env) && tile && env->IsInstanceOf(tile, h.InventoryInterface);
    }
    }
    return false;
}

bool BlockRegistryName(const java::BlockPos& pos, std::string& out) {
    Frame f;
    BlockRead read;
    if (!read.read(f, pos)) return false;
    jobject registry = f.env->GetStaticObjectField(h.Block, h.BlockRegistry);
    if (!Good(f.env) || !registry) return false;
    jobject key = f.env->CallObjectMethod(registry, h.RegistryName, read.block);
    if (!Good(f.env) || !key) return false;
    auto name = static_cast<jstring>(f.env->CallObjectMethod(key, h.ResourceString));
    return Good(f.env) && StringText(f.env, name, out);
}

bool SelectedSlot(int& out) {
    out = -1;
    Frame frame;
    jobject inventory = InventoryObject(frame);
    if (!inventory) return false;
    const int value = frame.env->GetIntField(inventory, h.SelectedSlot);
    if (!Good(frame.env)) return false;
    out = value;
    return true;
}

bool SetSelectedSlot(int slot) {
    if (slot < 0 || slot >= 9) return false;
    Frame frame;
    jobject inventory = InventoryObject(frame);
    if (!inventory) return false;
    frame.env->SetIntField(inventory, h.SelectedSlot, static_cast<jint>(slot));
    return Good(frame.env);
}

bool HotbarSlotPlaceable(int slot) {
    if (slot < 0 || slot >= 9) return false;
    Frame frame;
    if (!frame) return false;
    jobject stack = nullptr;
    if (!InventoryStack(slot, false, stack) || !stack) return false;
    java::ItemFacts facts;
    if (!lunar::ReadItemFacts(stack, facts) || facts.empty || !(facts.kinds & java::kItemBlock) ||
        (facts.kinds & java::kItemSkullBlock) || facts.blacklistedBlock || facts.blockKinds) return false;
    std::string name;
    if (!ItemDisplayName(stack, name)) return false;
    return name.find("Click") == std::string::npos && name.find("\xe7\x82\xb9\xe5\x87\xbb") == std::string::npos;
}

int HotbarSlotCount(int slot) {
    if (slot < 0 || slot >= 9) return 0;
    Frame frame;
    if (!frame) return 0;
    jobject stack = nullptr;
    if (!InventoryStack(slot, false, stack) || !stack) return 0;
    const int count = frame.env->GetIntField(stack, h.StackCount);
    return Good(frame.env) ? count : 0;
}

int FacingStep(int facing, int axis) {
    Frame frame;
    jobject value = FacingObject(frame, facing);
    if (!value || axis < 0 || axis > 2) return 0;
    const jmethodID getters[] = {h.FacingX, h.FacingY, h.FacingZ};
    const int result = frame.env->CallIntMethod(value, getters[axis]);
    return Good(frame.env) ? result : 0;
}

int FacingOpposite(int facing) {
    if (IsLunar1710()) return facing >= 0 && facing < 6 ? (facing ^ 1) : -1;
    Frame frame;
    jobject value = FacingObject(frame, facing);
    if (!value) return facing;
    jobject opposite = frame.env->CallObjectMethod(value, h.FacingOpposite);
    if (!Good(frame.env) || !opposite) return facing;
    const int result = frame.env->CallIntMethod(opposite, h.Ordinal);
    return Good(frame.env) ? result : facing;
}

bool EntityMotion(void* entity, rotation::Vec3& out) {
    Frame f;
    if (!f || !entity) return false;
    auto object = static_cast<jobject>(entity);
    out = {f.env->GetDoubleField(object, h.MotionX), f.env->GetDoubleField(object, h.MotionY),
           f.env->GetDoubleField(object, h.MotionZ)};
    return Good(f.env);
}

bool ThrowableMob(void* entity) {
    Frame f;
    return f && entity && f.env->IsInstanceOf(static_cast<jobject>(entity), h.Monster);
}

bool ThrowableLook(rotation::Vec3& out) {
    Frame f;
    auto player = f.player();
    if (!player) return false;
    auto vector = f.env->CallObjectMethod(player, h.ThrowableLook);
    if (!Good(f.env) || !vector) return false;
    out = {f.env->GetDoubleField(vector, h.VecX), f.env->GetDoubleField(vector, h.VecY),
           f.env->GetDoubleField(vector, h.VecZ)};
    return Good(f.env);
}

bool ThrowableCanSee(void* entity) {
    Frame f;
    auto player = f.player();
    if (!player || !entity) return false;
    const bool visible = f.env->CallBooleanMethod(player, h.ThrowableCanSee, static_cast<jobject>(entity));
    return Good(f.env) && visible;
}

bool ThrowableSameTeam(void* entity) {
    Frame f;
    auto player = f.player();
    if (!player || !entity) return false;
    auto other = static_cast<jobject>(entity);
    auto a = f.env->CallObjectMethod(player, h.ThrowableTeam);
    if (!Good(f.env) || !a) return false;
    auto b = f.env->CallObjectMethod(other, h.ThrowableTeam);
    if (!Good(f.env) || !b) return false;
    // PlayerUtil.sameTeam compares Java charAt(1), not team identity.
    auto secondChar = [&](jobject object, jchar& out) {
        auto chat = f.env->CallObjectMethod(object, h.DisplayName);
        if (!Good(f.env) || !chat) return false;
        auto text = static_cast<jstring>(f.env->CallObjectMethod(chat, h.FormattedText));
        if (!Good(f.env) || !text || f.env->GetStringLength(text) < 2) return false;
        f.env->GetStringRegion(text, 1, 1, &out);
        return Good(f.env);
    };
    jchar ac = 0, bc = 0;
    return secondChar(player, ac) && secondChar(other, bc) && ac == bc;
}

bool ThrowHeldItem() {
    Frame f;
    auto player = f.player();
    if (!player) return false;
    auto world = f.env->GetObjectField(f.mc, h.Level);
    auto controller = f.env->GetObjectField(f.mc, h.GameController);
    if (!Good(f.env) || !world || !controller) return false;
    auto stack = f.env->CallObjectMethod(player, h.HeldItem);
    if (!Good(f.env) || !stack) return false;
    // Vanilla projectile use synchronizes the slot and sends one C08. It does
    // not need an attack swing or a second C08 with an already-consumed stack.
    f.env->CallBooleanMethod(controller, h.StartUseItem, player, world, stack);
    return Good(f.env);
}

bool UseItemOnBlock(const java::BlockPos& pos, int facing, const rotation::Vec3& hitVector) {
    Frame frame;
    jobject direction = FacingObject(frame, facing);
    jobject player = frame.player();
    if (!direction || !player) return false;
    jobject world = frame.env->GetObjectField(frame.mc, h.Level);
    jobject controller = frame.env->GetObjectField(frame.mc, h.GameController);
    if (!Good(frame.env) || !world || !controller) return false;
    jobject stack = frame.env->CallObjectMethod(player, h.HeldItem);
    // 1.8 uses null for an empty hand; the controller can still open a chest.
    if (!Good(frame.env)) return false;
    if (IsLunar1710()) {
        auto vector = frame.env->NewObject(h.Vec, h.VecCtor, hitVector.x, hitVector.y, hitVector.z);
        if (!Good(frame.env) || !vector) return false;
        const bool placed = frame.env->CallBooleanMethod(controller, h.UseBlock, player, world, stack,
            pos.x, pos.y, pos.z, facing, vector) == JNI_TRUE;
        return Good(frame.env) && placed;
    }
    jobject position = frame.env->NewObject(h.BlockPos, h.BlockPosCtor,
        static_cast<jint>(pos.x), static_cast<jint>(pos.y), static_cast<jint>(pos.z));
    if (!Good(frame.env) || !position) return false;
    jobject vector = frame.env->NewObject(h.Vec, h.VecCtor,
        static_cast<jdouble>(hitVector.x), static_cast<jdouble>(hitVector.y), static_cast<jdouble>(hitVector.z));
    if (!Good(frame.env) || !vector) return false;
    const bool result = frame.env->CallBooleanMethod(controller, h.UseBlock, player, world, stack, position, direction, vector) == JNI_TRUE;
    return Good(frame.env) && result;
}

bool SendMoveRotation(float yaw, float pitch, bool onGround) {
    Frame frame;
    if (!frame) return false;
    jobject packet = frame.env->NewObject(h.RotationPacket, h.RotationPacketCtor,
        static_cast<jfloat>(yaw), static_cast<jfloat>(pitch), onGround ? JNI_TRUE : JNI_FALSE);
    return Good(frame.env) && packet && java::SendPacket(packet);
}

bool SendSwingPacket() {
    Frame frame;
    if (!frame) return false;
    jobject packet = frame.env->NewObject(h.SwingPacket, h.SwingPacketCtor);
    return Good(frame.env) && packet && java::SendPacket(packet);
}

bool GameModeDestroying() {
    Frame frame;
    if (!frame) return false;
    jobject controller = frame.env->GetObjectField(frame.mc, h.GameController);
    if (!Good(frame.env) || !controller) return false;
    const bool result = frame.env->GetBooleanField(controller, h.Destroying) == JNI_TRUE;
    return Good(frame.env) && result;
}

bool HitBlockPos(java::BlockPos& out) {
    out = {};
    Frame frame;
    jobject hit = BlockTarget(frame);
    if (!hit) return false;
    if (IsLunar1710()) return HitCoordinates17(frame.env, hit, out);
    jobject pos = frame.env->CallObjectMethod(hit, h.HitBlockPos);
    if (!Good(frame.env) || !pos) return false;
    java::BlockPos value;
    value.x = frame.env->CallIntMethod(pos, h.BlockX);
    value.y = frame.env->CallIntMethod(pos, h.BlockY);
    value.z = frame.env->CallIntMethod(pos, h.BlockZ);
    if (!Good(frame.env)) return false;
    out = value;
    return true;
}

bool BestToolSlotFor(const java::BlockPos& pos, int& out) {
    out = -1;
    Frame frame;
    BlockRead read;
    if (!read.read(frame, pos)) return false;
    if (frame.env->IsInstanceOf(read.block, h.Air)) return true;
    const bool web = SameStatic(frame.env, read.block, h.Blocks, h.Block_web);
    const bool skipEfficiency = frame.env->IsInstanceOf(read.block, h.Ore) || frame.env->IsInstanceOf(read.block, h.RedstoneOre);
    float best = 1.0f;
    for (int slot = 0; slot < 9; ++slot) {
        jobject stack = nullptr;
        if (!InventoryStack(slot, false, stack)) return false;
        if (!stack) continue;
        java::ItemFacts facts;
        if (!lunar::ReadItemFacts(stack, facts)) return false;
        if (!facts.empty && !inventory::IsGodItem(facts) && (!(facts.kinds & java::kItemSword) || web)) {
            float speed = frame.env->CallFloatMethod(stack, h.StackSpeed, read.block);
            if (!Good(frame.env)) return false;
            if (speed > 1 && !skipEfficiency && facts.efficiency > 0)
                speed += static_cast<float>(facts.efficiency * facts.efficiency + 1);
            if (speed > best) { best = speed; out = slot; }
        }
        frame.env->DeleteLocalRef(stack);
    }
    return true;
}

bool ReadCompassView(float partialTick, java::CompassView& out) {
    out = {};
    auto player = PlayerEntity();
    rotation::Vec3 now, before;
    float yaw = 0, pitch = 0, previousYaw = 0, previousPitch = 0;
    if (!player || !player.PositionPair(now, before) ||
        !PlayerRotation(yaw, pitch) || !PlayerRotation(previousYaw, previousPitch, true)) return false;
    out.x = before.x + (now.x - before.x) * partialTick;
    out.z = before.z + (now.z - before.z) * partialTick;
    out.yaw = previousYaw + (yaw - previousYaw) * partialTick;
    return true;
}

bool WorldSpawn(int& x, int& z) {
    x = z = 0;
    Frame frame;
    if (!frame) return false;
    jobject world = frame.env->GetObjectField(frame.mc, h.Level);
    if (!Good(frame.env) || !world) return false;
    jobject provider = frame.env->GetObjectField(world, h.Provider);
    if (!Good(frame.env) || !provider) return false;
    const bool natural = frame.env->CallBooleanMethod(provider, h.SurfaceWorld) == JNI_TRUE;
    if (!Good(frame.env) || !natural) return false;
    jobject pos = frame.env->CallObjectMethod(world, h.SpawnPoint);
    if (!Good(frame.env) || !pos) return false;
    const int px = IsLunar1710() ? frame.env->GetIntField(pos, h.SpawnX17) : frame.env->CallIntMethod(pos, h.BlockX);
    const int pz = IsLunar1710() ? frame.env->GetIntField(pos, h.SpawnZ17) : frame.env->CallIntMethod(pos, h.BlockZ);
    if (!Good(frame.env)) return false;
    x = px;
    z = pz;
    return true;
}

bool WindowMetrics(int& width, int& height, double& guiScale) {
    width = height = 0;
    guiScale = 1;
    Frame frame;
    if (!frame) return false;
    const int w = frame.env->GetIntField(frame.mc, h.DisplayWidth);
    const int ht = frame.env->GetIntField(frame.mc, h.DisplayHeight);
    jobject resolution = IsLunar1710() ? frame.env->NewObject(h.ScaledResolution, h.ScaledResolutionCtor, frame.mc, w, ht)
        : frame.env->NewObject(h.ScaledResolution, h.ScaledResolutionCtor, frame.mc);
    if (!Good(frame.env) || !resolution) return false;
    const int scale = frame.env->CallIntMethod(resolution, h.ScaleFactor);
    if (!Good(frame.env) || w <= 0 || ht <= 0 || scale <= 0) return false;
    width = w;
    height = ht;
    guiScale = scale;
    return true;
}

unsigned int PlayerSkinTexture(const java::EntityHandle& entity) {
    Frame frame;
    jobject player = static_cast<jobject>(entity.raw());
    if (!frame || !player || !frame.env->IsInstanceOf(player, h.ClientPlayer)) return 0;
    jobject location = frame.env->CallObjectMethod(player, h.SkinLocation);
    if (!Good(frame.env) || !location) return 0;
    jobject manager = frame.env->CallObjectMethod(frame.mc, h.GetTextureManager);
    if (!Good(frame.env) || !manager) return 0;
    jobject texture = frame.env->CallObjectMethod(manager, h.GetTexture, location);
    if (!Good(frame.env) || !texture) return 0;
    const int id = frame.env->CallIntMethod(texture, h.TextureId);
    return Good(frame.env) && id > 0 ? static_cast<unsigned int>(id) : 0;
}

bool ReadHealthScore(jobject packet, std::string& owner, int& score) {
    owner.clear();
    score = 0;
    Frame frame;
    if (!frame || !packet || !frame.env->IsInstanceOf(packet, h.ScorePacket)) return false;
    int ordinal = 0;
    if (IsLunar1710()) ordinal = frame.env->CallIntMethod(packet, h.ScoreAction);
    else {
        jobject action = frame.env->CallObjectMethod(packet, h.ScoreAction);
        if (!Good(frame.env) || !action) return false;
        ordinal = frame.env->CallIntMethod(action, h.Ordinal);
    }
    if (!Good(frame.env) || ordinal != 0) return false;
    auto name = static_cast<jstring>(frame.env->CallObjectMethod(packet, h.ScoreObjective));
    std::string objective;
    if (!StringText(frame.env, name, objective) || (objective != "health" && objective != "belowHealth")) return false;
    auto player = static_cast<jstring>(frame.env->CallObjectMethod(packet, h.ScoreOwner));
    std::string result;
    if (!StringText(frame.env, player, result) || result.empty()) return false;
    const int value = frame.env->CallIntMethod(packet, h.ScoreValue);
    if (!Good(frame.env)) return false;
    owner = std::move(result);
    score = value;
    return true;
}

bool ReadCameraView(float partialTick, java::CameraView& out) {
    out = {};
    Frame frame;
    if (!frame) return false;
    jobject world = frame.env->GetObjectField(frame.mc, h.Level);
    if (!Good(frame.env) || !world) return false;
    jobject entity = (IsLunar1710() ? frame.env->GetObjectField(frame.mc, h.View17) : frame.env->CallObjectMethod(frame.mc, h.ViewEntity));
    if (!Good(frame.env) || !entity) return false;
    rotation::Vec3 now, before;
    if (!EntityRenderPositionPair(entity, now, before)) return false;
    java::CameraView view;
    view.x = before.x + (now.x - before.x) * partialTick;
    view.y = before.y + (now.y - before.y) * partialTick;
    view.z = before.z + (now.z - before.z) * partialTick;
    // ActiveRenderInfo's 1.7 matrix uses the original posY reference.
    if (IsLunar1710()) view.y += frame.env->GetFloatField(entity,h.YOffset17);
    view.yaw = frame.env->GetFloatField(entity, h.Yaw);
    view.pitch = frame.env->GetFloatField(entity, h.Pitch);
    java::FreeLookAngles(view.yaw, view.pitch);
    jobject projection = frame.env->GetStaticObjectField(h.ActiveRenderInfo, h.ProjectionBuffer);
    jobject modelView = frame.env->GetStaticObjectField(h.ActiveRenderInfo, h.ModelViewBuffer);
    if (!Good(frame.env) || !projection || !modelView) return false;
    for (int i = 0; i < 16; ++i) {
        view.projection[i] = frame.env->CallFloatMethod(projection, h.BufferFloat, static_cast<jint>(i));
        view.modelView[i] = frame.env->CallFloatMethod(modelView, h.BufferFloat, static_cast<jint>(i));
        if (!Good(frame.env) || !std::isfinite(view.projection[i]) || !std::isfinite(view.modelView[i])) return false;
    }
    if (view.projection[0] == 0 || view.projection[5] == 0) return false;
    view.fov = 2 * std::atan(1.0 / view.projection[5]) * (180.0 / 3.14159265358979323846);
    view.hasModelView = true;
    double scale = 1;
    if (!WindowMetrics(view.windowWidth, view.windowHeight, scale)) return false;
    out = view;
    return true;
}

namespace {
bool ReadBlockPosition(JNIEnv* env, jobject pos, java::BlockPos& out) {
    if (!pos) return false;
    out.x = env->CallIntMethod(pos, h.BlockX);
    out.y = env->CallIntMethod(pos, h.BlockY);
    out.z = env->CallIntMethod(pos, h.BlockZ);
    return Good(env);
}

bool ChestBounds(Frame& frame, BlockRead& read, java::ChestBox& out) {
    const auto& p = read.coordinates;
    if (IsLunar1710()) frame.env->CallVoidMethod(read.block, h.BlockBounds, read.world, p.x, p.y, p.z);
    else frame.env->CallVoidMethod(read.block, h.BlockBounds, read.world, read.position);
    if (!Good(frame.env)) return false;
    jobject box = IsLunar1710() ? frame.env->CallObjectMethod(read.block, h.SelectedBox, read.world, p.x, p.y, p.z)
        : frame.env->CallObjectMethod(read.block, h.SelectedBox, read.world, read.position);
    if (!Good(frame.env) || !box) return false;
    out.minX = frame.env->GetDoubleField(box, h.MinX);
    out.minY = frame.env->GetDoubleField(box, h.MinY);
    out.minZ = frame.env->GetDoubleField(box, h.MinZ);
    out.maxX = frame.env->GetDoubleField(box, h.MaxX);
    out.maxY = frame.env->GetDoubleField(box, h.MaxY);
    out.maxZ = frame.env->GetDoubleField(box, h.MaxZ);
    frame.env->DeleteLocalRef(box);
    return Good(frame.env);
}
}

bool LoadedChestBoxes(std::vector<java::ChestBox>& out) {
    out.clear();
    Frame frame;
    if (!frame) return false;
    jobject world = frame.env->GetObjectField(frame.mc, h.Level);
    if (!Good(frame.env) || !world) return false;
    jobject tiles = frame.env->GetObjectField(world, h.LoadedTiles);
    if (!Good(frame.env) || !tiles) return false;
    const int count = frame.env->CallIntMethod(tiles, h.ListSize);
    if (!Good(frame.env)) return false;
    for (int i = 0; i < count; ++i) {
        Frame entry;
        if (!entry) return false;
        auto* env = entry.env;
        jobject tile = env->CallObjectMethod(tiles, h.ListGet, static_cast<jint>(i));
        if (!Good(env)) return false;
        if (!tile || !env->IsInstanceOf(tile, h.ChestTile)) continue;
        java::BlockPos pos;
        if (IsLunar1710()) {
            pos = {env->GetIntField(tile, h.TileX17), env->GetIntField(tile, h.TileY17), env->GetIntField(tile, h.TileZ17)};
            if (!Good(env)) return false;
        } else {
            jobject position = env->CallObjectMethod(tile, h.TilePosition);
            if (!Good(env) || !position) continue;
            if (!ReadBlockPosition(env, position, pos)) return false;
        }
        BlockRead read;
        if (!read.read(entry, pos) || !env->IsInstanceOf(read.block, h.ChestBlock)) continue;
        java::ChestBox box;
        if (!ChestBounds(entry, read, box)) return false;
        bool duplicate = false;
        const int offsets[4][2] = {{-1, 0}, {0, -1}, {1, 0}, {0, 1}};
        for (const auto& offset : offsets) {
            Frame neighbourFrame;
            BlockRead neighbour;
            const java::BlockPos neighbourPos{pos.x + offset[0], pos.y, pos.z + offset[1]};
            if (!neighbour.read(neighbourFrame, neighbourPos)) return false;
            if (!env->IsSameObject(read.block, neighbour.block)) continue;
            if (offset[0] < 0 || offset[1] < 0) { duplicate = true; break; }
            java::ChestBox second;
            if (!ChestBounds(neighbourFrame, neighbour, second)) return false;
            box.minX = std::min(box.minX, second.minX);
            box.minY = std::min(box.minY, second.minY);
            box.minZ = std::min(box.minZ, second.minZ);
            box.maxX = std::max(box.maxX, second.maxX);
            box.maxY = std::max(box.maxY, second.maxY);
            box.maxZ = std::max(box.maxZ, second.maxZ);
        }
        if (duplicate) continue;
        box.x = pos.x;
        box.y = pos.y;
        box.z = pos.z;
        out.push_back(box);
    }
    return true;
}

bool ReadChestOpenEvent(jobject packet, int& x, int& y, int& z) {
    x = y = z = 0;
    Frame frame;
    if (!frame || !packet || !frame.env->IsInstanceOf(packet, h.BlockActionPacket)) return false;
    jobject block = frame.env->CallObjectMethod(packet, h.BlockActionType);
    if (!Good(frame.env) || !block || !frame.env->IsInstanceOf(block, h.ChestBlock)) return false;
    const int id = frame.env->CallIntMethod(packet, h.BlockActionId);
    const int parameter = frame.env->CallIntMethod(packet, h.BlockActionParameter);
    if (!Good(frame.env) || id != 1 || parameter != 1) return false;
    if (IsLunar1710()) {
        x = frame.env->CallIntMethod(packet, h.BlockActionX17);
        y = frame.env->CallIntMethod(packet, h.BlockActionY17);
        z = frame.env->CallIntMethod(packet, h.BlockActionZ17);
        return Good(frame.env);
    }
    jobject position = frame.env->CallObjectMethod(packet, h.BlockActionPosition);
    if (!Good(frame.env)) return false;
    java::BlockPos pos;
    if (!ReadBlockPosition(frame.env, position, pos)) return false;
    x = pos.x;
    y = pos.y;
    z = pos.z;
    return true;
}

bool IsPacket(jobject packet, PacketKind kind) {
    Frame frame;
    if (!frame || !packet) return false;
    auto is = [&](jclass type) { return frame.env->IsInstanceOf(packet, type) == JNI_TRUE; };
    bool value = false;
    switch (kind) {
    case PacketKind::UseItem: value = is(h.UsePacket); break;
    case PacketKind::UseItemOnly:
        value = is(h.UsePacket) && frame.env->CallIntMethod(packet, h.UseDirection) == 255; break;
    case PacketKind::VelocityDelay:
        value = is(h.VelocityPacket) || is(h.EntityMovePacket) || is(h.TeleportPacket) ||
            is(h.PingPacket) || is(h.PositionPacket); break;
    case PacketKind::Motion: value = is(h.VelocityPacket); break;
    case PacketKind::PositionOrExplode: value = is(h.PositionPacket) || is(h.ExplodePacket); break;
    case PacketKind::Ping: value = is(h.PingPacket); break;
    case PacketKind::Explode: value = is(h.ExplodePacket); break;
    case PacketKind::Position: value = is(h.PositionPacket); break;
    case PacketKind::Disconnect: value = is(h.DisconnectPacket); break;
    case PacketKind::Respawn: value = is(h.RespawnPacket); break;
    case PacketKind::ContainerClose: value = is(h.ClosePacket); break;
    case PacketKind::MovePlayer: value = is(h.MovePacket); break;
    case PacketKind::UseItemOn:
        value = is(h.UsePacket) && frame.env->CallIntMethod(packet, h.UseDirection) != 255; break;
    case PacketKind::Interact: value = is(h.InteractPacket); break;
    case PacketKind::PlayerAction: value = is(h.ActionPacket); break;
    case PacketKind::EntityTeleport: value = is(h.TeleportPacket); break;
    case PacketKind::Join: value = is(h.JoinPacket); break;
    }
    return Good(frame.env) && value;
}

bool SendPacket(jobject packet) {
    Frame frame;
    if (!frame || !packet) return false;
    jobject listener = frame.env->CallObjectMethod(frame.mc, h.CurrentNetHandler);
    if (!Good(frame.env) || !listener) return false;
    frame.env->CallVoidMethod(listener, h.SendPacket, packet);
    return Good(frame.env);
}

bool SendStatusOnlyPacket(bool onGround) {
    Frame frame;
    if (!frame) return false;
    jobject packet = frame.env->NewObject(h.MovePacket, h.StatusPacketCtor, onGround ? JNI_TRUE : JNI_FALSE);
    return Good(frame.env) && packet && java::SendPacketNoEvent(packet);
}

bool ReplayIncoming(JNIEnv* env, jobject packet, jobject listener) {
    if (!env || !h.ready || !packet || !listener) return false;
    env->CallVoidMethod(packet, h.ProcessPacket, listener);
    return Good(env);
}

bool HandleIncomingPacket(jobject packet) {
    Frame frame;
    if (!frame || !packet) return false;
    jobject listener = frame.env->CallObjectMethod(frame.mc, h.CurrentNetHandler);
    return Good(frame.env) && ReplayIncoming(frame.env, packet, listener);
}

bool ReadMotionPacket(jobject packet, int& id, int& x, int& y, int& z) {
    Frame frame;
    if (!frame || !packet || !frame.env->IsInstanceOf(packet, h.VelocityPacket)) return false;
    const int entityId = frame.env->CallIntMethod(packet, h.VelocityId);
    const int xa = frame.env->CallIntMethod(packet, h.VelocityX);
    const int ya = frame.env->CallIntMethod(packet, h.VelocityY);
    const int za = frame.env->CallIntMethod(packet, h.VelocityZ);
    if (!Good(frame.env)) return false;
    id = entityId; x = xa; y = ya; z = za;
    return true;
}

bool EntityTypes(const java::EntityHandle& entity, unsigned& out) {
    out = 0;
    Frame frame;
    if (!frame || !entity) return false;
    auto obj = static_cast<jobject>(entity.raw());
    if (frame.env->IsInstanceOf(obj, h.Living)) out |= java::kTypeLiving;
    if (frame.env->IsInstanceOf(obj, h.Player)) out |= java::kTypePlayer;
    if (frame.env->IsInstanceOf(obj, h.Villager)) out |= java::kTypeVillager;
    if (frame.env->IsInstanceOf(obj, h.Monster) || frame.env->IsInstanceOf(obj, h.Dragon) ||
        frame.env->IsInstanceOf(obj, h.DragonPart)) out |= java::kTypeMonster;
    if (frame.env->IsInstanceOf(obj, h.Animal) || frame.env->IsInstanceOf(obj, h.Ambient) ||
        frame.env->IsInstanceOf(obj, h.WaterAnimal)) out |= java::kTypeAnimal;
    return Good(frame.env);
}

namespace {
jobject Vector(Frame& frame, const rotation::Vec3& value) {
    jobject result = frame.env->NewObject(h.Vec, h.VecCtor, value.x, value.y, value.z);
    return Good(frame.env) ? result : nullptr;
}

bool ReadVector(Frame& frame, jobject object, rotation::Vec3& out) {
    if (!object) return false;
    rotation::Vec3 value;
    value.x = frame.env->GetDoubleField(object, h.VecX);
    value.y = frame.env->GetDoubleField(object, h.VecY);
    value.z = frame.env->GetDoubleField(object, h.VecZ);
    if (!Good(frame.env)) return false;
    out = value;
    return true;
}
}

bool WorldClip(const rotation::Vec3& from, const rotation::Vec3& to,
               const java::EntityHandle& source, java::BlockHitResultInfo& out, java::ClipMode mode) {
    out = {};
    Frame frame;
    if (!frame || !source) return false;
    jobject world = frame.env->GetObjectField(frame.mc, h.Level);
    if (!Good(frame.env) || !world) return false;
    jobject start = Vector(frame, from), end = Vector(frame, to);
    if (!start || !end) return false;
    jobject result = frame.env->CallObjectMethod(world, h.RayTrace, start, end,
                                                mode==java::ClipMode::WaterSource?JNI_TRUE:JNI_FALSE,
                                                mode==java::ClipMode::Collision?JNI_TRUE:JNI_FALSE, JNI_FALSE);
    if (!Good(frame.env)) return false;
    java::BlockHitResultInfo value;
    value.location = to;
    if (!result) { out = value; return true; }
    jobject kind = frame.env->GetObjectField(result, h.HitKind);
    if (!Good(frame.env) || !kind) return false;
    const jint ordinal = frame.env->CallIntMethod(kind, h.Ordinal);
    if (!Good(frame.env)) return false;
    value.kind = ordinal == 1 ? java::HitKind::Block : ordinal == 2 ? java::HitKind::Entity : java::HitKind::Miss;
    jobject location = frame.env->GetObjectField(result, h.HitVec);
    if (!Good(frame.env) || !ReadVector(frame, location, value.location)) return false;
    if (value.kind == java::HitKind::Block) {
        if (IsLunar1710()) {
            if (!HitCoordinates17(frame.env, result, value.blockPos)) return false;
            value.hasBlockPos = true;
            value.direction = frame.env->GetIntField(result, h.HitSide);
            if (!Good(frame.env)) return false;
        } else {
        jobject pos = frame.env->CallObjectMethod(result, h.HitBlockPos);
        if (!Good(frame.env) || !pos) return false;
        value.blockPos.x = frame.env->CallIntMethod(pos, h.BlockX);
        value.blockPos.y = frame.env->CallIntMethod(pos, h.BlockY);
        value.blockPos.z = frame.env->CallIntMethod(pos, h.BlockZ);
        if (!Good(frame.env)) return false;
        value.hasBlockPos = true;
        jobject side = frame.env->GetObjectField(result, h.HitSide);
        if (!Good(frame.env) || !side) return false;
        value.direction = frame.env->CallIntMethod(side, h.Ordinal);
        if (!Good(frame.env)) return false;
        }
    }
    out = value;
    return true;
}

java::EntityHandle MakeEntityHit(const java::EntityHandle& entity, const rotation::Vec3& location) {
    Frame frame;
    if (!frame || !entity) return {};
    jobject vec = Vector(frame, location);
    if (!vec) return {};
    jobject hit = frame.env->NewObject(h.Hit, h.EntityHitCtor, static_cast<jobject>(entity.raw()), vec);
    return Good(frame.env) ? java::AdoptEntity(hit) : java::EntityHandle{};
}

bool BoxClip(const java::Box& box, const rotation::Vec3& from, const rotation::Vec3& to,
             bool& hit, rotation::Vec3& out) {
    hit = false;
    Frame frame;
    if (!frame) return false;
    jobject bounds = frame.env->NewObject(h.Box, h.BoxCtor, box.minX, box.minY, box.minZ,
                                         box.maxX, box.maxY, box.maxZ);
    if (!Good(frame.env) || !bounds) return false;
    jobject start = Vector(frame, from), end = Vector(frame, to);
    if (!start || !end) return false;
    jobject result = frame.env->CallObjectMethod(bounds, h.BoxIntercept, start, end);
    if (!Good(frame.env)) return false;
    if (!result) return true;
    jobject point = frame.env->GetObjectField(result, h.HitVec);
    if (!Good(frame.env) || !ReadVector(frame, point, out)) return false;
    hit = true;
    return true;
}

bool DirectionFromRotation(float pitch, float yaw, rotation::Vec3& out) {
    if (IsLunar1710()) {
        if (!std::isfinite(pitch) || !std::isfinite(yaw)) return false;
        constexpr double radians = 3.14159265358979323846 / 180.0;
        const double p = pitch*radians, y = yaw*radians;
        out = {-std::sin(y)*std::cos(p), -std::sin(p), std::cos(y)*std::cos(p)};
        return true;
    }
    Frame frame;
    jobject player = frame.player();
    if (!player || !std::isfinite(pitch) || !std::isfinite(yaw)) return false;
    jobject vec = frame.env->CallObjectMethod(player, h.LookVector, pitch, yaw);
    return Good(frame.env) && ReadVector(frame, vec, out);
}

namespace {
struct MotionState {
    float yaw = 0;
    float pitch = 0;
    bool ground = false;
    bool active = false;
};
MotionState motion;
bool tickObserved = false;
bool inputObserved = false;
bool motionObserved = false;
bool playerTickRunning = false;

template<class E>
bool Dispatch(E& event) noexcept {
    if (IsCircuitOpen()) return false;
    try { return event::EventBus::Get().Post(event); }
    catch (const std::exception& error) { LogTo("Lunar: event handler raised '%s'", error.what()); }
    catch (...) { LogTo("Lunar: event handler raised an unknown exception"); }
    return false;
}

bool Local(JNIEnv* env, jobject object) {
    if (!object || IsCircuitOpen()) return false;
    jobject player = compat::LocalPlayer(env);
    const bool same = player && env->IsSameObject(player, object);
    if (player) env->DeleteLocalRef(player);
    return same;
}

void JNICALL GameTickHook(JNIEnv* env, jclass) {
    watchdogBeforeMovement.store(false);
    MaintainWatchdogPhase(env, false);
    {
        BlockLagLock lock;
        if (!blockLagActive.load() || (grimBlockLag && GetTickCount64() >= grimBlockDeadline + 100))
            DrainBlockPackets(env, true);
    }
    if (!tickObserved) { tickObserved = true; LogTo("Lunar: game tick hook observed"); }
    event::GameTickEvent e(event::EventType::Pre);
    Dispatch(e);
    module::autoblock::SynchronizePacketDelays();
}

jboolean JNICALL PlayerTickHook(JNIEnv*, jclass, jobject player) {
    playerTickRunning = false;
    if (IsCircuitOpen()) return JNI_FALSE;
    if (java::ShouldSkipPlayerTick(player)) return JNI_TRUE;
    event::PlayerTickEvent e;
    playerTickRunning = !Dispatch(e);
    return playerTickRunning ? JNI_FALSE : JNI_TRUE;
}

void JNICALL PostPlayerTickHook(JNIEnv*, jclass) {
    if (!std::exchange(playerTickRunning, false)) return;
    event::PostPlayerTickEvent e;
    Dispatch(e);
}

void JNICALL PostGameTickHook(JNIEnv*, jclass) {
    event::GameTickEvent e(event::EventType::Post);
    Dispatch(e);
}

void JNICALL LivingUpdateHook(JNIEnv* env, jclass, jobject player) {
    if (Local(env, player)) {
        event::LivingUpdateEvent e;
        Dispatch(e);
    }
    env->CallNonvirtualVoidMethod(player, h.ClientPlayer, h.ParentLivingUpdate);
}

void JNICALL AttackMotionXHook(JNIEnv* env, jclass, jobject player, jdouble value) {
    if (Local(env, player)) {
        const double factor = IsCircuitOpen() ? 0.6 : java::BeginHitSlowdown(env, player);
        if (predictionBlockLag.load()) return;
        if (factor != 0.6) value = env->GetDoubleField(player, h.MotionX) * factor;
    }
    env->SetDoubleField(player, h.MotionX, value);
}
void JNICALL AttackMotionZHook(JNIEnv* env, jclass, jobject player, jdouble value) {
    if (Local(env, player)) {
        if (predictionBlockLag.load()) return;
        const double factor = java::HitSlowdown();
        if (factor != 0.6) value = env->GetDoubleField(player, h.MotionZ) * factor;
    }
    env->SetDoubleField(player, h.MotionZ, value);
}
void JNICALL AttackSprintingHook(JNIEnv* env, jclass, jobject player, jboolean value) {
    if (Local(env, player)) {
        value = java::EndHitSprint(value != JNI_FALSE) ? JNI_TRUE : JNI_FALSE;
        if (predictionBlockLag.load()) return;
    }
    env->CallVoidMethod(player, h.SetSprinting, value);
}

void JNICALL InputHook(JNIEnv* env, jclass, jobject input) {
    env->CallVoidMethod(input, h.UpdateInput);
    if (env->ExceptionCheck() || IsCircuitOpen()) return;
    if (!inputObserved) { inputObserved = true; LogTo("Lunar: movement input hook observed"); }
    if (!event::EventBus::Get().HasListeners<event::MoveInputEvent>()) return;
    const bool sneak = env->GetBooleanField(input, h.MoveSneak);
    float forward = env->GetFloatField(input, h.MoveForward);
    float strafe = env->GetFloatField(input, h.MoveStrafe);
    const bool jump = env->GetBooleanField(input, h.MoveJump);
    if (env->ExceptionCheck()) return;
    if (sneak) { forward = static_cast<float>(forward / 0.3); strafe = static_cast<float>(strafe / 0.3); }
    event::MoveInputEvent e(forward, strafe, jump, sneak, 0.3);
    Dispatch(e);
    if (e.sneak) {
        e.forward = static_cast<float>(e.forward * e.sneakSlowDownMultiplier);
        e.strafe = static_cast<float>(e.strafe * e.sneakSlowDownMultiplier);
    }
    if (e.hasFinalForward) e.forward = e.finalForward;
    if (e.hasFinalStrafe) e.strafe = e.finalStrafe;
    if(java::FreeCamActive()){e.forward=e.strafe=0;e.jump=e.sneak=false;}
    env->SetFloatField(input, h.MoveForward, e.forward);
    env->SetFloatField(input, h.MoveStrafe, e.strafe);
    env->SetBooleanField(input, h.MoveJump, e.jump ? JNI_TRUE : JNI_FALSE);
    env->SetBooleanField(input, h.MoveSneak, e.sneak ? JNI_TRUE : JNI_FALSE);
}

void JNICALL AppliedVelocityHook(JNIEnv* env, jclass, jobject entity, jdouble x, jdouble y, jdouble z) {
    env->CallVoidMethod(entity, h.SetVelocity, x, y, z);
    if (env->ExceptionCheck() || IsCircuitOpen() || !Local(env, entity)) return;
    event::AppliedVelocityEvent e(x, y, z);
    Dispatch(e);
}

void JNICALL WalkingHook(JNIEnv* env, jclass, jobject player) {
    if (motion.active || IsCircuitOpen()) { env->CallVoidMethod(player, h.WalkingUpdate); return; }
    if (!motionObserved) { motionObserved = true; LogTo("Lunar: motion hook observed"); }
    const float yaw = env->GetFloatField(player, h.Yaw);
    const float pitch = env->GetFloatField(player, h.Pitch);
    const bool ground = env->GetBooleanField(player, h.OnGround);
    const double x = env->GetDoubleField(player, h.X), y = env->GetDoubleField(player, h.Y);
    const double z = env->GetDoubleField(player, h.Z);
    const int ticks = env->GetIntField(player, h.Ticks);
    if (env->ExceptionCheck()) return;
    if (ticks <= 1) {
        event::RespawnEvent respawn;
        Dispatch(respawn);
        module::autoblock::SynchronizePacketDelays(true);
        MaintainWatchdogPhase(env, true);
    }
    struct WalkingPhase {
        WalkingPhase() { watchdogBeforeMovement.store(true); }
        ~WalkingPhase() { watchdogBeforeMovement.store(false); }
    } phase;
    event::MotionEvent pre(event::EventType::Pre, x, y, z, yaw, pitch, ground);
    Dispatch(pre);
    motion = {pre.yaw, pre.pitch, pre.onGround, true};
    env->CallVoidMethod(player, h.WalkingUpdate);
    motion.active = false;
    if (env->ExceptionCheck()) return;
    event::MotionEvent post(event::EventType::Post, pre.yaw, pre.pitch);
    Dispatch(post);
}

jfloat JNICALL MotionYawHook(JNIEnv* env, jclass, jobject player) {
    if(java::FreeCamActive())return env->GetFloatField(player,h.Yaw);
    return motion.active && !IsCircuitOpen() ? motion.yaw : env->GetFloatField(player, h.Yaw);
}
jfloat JNICALL MotionPitchHook(JNIEnv* env, jclass, jobject player) {
    if(java::FreeCamActive())return env->GetFloatField(player,h.Pitch);
    return motion.active && !IsCircuitOpen() ? motion.pitch : env->GetFloatField(player, h.Pitch);
}
jboolean JNICALL MotionGroundHook(JNIEnv* env, jclass, jobject player) {
    if(java::FreeCamActive())return env->GetBooleanField(player,h.OnGround);
    return motion.active && !IsCircuitOpen() ? (motion.ground ? JNI_TRUE : JNI_FALSE) :
        env->GetBooleanField(player, h.OnGround);
}
jboolean JNICALL InventoryAccelerationHook(JNIEnv* env, jclass, jobject entity) {
    return Local(env, entity) && module::invmove::BlockAcceleration() ? JNI_TRUE : JNI_FALSE;
}
void JNICALL InventoryMoveHook(JNIEnv* env, jclass, jobject entity, jdouble x, jdouble y, jdouble z) {
    if(Local(env, entity) && module::invmove::ZeroHorizontalDisplacement())x=z=0;
    // Redirect the call site, not moveEntity itself: vanilla collisions and all
    // non-local entities still execute their original movement implementation.
    mc263::access::Frame f(env);
    if(f)f.invoke(entity,{"net.minecraft.entity.Entity","moveEntity","(DDD)V"},x,y,z);
}
jfloat JNICALL StrafeHook(JNIEnv* env, jclass, jobject entity) {
    const float yaw = env->GetFloatField(entity, h.Yaw);
    if (!Local(env, entity)) return yaw;
    event::StrafeEvent e(yaw);
    Dispatch(e);
    return e.yaw;
}
jfloat JNICALL JumpHook(JNIEnv* env, jclass, jobject entity) {
    const float yaw = env->GetFloatField(entity, h.Yaw);
    if (!Local(env, entity)) return yaw;
    event::JumpEvent e(yaw);
    Dispatch(e);
    return e.yaw;
}
jobject JNICALL LookHook(JNIEnv* env, jclass, jobject entity, jfloat pitch, jfloat yaw) {
    event::RayTraceEvent e(entity, yaw, pitch);
    Dispatch(e);
    if (IsLunar1710()) {
        rotation::Vec3 v;
        if (!DirectionFromRotation(e.pitch, e.yaw, v)) return nullptr;
        return env->NewObject(h.Vec, h.VecCtor, v.x, v.y, v.z);
    }
    return env->CallObjectMethod(entity, h.LookVector, e.pitch, e.yaw);
}
jobject JNICALL Look17Hook(JNIEnv* env,jclass,jobject original,jobject entity,jfloat partial) {
    if (IsCircuitOpen()) return original;
    const float yaw=env->GetFloatField(entity,h.Yaw),pitch=env->GetFloatField(entity,h.Pitch);
    const float oldYaw=env->GetFloatField(entity,h.PreviousYaw),oldPitch=env->GetFloatField(entity,h.PreviousPitch);
    const float y=oldYaw+(yaw-oldYaw)*partial,p=oldPitch+(pitch-oldPitch)*partial;
    event::RayTraceEvent e(entity,y,p);
    Dispatch(e);
    if (e.yaw==y && e.pitch==p) return original;
    rotation::Vec3 v;
    if (!DirectionFromRotation(e.pitch,e.yaw,v)) return original;
    return env->NewObject(h.Vec,h.VecCtor,v.x,v.y,v.z);
}

jfloat AnimationComponent(JNIEnv* env, jobject entity, jfieldID field, int component) {
    const float original = env->GetFloatField(entity, field);
    if (!Local(env, entity)) return original;
    event::RotationAnimationEvent e(entity, env->GetFloatField(entity, h.Yaw),
        env->GetFloatField(entity, h.PreviousYaw), env->GetFloatField(entity, h.Pitch),
        env->GetFloatField(entity, h.PreviousPitch));
    float* values[] = {&e.yaw, &e.lastYaw, &e.pitch, &e.lastPitch};
    *values[component] = original;
    Dispatch(e);
    return *values[component];
}
jfloat JNICALL BodyYawHook(JNIEnv* env, jclass, jobject entity) {
    return AnimationComponent(env, entity, h.Yaw, 0);
}
jfloat JNICALL HeadYawHook(JNIEnv* env, jclass, jobject entity) {
    return AnimationComponent(env, entity, h.HeadYaw, 0);
}
jfloat JNICALL PreviousHeadYawHook(JNIEnv* env, jclass, jobject entity) {
    return AnimationComponent(env, entity, h.PreviousHeadYaw, 1);
}
jfloat JNICALL RenderPitchHook(JNIEnv* env, jclass, jobject entity) {
    return AnimationComponent(env, entity, h.Pitch, 2);
}
jfloat JNICALL PreviousRenderPitchHook(JNIEnv* env, jclass, jobject entity) {
    return AnimationComponent(env, entity, h.PreviousPitch, 3);
}
jfloat JNICALL AttackYawHook(JNIEnv* env, jclass, jobject entity) {
    const float yaw = env->GetFloatField(entity, h.Yaw);
    if (!Local(env, entity)) return yaw;
    event::AttackYawEvent e(yaw);
    Dispatch(e);
    return e.yaw;
}
jfloat ItemRotation(JNIEnv* env, jobject player, bool pitch) {
    const float yawValue = env->GetFloatField(player, h.Yaw), pitchValue = env->GetFloatField(player, h.Pitch);
    event::UseItemRayTraceEvent e(yawValue, pitchValue);
    if (Local(env, player)) Dispatch(e);
    return pitch ? e.pitch : e.yaw;
}
jfloat JNICALL ItemYawHook(JNIEnv* env, jclass, jobject entity) { return ItemRotation(env, entity, false); }
jfloat JNICALL ItemPitchHook(JNIEnv* env, jclass, jobject entity) { return ItemRotation(env, entity, true); }

std::atomic<bool> incomingObserved{false};
std::atomic<bool> outgoingObserved{false};

class NetworkScope {
    bool owner = false;
public:
    NetworkScope() {
        if (networkReentrySlot != TLS_OUT_OF_INDEXES && !TlsGetValue(networkReentrySlot))
            owner = TlsSetValue(networkReentrySlot, reinterpret_cast<void*>(1)) != FALSE;
    }
    ~NetworkScope() { if (owner) TlsSetValue(networkReentrySlot, nullptr); }
    explicit operator bool() const { return owner; }
};

bool ClientNetwork(JNIEnv* env, jobject manager) {
    if (IsLunar1710()) { const bool client = env->GetBooleanField(manager, h.ClientNetwork17); return Good(env) && client; }
    jobject direction = env->GetObjectField(manager, h.NetworkDirection);
    jobject clientbound = env->GetStaticObjectField(h.PacketDirection, h.Clientbound);
    const bool clean = Good(env);
    const bool result = clean && direction && clientbound && env->IsSameObject(direction, clientbound);
    if (direction) env->DeleteLocalRef(direction);
    if (clientbound) env->DeleteLocalRef(clientbound);
    return result;
}

void JNICALL PacketOutHook(JNIEnv* env, jclass, jobject manager, jobject packet, jobject callbacks) {
    if(java::FreeCamBlocksPacket(env,packet))return;
    bool movement = false;
    bool client = false;
    if (!IsCircuitOpen() && ClientNetwork(env, manager)) {
        client = true;
        if (!outgoingObserved.exchange(true)) LogTo("Lunar: outgoing packet hook observed");
        NetworkScope scope;
        const bool passthrough = java::ConsumePassthrough(env, packet);
        if (scope && !passthrough) {
            event::PacketEvent e(event::EventType::Outgoing, packet);
            if (Dispatch(e)) return;
        }
        // Match Leader's packet observation before Blink takes ownership.
        // Replayed packets skip this path and must not mutate the local state.
        if (!passthrough) {
            if (env->IsInstanceOf(packet, h.MovePacket)) {
                blockInputMovementSequence.fetch_add(1);
                blockInputAttacked.store(false);
                hypixelDigging.store(false); hypixelPlacing.store(false);
            }
            if (env->IsInstanceOf(packet, h.ActionPacket)) hypixelDigging.store(true);
            if (env->IsInstanceOf(packet, h.UsePacket)) hypixelPlacing.store(true);
            if (IsAttackPacket(packet)) blockInputAttacked.store(true);
            if (BlockReleasePacket(env, packet)) hypixelBlockingState.store(false);
            if (env->IsInstanceOf(packet, h.HeldSlotPacket)) {
                hypixelBlockingState.store(false);
                if (hypixelBlockManaged.load()) {
                    jobject player = compat::LocalPlayer(env);
                    if (player) { env->CallVoidMethod(player, h.StopItemInUse); env->DeleteLocalRef(player); }
                    Good(env);
                }
            }
        }
        // Queued actions also prevent a new local slot transaction. Only an
        // actually dispatched movement packet opens the next action interval.
        if (SlotOrderAction(env, packet)) heldSlotOrderReady.store(false);
        if (StageWatchdogPacket(env, manager, packet, callbacks, passthrough)) return;
        if (HoldBlockPacket(env, manager, packet, callbacks, passthrough)) return;
        movement = env->IsInstanceOf(packet, h.MovePacket);
    }
    if (client && module::displace::HoldOutput(env, manager, packet, callbacks)) {
        if (movement) watchdogBeforeMovement.store(false);
        return;
    }
    env->CallNonvirtualVoidMethod(manager, h.NetworkManager, h.DispatchPacket, packet, callbacks);
    if (movement && !env->ExceptionCheck()) {
        movementPacketSequence.fetch_add(1);
        watchdogBeforeMovement.store(false);
    }
    if (client && !env->ExceptionCheck()) RecordBlockPacket(env, packet);
}

void ProcessClientPacket(JNIEnv* env, jobject packet, jobject listener) {
    if (!IsCircuitOpen() && listener && env->IsInstanceOf(listener, h.NetHandler)) {
        const bool disconnected = env->IsInstanceOf(packet, h.DisconnectPacket);
        if (disconnected || env->IsInstanceOf(packet, h.RespawnPacket) || env->IsInstanceOf(packet, h.JoinPacket))
            MaintainWatchdogPhase(env, true);
        else if (env->IsInstanceOf(packet, h.PositionPacket)) MaintainWatchdogPhase(env, false, true);
        module::autoblock::SynchronizePacketDelays(disconnected ||
            env->IsInstanceOf(packet, h.RespawnPacket) || env->IsInstanceOf(packet, h.JoinPacket));
        if (disconnected || env->IsInstanceOf(packet, h.PositionPacket) || env->IsInstanceOf(packet, h.RespawnPacket)) {
            BlockLagLock lock;
            if (hypixelBlockLag || slinkyBlockLag || predictionBlockLag.load()) DrainBlockPackets(env, !disconnected);
            else if (grimBlockLag) DrainBlockPackets(env, !disconnected, true);
            grimResetSequence.fetch_add(1);
            module::autoblock::ResetCoverage();
        }
        if (!incomingObserved.exchange(true)) LogTo("Lunar: incoming packet hook observed on client thread");
        if (java::IncomingPacketsWanted()) {
            event::PacketEvent e(event::EventType::Incoming, packet);
            const bool cancelled = Dispatch(e);
            module::autoblock::SynchronizePacketDelays();
            if (cancelled) return;
        }
    }
    env->CallVoidMethod(packet, h.ProcessPacket, listener);
}

jboolean JNICALL ClientPacketTaskHook(JNIEnv* env, jclass, jobject task) {
    if (!incomingTasks.Take(env, task)) return JNI_FALSE;
    jobject packet = env->GetObjectField(task, h.TaskPacket);
    jobject listener = env->GetObjectField(task, h.TaskListener);
    if (!env->ExceptionCheck() && packet && listener)
        ProcessClientPacket(env, packet, listener);
    if (packet) env->DeleteLocalRef(packet);
    if (listener) env->DeleteLocalRef(listener);
    // Let vanilla FutureTask/Util.runTask report Java exceptions. Do not run
    // the original Runnable body a second time, even for cancelled packets.
    return JNI_TRUE;
}

void JNICALL PacketInHook(JNIEnv* env, jclass, jobject packet, jobject listener) {
    if (IsCircuitOpen() || !listener || !env->IsInstanceOf(listener, h.NetHandler)) {
        env->CallVoidMethod(packet, h.ProcessPacket, listener);
        return;
    }
    // 1.7 invokes this redirect only while draining processReceivedPackets on
    // the client thread; it has no PacketThreadUtil task class.
    if (IsLunar1710()) {
        ProcessClientPacket(env,packet,listener);
        return;
    }
    jobject mc = Minecraft(env);
    if (!mc || env->ExceptionCheck()) return;
    const bool clientThread = env->CallBooleanMethod(mc, h.IsClientThread) == JNI_TRUE;
    if (env->ExceptionCheck()) { env->DeleteLocalRef(mc); return; }
    if (clientThread) {
        env->DeleteLocalRef(mc);
        ProcessClientPacket(env, packet, listener);
        return;
    }
    // Never take a module/Blink lock on Netty. Minecraft executes scheduled
    // tasks while holding its queue monitor; replaying from Netty under a
    // module lock otherwise inverts that lock order with AppliedVelocity.
    jobject task = env->NewObject(h.PacketTask, h.PacketTaskCtor, packet, listener);
    if (task && !env->ExceptionCheck()) {
        if (incomingTasks.Add(env, task)) {
            // The tag lock is already released before entering Java's queue.
            jobject future = env->CallObjectMethod(mc, h.ScheduleTask, task);
            if (env->ExceptionCheck()) incomingTasks.Take(env, task);
            if (future) env->DeleteLocalRef(future);
        } else if (!env->ExceptionCheck()) {
            // Native allocation failure: preserve vanilla delivery without
            // invoking modules on this thread or dropping the packet.
            env->CallVoidMethod(packet, h.ProcessPacket, listener);
        }
    }
    if (task) env->DeleteLocalRef(task);
    env->DeleteLocalRef(mc);
}

void JNICALL BlockVanillaUseHook(JNIEnv* env, jclass, jobject minecraft) {
    if(java::FreeCamBlocksMouse(true))return;
    if (!IsCircuitOpen() && module::blockin::IsPlacing()) return;
    if (!IsCircuitOpen() && hypixelBlockManaged.load()) return;
    if (!IsCircuitOpen() && autoBlockManaged.load() && HoldingSword()) {
        BlockLagLock lock;
        if (blockLagActive.load() && !grimReblockPending) return;
    }
    if (!IsCircuitOpen()) {
        event::ClickEvent e;
        if (Dispatch(e)) return;
    }
    env->CallVoidMethod(minecraft, h.RightClickMouse);
    const bool failed = env->ExceptionCheck();
    if (!failed) {
        BlockLagLock lock;
        if (grimReblockPending) DrainBlockPackets(env, true);
    }
}

jboolean JNICALL BlockManualSwordUseHook(JNIEnv* env, jclass, jobject controller,
                                         jobject player, jobject world, jobject stack) {
    // rightClickMouse has already tried block/entity interaction. Only suppress
    // its final item-use fallback; opening containers must remain vanilla.
    if (!IsCircuitOpen() && autoBlockManualSuppressed.load() && HoldingSword()) return JNI_FALSE;
    return env->CallBooleanMethod(controller, h.StartUseItem, player, world, stack);
}

void JNICALL BlockVanillaReleaseHook(JNIEnv* env, jclass, jobject controller, jobject player) {
    if (!IsCircuitOpen() && hypixelBlockManaged.load()) return;
    env->CallVoidMethod(controller, h.StopUseItem, player);
}

void JNICALL BlockVanillaHitBlockHook(JNIEnv* env, jclass, jobject minecraft, jboolean leftDown) {
    if(java::FreeCamBlocksMouse(false))return;
    if (!IsCircuitOpen() && module::blockin::IsPlacing()) return;
    if (!IsCircuitOpen() && module::breaker::OwnsMining()) return;
    if (!IsCircuitOpen() && hypixelBlockManaged.load()) return;
    env->CallVoidMethod(minecraft, h.ClickBlock, leftDown);
}

void JNICALL BlockMouseKeyHook(JNIEnv* env, jclass, jint key, jboolean pressed) {
    if (!IsCircuitOpen() && module::blockin::IsPlacing() && key < 0) return;
    if (!IsCircuitOpen() && autoBlockManaged.load() &&
        key == -99 && HoldingSword()) return;
    env->CallStaticVoidMethod(h.KeyBinding, h.KeyState, key, pressed);
}

jobject JNICALL PrePlayerInteractHook(JNIEnv* env, jclass, jobject settings) {
    event::PrePlayerInteractEvent e;
    Dispatch(e);
    java::SuppressFreeCamKeys();
    return env->GetObjectField(settings, h.ChatVisibility);
}

void JNICALL ClientRotationHook(JNIEnv* env, jclass, jobject renderer, jfloat partialTicks) {
    event::ClientRotationEvent e;
    Dispatch(e);
    env->CallVoidMethod(renderer, h.Pick, partialTicks);
}

void JNICALL CursorPickBeginHook(JNIEnv*, jclass) {
    event::CursorPickEvent e(event::EventType::Pre); Dispatch(e);
}
void JNICALL CursorPickEndHook(JNIEnv*, jclass, jfloat partialTicks) {
    event::CursorPickEvent e(event::EventType::Post, partialTicks); Dispatch(e);
}

float worldRenderPartial = 0;
bool renderingWorldFrame = false;
void JNICALL WorldCameraParticlesHook(JNIEnv* env, jclass type, jboolean front) {
    java::FreeLookParticlesHook(env, type, front);
    if (renderingWorldFrame && !env->ExceptionCheck() && !IsCircuitOpen()) {
        java::CameraView view;
        // ActiveRenderInfo has just copied the world matrices. Capture the
        // matching origin now, before any hand/GUI pass or later camera change.
        if (nova::compat::lunar::ReadCameraView(worldRenderPartial, view))
            java::PublishWorldCamera(view);
    }
}

void JNICALL RenderFrameHook(JNIEnv* env, jclass, jobject renderer, jfloat partialTicks, jlong time) {
    java::InvalidateWorldCamera();
    event::RenderFrameEvent e;
    Dispatch(e);
    worldRenderPartial = partialTicks;
    renderingWorldFrame = true;
    if (IsLunar1710()) env->CallVoidMethod(renderer, h.RenderFrame, partialTicks);
    else env->CallVoidMethod(renderer, h.RenderFrame, partialTicks, time);
    renderingWorldFrame = false;
    if (env->ExceptionCheck()) java::InvalidateWorldCamera();
}

jint JNICALL BlockUseCountHook(JNIEnv* env, jclass, jobject player) {
    const jint actual = env->CallIntMethod(player, h.ItemUseCount);
    if (env->ExceptionCheck() || actual > 0 || IsCircuitOpen() || !autoBlockAnimation.load())
        return actual;
    return Local(env, player) && HoldingSword() ? 1 : actual;
}

const bridge::HookDef lunarHooks[] = {
    {"cursorPickBegin", "()V", reinterpret_cast<void*>(&CursorPickBeginHook)},
    {"cursorPickEnd", "(F)V", reinterpret_cast<void*>(&CursorPickEndHook)},
    {"clientRotation", "(Lnet/minecraft/client/renderer/EntityRenderer;F)V", reinterpret_cast<void*>(&ClientRotationHook)},
    {"freeLookMouse", "(Lnet/minecraft/client/Minecraft;)Z", reinterpret_cast<void*>(&nova::java::FreeLookMouseHook)},
    {"freeLookYaw", "(Lnet/minecraft/entity/Entity;)F", reinterpret_cast<void*>(&nova::java::FreeLookYawHook)},
    {"freeLookPitch", "(Lnet/minecraft/entity/Entity;)F", reinterpret_cast<void*>(&nova::java::FreeLookPitchHook)},
    {"freeLookPreviousYaw", "(Lnet/minecraft/entity/Entity;)F", reinterpret_cast<void*>(&nova::java::FreeLookPreviousYawHook)},
    {"freeLookPreviousPitch", "(Lnet/minecraft/entity/Entity;)F", reinterpret_cast<void*>(&nova::java::FreeLookPreviousPitchHook)},
    {"freeLookBillboard", "(Lnet/minecraft/client/renderer/entity/RenderManager;)V", reinterpret_cast<void*>(&nova::java::FreeLookBillboardHook)},
    {"freeLookParticles", "(Z)V", reinterpret_cast<void*>(&WorldCameraParticlesHook)},
    {"clientPacketTask", "(Ljava/lang/Object;)Z", reinterpret_cast<void*>(&ClientPacketTaskHook)},
    {"keepFov", "(FZ)F", reinterpret_cast<void*>(&nova::module::scaffold::KeepFovFloatHook)},
    {"spoofSlot", "(Lnet/minecraft/entity/player/InventoryPlayer;)I", reinterpret_cast<void*>(&java::SpoofSlotHook)},
    {"visualInventoryHand", "(Lnet/minecraft/entity/player/InventoryPlayer;)Lnet/minecraft/item/ItemStack;", reinterpret_cast<void*>(&java::VisualInventoryHandHook)},
    {"postPlayerTick", "()V", reinterpret_cast<void*>(&PostPlayerTickHook)},
    {"blockVanillaHitBlock", "(Lnet/minecraft/client/Minecraft;Z)V", reinterpret_cast<void*>(&BlockVanillaHitBlockHook)},
    {"freeCamClick", "(Lnet/minecraft/client/Minecraft;)V", reinterpret_cast<void*>(&java::FreeCamClickHook)},
    {"freeCamAngles", "(Lnet/minecraft/entity/Entity;FF)V", reinterpret_cast<void*>(&java::FreeCamAnglesHook)},
    {"freeCamControlled", "(Lnet/minecraft/client/entity/EntityPlayerSP;)Z", reinterpret_cast<void*>(&java::FreeCamControlledHook)},
    {"blockVanillaRelease", "(Lnet/minecraft/client/multiplayer/PlayerControllerMP;Lnet/minecraft/entity/player/EntityPlayer;)V", reinterpret_cast<void*>(&BlockVanillaReleaseHook)},
    {"appliedVelocity", "(Lnet/minecraft/entity/Entity;DDD)V", reinterpret_cast<void*>(&AppliedVelocityHook)},
    {"renderFrame", "(Lnet/minecraft/client/renderer/EntityRenderer;FJ)V", reinterpret_cast<void*>(&RenderFrameHook)},
    {"prePlayerInteract", "(Lnet/minecraft/client/settings/GameSettings;)Lnet/minecraft/entity/player/EntityPlayer$EnumChatVisibility;", reinterpret_cast<void*>(&PrePlayerInteractHook)},
    {"blockMouseKey", "(IZ)V", reinterpret_cast<void*>(&BlockMouseKeyHook)},
    {"blockVanillaUse", "(Lnet/minecraft/client/Minecraft;)V", reinterpret_cast<void*>(&BlockVanillaUseHook)},
    {"blockManualSwordUse", "(Lnet/minecraft/client/multiplayer/PlayerControllerMP;Lnet/minecraft/entity/player/EntityPlayer;Lnet/minecraft/world/World;Lnet/minecraft/item/ItemStack;)Z", reinterpret_cast<void*>(&BlockManualSwordUseHook)},
    {"blockUseCount", "(Lnet/minecraft/client/entity/AbstractClientPlayer;)I", reinterpret_cast<void*>(&BlockUseCountHook)},
    {"gameTick", "()V", reinterpret_cast<void*>(&GameTickHook)},
    {"postGameTick", "()V", reinterpret_cast<void*>(&PostGameTickHook)},
    {"livingUpdate", "(Lnet/minecraft/client/entity/AbstractClientPlayer;)V", reinterpret_cast<void*>(&LivingUpdateHook)},
    {"attackMotionX", "(Lnet/minecraft/entity/player/EntityPlayer;D)V", reinterpret_cast<void*>(&AttackMotionXHook)},
    {"attackMotionZ", "(Lnet/minecraft/entity/player/EntityPlayer;D)V", reinterpret_cast<void*>(&AttackMotionZHook)},
    {"attackSprinting", "(Lnet/minecraft/entity/player/EntityPlayer;Z)V", reinterpret_cast<void*>(&AttackSprintingHook)},
    {"playerTick", "(Lnet/minecraft/client/entity/EntityPlayerSP;)Z", reinterpret_cast<void*>(&PlayerTickHook)},
    {"input", "(Lnet/minecraft/util/MovementInput;)V", reinterpret_cast<void*>(&InputHook)},
    {"walking", "(Lnet/minecraft/client/entity/EntityPlayerSP;)V", reinterpret_cast<void*>(&WalkingHook)},
    {"motionYaw", "(Lnet/minecraft/client/entity/EntityPlayerSP;)F", reinterpret_cast<void*>(&MotionYawHook)},
    {"motionPitch", "(Lnet/minecraft/client/entity/EntityPlayerSP;)F", reinterpret_cast<void*>(&MotionPitchHook)},
    {"motionGround", "(Lnet/minecraft/client/entity/EntityPlayerSP;)Z", reinterpret_cast<void*>(&MotionGroundHook)},
    {"strafeYaw", "(Lnet/minecraft/entity/Entity;)F", reinterpret_cast<void*>(&StrafeHook)},
    {"inventoryAcceleration", "(Lnet/minecraft/entity/Entity;)Z", reinterpret_cast<void*>(&InventoryAccelerationHook)},
    {"inventoryMove", "(Lnet/minecraft/entity/EntityLivingBase;DDD)V", reinterpret_cast<void*>(&InventoryMoveHook)},
    {"jumpYaw", "(Lnet/minecraft/entity/EntityLivingBase;)F", reinterpret_cast<void*>(&JumpHook)},
    {"viewVector", "(Lnet/minecraft/entity/Entity;FF)Lnet/minecraft/util/Vec3;", reinterpret_cast<void*>(&LookHook)},
    {"livingViewVector", "(Lnet/minecraft/entity/EntityLivingBase;FF)Lnet/minecraft/util/Vec3;", reinterpret_cast<void*>(&LookHook)},
    {"playerViewVector", "(Lnet/minecraft/client/entity/EntityPlayerSP;FF)Lnet/minecraft/util/Vec3;", reinterpret_cast<void*>(&LookHook)},
    {"bodyYaw", "(Lnet/minecraft/entity/EntityLivingBase;)F", reinterpret_cast<void*>(&BodyYawHook)},
    {"headYaw", "(Lnet/minecraft/entity/EntityLivingBase;)F", reinterpret_cast<void*>(&HeadYawHook)},
    {"previousHeadYaw", "(Lnet/minecraft/entity/EntityLivingBase;)F", reinterpret_cast<void*>(&PreviousHeadYawHook)},
    {"renderPitch", "(Lnet/minecraft/entity/EntityLivingBase;)F", reinterpret_cast<void*>(&RenderPitchHook)},
    {"previousRenderPitch", "(Lnet/minecraft/entity/EntityLivingBase;)F", reinterpret_cast<void*>(&PreviousRenderPitchHook)},
    {"attackTarget", "(Ljava/lang/Object;)V", reinterpret_cast<void*>(&nova::java::BacktrackAttackHook)},
    {"attackYaw", "(Lnet/minecraft/entity/player/EntityPlayer;)F", reinterpret_cast<void*>(&AttackYawHook)},
    {"itemYaw", "(Lnet/minecraft/entity/player/EntityPlayer;)F", reinterpret_cast<void*>(&ItemYawHook)},
    {"itemPitch", "(Lnet/minecraft/entity/player/EntityPlayer;)F", reinterpret_cast<void*>(&ItemPitchHook)},
    {"packetOut", "(Lnet/minecraft/network/NetworkManager;Lnet/minecraft/network/Packet;[Lio/netty/util/concurrent/GenericFutureListener;)V", reinterpret_cast<void*>(&PacketOutHook)},
    {"packetIn", "(Lnet/minecraft/network/Packet;Lnet/minecraft/network/INetHandler;)V", reinterpret_cast<void*>(&PacketInHook)},
    {"accountBrowserAdd","(Ljava/lang/Object;)V",reinterpret_cast<void*>(&minecraft::accounts::BrowserAddAccount)},
    {"accountBrowserSelect","(Ljava/lang/String;)Ljava/util/Optional;",reinterpret_cast<void*>(&minecraft::accounts::BrowserSelectAccount)},
    {"accountScreenRequiresLogin","(Ljava/lang/Object;)Z",reinterpret_cast<void*>(&minecraft::accounts::ScreenRequiresLogin)},
    {"accountCanOpenMultiplayer","(Ljava/lang/Object;)Z",reinterpret_cast<void*>(&minecraft::accounts::CanOpenMultiplayer)},
    {"accountBrowserRefresh","(Ljava/lang/Object;Ljava/lang/Object;)V",reinterpret_cast<void*>(&minecraft::accounts::BrowserRefreshAccount)}
};
}

namespace {
// Namespace initialization avoids MSVC local-static TLS guards in the reflective DLL.
const std::vector<bridge::HookDef> lunar1710Hooks = [] {
            std::vector<bridge::HookDef> result(std::begin(lunarHooks), std::end(lunarHooks));
            const bridge::HookDef extra[] = {
                {"look17", "(Lnet/minecraft/util/Vec3;Lnet/minecraft/entity/EntityLivingBase;F)Lnet/minecraft/util/Vec3;", reinterpret_cast<void*>(&Look17Hook)},
                {"walking17", "(Lnet/minecraft/client/entity/EntityClientPlayerMP;)V", reinterpret_cast<void*>(&WalkingHook)},
                {"playerTick17", "(Lnet/minecraft/client/entity/EntityClientPlayerMP;)Z", reinterpret_cast<void*>(&PlayerTickHook)},
                {"motionYaw17", "(Lnet/minecraft/client/entity/EntityClientPlayerMP;)F", reinterpret_cast<void*>(&MotionYawHook)},
                {"motionPitch17", "(Lnet/minecraft/client/entity/EntityClientPlayerMP;)F", reinterpret_cast<void*>(&MotionPitchHook)},
                {"motionGround17", "(Lnet/minecraft/client/entity/EntityClientPlayerMP;)Z", reinterpret_cast<void*>(&MotionGroundHook)},
                {"freeLookYaw17", "(Lnet/minecraft/entity/EntityLivingBase;)F", reinterpret_cast<void*>(&java::FreeLookYawHook)},
                {"freeLookPitch17", "(Lnet/minecraft/entity/EntityLivingBase;)F", reinterpret_cast<void*>(&java::FreeLookPitchHook)},
                {"freeLookPreviousYaw17", "(Lnet/minecraft/entity/EntityLivingBase;)F", reinterpret_cast<void*>(&java::FreeLookPreviousYawHook)},
                {"freeLookPreviousPitch17", "(Lnet/minecraft/entity/EntityLivingBase;)F", reinterpret_cast<void*>(&java::FreeLookPreviousPitchHook)},
                {"renderFrame17", "(Lnet/minecraft/client/renderer/EntityRenderer;F)V", reinterpret_cast<void*>(+[](JNIEnv* env, jclass cls, jobject renderer, jfloat ticks) {
                    RenderFrameHook(env, cls, renderer, ticks, 0);
                })}
            };
            result.insert(result.end(), std::begin(extra), std::end(extra));
            return result;
        }();
}
const bridge::HookDef* Hooks(int& count) {
    if (IsLunar1710()) {
        count = static_cast<int>(lunar1710Hooks.size());
        return lunar1710Hooks.data();
    }
    count = static_cast<int>(sizeof(lunarHooks) / sizeof(lunarHooks[0]));
    return lunarHooks;
}

namespace {
bool ValidateItemSamples() {
    Frame frame;
    if (!frame) return false;
    auto* env = frame.env;
    const jmethodID ctor = env->GetMethodID(h.Stack, "<init>", "(Lnet/minecraft/item/Item;II)V");
    const jmethodID blockCtor = env->GetMethodID(h.Stack, "<init>", "(Lnet/minecraft/block/Block;II)V");
    const jmethodID enchant = env->GetMethodID(h.Stack, "addEnchantment", "(Lnet/minecraft/enchantment/Enchantment;I)V");
    const jmethodID rename = env->GetMethodID(h.Stack, "setStackDisplayName", "(Ljava/lang/String;)Lnet/minecraft/item/ItemStack;");
    if (!Good(env) || !ctor || !blockCtor || !enchant || !rename) return false;
    java::ItemFacts facts;
    if (!java::ReadItemFacts(nullptr, facts) || !facts.empty) return false;
    jobject apple = env->GetStaticObjectField(h.Items, h.ItemGoldenApple);
    if (!Good(env) || !apple) return false;
    for (int damage = 0; damage <= 1; ++damage) {
        jobject stack = env->NewObject(h.Stack, ctor, apple, static_cast<jint>(3), static_cast<jint>(damage));
        if (!Good(env) || !stack || !java::ReadItemFacts(stack, facts) || facts.empty || facts.count != 3 ||
            facts.item != (damage ? java::KnownItem::EnchantedGoldenApple : java::KnownItem::GoldenApple)) return false;
        env->DeleteLocalRef(stack);
    }
    jobject axe = env->GetStaticObjectField(h.Items, h.ItemDiamondAxe);
    if (!Good(env) || !axe) return false;
    jobject axeStack = env->NewObject(h.Stack, ctor, axe, static_cast<jint>(1), static_cast<jint>(0));
    if (!Good(env) || !axeStack || !java::ReadItemFacts(axeStack, facts) ||
        !(facts.kinds & java::kItemAxe) || facts.destroySpeed != 8 || facts.maxDamage != 1561) return false;
    const jfieldID swordField = env->GetStaticFieldID(h.Items, "diamond_sword", "Lnet/minecraft/item/Item;");
    const jfieldID armorField = env->GetStaticFieldID(h.Items, "diamond_helmet", "Lnet/minecraft/item/ItemArmor;");
    if (!Good(env) || !swordField || !armorField) return false;
    jobject sword = env->GetStaticObjectField(h.Items, swordField);
    jobject armor = env->GetStaticObjectField(h.Items, armorField);
    jobject sharpness = env->GetStaticObjectField(h.Enchantment, h.Enchant_sharpness);
    if (!Good(env) || !sword || !armor || !sharpness) return false;
    jobject swordStack = env->NewObject(h.Stack, ctor, sword, static_cast<jint>(1), static_cast<jint>(7));
    if (!Good(env) || !swordStack) return false;
    env->CallVoidMethod(swordStack, enchant, sharpness, static_cast<jint>(5));
    if (!Good(env) || !java::ReadItemFacts(swordStack, facts) || !(facts.kinds & java::kItemSword) ||
        facts.damageValue != 7 || facts.swordDamage != 7 || facts.sharpness != 5 || facts.sharpnessBonus != 6.25f) return false;
    jstring customName = env->NewStringUTF("Nova mapping sample");
    if (!Good(env) || !customName) return false;
    env->CallObjectMethod(swordStack, rename, customName);
    std::string name;
    if (!Good(env) || !java::ItemDisplayName(swordStack, name) || name != "Nova mapping sample") return false;
    jobject armorStack = env->NewObject(h.Stack, ctor, armor, static_cast<jint>(1), static_cast<jint>(0));
    if (!Good(env) || !armorStack || !java::ReadItemFacts(armorStack, facts) ||
        !(facts.kinds & java::kItemArmor) || facts.armorSlot != java::kArmorHead || facts.material != java::KnownMaterial::Diamond) return false;
    const jfieldID blocks[] = {h.Block_stone, h.Block_chest};
    for (int i = 0; i < 2; ++i) {
        jobject block = env->GetStaticObjectField(h.Blocks, blocks[i]);
        if (!Good(env) || !block) return false;
        jobject stack = env->NewObject(h.Stack, blockCtor, block, static_cast<jint>(64), static_cast<jint>(0));
        if (!Good(env) || !stack || !java::ReadItemFacts(stack, facts) ||
            !(facts.kinds & java::kItemBlock) || facts.count != 64 || facts.blacklistedBlock != (i == 1)) return false;
        env->DeleteLocalRef(stack);
        env->DeleteLocalRef(block);
    }
    return true;
}
}

void ValidateWorld() {
    if (worldValidated || !compat::ValidatePlayer()) return;
    worldValidated = true;
    auto player = java::PlayerEntity();
    rotation::Vec3 now, before, eye, look, intersection;
    java::Box box;
    java::BlockHitResultInfo hit;
    std::vector<java::EntityHandle> players, entities;
    float health = 0, absorption = 0, yaw = 0, pitch = 0;
    unsigned types = 0;
    int id = 0, ticks = 0;
    bool clipped = false;
    const bool basic = player && player.PositionPair(now, before) &&
        player.EyePosition(1.0f, eye) && player.BoundingBox(box) &&
        player.Health(health, absorption) && java::EntityId(player, id) &&
        player.TickCount(ticks) && java::EntityTypes(player, types) &&
        java::PlayerRotation(yaw, pitch) && java::Vec3DirectionFromRotation(pitch, yaw, look);
    const bool lists = basic && java::WorldPlayers(players) &&
        java::WorldEntities(player, java::BoxInflate(box, 8.0), entities);
    const rotation::Vec3 end{eye.x + look.x * 5, eye.y + look.y * 5, eye.z + look.z * 5};
    const bool ray = basic && java::WorldClip(eye, end, player, hit) &&
        java::BoxClip(box, eye, end, clipped, intersection);
    auto found = java::EntityById(id);
    auto entityHit = java::MakeEntityHit(player, eye);
    const bool identity = basic && found && player.IsSame(found) && entityHit;
    LogTo("Lunar: world validation basic=%d lists=%d ray=%d identity=%d players=%zu entities=%zu ticks=%d",
          basic, lists, ray, identity, players.size(), entities.size(), ticks);
    Frame frame;
    if (!frame) return;
    jobject velocity = frame.env->NewObject(h.VelocityPacket, h.VelocityCtor, static_cast<jint>(42),
                                           static_cast<jdouble>(0.1), static_cast<jdouble>(0.2), static_cast<jdouble>(-0.3));
    if (!Good(frame.env) || !velocity) { LogTo("Lunar: network value validation failed to create sample"); return; }
    jobject use = UsePacket(frame.env, nullptr);
    if (!Good(frame.env) || !use) { LogTo("Lunar: network value validation failed to create use sample"); return; }
    int packetId = 0, xa = 0, ya = 0, za = 0;
    const bool packets = java::IsMotionPacket(velocity) && java::IsVelocityDelayPacket(velocity) &&
        !java::IsPositionOrExplodePacket(velocity) && !java::IsMotionPacket(use) &&
        java::ReadMotionPacket(velocity, packetId, xa, ya, za) &&
        packetId == 42 && xa == 800 && ya == 1600 && za == -2400 &&
        java::IsUseItemPacket(use) && java::IsUseItemOnlyPacket(use);
    int placeDelay = 0;
    float speed = 0;
    bool sprinting = false;
    rotation::Vec3 movement;
    const bool movementRead = RightClickDelay(placeDelay) && PlayerSpeed(speed) &&
        PlayerSprinting(sprinting) && PlayerDeltaMovement(movement);
    LogTo("Lunar: network values=%d movement/placement reads=%d", packets, movementRead);
    jobject menu = nullptr;
    int windowId = -1;
    const int itemCount = java::InventoryItemCount();
    const int armorCount = java::InventoryArmorCount();
    bool inventory = itemCount == 36 && armorCount == 4 && java::PlayerMenu(menu) &&
        java::MenuIsPlayerInventory(menu) && !java::MenuIsChest(menu) &&
        java::MenuContainerId(menu, windowId) && windowId == 0;
    int occupied = 0;
    for (int i = 0; inventory && i < itemCount + armorCount; ++i) {
        const bool armor = i >= itemCount;
        const int index = armor ? i - itemCount : i;
        const int menuIndex = armor ? 8 - index : (index < 9 ? index + 36 : index);
        jobject stack = nullptr, slotStack = nullptr;
        inventory = (armor ? java::InventoryArmor(index, stack) : java::InventorySlot(index, stack)) &&
            java::MenuSlotStack(menu, menuIndex, slotStack) && frame.env->IsSameObject(stack, slotStack);
        if (stack) { ++occupied; frame.env->DeleteLocalRef(stack); }
        if (slotStack) frame.env->DeleteLocalRef(slotStack);
    }
    jobject absent = nullptr;
    inventory = inventory && !java::InventorySlot(-1, absent) &&
        !java::InventorySlot(36, absent) && !java::InventoryOffhand(0, absent) &&
        !java::MenuSlotStack(menu, -1, absent) && !java::MenuSlotStack(menu, 45, absent);
    if (menu) frame.env->DeleteLocalRef(menu);
    std::string chestTitle;
    const bool translation = java::TranslatedText("container.chest", chestTitle) && !chestTitle.empty();
    jobject close = frame.env->NewObject(h.ClosePacket, h.ClosePacketCtor, static_cast<jint>(17));
    const bool inventoryPackets = Good(frame.env) && close && java::PacketIsContainerClose(close) &&
        !java::PacketIsMovePlayer(close) && java::PacketIsUseItem(use) && !java::PacketIsUseItemOn(use);
    LogTo("Lunar: inventory slots=%d items=%d armor=%d occupied=%d translation=%d packets=%d",
          inventory, itemCount, armorCount, occupied, translation, inventoryPackets);
    const bool itemSamples = ValidateItemSamples();
    inventory::Snapshot snapshot;
    const bool snapshotRead = inventory::Scan(snapshot);
    java::CompassView compass;
    java::CameraView camera;
    std::vector<java::ChestBox> chests;
    const bool compassRead = java::ReadCompassView(FrameTime(), compass);
    const bool cameraRead = java::ReadCameraView(FrameTime(), camera);
    const bool chestRead = java::LoadedChestBoxes(chests);
    java::BlockPos target;
    int bestTool = -1, selected = -1;
    const bool hasTarget = HitBlockPos(target);
    const bool toolRead = !hasTarget || BestToolSlotFor(target, bestTool);
    const bool selectedRead = SelectedSlot(selected);
    const int placeable = java::CountPlaceableBlocks();
    const jmethodID singleplayer = frame.env->GetMethodID(h.Minecraft, "isSingleplayer", "()Z");
    const bool singleplayerBound = Good(frame.env) && singleplayer;
    const bool singleplayerWorld = singleplayerBound && frame.env->CallBooleanMethod(frame.mc, singleplayer) == JNI_TRUE;
    Good(frame.env);
    LogTo("Lunar: full adapters samples=%d snapshot=%d compass=%d camera=%d chests=%d/%zu tool=%d target=%d selected=%d/%d blocks=%d singleplayer=%d skin=%u",
        itemSamples, snapshotRead, compassRead, cameraRead, chestRead, chests.size(), toolRead, hasTarget,
        selectedRead, selected, placeable, singleplayerWorld, java::PlayerSkinTexture(player));
}

float FrameTime() {
    Frame frame;
    if (!frame) return 0;
    jobject timer = frame.env->GetObjectField(frame.mc, h.GameTimer);
    if (!Good(frame.env) || !timer) return 0;
    const float value = frame.env->GetFloatField(timer, h.PartialTicks);
    return Good(frame.env) ? value : 0;
}
}
