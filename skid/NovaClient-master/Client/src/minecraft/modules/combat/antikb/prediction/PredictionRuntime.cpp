#include "minecraft/modules/misc/antibot/AntiBot.h"
#include "minecraft/modules/combat/antikb/prediction/PredictionRuntime.h"
#include "minecraft/modules/combat/autoblock/AutoBlockRuntime.h"
#include "minecraft/modules/combat/killaura/KillAura.h"
#include "minecraft/modules/ModuleManager.h"
#include "minecraft/modules/player/scaffold/Scaffold.h"
#include "minecraft/compat/lunar189/LunarRuntime.h"
#include "minecraft/compat/Profile.h"
#include "minecraft/events/GameEvents.h"
#include "minecraft/utils/rotation/RotationManager.h"
#include "minecraft/utils/rotation/RayCastUtil.h"
#include "minecraft/utils/rotation/Mth.h"
#include "minecraft/java/api/game_network.h"
#include <atomic>
#include <deque>
#include <memory>
#include <mutex>

namespace nova::module::prediction {
namespace {
namespace lunar = compat::lunar;
std::atomic<bool> ownsIncoming{false}, stopped{false}, cancel{false};

struct Runtime {
    // Lunar delivers incoming PacketEvents on the client task queue. Replay
    // can synchronously call AppliedVelocity on that same thread. Do not move
    // Packet back to Netty: holding this lock while enqueueing a vanilla task
    // inverts Minecraft's scheduledTasks monitor against AppliedVelocity.
    std::recursive_mutex mutex;
    PredictionSettings settings;
    PredictionVelocity state;
    bool active = false;
    unsigned long long delayStarted = 0;
    std::deque<jobject> packets;
    java::EntityHandle world;
    rotation::Rotation reportedRotation;
    rotation::Rotation targetRotation;
    rotation::Rotation appliedRotation;
    bool ownsRotation = false;
    event::Subscription packet, pre, post, applied, input, living, tick;

    struct IO {
        Runtime& r;
        java::EntityHandle ray;
        bool OnGround() const { return lunar::PlayerOnGround(); }
        bool Sprinting() const { bool v = false; return lunar::PlayerSprinting(v) && v; }
        bool LiquidWeb() const { return lunar::PredictionLiquidWeb(); }
        bool JumpBoost() const { return lunar::PredictionJumpBoost(); }
        bool ForwardPressed() const { return lunar::PredictionForwardPressed(); }
        void Jump() { lunar::PredictionJump(); }
        int HurtTime() const { int v = 0; auto p = lunar::PlayerEntity(); if (p) p.HurtTime(v); return v; }
        double MotionY() const { rotation::Vec3 v; lunar::PlayerDeltaMovement(v); return v.y; }
        bool AuraEnabled() const { return killaura::IsEnabled(); }
        bool AuraTarget() const { return killaura::HasTargets() && bool(lunar::EntityById(killaura::TargetId())); }
        bool AuraBlocking() const { return AuraEnabled() && lunar::HypixelPlayerBlocking(); }
        bool CanReduce(int phase) const { return autoblock::VelocityCanReduce(phase); }
        bool ShouldBlock() const { return autoblock::VelocityShouldBlock(); }
        bool StartBlink() { return lunar::BeginPredictionBlink(); }
        void ReleaseBlink() { lunar::EndPredictionBlink(); }
        void ReleaseDelay() { r.Drain(true); }
        void ComputeRotation(double x, double z) {
            const auto current = r.reportedRotation;
            const float yaw = static_cast<float>(std::atan2(z, x) * 180.0 / 3.141592653589793) - 90.f;
            const float dy = rotation::mth::WrapDegrees(yaw - current.yaw);
            const float dp = rotation::mth::WrapDegrees(-current.pitch);
            const auto quantize = [](float angle) {
                return static_cast<float>(double(angle) - std::fmod(double(angle), double(.0096f)));
            };
            r.targetRotation = {quantize(current.yaw + (std::abs(dy) <= 1 ? 0 : dy)),
                                quantize(current.pitch + (std::abs(dp) <= 1 ? 0 : dp))};
        }
        void Rotate() {
            if (scaffold::IsEnabled()) return;
            if (rotation::SetDirectRotations(r.targetRotation)) {
                rotation::State().priority = rotation::RotationPriority::Highest;
                // Leader rotates movement/jump yaw without correcting the raw keys.
                rotation::State().moveInputFix = false;
                r.ownsRotation = true;
                r.appliedRotation = rotation::State().rotations;
            }
        }
        bool RayPlayer() {
            auto hit = rotation::LeaderRayCast(r.reportedRotation, 3.0);
            ray = std::move(hit.entity);
            return hit.type == rotation::HitResultType::Entity && ray && ray.IsPlayer() &&
                   !ray.IsSame(lunar::PlayerEntity()) && !antibot::IsBot(ray);
        }
        void Attack(bool aura, bool keepSprint) {
            auto target = aura ? lunar::EntityById(killaura::TargetId()) : std::move(ray);
            if (!target || antibot::IsBot(target)) return;
            event::AttackEntityEvent e(static_cast<jobject>(target.raw()));
            (void)event::EventBus::Get().Post(e);
            lunar::SendSwingPacket();
            lunar::SendAttackPacket(target);
            rotation::Vec3 motion;
            if (lunar::PlayerDeltaMovement(motion))
                lunar::SetPlayerDeltaMovement(motion.x * .6, motion.y, motion.z * .6);
            if (!keepSprint) lunar::SetPlayerSprinting(false);
            if (r.settings.debug) LogTo("Prediction: reduce attack (%s)", aura ? "aura" : "ray");
        }
    };

