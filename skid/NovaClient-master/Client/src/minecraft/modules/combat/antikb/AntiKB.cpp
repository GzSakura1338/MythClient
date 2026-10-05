#include "minecraft/modules/misc/antibot/AntiBot.h"
#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <deque>
#include <format>
#include <memory>
#include <mutex>
#include <string>

#include "minecraft/events/EventBus.h"
#include "minecraft/events/GameEvents.h"
#include "minecraft/compat/Profile.h"
#include "minecraft/compat/lunar189/LunarRuntime.h"
#include "minecraft/modules/Module.h"
#include "minecraft/modules/Registry.h"
#include "minecraft/modules/combat/antikb/AntiKB.h"
#include "minecraft/modules/combat/antikb/mix/MixModuleBase.h"
#include "minecraft/modules/combat/antikb/mix/MixRuntime.h"
#include "minecraft/modules/combat/antikb/prediction/PredictionRuntime.h"
#include "minecraft/modules/combat/killaura/KillAura.h"
#include "minecraft/modules/misc/teams/Teams.h"
#include "minecraft/modules/player/scaffold/Scaffold.h"
#include "ui/clickgui/ClickGui.h"
#include "minecraft/java/api/base.h"
#include "minecraft/java/api/game_click.h"
#include "minecraft/java/api/game_entity.h"
#include "minecraft/java/api/game_network.h"
#include "minecraft/java/api/game_place.h"
#include "minecraft/java/api/game_player.h"
#include "minecraft/utils/rotation/Mth.h"
#include "minecraft/utils/rotation/RotationManager.h"

namespace nova::module {
namespace {

std::atomic<bool> g_reduceDelay{false};
std::atomic<bool> g_watchdogQueue{false};
std::atomic<bool> g_watchdogRecovering{false};
std::atomic<unsigned> g_watchdogReplayDepth{0};
std::atomic<bool> g_enabled{false};
std::atomic<bool> g_delayWindow{false};
std::atomic<int> g_attackQueue{0};
std::atomic<float> g_yaw{0};

// Epsilon Velocity's Delay and Reduce share no pending state across mode changes.
class AntiKbModule final : public MixModuleBase {
public:
    AntiKbModule() {
        mode_ = &addMode("Mode", 0, {"Delay", "Reduce", "Prediction", "Watchdog Reduce", "Mix"},
            BuildPredictionSettings([this] { return mode_ && mode_->get() == 2; }));
        AddMixSettings([this] { return mode_->get() == 4; });
        mix::Initialize();
        prediction::Initialize();
        auto delay = [this] { return mode_->get() == 0; };
        auto reduce = [this] { return mode_->get() == 1; };
        auto whole = [](double value) { return std::format("{:.0f}", value); };
        delayTicks_ = &addNumber("Delay Ticks", 1, 5, 1, 3, {},
            whole, delay);
        jumpReset_ = &addBool("Jump Reset", false, {}, delay);
        auto delayTurnTicks = subNumber("Turn Ticks", 1, 10, 1, 1, whole);
        delayRotationTicks_ = static_cast<Properties::NumberProp*>(delayTurnTicks.getProp());
        delayRotation_ = &addBool("Delay Rotation", false, {delayTurnTicks}, delay);
        attackCounts_ = &addNumber("Attack Counts", 1, 5, 1, 1, {}, whole, reduce);
        sprintTicks_ = &addNumber("Sprint Ticks", 1, 5, 1, 3, {}, whole, reduce);
        maxDelay_ = &addNumber("Max Delay", 0, 1000, 50, 1000, {}, whole, reduce);
        swingHand_ = &addBool("Swing Hand", false, {}, reduce);
        auto watchdog = [this] { return mode_->get() == 3; };
        watchdogGround_ = &addBool("Cancel On Ground", true, {}, watchdog);
        watchdogAttack_ = &addBool("Cancel On Attack", true, {}, watchdog);
        // Kept for reference/config parity: Rise declares but never reads it.
        watchdogExplosions_ = &addBool("Cancel Explosions", true, {}, watchdog);
        auto attempts = subNumber("Attack Attempts", 1, 5, 1, 3, whole);
        watchdogAttempts_ = static_cast<Properties::NumberProp*>(attempts.getProp());
        auto interval = subNumber("Attack Interval (ms)", 50, 1000, 50, 50, whole);
        watchdogInterval_ = static_cast<Properties::NumberProp*>(interval.getProp());
        watchdogAutoAttack_ = &addBool("Auto Attack", false, {attempts, interval}, watchdog);
        watchdogForceSlowdown_ = &addBool("Force 0.6", false, {}, watchdog);
        auto rotationTicks = subNumber("Rotation Ticks", 0, 5, 1, 3, whole);
        watchdogRotationTicks_ = static_cast<Properties::NumberProp*>(rotationTicks.getProp());
        watchdogRotation_ = &addBool("Rotation", true, {rotationTicks}, watchdog);
        auto& bus = event::EventBus::Get();
        slowdown_ = bus.Subscribe<event::AttackSlowdownEvent>(
            [this](event::AttackSlowdownEvent& e) {
                if (!isEnabled() || mode_->get() != 3 || !watchdogForceSlowdown_->get() ||
                    !compat::IsLegacyLunar() || e.hurtTime <= 0) return;
                // Override KeepSprint's coefficient, not the already-reduced
                // motion. Vanilla applies this once to X/Z and preserves Y.
                e.slowdown = 0.6;
            }, event::Priority::Lowest);
        packet_ = bus.Subscribe<event::PacketEvent>(
            [this](event::PacketEvent& e) { OnPacket(e); });
        tick_ = bus.Subscribe<event::PlayerTickEvent>(
            [this](event::PlayerTickEvent&) { OnPlayerTick(); }, event::Priority::Low);
        input_ = bus.Subscribe<event::MoveInputEvent>(
            [this](event::MoveInputEvent& e) { OnInput(e); }, event::Priority::Low);
        motion_ = bus.Subscribe<event::MotionEvent>([this](event::MotionEvent& e) {
            if (e.type != event::EventType::Pre || !isEnabled()) return;
            SyncMode();
            if (mode_->get() == 3 && watchdogGround_->get() && java::PlayerOnGround())
                ReleaseWatchdog();
        });
        respawn_ = bus.Subscribe<event::RespawnEvent>([this](event::RespawnEvent&) {
            if (isEnabled() && mode_->get() == 3) {
                Reset(false);
            }
        });
        world_ = bus.Subscribe<event::GameTickEvent>(
            [this](event::GameTickEvent& e) {
                if (!isEnabled() || e.type != event::EventType::Pre) return;
                SyncMode();
                if (!java::PlayerEntity()) {
                    Reset(false);
                }
                else {
                    OnWatchdogTick();
                    OnDelayTick();
                    OnReduceTick();
                }
            }, event::Priority::Highest);
    }

