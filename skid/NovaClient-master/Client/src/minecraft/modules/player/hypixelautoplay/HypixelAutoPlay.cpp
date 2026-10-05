#include "minecraft/modules/Module.h"
#include "minecraft/modules/Registry.h"
#include "minecraft/modules/player/hypixelautoplay/AutoPlayLogic.h"
#include "minecraft/events/GameEvents.h"
#include "minecraft/java/api/game_chat.h"
#include "minecraft/java/api/game_network.h"
#include "minecraft/java/api/game_entity.h"
#include "ui/notifications/Notifications.h"
#include <mutex>
#include <utility>
#include <atomic>
#include <chrono>
#include <cmath>
#include <format>

namespace nova::module {
namespace {
long long NowMillis() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
}
class HypixelAutoPlayModule final : public Module {
public:
    HypixelAutoPlayModule() : Module("HypixelAutoPlay", Category::Player, "General", "General") {
        addMode("Mode", 0, {"Hypixel"});
        delay_ = &addNumber("Delay", 0, 30, 0.1, 5, {},
            [](double value) { return std::format("{:.1f} s", value); });
        auto& bus = event::EventBus::Get();
        packet_ = bus.Subscribe<event::PacketEvent>([this](auto& e) { OnPacket(e); }, event::Priority::Lowest);
        tick_ = bus.Subscribe<event::GameTickEvent>([this](auto& e) { OnTick(e); });
        respawn_ = bus.Subscribe<event::RespawnEvent>([this](auto&) { Clear(); });
    }
    ~HypixelAutoPlayModule() override {
        packet_.Reset(); tick_.Reset(); respawn_.Reset();
        onDisable();
    }
    std::string suffix() const override { return "Hypixel"; }
private:
    void onEnable() override {
        onUpdate();
        std::lock_guard lock(mutex_);
        pending_.Clear(); ++generation_;
        if (!active_) { active_ = true; java::SetIncomingPacketsWanted(true); }
    }
    void onDisable() override {
        std::lock_guard lock(mutex_);
        pending_.Clear(); ++generation_;
        if (active_) { active_ = false; java::SetIncomingPacketsWanted(false); }
    }
    void onUpdate() override { delayMs_.store(static_cast<int>(std::lround(delay_->get() * 1000))); }
    void Clear() {
        std::lock_guard lock(mutex_);
        pending_.Clear(); ++generation_;
    }
    void OnPacket(event::PacketEvent& e) {
        if (e.type != event::EventType::Incoming) return;
        unsigned long long generation;
        {
            std::lock_guard lock(mutex_);
            if (!active_) return;
            generation = generation_;
        }
        if (java::IsDisconnectPacket(e.packet) || java::IsRespawnPacket(e.packet) || java::IsChatSessionStart(e.packet)) { Clear(); return; }
        if (e.IsCancelled()) return;
        std::vector<std::string> commands;
        if (!java::ReadAutoPlayCommands(e.packet, commands)) return;
        std::lock_guard lock(mutex_);
        if (!active_ || generation != generation_) return;
        pending_.Add(commands, NowMillis(), delayMs_.load());
    }
    void OnTick(event::GameTickEvent& e) {
        if (e.type != event::EventType::Pre) return;
        {
            std::lock_guard lock(mutex_);
            if (!active_) return;
        }
        if (!java::PlayerEntity()) { Clear(); return; }
        std::vector<std::string> commands;
        unsigned long long generation;
        {
            std::lock_guard lock(mutex_);
            if (!active_) return;
            commands = pending_.TakeReady(NowMillis()); generation = generation_;
        }
        for (const auto& command : commands) {
            {
                std::lock_guard lock(mutex_);
                if (!active_ || generation != generation_) return;
            }
            if (java::SendPlayCommand(command))
                notifications::Post("Auto Play: joining a new game", notifications::Type::Success);
        }
    }
    std::mutex mutex_;
    bool active_ = false;
    unsigned long long generation_ = 0;
    autoplay::ReplayQueue pending_;
    Properties::NumberProp* delay_ = nullptr;
    std::atomic<int> delayMs_{5000};
    event::Subscription packet_, tick_, respawn_;
};
}
std::unique_ptr<Module> MakeHypixelAutoPlayModule() { return std::make_unique<HypixelAutoPlayModule>(); }
}
