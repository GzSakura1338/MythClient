#include "minecraft/modules/combat/antikb/mix/MixRuntime.h"
#include "minecraft/modules/combat/autoblock/AutoBlockRuntime.h"
#include "minecraft/modules/combat/killaura/KillAura.h"
#include "minecraft/compat/lunar189/LunarRuntime.h"
#include "minecraft/compat/Profile.h"
#include "minecraft/events/EventBus.h"
#include "minecraft/events/GameEvents.h"
#include "minecraft/java/api/game_network.h"
#include "minecraft/utils/rotation/RayCastUtil.h"
#include <atomic>
#include <deque>
#include <memory>
#include <mutex>

namespace nova::module::mix {
namespace {
namespace lunar = compat::lunar;
std::atomic<bool> ownsIncoming{false};
struct Runtime {
    // Incoming callbacks run on the client thread; replay reenters AppliedVelocity.
    std::recursive_mutex mutex;
    Settings settings;
    Velocity state;
    bool active = false, replaying = false, badActions = false;
    std::deque<jobject> packets;
    java::EntityHandle world, player;
    event::Subscription packet, pre, post, applied, living, tick;

    struct IO {
        Runtime& r;
        java::EntityHandle target;
        bool OnGround() const { return lunar::PlayerOnGround(); }
        bool LiquidWeb() const { return lunar::PredictionLiquidWeb(); }
        bool ForwardPressed() const { return lunar::PredictionForwardPressed(); }
        bool JumpBoost() const { return lunar::PredictionJumpBoost(); }
        void Jump() { lunar::PredictionJump(); }
        bool Sprinting() const { bool value = false; return lunar::PlayerSprinting(value) && value; }
        bool Blocking() const { return lunar::HypixelPlayerBlocking(); }
        bool CanDelay() const {
            return OnGround() && (!killaura::IsEnabled() || !autoblock::Active() || !killaura::HasBlockTarget());
        }
        bool CanReduce() const {
            // The reference's external canReduce producer was not supplied.
            // Do not insert raw attacks into Lag New's owned attack/use cycle.
            if (autoblock::AttackCycleActive() && !autoblock::HypixelActive()) return false;
            return !killaura::IsEnabled() || autoblock::VelocityCanReduce(0);
        }
        bool BadPackets() const {
            return r.badActions || lunar::BlockInputAttacked() || !lunar::HeldSlotChangeReady() || lunar::HypixelPlayerBusy();
        }
        bool Target(bool airPush) {
            unsigned types = 0;
            if (lunar::AttackTargetEntity(target) && java::EntityTypes(target, types) && (types & java::kTypeLiving))
                return true;
            target.Reset();
            if (!airPush) return false;
            auto self = lunar::PlayerEntity();
            rotation::Vec3 eye;
            std::vector<java::EntityHandle> entities;
            if (!self || !self.EyePosition(1.f, eye) || !lunar::WorldLoadedEntities(entities)) return false;
            double farthest = 0;
            for (auto& entity : entities) {
                if (!entity || entity.IsSame(self) || !java::EntityTypes(entity, types) || !(types & java::kTypeLiving) ||
                    !(types & (java::kTypePlayer | java::kTypeMonster | java::kTypeAnimal | java::kTypeVillager))) continue;
                const double distance = rotation::EntityDistance(eye, entity);
                if (distance >= 11 && distance > farthest) { farthest = distance; target = std::move(entity); }
            }
            return bool(target);
        }
        void Attack() {
            // Raw C0A -> C02, not vanilla attackEntity (which would apply friction twice).
            if (!lunar::SendSwingPacket() || !lunar::SendAttackPacket(target)) return;
            rotation::Vec3 motion;
            if (lunar::PlayerDeltaMovement(motion))
                lunar::SetPlayerDeltaMovement(motion.x * .6, motion.y, motion.z * .6);
            lunar::SetPlayerSprinting(false);
        }
        void Release() { r.Drain(true); }
    };