    ~AntiKbModule() override {
        packet_.Reset();
        tick_.Reset();
        input_.Reset();
        world_.Reset();
        motion_.Reset();
        respawn_.Reset();
        slowdown_.Reset();
        if (active_) java::SetIncomingPacketsWanted(false);
        g_enabled.store(false);
        Reset(false);
        prediction::Shutdown();
        mix::Shutdown();
    }

    std::string suffix() const override {
        std::lock_guard lock(packetLock_);
        if (mode_->get() == 2) return "Prediction";
        if (mode_->get() == 3) return "Watchdog Reduce";
        if (mode_->get() == 4) return "Mix";
        if (mode_->get() == 1)
            return reducing_ ? std::format("Reduce {}ms", NowMillis() - startDelay_) : "Reduce";
        return std::format("Delay {}t", ticksRemaining_);
    }

private:
    void onUpdate() override {
        SyncMode();
        if (!watchdogAutoAttack_->get()) {
            std::lock_guard lock(packetLock_);
            ClearWatchdogAttacks();
        }
        prediction::Configure(isEnabled() && mode_->get() == 2, ReadPredictionSettings());
        mix::Configure(isEnabled() && mode_->get() == 4, ReadMixSettings());
    }

    void SyncMode() {
        std::deque<jobject> pending;
        bool replay = false;
        {
            std::lock_guard lock(packetLock_);
            if (currentMode_ == mode_->get()) return;
            replay = currentMode_ == 3;
            currentMode_ = mode_->get();
            ClearState();
            ClearWatchdogContext();
            pending.swap(packets_);
        }
        // Epsilon's mode callback discards the previous mode's queue.
        Finish(std::move(pending), replay, replay);
        prediction::Configure(isEnabled() && mode_->get() == 2, ReadPredictionSettings());
        mix::Configure(isEnabled() && mode_->get() == 4, ReadMixSettings());
    }

