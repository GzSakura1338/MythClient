#include <algorithm>
#include <chrono>
#include <cmath>
#include <format>
#include <memory>
#include <random>

#include "minecraft/events/EventBus.h"
#include "minecraft/events/GameEvents.h"
#include "minecraft/modules/Module.h"
#include "minecraft/modules/player/automlg/AutoMLG.h"
#include "minecraft/modules/player/autocharge/AutoCharge.h"
#include "minecraft/modules/Registry.h"
#include "minecraft/java/api/base.h"
#include "minecraft/java/api/game_click.h"
#include "minecraft/java/api/game_input.h"
#include "minecraft/java/api/game_player.h"
#include "ui/clickgui/ClickGui.h"
#include "utils/Keyboard.h"
#include "minecraft/compat/Profile.h"
#include "minecraft/host/render/OverlayHost.h"

namespace nova::module {
namespace {

[[nodiscard]] ULONGLONG NowMillis() {
    return static_cast<ULONGLONG>(std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count());
}

class AutoClickerModule final : public Module {
public:
    AutoClickerModule() : Module("AutoClicker", Category::Combat, "General", "General") {
        const auto cpsFormat = [](double value) {
            if (value == std::rint(value))
                return std::format("{:.0f}", value);
            return std::format("{:.1f}", value);
        };
        minCps_ = &addNumber("Min CPS", 1, 20, 0.5, 10, {}, cpsFormat);
        maxCps_ = &addNumber("Max CPS", 1, 20, 0.5, 10, {}, cpsFormat);
        gaussian_ = &addBool("Gaussian random", false);
        simulateExhaust_ = &addBool("Simulate exhaust", true);
        notUsingItem_ = &addBool("Not using item");
        breakBlocks_ = &addBool("Break blocks");
        weaponOnly_ = &addBool("Weapon only");
        disableCreative_ = &addBool("Disable in creative");
        inventory_ = &addBool("Inventory");
        startDelay_ = &addNumber("Start delay", 0, 250, 10, 100, {}, [](double value) {
            return std::format("{:.0f} ms", value);
        });
        motion_ = event::EventBus::Get().Subscribe<event::MotionEvent>(
            [this](event::MotionEvent& e) {
                if (e.type == event::EventType::Pre) UpdateInventory();
            });
    }

private:
    void onEnable() override {
        nextClickTime_ = 0;
        inventoryNextClickTime_ = 0;
        holdingBlockBreak_ = false;
    }

    void onDisable() override {
        nova::glhook::CancelAttackMousePulse();
        nextClickTime_ = 0;
        inventoryNextClickTime_ = 0;
        holdingBlockBreak_ = false;
        nova::java::SetAttackDown(false);
    }

    void onUpdate() override {
        // Rendering may run after the movement packet. Only cancel here;
        // container clicks must execute in Motion Pre on the game tick.
        if (!CanClickInventory()) inventoryNextClickTime_ = 0;

        if (automlg::IsWorking() || autocharge::IsWorking() || !nova::input::IsProcessForeground() || !(GetAsyncKeyState(VK_LBUTTON)&0x8000)) {
            nova::glhook::CancelAttackMousePulse();
            nextClickTime_ = 0;
            holdingBlockBreak_ = false;
            nova::java::SetAttackDown(false);
            return;
        }

        const ULONGLONG now = NowMillis();
        if (nextClickTime_ == 0)
            nextClickTime_ = now + NextDelay();

        const int clicks=nextClickTime_<=now?1:0;
        if(clicks)nextClickTime_=now+NextDelay();

        if (notUsingItem_->get() && nova::java::PlayerUsingItem()) {
            nova::glhook::CancelAttackMousePulse();
            return;
        }
        if (disableCreative_->get() && nova::java::PlayerInCreative()) {
            nova::glhook::CancelAttackMousePulse();
            return;
        }
        if (nova::java::ScreenOpen() || nova::clickgui::IsOpen()) {
            nova::glhook::CancelAttackMousePulse();
            nextClickTime_ = 0;
            holdingBlockBreak_ = false;
            nova::java::SetAttackDown(false);
            return;
        }
        if (!nova::java::MouseGrabbed()) {
            nova::glhook::CancelAttackMousePulse();
            return;
        }
        if (weaponOnly_->get() && !nova::java::HoldingWeapon()) {
            nova::glhook::CancelAttackMousePulse();
            return;
        }

        if (breakBlocks_->get()) {
            if (!nova::java::PlayerCanEdit()) {
                if (holdingBlockBreak_) {
                    nova::java::SetAttackDown(false);
                    holdingBlockBreak_ = false;
                }
            } else if (nova::java::TargetedBlockBreakable()) {
                nova::glhook::CancelAttackMousePulse();
                if (!holdingBlockBreak_) {
                    nova::java::SetAttackDown(true);
                    holdingBlockBreak_ = true;
                }
                return;
            } else if (holdingBlockBreak_) {
                nova::java::SetAttackDown(false);
                holdingBlockBreak_ = false;
                return;
            }
        }

        for (int i = 0; i < clicks; ++i) {
            // Air swings leave a vanilla miss cooldown. Clear it on both paths
            // so moving the crosshair onto an entity does not suppress clicks.
            nova::java::ClearMissTime();
            if(nova::compat::IsLegacyLunar()) {
                // Native mouse events update Lunar's CPS HUD and are consumed
                // before movement by runTick, without direct attack packets.
                nova::glhook::QueueAttackMousePulse();
                continue;
            }
            nova::java::ClickAttack();
            nova::java::SetAttackDown(true);
        }
    }

