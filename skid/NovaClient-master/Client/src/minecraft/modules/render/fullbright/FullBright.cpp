#include <algorithm>
#include <chrono>
#include <cmath>
#include <memory>

#include "minecraft/modules/Module.h"
#include "minecraft/compat/Profile.h"
#include "minecraft/modules/Registry.h"
#include "minecraft/java/api/game_options.h"
#include "minecraft/java/api/game_player.h"

namespace nova::module {
namespace {

constexpr int kGammaMode = 0;
constexpr int kNightVisionMode = 1;

constexpr double kGammaStep = 0.1;
constexpr double kTickSeconds = 0.05;

using Clock = std::chrono::steady_clock;

class FullBrightModule final : public Module {
public:
    FullBrightModule() : Module("FullBright", Category::Render, "General", "General") {
        mode_ = &addMode("Mode", kGammaMode, std::vector<std::string>{"Gamma", "NightVision"});
        brightness_ = &addNumber("Brightness", 1.0, 15.0, 1.0, 15.0, {}, {},
                                 [this] { return mode_->get() == kGammaMode; });
    }

private:
    void onEnable() override {
        // Config may enable us before the option is readable. Capture the
        // baseline lazily and retry, rather than permanently failing this enable.
        haveSaved_ = false;
        current_ = 0.0;
        applied_ = false;
        elapsed_ = 0.0;
        last_ = Clock::now();
    }

    void onDisable() override {
        RestoreGamma();
        nova::java::RemoveNightVision();
    }

    void onUpdate() override {
        const auto now = Clock::now();
        elapsed_ += std::chrono::duration<double>(now - last_).count();
        last_ = now;

        if (elapsed_ < kTickSeconds)
            return;
        elapsed_ = std::min(elapsed_ - kTickSeconds, kTickSeconds);
        PlayerTick();
    }

    void PlayerTick() {
        if (mode_->get() == kNightVisionMode) {
            RestoreGamma();
            nova::java::AddNightVision();
            return;
        }

        nova::java::RemoveNightVision();
        double actual = 0.0;
        if (!nova::java::GetGamma(&actual) || !std::isfinite(actual))
            return;
        if (!haveSaved_) {
            savedGamma_ = actual;
            current_ = actual;
            haveSaved_ = true;
        }

        const double target = brightness_->get();
        const double next = current_ < target ? std::min(target, current_ + kGammaStep)
                                             : std::max(target, current_ - kGammaStep);
        // Reassert the target if Minecraft/Lunar reloads its options after we
        // reach it. Track only successful writes, so transient failures retry.
        if (std::abs(actual - next) > 0.00001) {
            if (!nova::java::SetGamma(next))
                return;
            applied_ = true;
        }
        current_ = next;
    }

    void RestoreGamma() {
        if (applied_ && !nova::java::SetGamma(savedGamma_))
            return;
        applied_ = false;
        haveSaved_ = false;
        current_ = savedGamma_;
    }

    Properties::ModeProperty* mode_ = nullptr;
    Properties::NumberProp* brightness_ = nullptr;

    double savedGamma_ = 0.0;
    double current_ = 0.0;
    double elapsed_ = 0.0;
    bool haveSaved_ = false;
    bool applied_ = false;
    Clock::time_point last_ = Clock::now();
};

}

std::unique_ptr<Module> MakeFullBrightModule() {
    return std::make_unique<FullBrightModule>();
}

}