    void ClearState() {
        ClearWatchdogAttacks();
        ClearWatchdogRotation();
        ticksRemaining_ = 0;
        delayYawValid_ = false;
        delayRotationSubmitted_ = false;
        delayYaw_ = 0;
        delayRotateLeft_ = 0;
        delayRotateYaw_ = 0;
        g_delayWindow.store(false);
        lastLag_ = startDelay_ = 0;
        reducing_ = jump_ = false;
        sprintQueue_ = 0;
        g_reduceDelay.store(false);
        g_watchdogQueue.store(false);
        g_attackQueue.store(0);
        g_yaw.store(0);
    }

    static long long NowMillis() {
        return std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now().time_since_epoch()).count();
    }

    static bool IsSelfKnockback(jobject packet) {
        if (!java::IsMotionPacket(packet)) return false;
        int id = 0, x = 0, y = 0, z = 0, selfId = 0;
        const auto self = java::PlayerEntity();
        return self && java::EntityId(self, selfId) &&
            java::ReadMotionPacket(packet, id, x, y, z) &&
            id == selfId && y > 0 && (x != 0 || z != 0);
    }

    // Game calls run outside packetLock_: replay may dispatch more module events.
    static void Finish(std::deque<jobject> pending, bool replay, bool watchdog = false) {
        if (pending.empty()) return;
        JniAttach attach;
        if (!attach) return;
        // Rise uses PacketUtil.receive(), which republishes PacketReceiveEvent.
        // Its flushing guard excludes this module for the entire detached batch.
        // Keep ownership through callbacks so Backtrack cannot seize the replay.
        struct Flushing {
            bool active;
            explicit Flushing(bool value) : active(value) { if (active) ++g_watchdogReplayDepth; }
            ~Flushing() { if (active) --g_watchdogReplayDepth; }
        } flushing(replay && watchdog);
        for (jobject packet : pending) {
            bool process = replay;
            if (replay && watchdog) {
                event::PacketEvent e(event::EventType::Incoming, packet);
                try { process = !event::EventBus::Get().Post(e); }
                catch (...) {
                    // Fail open if a native listener throws, retaining FIFO and
                    // reference cleanup for the rest of the detached batch.
                    LogTo("AntiKB Watchdog Reduce: replay listener failed; passing packet");
                }
            }
            if (process) java::HandleIncomingPacket(packet);
            attach.env->DeleteGlobalRef(packet);
        }
    }

    void Reset(bool replay) {
        std::deque<jobject> pending;
        bool watchdog = false;
        {
            std::lock_guard lock(packetLock_);
            watchdog = currentMode_ == 3;
            ClearState();
            ClearWatchdogContext();
            pending.swap(packets_);
        }
        Finish(std::move(pending), replay, watchdog);
    }

    void onEnable() override {
        Reset(false);
        {
            std::lock_guard lock(packetLock_);
            active_ = true;
            g_enabled.store(true);
            currentMode_ = mode_->get();
            g_watchdogRecovering.store(currentMode_ == 3);
        }
        java::SetIncomingPacketsWanted(true);
        prediction::Configure(mode_->get() == 2, ReadPredictionSettings());
        mix::Configure(mode_->get() == 4, ReadMixSettings());
    }

    void onDisable() override {
        {
            std::lock_guard lock(packetLock_);
            active_ = false;
            g_enabled.store(false);
        }
        java::SetIncomingPacketsWanted(false);
        Reset(true);
        prediction::Configure(false, ReadPredictionSettings());
        mix::Configure(false, ReadMixSettings());
    }

