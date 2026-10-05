#include "minecraft/modules/misc/antibot/AntiBot.h"
#include <atomic>
#include <chrono>
#include <cmath>
#include <deque>
#include <format>
#include <memory>
#include <string>
#include <utility>

#include "minecraft/events/EventBus.h"
#include "minecraft/events/GameEvents.h"
#include "minecraft/modules/Module.h"
#include "minecraft/modules/Registry.h"
#include "minecraft/modules/combat/killaura/KillAura.h"
#include "minecraft/modules/combat/velocity/Velocity.h"
#include "minecraft/modules/player/scaffold/Scaffold.h"
#include "minecraft/java/api/base.h"
#include "minecraft/java/api/game_click.h"
#include "minecraft/java/api/game_entity.h"
#include "minecraft/java/api/game_network.h"
#include "minecraft/java/api/game_player.h"
#include "minecraft/utils/rotation/Mth.h"
#include "minecraft/utils/rotation/Rotation.h"
#include "minecraft/utils/rotation/RotationManager.h"

// Velocity -- a port of Starain's module of the same name.
//
// The idea is to take the knockback the server sends and hold it: the packet is
// swallowed instead of being handed to the game, and while it is held the client
// keeps playing as though the hit never landed -- it walks forward at whoever hit
// it, jumps on landing, and repays a few attacks of its own. Once the player is
// back on the ground (or the hold runs past Max Delay) the packet is replayed and
// the knockback finally takes effect.
//
// Everything here talks to the game through the same calls Starain makes, so the
// two behave identically; what differs is only how the calls are reached.

namespace nova::module {
namespace {

namespace event = nova::event;

// Starain measures every guard in this module against the last teleport or
// explosion, so a knockback that arrives right after one is ignored.
constexpr long long kLagGuardMillis = 100;

// Velocity's own aim speed, forwarded to the rotation manager verbatim.
constexpr double kSprintRotationSpeed = 180.0;

[[nodiscard]] long long NowMillis() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
               std::chrono::steady_clock::now().time_since_epoch())
        .count();
}

[[nodiscard]] std::string WholeNumber(double value) {
    return std::format("{:.0f}", value);
}

// The horizontal direction a knockback is pushing towards, as a look yaw.
[[nodiscard]] float YawFromMotion(int motionX, int motionZ) {
    constexpr double kRadiansToDegrees = 57.29577951308232;
    const double degrees =
        std::atan2(static_cast<double>(motionZ), static_cast<double>(motionX)) * kRadiansToDegrees;
    return rotation::mth::WrapDegrees(static_cast<float>(degrees + 90.0));
}

// The packet queue sits between two threads: the netty I/O thread parks packets
// in OnPacket, the game thread drains them in Flush. Nothing else is shared
// across threads, so one lock over the queue is the whole of the discipline.
std::deque<jobject> g_packets;
SRWLOCK g_packetLock = SRWLOCK_INIT;

// The state KillAura reads. Atomics because OnPacket writes them from the I/O
// thread while the game thread reads them.
std::atomic<bool>  g_delay{false};
std::atomic<int>   g_attackQueue{0};
std::atomic<int>   g_sprintQueue{0};
std::atomic<bool>  g_jump{false};
std::atomic<float> g_yaw{0.0f};
std::atomic<long long> g_lag{0};
std::atomic<long long> g_startDelay{0};

class PacketLock {
public:
    PacketLock() { AcquireSRWLockExclusive(&g_packetLock); }
    ~PacketLock() { ReleaseSRWLockExclusive(&g_packetLock); }
    PacketLock(const PacketLock&) = delete;
    PacketLock& operator=(const PacketLock&) = delete;
};

void Keep(jobject packet) {
    JniAttach attach;
    if (!attach)
        return;

    jobject kept = attach.env->NewGlobalRef(packet);
    if (kept == nullptr)
        return;

    PacketLock guard;
    g_packets.push_back(kept);
}

void ReleaseAll() {
    std::deque<jobject> taken;
    {
        PacketLock guard;
        taken.swap(g_packets);
    }

    JniAttach attach;
    if (!attach)
        return;
    for (jobject packet : taken)
        attach.env->DeleteGlobalRef(packet);
}

[[nodiscard]] bool HoveredIsPlayer() {
    java::EntityHandle hovered;
    return java::AttackTargetEntity(hovered) && hovered.IsPlayer() && !antibot::IsBot(hovered);
}

