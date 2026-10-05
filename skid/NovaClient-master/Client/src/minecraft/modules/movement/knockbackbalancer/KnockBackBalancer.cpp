#include "minecraft/modules/Module.h"
#include "minecraft/modules/Registry.h"
#include "minecraft/events/GameEvents.h"
#include "minecraft/java/api/game_entity.h"
#include "minecraft/java/api/game_network.h"
#include <deque>
#include <format>
#include <mutex>

namespace nova::module {
namespace {
// Naven Modern LongJump's incoming FIFO, controlled by the module toggle:
// hold from the first own velocity until disabled, then replay the entire FIFO.
class KnockBackBalancerModule final : public Module {
public:
    KnockBackBalancerModule() : Module("KnockBackBalancer", Category::Movement, "General", "General") {
        auto& bus = event::EventBus::Get();
        packet_ = bus.Subscribe<event::PacketEvent>([this](auto& e) { Packet(e); }, event::Priority::Lowest);
        tick_ = bus.Subscribe<event::GameTickEvent>([this](auto& e) {
            if (e.type == event::EventType::Pre) Tick();
        }, event::Priority::Highest);
        respawn_ = bus.Subscribe<event::RespawnEvent>([this](auto&) { Drop(); });
    }
    ~KnockBackBalancerModule() override {
        packet_.Reset(); tick_.Reset(); respawn_.Reset();
        Drop();
        if (wanted_) java::SetIncomingPacketsWanted(false);
    }
    std::string suffix() const override {
        std::lock_guard lock(mutex_);
        return holding_ ? std::format("{} KB", received_) : "Waiting";
    }
private:
    mutable std::mutex mutex_;
    std::deque<jobject> packets_;
    java::EntityHandle player_, world_;
    bool active_ = false, wanted_ = false, holding_ = false, draining_ = false;
    std::size_t received_ = 0;
    event::Subscription packet_, tick_, respawn_;

    bool SameContext() const {
        const auto player = java::PlayerEntity(), world = java::LevelEntity();
        return player && world && player_.IsSame(player) && world_.IsSame(world);
    }
    void Drop() {
        JniAttach attach;
        std::lock_guard lock(mutex_);
        active_ = holding_ = false;
        if (attach) {
            for (const auto packet : packets_) attach.env->DeleteGlobalRef(packet);
            packets_.clear();
        }
        player_.Reset(); world_.Reset();
        setEnabled(false);
    }
    // Never hold mutex_ while entering vanilla packet handling: it may acquire
    // Minecraft's scheduled-task monitor or synchronously call another module.
    void Drain() {
        JniAttach attach;
        if (!attach) return;
        {
            std::lock_guard lock(mutex_);
            if (draining_) return;
            draining_ = true;
        }
        for (;;) {
            jobject packet{};
            bool replay = false;
            {
                std::lock_guard lock(mutex_);
                if (packets_.empty()) {
                    draining_ = false;
                    active_ = holding_ = false; setEnabled(false);
                    return;
                }
                replay = SameContext();
                if (!replay) active_ = false;
                packet = packets_.front(); packets_.pop_front();
            }
            if (replay) java::HandleIncomingPacket(packet);
            attach.env->DeleteGlobalRef(packet);
        }
    }
    void onEnable() override {
        // Normal disable has already replayed the previous session. Any stale
        // world/session references are discarded rather than crossing worlds.
        Drop();
        std::lock_guard lock(mutex_);
        setEnabled(true);
        player_ = java::PlayerEntity(); world_ = java::LevelEntity();
        active_ = true; holding_ = false;
        received_ = 0;
        if (!wanted_) { java::SetIncomingPacketsWanted(true); wanted_ = true; }
    }
    void onDisable() override {
        Drain();
        if (wanted_) { java::SetIncomingPacketsWanted(false); wanted_ = false; }
    }
    void Tick() {
        bool invalid = false;
        {
            std::lock_guard lock(mutex_);
            if (!active_) return;
            // Enabling in the main menu arms the module for the first world.
            if (!player_ && !world_ && !holding_) {
                player_ = java::PlayerEntity(); world_ = java::LevelEntity();
                if (!player_ || !world_) { player_.Reset(); world_.Reset(); return; }
            }
            invalid = !SameContext();
        }
        if (invalid) { Drop(); return; }
    }
    void Packet(event::PacketEvent& e) {
        if (e.type != event::EventType::Incoming || !e.packet) return;
        bool flush = false, drop = false;
        {
            std::lock_guard lock(mutex_);
            if (!active_) return;
            if (java::IsDisconnectPacket(e.packet) || java::IsRespawnPacket(e.packet)) drop = true;
            else if (!SameContext()) {
                // No player yet: let the login stream through; Tick arms the world.
                if (!player_ && !world_ && !holding_) return;
                drop = true;
            } else if (e.IsCancelled()) {
                // Respect another owner's cancellation without releasing our KB.
                return;
            } else {
                int id = 0, x = 0, y = 0, z = 0, self = 0;
                const bool knockback = java::IsMotionPacket(e.packet) && java::EntityId(player_, self) &&
                    java::ReadMotionPacket(e.packet, id, x, y, z) && id == self;
                if (!holding_ && !knockback) return;
                JniAttach attach;
                jobject kept = attach ? attach.env->NewGlobalRef(e.packet) : nullptr;
                if (!kept) {
                    if (attach && attach.env->ExceptionCheck()) attach.env->ExceptionClear();
                    flush = true;
                } else {
                    try { packets_.push_back(kept); }
                    catch (...) { attach.env->DeleteGlobalRef(kept); flush = true; }
                    if (!flush) {
                        if (knockback) ++received_;
                        holding_ = true;
                        e.Cancel();
                    }
                }
            }
        }
        if (drop) Drop();
        else if (flush) Drain();
    }
};
}
std::unique_ptr<Module> MakeKnockBackBalancerModule() { return std::make_unique<KnockBackBalancerModule>(); }
}