    void OnPacket(event::PacketEvent& e) {
        if (mode_->get() == 3 && e.type == event::EventType::Outgoing && e.packet) {
            // Rise releases on the ATTACK event even if another listener has
            // already cancelled/queued it. INTERACT does not release the queue.
            if (isEnabled() && compat::IsLegacyLunar() && watchdogAttack_->get() &&
                compat::lunar::IsAttackPacket(e.packet)) ReleaseWatchdog();
            return;
        }
        if (e.type != event::EventType::Incoming || !e.packet || g_watchdogReplayDepth.load() != 0) return;
        SyncMode();
        std::unique_lock lock(packetLock_);
        if (!active_) return;
        if (currentMode_ == 2 || currentMode_ == 4) return;
        if (currentMode_ == 3) {
            OnWatchdogPacket(e, lock);
            return;
        }
        if (e.IsCancelled()) return;
        if (java::IsDisconnectPacket(e.packet)) {
            std::deque<jobject> pending;
            ClearState();
            pending.swap(packets_);
            lock.unlock();
            Finish(std::move(pending), false);
            return;
        }
        if (!java::PlayerEntity()) return;

        const long long now = NowMillis();
        if (currentMode_ == 1) {
            // Queue only the reference's six packet families. In particular,
            // explosions pass through and force a flush on the next client tick.
            if (reducing_ && java::IsReduceDelayPacket(e.packet)) Keep(e);
            if (java::IsPlayerPositionPacket(e.packet) || java::IsExplodePacket(e.packet))
                lastLag_ = now;
            int id = 0, x = 0, y = 0, z = 0, selfId = 0;
            const auto self = java::PlayerEntity();
            if (java::IsMotionPacket(e.packet) && java::EntityId(self, selfId) &&
                java::ReadMotionPacket(e.packet, id, x, y, z) && id == selfId) {
                if (now - lastLag_ >= 100 && y > 0 && (x != 0 || z != 0) && !reducing_ && Keep(e)) {
                    reducing_ = true;
                    startDelay_ = now;
                    g_reduceDelay.store(true);
                }
                // The reference updates yaw even during the lag guard and for
                // own motion that does not qualify as a new knockback.
                g_yaw.store(rotation::mth::WrapDegrees(static_cast<float>(
                    std::atan2(static_cast<double>(z), static_cast<double>(x)) *
                    57.29577951308232 + 90.0)));
            }
            return;
        }
        if (java::IsPlayerPositionPacket(e.packet) || java::IsExplodePacket(e.packet))
            lastLag_ = now;

        // Ownership includes the pending-release frame and the replay itself.
        // Otherwise fresh network packets can overtake the detached replay batch.
        if (g_delayWindow.load()) {
            Keep(e);
            return;
        }
        if (now - lastLag_ >= 100 && IsSelfKnockback(e.packet) && Keep(e)) {
            ticksRemaining_ = std::clamp(static_cast<int>(delayTicks_->get()), 1, 5);
            int id=0,x=0,y=0,z=0;
            delayYawValid_ = java::ReadMotionPacket(e.packet,id,x,y,z) && (x!=0 || z!=0);
            // Velocity's +90 yaw faces the source, opposite the incoming X/Z.
            delayYaw_ = delayYawValid_ ? rotation::mth::WrapDegrees(static_cast<float>(
                std::atan2(static_cast<double>(z),static_cast<double>(x))*57.29577951308232+90.0)) : 0;
            delayRotationSubmitted_ = false;
            g_delayWindow.store(true);
            // Hold the triggering velocity too: knockback begins on replay,
            // after the configured player ticks, not when this window starts.
        }
    }

    void OnPlayerTick() {
        SyncMode();
        if (!isEnabled()) return;
        if (mode_->get() == 3) { OnWatchdogAttack(); return; }
        if (mode_->get() >= 2) return;
        if (mode_->get() == 1) {
            if (g_attackQueue.load() < 1) return;
            java::EntityHandle target = LogicalTarget();
            if (target && target.IsPlayer() && target.IsAlive() && !antibot::IsBot(target)) {
                const auto swing = [this] {
                    if (swingHand_->get()) java::SwingHand();
                    else java::SendSwingPacket();
                };
                // Vanilla 1.8 swings before attacking; 1.9+ uses the reverse.
                const bool legacy = compat::IsLegacyLunar();
                if (legacy) swing();
                java::AttackEntity(target);
                if (!legacy) swing();
            }
            // A miss consumes a slot too; never defer it through AutoBlock.
            int left = g_attackQueue.load();
            while (left > 0 && !g_attackQueue.compare_exchange_weak(left, left - 1)) {}
            return;
        }
        std::lock_guard lock(packetLock_);
        if (active_ && ticksRemaining_ > 0) --ticksRemaining_;
        // Never replay here: runTick has already handled attacks before entering
        // EntityPlayerSP.onUpdate. A velocity applied here misses attack slowdown,
        // and transaction replies split interaction packets from their movement.
    }