class VelocityModule final : public Module {
public:
    VelocityModule() : Module("Velocity", Category::Combat, "General", "General") {
        counts_ = &addNumber("Attack Counts", 0, 5, 1, 3, {}, WholeNumber);
        sprintTicks_ = &addNumber("SprintTicks", 0, 5, 1, 3, {}, WholeNumber);
        maxDelay_ = &addNumber("Max Delay", 0, 1000, 50, 1000, {}, WholeNumber);
        // Not one of the original's settings: it always turns the player to face
        // whoever knocked them back, and there is no way to ask it not to. This is
        // the switch for that one behaviour, and it defaults on so the module still
        // behaves like the original out of the box.
        rotation_ = &addBool("Rotation", true);

        event::EventBus& bus = event::EventBus::Get();
        packet_ = bus.Subscribe<event::PacketEvent>(
            [this](event::PacketEvent& e) { OnPacket(e); });
        gameTick_ = bus.Subscribe<event::GameTickEvent>(
            [this](event::GameTickEvent& e) { OnGameTick(e); });
        playerTick_ = bus.Subscribe<event::PlayerTickEvent>(
            [this](event::PlayerTickEvent&) { OnPlayerTick(); }, event::Priority::Low);
        moveInput_ = bus.Subscribe<event::MoveInputEvent>(
            [this](event::MoveInputEvent& e) { OnMoveInput(e); }, event::Priority::Low);
    }

    ~VelocityModule() override { ReleaseAll(); }

private:
    void onEnable() override {
        // The inbound hook publishes on the netty thread; it stays inert until
        // somebody says they want inbound packets.
        nova::java::SetIncomingPacketsWanted(true);
    }

    void onDisable() override {
        nova::java::SetIncomingPacketsWanted(false);
        g_delay.store(false, std::memory_order_release);
        ReleaseAll();
        g_attackQueue.store(0, std::memory_order_release);
        g_jump.store(false, std::memory_order_release);
        // sprintQueue is deliberately left alone. The original clears delay, the
        // packet queue, attackQueue and jump here and stops short of this one, so a
        // sprint that is still being paid out when the module is switched off
        // carries on if it is switched back on. Clearing it belongs to the tidier
        // version of this module, not to this one.
    }

    // Starain's getSuffix: how long the knockback has been held, shown beside the
    // name in the array list while a hold is on.
    [[nodiscard]] std::string suffix() const override {
        if (!g_delay.load(std::memory_order_acquire))
            return {};
        return std::format("{}ms", NowMillis() - g_startDelay.load(std::memory_order_acquire));
    }

    // Runs on the netty I/O thread.
    void OnPacket(event::PacketEvent& e) {
        if (!isEnabled())
            return;
        if (e.type != event::EventType::Incoming || e.packet == nullptr)
            return;

        const long long now = NowMillis();

        // While the knockback is held, everything that would move us is held too.
        if (g_delay.load(std::memory_order_acquire) &&
            nova::java::IsVelocityDelayPacket(e.packet)) {
            e.Cancel();
            Keep(e.packet);
        }

        if (nova::java::IsPositionOrExplodePacket(e.packet))
            g_lag.store(now, std::memory_order_release);

        if (!nova::java::IsMotionPacket(e.packet))
            return;

        int id = 0, xa = 0, ya = 0, za = 0;
        if (!nova::java::ReadMotionPacket(e.packet, id, xa, ya, za))
            return;

        java::EntityHandle self = java::PlayerEntity();
        int selfId = 0;
        if (!self.Valid() || !java::EntityId(self, selfId) || selfId != id)
            return;

        const long long lag = g_lag.load(std::memory_order_acquire);
        if (now - lag < kLagGuardMillis)
            return;

        // Upwards plus sideways is a knockback; a pure vertical motion is a jump
        // or a fall and is left alone.
        const bool knockback = ya > 0 && (xa != 0 || za != 0);
        if (knockback && !g_delay.load(std::memory_order_acquire)) {
            g_delay.store(true, std::memory_order_release);
            e.Cancel();
            Keep(e.packet);
            g_startDelay.store(now, std::memory_order_release);
        }

        g_yaw.store(YawFromMotion(xa, za), std::memory_order_release);
    }

