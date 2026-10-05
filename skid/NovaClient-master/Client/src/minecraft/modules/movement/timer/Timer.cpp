#include <algorithm>
#include <format>
#include <mutex>
#include <random>
#include "minecraft/modules/Module.h"
#include "minecraft/modules/Registry.h"
#include "minecraft/modules/movement/timer/TimerBalance.h"
#include "minecraft/compat/lunar189/LunarRuntime.h"
#include "minecraft/compat/easecation/EaseCation.h"
#include "minecraft/compat/neoforge12110/NeoForge12110.h"
#include "minecraft/compat/Profile.h"
#include "minecraft/events/GameEvents.h"
#include "minecraft/java/api/game_click.h"
#include "minecraft/java/api/game_player.h"
#include "minecraft/java/api/game_entity.h"
#include "minecraft/java/api/game_inventory.h"
#include "ui/clickgui/ClickGui.h"

namespace nova::module {
namespace {
class TimerModule final : public Module {
public:
    TimerModule() : Module("Timer", Category::Movement, "General", "General") {
        auto decimal = [](double v) { return std::format("{:.2f}", v); };
        minimum_ = &addNumber("Min speed", .1, 20, .05, 1, {}, decimal);
        maximum_ = &addNumber("Max speed", .1, 20, .05, 2, {}, decimal);
        auto speed = subNumber("Compensate speed", 1, 5, .05, 1.5, decimal);
        releaseSpeed_ = static_cast<Properties::NumberProp*>(speed.getProp());
        auto multiplier = subNumber("Balance multiplier", .1, 10, .05, 1, decimal);
        multiplier_ = static_cast<Properties::NumberProp*>(multiplier.getProp());
        compensate_ = &addBool("Compensate low timer", false, {std::move(speed), std::move(multiplier)});
        setBindGate([this] {
            if (java::ScreenOpen() || clickgui::IsOpen()) return false;
            std::lock_guard lock(mutex_);
            if (!active_ || !compensate_->get() || !balance_.Press()) return true;
            Apply(float(releaseSpeed_->get()));
            return false; // Rise cancels the second press and all release-phase presses.
        });
        motion_ = event::EventBus::Get().Subscribe<event::MotionEvent>([this](auto& e) {
            if (e.type != event::EventType::Pre) return;
            std::lock_guard lock(mutex_);
            if (!active_ || finished_) return;
            if (balance_.compensating && !compensate_->get()) balance_.Reset();
            const double low = std::min(minimum_->get(), maximum_->get());
            const double high = std::max(minimum_->get(), maximum_->get());
            Apply(balance_.compensating ? float(releaseSpeed_->get()) :
                float(std::uniform_real_distribution<double>(low, high)(random_)));
        });
        packet_ = event::EventBus::Get().Subscribe<event::PacketEvent>([this](auto& e) {
            if (e.type != event::EventType::Outgoing || e.IsCancelled() || !java::PacketIsMovePlayer(e.packet)) return;
            std::lock_guard lock(mutex_);
            if (!active_ || finished_ || !compensate_->get()) return;
            if (balance_.Movement(minimum_->get() < 1 || appliedSpeed_ < 1, multiplier_->get())) {
                finished_ = true;
                Apply(1);
            }
        }, event::Priority::Lowest);
    }
    std::string suffix() const override {
        std::lock_guard lock(mutex_);
        return std::format("{} {:.1f}", balance_.compensating ? "Release" : "Balance", balance_.value);
    }
private:
    void Apply(float speed) {
        const bool applied = compat::IsNeoForge12110()?compat::neoforge12110::SetTimerSpeed(speed):
            compat::UsesGlfwModernRuntime() ? compat::easecation::SetTimerSpeed(speed)
                                                   : compat::lunar::SetTimerSpeed(speed);
        if (applied) appliedSpeed_ = speed;
    }
    void onEnable() override {
        std::lock_guard lock(mutex_);
        balance_.Reset(); finished_ = false; active_ = true;
    }
    void onDisable() override {
        std::lock_guard lock(mutex_);
        active_ = false; finished_ = false; balance_.Reset(); Apply(1);
    }
    void onUpdate() override {
        std::lock_guard lock(mutex_);
        // Complete property changes on the minecraft/modules/render thread, not Netty.
        if (finished_) { setEnabled(false); return; }
        if (!java::PlayerEntity().Valid()) { balance_.Reset(); Apply(1); }
    }
    Properties::NumberProp *minimum_, *maximum_, *releaseSpeed_, *multiplier_;
    Properties::BoolProperty* compensate_;
    mutable std::mutex mutex_;
    timer::Balance balance_;
    bool active_ = false, finished_ = false;
    float appliedSpeed_ = 1;
    std::mt19937 random_{std::random_device{}()};
    event::Subscription motion_, packet_;
};
}
std::unique_ptr<Module> MakeTimerModule() { return std::make_unique<TimerModule>(); }
}