    [[nodiscard]] bool CanClickInventory() const {
        return isEnabled() && inventory_->get() && !nova::clickgui::IsOpen() &&
            nova::input::IsProcessForeground() &&
            (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0 &&
            nova::java::InventoryScreenOpen();
    }

    void UpdateInventory() {
        // Re-read the physical button here: the render-frame input cache may
        // be stale when several game ticks run before the next rendered frame.
        if (!CanClickInventory()) {
            inventoryNextClickTime_ = 0;
            return;
        }

        const ULONGLONG now = NowMillis();
        if (inventoryNextClickTime_ == 0)
            inventoryNextClickTime_ = now + static_cast<ULONGLONG>(startDelay_->get());

        if (now < inventoryNextClickTime_) return;

        // Drop missed intervals rather than replaying clicks after a stall.
        // Resolve the current screen/hovered slot inside ClickHoveredSlot.
        inventoryNextClickTime_ = now + NextDelay();
        const bool quickMove = (GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0;
        nova::java::ClickHoveredSlot(quickMove);
    }

    [[nodiscard]] int NextInt(int bound) {
        if (bound <= 1)
            return 0;
        return std::uniform_int_distribution<int>(0, bound - 1)(rng_);
    }

    [[nodiscard]] int NextDelay() {
        const double low = std::max(1.0, std::min(minCps_->get(), maxCps_->get()));
        const double high = std::max(low, std::max(minCps_->get(), maxCps_->get()));
        double target = low;
        if (high > low) {
            if (gaussian_->get()) {
                // Truncated normal: reject outliers instead of accumulating them at the endpoints.
                std::normal_distribution<double> normal((low + high) * .5, (high - low) / 6.0);
                do { target = normal(rng_); } while (target < low || target > high);
            } else target = std::uniform_real_distribution<double>(low, high)(rng_);
        }
        const int baseDelay = static_cast<int>(std::lround(1000.0 / target));
        if (gaussian_->get() || high == low) return baseDelay;

        int finalDelay;
        if (simulateExhaust_->get()) {
            const int variation = NextInt(baseDelay + 1) - (baseDelay / 2);
            finalDelay = baseDelay + variation;

            if (NextInt(100) < 15) {
                if (NextInt(2) == 0)
                    finalDelay = 25 + NextInt(16);
                else
                    finalDelay = baseDelay + 50 + NextInt(41);
            }

            if (NextInt(100) < 8) {
                const int spikeMult = 50 + NextInt(151);
                finalDelay = (finalDelay * spikeMult) / 100;
            }

            if (NextInt(100) < 10)
                finalDelay += 10 + NextInt(26);
        } else {
            finalDelay = baseDelay;
        }

        return std::clamp(finalDelay, static_cast<int>(std::ceil(1000.0 / high)),
                          static_cast<int>(std::ceil(1000.0 / low)));
    }

    Properties::NumberProp* minCps_ = nullptr;
    Properties::NumberProp* maxCps_ = nullptr;
    Properties::BoolProperty* gaussian_ = nullptr;
    Properties::BoolProperty* simulateExhaust_ = nullptr;
    Properties::BoolProperty* notUsingItem_ = nullptr;
    Properties::BoolProperty* breakBlocks_ = nullptr;
    Properties::BoolProperty* weaponOnly_ = nullptr;
    Properties::BoolProperty* disableCreative_ = nullptr;
    Properties::BoolProperty* inventory_ = nullptr;
    Properties::NumberProp* startDelay_ = nullptr;

    std::mt19937 rng_{std::random_device{}()};
    ULONGLONG nextClickTime_ = 0;
    ULONGLONG inventoryNextClickTime_ = 0;
    bool holdingBlockBreak_ = false;
    event::Subscription motion_;
};

}

std::unique_ptr<Module> MakeAutoClickerModule() {
    return std::make_unique<AutoClickerModule>();
}

}