    void ReleaseWatchdog() {
        if (g_watchdogReplayDepth.load() != 0) return;
        std::deque<jobject> pending;
        std::uint64_t generation = 0;
        float releasedYaw = 0;
        bool hasReleasedYaw = false;
        {
            std::unique_lock lock(packetLock_);
            if (!active_ || currentMode_ != 3 || !WatchdogContext(lock) || packets_.empty()) return;
            pending.swap(packets_);
            g_watchdogQueue.store(false);
            generation = watchdogAttackGeneration_;
            releasedYaw = watchdogPendingYaw_;
            hasReleasedYaw = watchdogPendingYawValid_;
            watchdogPendingYawValid_ = false;
        }
        // Rise v(): detach at the trigger and synchronously receive each packet.
        // AutoBlock observes our read-only ownership; this module never drives it.
        Finish(std::move(pending), true, true);
        {
            std::lock_guard lock(packetLock_);
            if (active_ && currentMode_ == 3 && watchdogReady_ && generation == watchdogAttackGeneration_) {
                const double ticks = watchdogRotationTicks_->get();
                watchdogRotateLeft_ = watchdogRotation_->get() && hasReleasedYaw
                    ? (std::isfinite(ticks) ? static_cast<int>(std::clamp(ticks, 0., 5.)) : 3) : 0;
                watchdogRotateYaw_ = releasedYaw;
            }
            // Replay can cause a correction, disable or world change. Never arm
            // attacks for that invalidated batch, or refill our own ongoing burst.
            if (active_ && currentMode_ == 3 && watchdogReady_ && watchdogAutoAttack_->get() &&
                generation == watchdogAttackGeneration_ && watchdogAttacks_ == 0 && !watchdogIssuingAttack_) {
                const double count = watchdogAttempts_->get();
                watchdogAttacks_ = std::isfinite(count) ? static_cast<int>(std::clamp(count, 1., 5.)) : 3;
                watchdogNextAttack_ = 0;
            }
        }
    }

    // Called under packetLock_. Separate from Reduce's exported attack queue:
    // publishing a queued AutoBlock request there makes AutoBlock wait on us.
    void ClearWatchdogAttacks() {
        watchdogAttacks_ = 0;
        watchdogNextAttack_ = 0;
        ++watchdogAttackGeneration_;
    }

    void ClearWatchdogRotation() {
        watchdogRotateLeft_ = 0;
        watchdogRotateYaw_ = watchdogPendingYaw_ = 0;
        watchdogPendingYawValid_ = false;
    }

    void OnWatchdogAttack() {
        {
            std::unique_lock lock(packetLock_);
            if (!active_ || currentMode_ != 3 || !compat::IsLegacyLunar()) return;
            if (!WatchdogContext(lock)) return;
            auto self = java::PlayerEntity();
            if (!watchdogAutoAttack_->get() || !watchdogReady_ || !self || !self.IsAlive() ||
                java::ScreenOpen() || clickgui::IsOpen() || scaffold::IsEnabled()) {
                ClearWatchdogAttacks(); return;
            }
            // Resume only after a new held velocity has followed its normal release path.
            if (g_watchdogQueue.load() || g_watchdogReplayDepth.load() != 0 || watchdogAttacks_ == 0) return;
            const auto now = NowMillis();
            if (now < watchdogNextAttack_) return;
            const double interval = watchdogInterval_->get();
            const auto ms = std::isfinite(interval) ? static_cast<long long>(std::clamp(interval, 50., 1000.)) : 50LL;
            watchdogNextAttack_ = now + ms; // Never catch up with a burst after a stalled tick.
            --watchdogAttacks_; // Like Velocity, a miss also consumes an attempt.
            watchdogIssuingAttack_ = true;
        }
        struct Issuing {
            AntiKbModule& owner;
            ~Issuing() { std::lock_guard lock(owner.packetLock_); owner.watchdogIssuingAttack_ = false; }
        } issuing{*this};
        // Game calls dispatch packet events, so never hold packetLock_ here.
        java::EntityHandle target;
        if (!java::AttackTargetEntity(target) || !target.IsPlayer() || !target.IsAlive() ||
            teams::IsTeammate(target) || antibot::IsBot(target)) return;
        if (killaura::QueueAutoBlockAttack()) return;
        java::AttackAndSwing(target);
    }

    void OnWatchdogTick() {
        bool rotate = false;
        float yaw = 0;
        {
            std::unique_lock lock(packetLock_);
            if (!active_ || currentMode_ != 3 || !compat::IsLegacyLunar()) return;
            if (!WatchdogContext(lock)) return;
            if (!watchdogReady_) {
                int ticks = 0;
                // Join packets can precede the first tick, and onGround alone
                // is unreliable during spawn. Let vanilla complete the initial
                // fall (or correction), then observe three grounded tick heads.
                if (watchdogPlayer_.TickCount(ticks) && ticks >= 20 && java::PlayerOnGround())
                    ++watchdogGroundTicks_;
                else watchdogGroundTicks_ = 0;
                if (watchdogGroundTicks_ >= 3) {
                    watchdogReady_ = true;
                    g_watchdogRecovering.store(false);
                }
            }
            if (!watchdogRotation_->get() || !watchdogReady_ || !watchdogPlayer_.IsAlive() ||
                java::ScreenOpen() || clickgui::IsOpen()) {
                watchdogRotateLeft_ = 0;
            } else if (watchdogRotateLeft_ > 0) {
                // Velocity spends this window even when another module owns aim.
                --watchdogRotateLeft_;
                rotate = !killaura::HasTargets() && !scaffold::IsEnabled();
                yaw = watchdogRotateYaw_;
            }
        }
        // Same request as Velocity: knockback yaw, current pitch, speed 180,
        // normal rotation priority/movement correction. Never rotate on Netty.
        if (rotate) rotation::SetRotations({yaw, rotation::GetRotation().pitch}, 180.0);
    }