    void OnGameTick(event::GameTickEvent& e) {
        if (!isEnabled() || e.type != event::EventType::Pre)
            return;

        const long long now = NowMillis();
        const long long lag = g_lag.load(std::memory_order_acquire);
        const long long startDelay = g_startDelay.load(std::memory_order_acquire);
        const long long maxDelay = static_cast<long long>(maxDelay_->get());

        if (g_delay.load(std::memory_order_acquire) &&
            (java::PlayerOnGround() || now - lag < kLagGuardMillis ||
             now - startDelay >= maxDelay)) {
            Flush();
            if (now - lag >= kLagGuardMillis) {
                g_attackQueue.store(static_cast<int>(counts_->get()), std::memory_order_release);
                g_sprintQueue.store(static_cast<int>(sprintTicks_->get()),
                                    std::memory_order_release);
            }
        }

        // Aim at whoever sent the knockback, unless KillAura or the scaffold is
        // already aiming -- two writers would fight over the same rotation.
        if (rotation_->get() && g_sprintQueue.load(std::memory_order_acquire) >= 1 &&
            !killaura::HasTargets() && !scaffold::IsEnabled()) {
            rotation::SetRotations(
                rotation::Rotation{g_yaw.load(std::memory_order_acquire),
                                   rotation::GetRotation().pitch},
                kSprintRotationSpeed);
        }
    }

    void OnPlayerTick() {
        if (!isEnabled())
            return;
        if (g_attackQueue.load(std::memory_order_acquire) < 1)
            return;

        if (killaura::QueueAutoBlockAttack()) {
            g_attackQueue.fetch_sub(1, std::memory_order_acq_rel);
            return;
        }

        java::EntityHandle hovered;
        if (java::AttackTargetEntity(hovered) && hovered.IsPlayer() && !antibot::IsBot(hovered)) {
            java::AttackAndSwing(hovered);
        }
        g_attackQueue.fetch_sub(1, std::memory_order_acq_rel);
    }

    void OnMoveInput(event::MoveInputEvent& e) {
        if (!isEnabled())
            return;

        const long long now = NowMillis();
        const bool lagClear = now - g_lag.load(std::memory_order_acquire) >= kLagGuardMillis;

        float fallDistance = 0.0f;
        const bool knowFall = java::PlayerFallDistance(fallDistance);

        // Held knockback, on the ground, looking at a player: walk into them.
        if (g_delay.load(std::memory_order_acquire) && knowFall && fallDistance == 0.0f &&
            lagClear && HoveredIsPlayer()) {
            e.forward = 1.0f;
            e.strafe = 0.0f;
        }

        if (g_jump.load(std::memory_order_acquire) && java::PlayerOnGround()) {
            if (lagClear) {
                e.jump = true;
                e.forward = 1.0f;
                e.strafe = 0.0f;
            }
            g_jump.store(false, std::memory_order_release);
        }

        // The decrement happens whether or not the conditions hold, exactly as
        // the original's post-decrement does.
        const int sprintLeft = g_sprintQueue.load(std::memory_order_acquire);
        if (sprintLeft >= 1 && lagClear && !killaura::HasTargets() && !scaffold::IsEnabled()) {
            e.forward = 1.0f;
            e.strafe = 0.0f;
        }
        g_sprintQueue.fetch_sub(1, std::memory_order_acq_rel);
    }

    // Replays everything that was held. The queue is emptied under the lock and
    // then drained without it, because handing a packet back to the game can call
    // arbitrary code and must not run with the I/O thread blocked behind us.
    void Flush() {
        if (!g_delay.load(std::memory_order_acquire))
            return;

        std::deque<jobject> taken;
        {
            PacketLock guard;
            taken.swap(g_packets);
        }

        JniAttach attach;
        for (jobject packet : taken) {
            nova::java::HandleIncomingPacket(packet);
            if (attach)
                attach.env->DeleteGlobalRef(packet);
        }

        g_delay.store(false, std::memory_order_release);
        g_jump.store(true, std::memory_order_release);
    }

    Properties::NumberProp* counts_ = nullptr;
    Properties::NumberProp* sprintTicks_ = nullptr;
    Properties::NumberProp* maxDelay_ = nullptr;
    Properties::BoolProperty* rotation_ = nullptr;

    event::Subscription packet_;
    event::Subscription gameTick_;
    event::Subscription playerTick_;
    event::Subscription moveInput_;
};

}

namespace velocity {

bool IsDelaying() { return g_delay.load(std::memory_order_acquire); }

int AttackQueue() { return g_attackQueue.load(std::memory_order_acquire); }

float Yaw() { return g_yaw.load(std::memory_order_acquire); }

}

std::unique_ptr<Module> MakeVelocityModule() {
    return std::make_unique<VelocityModule>();
}

}