    void Drain(bool replay) {
        if (packets.empty()) { ownsIncoming.store(false); return; }
        JniAttach attach;
        if (!attach) return;
        replaying = true;
        while (!packets.empty()) {
            jobject value = packets.front(); packets.pop_front();
            if (replay) {
                // Replay bypasses PacketEvent. Preserve the hurt marker for a
                // later velocity without recapturing this already owned batch.
                if (lunar::PredictionIncomingKind(value) == 3) state.Hurt();
                lunar::HandleIncomingPacket(value);
            }
            attach.env->DeleteGlobalRef(value);
        }
        replaying = false;
        ownsIncoming.store(false);
    }
    void Reset(bool replay) { Drain(replay); state = {}; badActions = false; }
    bool Ready() {
        if (!active) return false;
        auto nextWorld = lunar::LevelEntity(), nextPlayer = lunar::PlayerEntity();
        if (!nextWorld || !nextPlayer || lunar::EntityDead(nextPlayer.raw())) {
            Reset(false); world.Reset(); player.Reset(); return false;
        }
        if ((world && !world.IsSame(nextWorld)) || (player && !player.IsSame(nextPlayer))) Reset(false);
        world = std::move(nextWorld); player = std::move(nextPlayer);
        return true;
    }
    bool Keep(event::PacketEvent& e) {
        JniAttach attach;
        if (!attach) return false;
        jobject ref = attach.env->NewGlobalRef(e.packet);
        if (!ref) { if (attach.env->ExceptionCheck()) attach.env->ExceptionClear(); return false; }
        packets.push_back(ref); e.Cancel(); ownsIncoming.store(true); return true;
    }
    void Packet(event::PacketEvent& e) {
        if (!e.packet || e.IsCancelled()) return;
        std::lock_guard lock(mutex);
        if (!active || replaying) return;
        if (e.type == event::EventType::Outgoing) {
            if (java::PacketIsMovePlayer(e.packet)) badActions = false;
            else if (java::PacketIsInteract(e.packet) || java::PacketIsUseItem(e.packet) ||
                     java::PacketIsUseItemOn(e.packet) || java::PacketIsPlayerAction(e.packet) ||
                     java::PacketIsContainerClose(e.packet)) badActions = true;
            return;
        }
        if (e.type != event::EventType::Incoming) return;
        const int kind = lunar::PredictionIncomingKind(e.packet);
        if (kind == 2 || kind == 5) { Reset(false); world.Reset(); player.Reset(); return; }
        if (!Ready()) return;
        // Corrections must follow older queued packets and invalidate reduction/jump.
        if (kind == 6) { Reset(true); return; }
        // Keep transaction replies responsive without letting them overtake this FIFO.
        if (kind == 7 && state.delaying) { Drain(true); state.Finish(settings); return; }
        if (state.delaying) {
            if (kind == 1) return;
            if (packets.size() < 8192 && Keep(e)) return;
            Drain(true); state.Finish(settings); // allocation/queue limit: fail open
        }
        if (kind == 3) state.Hurt();
        int id = 0, x = 0, y = 0, z = 0, self = 0;
        if (!java::EntityId(player, self) || !lunar::ReadMotionPacket(e.packet, id, x, y, z) || id != self) return;
        IO io{*this};
        if (state.VelocityPacket(settings, y, io) && Keep(e)) state.Begin();
        else state.Passed(settings);
    }
    Runtime() {
        auto& bus = event::EventBus::Get();
        packet = bus.Subscribe<event::PacketEvent>([this](auto& e) { Packet(e); }, event::Priority::High);
        // Native input PRE precedes use-item physics and C03. PlayerTick PRE is
        // later in this bridge and must not be used to inject a late C02.
        pre = bus.Subscribe<event::PrePlayerInteractEvent>([this](auto&) {
            std::lock_guard lock(mutex);
            if (Ready()) { IO io{*this}; state.Pre(settings, io); }
        }, event::Priority::High);
        post = bus.Subscribe<event::PostPlayerTickEvent>([this](auto&) {
            std::lock_guard lock(mutex);
            if (Ready()) { IO io{*this}; state.Post(settings, io); }
        }, event::Priority::High);
        applied = bus.Subscribe<event::AppliedVelocityEvent>([this](auto&) {
            std::lock_guard lock(mutex); if (active) state.Applied(settings);
        });
        living = bus.Subscribe<event::LivingUpdateEvent>([this](auto&) {
            std::lock_guard lock(mutex);
            if (Ready()) { IO io{*this}; state.Living(settings, io); }
        });
        tick = bus.Subscribe<event::GameTickEvent>([this](auto& e) {
            std::lock_guard lock(mutex);
            if (e.type == event::EventType::Pre) (void)Ready();
        }, event::Priority::High);
    }
};
std::unique_ptr<Runtime> runtime;
}
void Initialize() { if (!runtime) runtime = std::make_unique<Runtime>(); }
void Configure(bool enabled, const Settings& settings) {
    if (!runtime) return;
    std::lock_guard lock(runtime->mutex);
    enabled = enabled && compat::IsLegacyLunar();
    if (runtime->active != enabled) {
        // Verify identities before replaying on disable/mode change.
        const bool replay = runtime->active && runtime->Ready();
        runtime->Reset(replay);
    }
    runtime->settings = settings;
    runtime->active = enabled;
}
void Shutdown() {
    if (!runtime) return;
    { std::lock_guard lock(runtime->mutex); runtime->active = false; runtime->Reset(false); }
    runtime.reset();
}
bool OwnsIncoming() { return ownsIncoming.load(); }
}