    void ClearWatchdogContext() {
        ClearWatchdogAttacks();
        ClearWatchdogRotation();
        watchdogPlayer_.Reset();
        watchdogWorld_.Reset();
        watchdogReady_ = false;
        watchdogGroundTicks_ = 0;
        g_watchdogRecovering.store(active_ && currentMode_ == 3);
    }

    // Caller owns packetLock_. Identity, not entity id, distinguishes worlds
    // and respawns even when no intervening tick observes a null player.
    bool WatchdogContext(std::unique_lock<std::mutex>& lock) {
        auto self = java::PlayerEntity();
        auto world = java::LevelEntity();
        if (self && world && watchdogPlayer_.IsSame(self) && watchdogWorld_.IsSame(world)) return true;
        std::deque<jobject> pending;
        ClearState();
        ClearWatchdogContext();
        watchdogPlayer_ = std::move(self);
        watchdogWorld_ = std::move(world);
        pending.swap(packets_);
        lock.unlock();
        // Discard old-world input; Slinky owns its own output lifecycle.
        Finish(std::move(pending), false);
        return false;
    }

    void OnWatchdogPacket(event::PacketEvent& e, std::unique_lock<std::mutex>& lock) {
        if (!compat::IsLegacyLunar()) return;
        if (java::IsDisconnectPacket(e.packet) || java::IsRespawnPacket(e.packet) ||
            compat::lunar::IsPacket(e.packet, compat::lunar::PacketKind::Join)) {
            std::deque<jobject> pending;
            ClearState(); ClearWatchdogContext(); pending.swap(packets_);
            lock.unlock();
            Finish(std::move(pending), false);
            return;
        }
        if (!WatchdogContext(lock)) return;
        if (java::IsPlayerPositionPacket(e.packet)) {
            // Apply the entire preceding FIFO BEFORE vanilla applies S08. Its
            // absolute axes clear velocity; a later S12 replay would undo that.
            // Preserve S32 acknowledgements instead of dropping confirmations.
            std::deque<jobject> pending;
            ClearState(); pending.swap(packets_);
            watchdogReady_ = false;
            watchdogGroundTicks_ = 0;
            g_watchdogRecovering.store(true);
            lock.unlock();
            Finish(std::move(pending), true, true);
            return;
        }
        if (!watchdogReady_) return;
        const auto self = java::PlayerEntity();
        if (!self) return;
        bool keep = false;
        bool velocity = false;
        int motionX = 0, motionZ = 0;
        if (java::IsMotionPacket(e.packet)) {
            int id=0,x=0,y=0,z=0,selfId=0;
            keep = java::EntityId(self,selfId) && java::ReadMotionPacket(e.packet,id,x,y,z) && id==selfId;
            velocity = keep;
            motionX = x; motionZ = z;
        } else if (!packets_.empty()) {
            // Remote absolute S18 and relative S14 updates share a position
            // baseline. Delaying only S18 lets S14 apply to stale coordinates,
            // then replay overwrites newer movement. Leave both on vanilla's
            // ordered path; only our velocity and its transactions are delayed.
            keep = java::IsPingPacket(e.packet);
        }
        // Rise has no queue-size release or timeout. JNI ownership must succeed
        // before cancelling the current packet; allocation failure passes it.
        if (keep && Keep(e)) {
            g_watchdogQueue.store(true);
            if (velocity) {
                // The last queued S12 supplies the motion that survives replay.
                // Unlike Velocity's trigger, Watchdog also holds vertical/zero
                // S12s; those must not invent an arbitrary 90-degree heading.
                watchdogPendingYawValid_ = motionX != 0 || motionZ != 0;
                if (watchdogPendingYawValid_)
                    watchdogPendingYaw_ = rotation::mth::WrapDegrees(static_cast<float>(
                        std::atan2(static_cast<double>(motionZ), static_cast<double>(motionX)) *
                        57.29577951308232 + 90.0));
            }
        }
    }

