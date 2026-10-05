#include "minecraft/modules/player/refill/Refill.h"
#include "minecraft/modules/player/autoreplenish/AutoReplenish.h"
#include "minecraft/modules/player/refill/RefillLogic.h"
#include "minecraft/modules/player/cheststealer/ChestStealer.h"
#include "minecraft/modules/Module.h"
#include "minecraft/modules/Registry.h"
#include "minecraft/events/GameEvents.h"
#include "minecraft/java/api/game_potion.h"
#include "minecraft/java/api/game_inventory.h"
#include "minecraft/java/api/game_click.h"
#include "ui/clickgui/ClickGui.h"
#include "minecraft/utils/inventory/InventoryUtils.h"
#include "minecraft/utils/rotation/Random.h"
#include "utils/Keyboard.h"
#include <atomic>
#include <format>

namespace nova::module {
namespace {
std::atomic<bool> working{false};

class RefillModule final : public Module {
public:
    RefillModule() : Module("Refill", Category::Player, "General", "General") {
        const auto integer = [](double value) { return std::format("{:.0f} ms", value); };
        minDelay_ = &addNumber("Min Delay", 0, 1000, 1, 125, {}, integer);
        delay_ = &addNumber("Delay", 0, 1000, 1, 150, {}, integer);
        closeDelay_ = &addNumber("Close Delay", 0, 1000, 1, 100, {}, integer);
        tick_ = event::EventBus::Get().Subscribe<event::GameTickEvent>([this](auto& e) {
            if (e.type == event::EventType::Pre) OnTick();
        }, event::Priority::High);
    }

    // IO for the one-shot session. Use the same Snapshot/Timer/Swap primitive as
    // InvManager; no cursor pickup sequence, synthetic input or packet bypass.
    bool Plan(refill::Move& move) {
        inventory::Snapshot snapshot;
        if (!inventory::Scan(snapshot) || inventory::ShouldDisableFeatures(snapshot)) return false;
        move.target = inventory::FirstEmptyHotbarSlot(snapshot);
        if (move.target < 0) return true;
        for (const auto& entry : snapshot.items) {
            if (entry.slot >= 9 && entry.slot < 36 && !entry.facts.empty &&
                java::InventoryHasHealingSplashPotion(entry.slot)) {
                move.source = entry.slot;
                break;
            }
        }
        return true;
    }
    bool Begin() {
        JniAttach attach;
        if (!attach) return false;
        jobject screen = java::CurrentScreen();
        if (!screen) {
            if (!java::OpenPlayerInventory(screen)) return false;
            opened_ = true;
        }
        screen_ = java::AdoptEntity(screen);
        if (screen) attach->DeleteLocalRef(screen);
        return screen_ && CurrentInventory();
    }
    bool MoveItem(const refill::Move& move) {
        if (!move.Valid() || !CurrentInventory() || !java::InventoryCursorEmpty()) return false;
        JniAttach attach;
        if (!attach) return false;
        jobject stack = nullptr;
        java::ItemFacts target;
        const bool empty = java::InventorySlot(move.target, stack) &&
            java::ReadItemFacts(stack, target) && target.empty;
        if (stack) attach->DeleteLocalRef(stack);
        if (!empty || !java::InventoryHasHealingSplashPotion(move.source)) return false;
        // Exactly InvManager::SwapItem's slot convention: main row index is the
        // menu source; hotbar index is the number-key/Swap button.
        if (!java::ClickMenuSlot(menuId_, move.source, move.target, java::ClickKind::Swap)) return false;
        // JNI success alone is not a moved item. Stop on a refused/no-op click
        // instead of clicking the same rejected source forever.
        return java::HotbarHasHealingSplashPotion(move.target);
    }
    bool MoveDue() const { return timer_.Delay(nextDelay_); }
    bool CloseDue() const { return timer_.Delay(static_cast<int>(closeDelay_->get())); }
    void ResetMoveDelay() {
        nextDelay_ = std::max(static_cast<int>(minDelay_->get()),
            static_cast<int>(delay_->get() + rotation::Gaussian(0, 50)));
        timer_.Reset();
    }
    void ResetCloseDelay() { timer_.Reset(); }
    void End() {
        // Never close a different screen, a new world's inventory, or an item
        // the user picked up with the cursor. A pre-opened inventory stays open.
        if (opened_ && SameWorld() && CurrentInventory() && java::InventoryCursorEmpty())
            java::CloseContainer();
        opened_ = false;
        screen_.Reset(); player_.Reset(); world_.Reset();
        menuId_ = -1;
        requested_.store(false);
        working.store(false);
        setEnabled(false);
    }

private:
    void onEnable() override { restart_.store(true); requested_.store(true); working.store(true); }
    void onDisable() override { requested_.store(false); }
    bool SameWorld() const {
        auto player = java::PlayerEntity();
        auto world = java::LevelEntity();
        return player && world && player_.IsSame(player) && world_.IsSame(world);
    }
    bool CurrentInventory() {
        JniAttach attach;
        if (!attach || !screen_) return false;
        auto current = java::CurrentScreen();
        const bool same = current && attach->IsSameObject(current, static_cast<jobject>(screen_.raw()));
        jobject menu = nullptr;
        int id = -1, playerId = -1;
        const bool valid = same && java::ScreenIsInventoryScreen(current) &&
            java::ScreenMenu(current, menu) && java::MenuContainerId(menu, id) &&
            java::PlayerMenuContainerId(playerId) && id == playerId &&
            (menuId_ < 0 || id == menuId_);
        if (menu) attach->DeleteLocalRef(menu);
        if (current) attach->DeleteLocalRef(current);
        if (valid) menuId_ = id;
        return valid;
    }
    void OnTick() {
        if(autoreplenish::IsWorking())return;
        // Enable/disable can originate from the render UI. JNI inventory actions
        // and ownership cleanup stay on the game tick thread.
        if (!requested_.load()) {
            if (session_.phase != refill::Session::Phase::Done) session_.Finish(*this);
            working.store(false);
            return;
        }
        if (restart_.exchange(false)) {
            if (opened_) { End(); requested_.store(true); working.store(true); setEnabled(true); }
            session_ = {};
            player_ = java::PlayerEntity(); world_ = java::LevelEntity();
        }
        if (!SameWorld() || !player_.IsAlive() || player_.IsSpectator()) { session_.Finish(*this); return; }
        if (session_.phase == refill::Session::Phase::Ready) {
            // Enabling from ClickGUI waits for it to close before displaying the inventory.
            if (clickgui::IsOpen() || !input::IsProcessForeground()) return;
            if (java::ScreenOpen() && !java::InventoryScreenOpen()) { session_.Finish(*this); return; }
            if (cheststealer::IsWorking() || !java::InventoryCursorEmpty()) { session_.Finish(*this); return; }
        } else if (clickgui::IsOpen() || !input::IsProcessForeground() ||
                   !CurrentInventory() || !java::InventoryCursorEmpty()) {
            session_.Finish(*this);
            return;
        }
        session_.Tick(*this);
    }

    Properties::NumberProp *minDelay_{}, *delay_{}, *closeDelay_{};
    std::atomic<bool> requested_{false}, restart_{false};
    refill::Session session_{refill::Session::Phase::Done};
    inventory::Timer timer_;
    int nextDelay_ = 0, menuId_ = -1;
    bool opened_ = false;
    java::EntityHandle screen_, player_, world_;
    event::Subscription tick_;
};
}
namespace refill { bool IsWorking() { return working.load(); } }
std::unique_ptr<Module> MakeRefillModule() { return std::make_unique<RefillModule>(); }
}
