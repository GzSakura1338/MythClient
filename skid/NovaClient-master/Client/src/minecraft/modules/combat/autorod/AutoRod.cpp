#include "minecraft/modules/combat/autorod/AutoRodLogic.h"
#include "minecraft/modules/combat/autorod/AutoRod.h"
#include "minecraft/modules/Module.h"
#include "minecraft/modules/player/automlg/AutoMLG.h"
#include "minecraft/modules/player/autocharge/AutoCharge.h"
#include "minecraft/modules/Registry.h"
#include "bind/PhysicalKey.h"
#include "minecraft/compat/Profile.h"
#include "minecraft/compat/lunar189/LunarRuntime.h"
#include "minecraft/events/GameEvents.h"
#include "minecraft/java/api/game_click.h"
#include "minecraft/java/api/game_inventory.h"
#include "minecraft/java/api/game_itemuse.h"
#include "minecraft/java/api/game_place.h"
#include "ui/clickgui/ClickGui.h"
#include "utils/Keyboard.h"
#include <algorithm>
#include <atomic>
#include <cmath>
#include <format>

namespace nova::module {
namespace {
std::atomic<bool> ownsItemUse{false};
struct RodIO {
    int binding;
    static java::KnownItem Item(int slot) {
        JniAttach attach;
        jobject stack = nullptr;
        java::ItemFacts facts;
        if (!attach || slot < 0 || slot > 8) return java::KnownItem::Nothing;
        const bool read = java::InventorySlot(slot, stack) && stack && java::ReadItemFacts(stack, facts);
        if (stack) attach.env->DeleteLocalRef(stack);
        if (!read || facts.empty || facts.count <= 0) return java::KnownItem::Nothing;
        return facts.item == java::KnownItem::Snowball || facts.item == java::KnownItem::Egg ||
            facts.item == java::KnownItem::FishingRod
            ? facts.item : java::KnownItem::Nothing;
    }
    bool Selected(int& slot) { return java::SelectedSlot(slot); }
    bool Select(int slot) { return java::SetSelectedSlot(slot); }
    bool Eligible(int slot) { return Item(slot) != java::KnownItem::Nothing; }
    bool IsRod(int slot) { return Item(slot) == java::KnownItem::FishingRod; }
    bool CanSwitch() { return !compat::IsLegacyLunar() || compat::lunar::HeldSlotChangeReady(); }
    bool Sync() { return java::SyncSelectedItem(); }
    bool Use() {
        int slot = -1;
        return bind::IsPhysicalBindingDown(binding) && Selected(slot) && java::UseSelectedItem(Item(slot));
    }
};

class AutoRodModule final : public Module {
public:
    AutoRodModule() : Module("AutoRod", Category::Combat, "General", "General"), key_(0, true) {
        // Like FreeLook, sample the physical binding so releases in menus cannot
        // leave a hold active. AutoRod always requires the physical key down.
        key_.setGate([] { return false; });
        addSettings({Properties::PropDesc("Key", &key_)});
        delay_ = &addNumber("Delay", 50, 2000, 10, 500, {},
            [](double value) { return std::format("{:.0f} ms", value); });
        switchDelay_ = &addNumber("Switch Delay", 0, 1000, 10, 0, {},
            [](double value) { return std::format("{:.0f} ms", value); });
        rodReturnDelay_ = &addNumber("Rod Return Delay", 50, 2000, 10, 500, {},
            [](double value) { return std::format("{:.0f} ms", value); });
        tick_ = event::EventBus::Get().Subscribe<event::GameTickEvent>([this](auto& e) {
            if (e.type == event::EventType::Pre) OnTick();
        });
    }
    ~AutoRodModule() override { ownsItemUse.store(false); }
private:
    void onEnable() override { state_.nextAttempt = 0; }
    void onDisable() override {
        keys_.active = false;
        ownsItemUse.store(state_.OwnsItemUse(false));
    }
    void OnTick() {
        if(automlg::IsWorking()||autocharge::IsWorking()){state_={};ownsItemUse.store(false);return;}
        if (!key_.hold()) key_.setHold(true);
        const int key = key_.keybind();
        const bool down = bind::IsPhysicalBindingDown(key);
        auto player = java::PlayerEntity();
        auto world = java::LevelEntity();
        if (!player || !world) {
            state_ = {}; player_.Reset(); world_.Reset();
            keys_.Update(key, down, false);
            ownsItemUse.store(false);
            return;
        }
        if (!player_ || !world_ || !player.IsSame(player_) || !world.IsSame(world_)) {
            // A previous world's pending slot must never be applied to a new player.
            state_ = {};
            player_ = std::move(player); world_ = std::move(world);
            keys_.Update(key, down, false);
        }
        const bool allowed = isEnabled() && player_.IsAlive() && !player_.IsSpectator() &&
            !clickgui::IsOpen() && !java::ScreenOpen() && input::IsProcessForeground();
        const bool active = keys_.Update(key, down, allowed);
        const double delay = delay_->get();
        const double switchDelay = switchDelay_->get();
        const double rodReturnDelay = rodReturnDelay_->get();
        RodIO io{key};
        ownsItemUse.store(state_.OwnsItemUse(active));
        state_.Tick(io, active, static_cast<std::uint64_t>(
            std::isfinite(delay) ? std::clamp(delay, 50.0, 2000.0) : 500.0), GetTickCount64(),
            static_cast<std::uint64_t>(std::isfinite(switchDelay) ? std::clamp(switchDelay, 0.0, 1000.0) : 0.0),
            static_cast<std::uint64_t>(std::isfinite(rodReturnDelay) ? std::clamp(rodReturnDelay, 50.0, 2000.0) : 500.0));
        ownsItemUse.store(state_.OwnsItemUse(active));
    }
    bind::BindProperty key_;
    autorod::KeyState keys_;
    autorod::State state_;
    java::EntityHandle player_, world_;
    Properties::NumberProp* delay_ = nullptr;
    Properties::NumberProp* switchDelay_ = nullptr;
    Properties::NumberProp* rodReturnDelay_ = nullptr;
    event::Subscription tick_;
};
}
namespace autorod {
bool OwnsItemUse() { return ownsItemUse.load(); }
}
std::unique_ptr<Module> MakeAutoRodModule() { return std::make_unique<AutoRodModule>(); }
}