    void OnDelayTick() {
        bool rotate=false;
        float releaseYaw=0;
        const bool blocked=scaffold::IsEnabled() || java::ScreenOpen() || clickgui::IsOpen();
        {
            std::lock_guard lock(packetLock_);
            if (!active_ || currentMode_!=0) return;
            if (g_delayWindow.load() && ticksRemaining_<=0 && !packets_.empty() && !delayRotationSubmitted_) {
                delayRotationSubmitted_=true;
                delayRotateYaw_=delayYaw_;
                delayRotateLeft_=delayYawValid_ && delayRotation_->get()
                    ? std::clamp(static_cast<int>(delayRotationTicks_->get()),1,10) : 0;
            }
            if (!delayRotation_->get() || blocked) delayRotateLeft_=0;
            if (delayRotateLeft_>0) {
                --delayRotateLeft_;rotate=true;releaseYaw=delayRotateYaw_;
            }
        }
        // The release tick counts as the first turn tick. Further ticks keep the
        // source-facing request alive without extending or replaying the KB queue.
        if (rotate) rotation::SetRotations({releaseYaw,rotation::GetRotation().pitch},180.0,
            nullptr,rotation::RotationPriority::Highest,false);
        // Start of runTick, before interaction, movement input and physics. Wait
        // for N player updates to complete, then apply velocity and acknowledgements
        // in arrival order at the next tick boundary, as normal packet tasks do.
        // Cap work per tick without releasing ownership of a remaining backlog.
        std::size_t replayed = 0;
        for (;;) {
            std::deque<jobject> pending;
            {
                std::lock_guard lock(packetLock_);
                if (!active_ || currentMode_ != 0 || !g_delayWindow.load() || ticksRemaining_ > 0)
                    return;
                if (packets_.empty()) {
                    g_delayWindow.store(false);
                    delayYawValid_=false;
                    delayRotationSubmitted_=false;
                    return;
                }
                if (replayed >= 4096) return;
                const auto count = std::min(packets_.size(), 4096 - replayed);
                for (std::size_t i = 0; i < count; ++i) {
                    pending.push_back(packets_.front());
                    packets_.pop_front();
                }
            }
            replayed += pending.size();
            Finish(std::move(pending), true);
            // Handle arrivals during replay before allowing packets to pass through.
        }
    }

    // Caller holds packetLock_. Only cancel after JNI ownership is secured.
    bool Keep(event::PacketEvent& e) {
        JniAttach attach;
        if (!attach) return false;
        jobject kept = attach.env->NewGlobalRef(e.packet);
        if (!kept) {
            if (attach.env->ExceptionCheck()) attach.env->ExceptionClear();
            return false;
        }
        packets_.push_back(kept);
        e.Cancel();
        return true;
    }

    static java::EntityHandle LogicalTarget() {
        // Nova's RotationManager refreshes the game's hit result via PickCursor
        // using the active rotation, retaining vanilla reach and occlusion.
        java::EntityHandle target;
        java::AttackTargetEntity(target);
        return target;
    }

    void OnReduceTick() {
        if (mode_->get() != 1) return;
        std::deque<jobject> pending;
        bool flushed = false;
        {
            std::lock_guard lock(packetLock_);
            const auto now = NowMillis();
            if (reducing_ && (java::PlayerOnGround() || now - lastLag_ < 100 ||
                now - startDelay_ >= static_cast<long long>(maxDelay_->get()))) {
                reducing_ = false;
                g_reduceDelay.store(false);
                pending.swap(packets_);
                flushed = true;
            }
        }
        Finish(std::move(pending), true);
        bool rotate = false;
        {
            std::lock_guard lock(packetLock_);
            if (!active_ || currentMode_ != 1) return;
            if (flushed) {
                jump_ = true;
                if (NowMillis() - lastLag_ >= 100) {
                    g_attackQueue.store(static_cast<int>(attackCounts_->get()));
                    sprintQueue_ = static_cast<int>(sprintTicks_->get());
                }
            }
            rotate = sprintQueue_ >= 1;
        }
        if (rotate && !killaura::HasTargets() && !scaffold::IsEnabled())
            rotation::SetRotations({g_yaw.load(), rotation::GetRotation().pitch}, 180.0,
                                  nullptr, rotation::RotationPriority::Highest);
    }