    void MoveInput(event::MoveInputEvent& e) const {
        if (active && settings.autoMove && state.rotateCounter > 0 && state.rotateCounter <= settings.rotateTicks) {
            e.forward = 1;
            e.hasFinalForward = true;
            e.finalForward = 1;
        }
    }

    void Publish() { stopped.store(active && state.stoppedBlock); cancel.store(active && state.cancelAura); }
    void ReleaseRotation() {
        if (!ownsRotation) return;
        const auto& current = rotation::State();
        if (current.direct && current.rotations.yaw == appliedRotation.yaw &&
            current.rotations.pitch == appliedRotation.pitch) rotation::ReleaseDirectRotations();
        ownsRotation = false;
    }
    void Drain(bool replay) {
        JniAttach attach;
        if (!attach) return;
        state.delayFlag = false;
        delayStarted = 0;
        if (settings.debug && !packets.empty())
            LogTo("Prediction: %s %zu incoming packets after %d ticks", replay ? "replay" : "discard", packets.size(), state.delayedTicks);
        // mutex remains held through replay: new network arrivals cannot overtake.
        while (!packets.empty()) {
            jobject p = packets.front(); packets.pop_front();
            if (replay) lunar::HandleIncomingPacket(p);
            attach.env->DeleteGlobalRef(p);
        }
        ownsIncoming.store(false);
    }
    bool ReleaseExpiredDelay() {
        if (!state.delayFlag) return false;
        // Real time is independent of player POST ticks, falling and landing.
        if (settings.delay && GetTickCount64() - delayStarted < 500 && state.delayedTicks < 10)
            return false;
        Drain(true);
        return true;
    }
    void Reset(bool replay) {
        Drain(replay);
        // Leader drops its outgoing queue on disable. Do not leave a stale owner.
        lunar::EndPredictionBlink(false);
        state = {};
        targetRotation = {};
        Publish();
    }
    bool Ready() {
        if (!active) return false;
        auto level = lunar::LevelEntity();
        auto player = lunar::PlayerEntity();
        if (!level || !player || lunar::EntityDead(player.raw())) {
            Reset(false); world.Reset(); return false;
        }
        if (world && !world.IsSame(level)) Reset(false);
        world = std::move(level);
        // A correction/world packet can force the native queue to finish.
        if (state.blinking && !lunar::PredictionBlinking()) {
            state.blinking = state.knockback = state.scheduled = false;
            state.knockbackTimer = -1;
        }
        return true;
    }
    bool Keep(event::PacketEvent& e) {
        JniAttach attach;
        if (!attach) return false;
        jobject p = attach.env->NewGlobalRef(e.packet);
        if (!p) { if (attach.env->ExceptionCheck()) attach.env->ExceptionClear(); return false; }
        packets.push_back(p); e.Cancel(); return true;
    }
    void Packet(event::PacketEvent& e) {
        if (e.type != event::EventType::Incoming || !e.packet || e.IsCancelled()) return;
        std::lock_guard lock(mutex);
        if (!active) return;
        const int kind = lunar::PredictionIncomingKind(e.packet);
        if (kind == 5 || kind == 2) { Reset(kind == 2); world.Reset(); return; }
        if (!Ready()) return;
        if (kind == 6 || kind == 7) {
            // Preserve FIFO before processing corrections or transaction replies.
            Drain(true);
            return;
        }
        if (ReleaseExpiredDelay()) return;
        if (state.delayFlag) {
            if (kind == 1 || kind == 4) return;
            // Fail open on allocation failure or an unbounded server stream.
            if (packets.size() < 8192 && Keep(e)) return;
            Drain(true); return;
        }
        if (kind == 3) state.Hurt();
        int id = 0, x = 0, y = 0, z = 0, self = 0;
        auto player = lunar::PlayerEntity();
        if (!player || !java::EntityId(player, self) || !lunar::ReadMotionPacket(e.packet, id, x, y, z) || id != self) return;
        const auto* stuck = ModuleManager::Get().Find("Stuck");
        const bool delay = state.ShouldDelayVelocity(settings, lunar::PlayerOnGround(), lunar::PredictionLiquidWeb());
        if (delay && !(stuck && stuck->isEnabled()) && Keep(e)) {
            state.BeginDelay(); delayStarted = GetTickCount64(); ownsIncoming.store(true);
            if (settings.debug) LogTo("Prediction: incoming delay active");
        } else state.PassedVelocity();
        Publish();
    }
    Runtime() {
        auto& bus = event::EventBus::Get();
        packet = bus.Subscribe<event::PacketEvent>([this](auto& e) { Packet(e); }, event::Priority::High);
        pre = bus.Subscribe<event::PlayerTickEvent>([this](auto& e) {
            std::lock_guard lock(mutex);
            if (e.IsCancelled() || !Ready()) return;
            // UpdateEvent yaw/pitch in Leader remain the PRE last-reported
            // snapshot even when Rotate changes its separate newYaw/newPitch.
            if (!lunar::PlayerReportedRotation(reportedRotation.yaw, reportedRotation.pitch)) return;
            IO io{*this}; state.Pre(settings, io); Publish();
        }, event::Priority::High);
        post = bus.Subscribe<event::PostPlayerTickEvent>([this](auto&) {
            std::lock_guard lock(mutex);
            if (!Ready()) return;
            IO io{*this}; state.Post(settings, io); Publish();
        }, event::Priority::High);
        applied = bus.Subscribe<event::AppliedVelocityEvent>([this](auto& e) {
            std::lock_guard lock(mutex);
            if (active) state.Applied(settings, e.x, e.y, e.z, lunar::PlayerOnGround());
        });
        input = bus.Subscribe<event::MoveInputEvent>([this](auto& e) {
            std::lock_guard lock(mutex);
            MoveInput(e);
        }, event::Priority::Low);
        living = bus.Subscribe<event::LivingUpdateEvent>([this](auto&) {
            std::lock_guard lock(mutex);
            if (active) { IO io{*this}; state.Living(io); }
        });
        tick = bus.Subscribe<event::GameTickEvent>([this](auto& e) {
            std::lock_guard lock(mutex);
            // Release last tick's request before KillAura submits its new aim.
            // This also cleans up a disable requested on the render thread.
            if (e.type == event::EventType::Pre) ReleaseRotation();
            if (!active) return;
            if (e.type == event::EventType::Pre) { if (Ready()) ReleaseExpiredDelay(); return; }
            if (state.delayFlag && state.delayedTicks < 2147483647) ++state.delayedTicks;
        }, event::Priority::High);
    }
};
std::unique_ptr<Runtime> runtime;
}
void Initialize() { if (!runtime) runtime = std::make_unique<Runtime>(); }
void Configure(bool enabled, const PredictionSettings& settings) {
    if (!runtime) return;
    std::lock_guard lock(runtime->mutex);
    enabled = enabled && compat::IsLegacyLunar();
    if (runtime->active != enabled || (enabled &&
        (runtime->settings.reduceMode != settings.reduceMode || runtime->settings.reduce != settings.reduce)))
        runtime->Reset(true);
    runtime->settings = settings;
    if (runtime->active && !settings.delay) runtime->Drain(true);
    runtime->active = enabled;
    runtime->Publish();
}
void Shutdown() {
    if (!runtime) return;
    { std::lock_guard lock(runtime->mutex); runtime->active = false; runtime->Reset(false); runtime->ReleaseRotation(); }
    runtime.reset();
}
bool OwnsIncoming() { return ownsIncoming.load(); }
bool StoppedBlock() { return stopped.load(); }
bool CancelsAura() { return cancel.load(); }
bool ConsumeExtraAttack(int& mode) {
    if (!runtime) return false;
    std::lock_guard lock(runtime->mutex);
    if (!runtime->active || !runtime->state.extraAttacked) return false;
    runtime->state.extraAttacked = false;
    mode = runtime->settings.reduceMode;
    return true;
}
void ClearExtraAttack() {
    if (!runtime) return;
    std::lock_guard lock(runtime->mutex);
    runtime->state.extraAttacked = false;
}
}