    void OnInput(event::MoveInputEvent& e) {
        SyncMode();
        if (!isEnabled()) return;
        if (mode_->get() >= 2) return;
        if (mode_->get() == 0) {
            if (jumpReset_->get() && java::PlayerOnGround() && java::PlayerHurtTime() == 9)
                e.jump = true;
            return;
        }
        float fall = 0;
        const bool knowFall = java::PlayerFallDistance(fall);
        const auto target = LogicalTarget();
        const bool ground = java::PlayerOnGround();
        const bool freeAim = !killaura::HasTargets() && !scaffold::IsEnabled();
        std::lock_guard lock(packetLock_);
        const bool lagClear = NowMillis() - lastLag_ >= 100;
        auto forward = [&] { e.forward = 1; e.strafe = 0; };
        if (reducing_ && knowFall && fall == 0 && target && target.IsPlayer() && !antibot::IsBot(target) && lagClear)
            forward();
        if (jump_ && ground) {
            if (lagClear) { e.jump = true; forward(); }
            jump_ = false;
        }
        if (sprintQueue_ >= 1 && freeAim && lagClear) forward();
        // Negative values have no behavior; saturating at zero avoids overflow.
        if (sprintQueue_ > 0) --sprintQueue_;
    }

    Properties::ModeProperty* mode_ = nullptr;
    Properties::NumberProp* attackCounts_ = nullptr;
    Properties::NumberProp* sprintTicks_ = nullptr;
    Properties::NumberProp* maxDelay_ = nullptr;
    Properties::BoolProperty* swingHand_ = nullptr;
    Properties::NumberProp* delayTicks_ = nullptr;
    Properties::BoolProperty* jumpReset_ = nullptr;
    Properties::BoolProperty* delayRotation_ = nullptr;
    Properties::NumberProp* delayRotationTicks_ = nullptr;
    int delayRotateLeft_ = 0;
    float delayRotateYaw_ = 0;
    float delayYaw_ = 0;
    bool delayYawValid_ = false, delayRotationSubmitted_ = false;
    Properties::BoolProperty *watchdogGround_ = nullptr, *watchdogAttack_ = nullptr, *watchdogExplosions_ = nullptr;
    Properties::BoolProperty* watchdogAutoAttack_ = nullptr;
    Properties::BoolProperty* watchdogForceSlowdown_ = nullptr;
    Properties::BoolProperty* watchdogRotation_ = nullptr;
    Properties::NumberProp* watchdogRotationTicks_ = nullptr;
    int watchdogRotateLeft_ = 0;
    float watchdogRotateYaw_ = 0, watchdogPendingYaw_ = 0;
    bool watchdogPendingYawValid_ = false;
    Properties::NumberProp *watchdogAttempts_ = nullptr, *watchdogInterval_ = nullptr;
    int watchdogAttacks_ = 0;
    long long watchdogNextAttack_ = 0;
    std::uint64_t watchdogAttackGeneration_ = 0;
    bool watchdogIssuingAttack_ = false;
    mutable std::mutex packetLock_;
    bool active_ = false;
    int currentMode_ = 0;
    bool reducing_ = false, jump_ = false;
    bool watchdogReady_ = false;
    int watchdogGroundTicks_ = 0;
    java::EntityHandle watchdogPlayer_, watchdogWorld_;
    int sprintQueue_ = 0;
    long long startDelay_ = 0;
    int ticksRemaining_ = 0;
    long long lastLag_ = 0;
    std::deque<jobject> packets_;
    event::Subscription packet_;
    event::Subscription tick_;
    event::Subscription input_;
    event::Subscription world_;
    event::Subscription motion_, respawn_;
    event::Subscription slowdown_;
};

}

namespace antikb {
bool IsEnabled() { return g_enabled.load(); }
bool IsDelaying() { return g_reduceDelay.load(); }
bool BlocksAutoBlockBlink() { return g_watchdogReplayDepth.load() != 0 || (g_enabled.load() && (g_watchdogQueue.load() || g_watchdogRecovering.load())); }
bool WatchdogHoldingPackets() { return g_enabled.load() && g_watchdogQueue.load(); }
bool WatchdogReplayingPackets() { return g_watchdogReplayDepth.load() != 0; }
int AttackQueue() { return g_attackQueue.load(); }
float Yaw() { return g_yaw.load(); }
bool OwnsIncomingDelayQueue() { return mix::OwnsIncoming() || prediction::OwnsIncoming() || g_watchdogReplayDepth.load() != 0 || (g_enabled.load() && (g_delayWindow.load() || g_reduceDelay.load() || g_watchdogQueue.load())); }
bool BlocksBacktrack() { return OwnsIncomingDelayQueue() || (g_enabled.load() && g_attackQueue.load() > 0); }
}

std::unique_ptr<Module> MakeAntiKbModule() {
    return std::make_unique<AntiKbModule>();
}

}
