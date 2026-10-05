#include "minecraft/modules/movement/elytrafly/EquipLogic.h"
#include "minecraft/modules/movement/elytrafly/AutoFire.h"
#include "minecraft/modules/Module.h"
#include "minecraft/modules/Registry.h"
#include "minecraft/modules/player/automlg/AutoMLG.h"
#include "minecraft/modules/player/autocharge/AutoCharge.h"
#include "minecraft/events/GameEvents.h"
#include "minecraft/compat/Profile.h"
#include "minecraft/java/api/game_inventory.h"
#include "minecraft/java/api/game_itemuse.h"
#include "minecraft/java/api/game_slotspoof.h"
#include "minecraft/java/api/game_place.h"
#include "minecraft/java/api/game_click.h"
#include "minecraft/utils/rotation/RotationManager.h"
#include "bind/PhysicalKey.h"
#include "utils/Keyboard.h"
#include "ui/clickgui/ClickGui.h"
#include "ui/notifications/Notifications.h"
#include <atomic>

namespace nova::module {
namespace {
class ElytraFly final : public Module {
public:
    ElytraFly() : Module("ElytraFly", Category::Movement, "General", "General"), key_(0, true) {
        key_.setGate([] { return false; });
        equip_ = &addBool("Auto Equip", true, {Properties::PropDesc("Key", &key_)});
        fire_ = &addBool("Auto Fire", false);
        auto& bus = event::EventBus::Get();
        tick_ = bus.Subscribe<event::GameTickEvent>([this](auto& e) {
            if (e.type == event::EventType::Pre) Tick();
        }, event::Priority::Highest);
        interact_ = bus.Subscribe<event::PrePlayerInteractEvent>([this](auto&) { Interact(); }, event::Priority::Low);
        respawn_ = bus.Subscribe<event::RespawnEvent>([this](auto&) { reset_.store(true); }, event::Priority::Highest);
    }
    ~ElytraFly() override { tick_.Reset(); interact_.Reset(); respawn_.Reset(); }
private:
    enum class Phase { Idle, Equip, ConfirmEquip, Worn, Restore, ConfirmRestore };
    struct SlotIO {
        bool Selected(int& out) { return java::SelectedSlot(out); }
        bool Select(int slot) { return java::SetSelectedSlot(slot); }
        bool Sync() { return java::SyncSelectedItem(); }
    };
    void onEnable() override { requested_.store(true); }
    void onDisable() override { requested_.store(false); }
    void Warn(const char* text) { notifications::Post(std::string("ElytraFly: ") + text, notifications::Type::Warning); }
    void Clear() {
        phase_ = Phase::Idle; source_ = -1; wait_ = 0; warned_ = false; equipSubmitted_ = false;
        original_.Reset();
        // A failed native use may already have selected the hotbar item. Keep
        // its world identity until the deferred selection restoration finishes.
        if (selection_.previous < 0) { player_.Reset(); world_.Reset(); }
        trigger_.desired = false;
    }
    bool SameWorld() const {
        auto player = java::PlayerEntity(), world = java::LevelEntity();
        return player && world && player.IsSame(player_) && world.IsSame(world_) &&
            player.IsAlive() && !player.IsSpectator();
    }
    bool Read(int slot, jobject& stack) {
        return slot < 0 ? java::InventoryArmor(2, stack) : java::InventorySlot(slot, stack);
    }
    bool Facts(int slot, java::ItemFacts& facts) {
        JniAttach attach; jobject stack = nullptr;
        if (!attach) return false;
        const bool ok = Read(slot, stack) && java::ReadItemFacts(stack, facts);
        if (stack) attach->DeleteLocalRef(stack);
        return ok;
    }
    java::EntityHandle Snapshot(int slot) {
        JniAttach attach; jobject stack = nullptr, copy = nullptr;
        if (!attach) return {};
        java::EntityHandle result;
        if (Read(slot, stack) && java::CopyItemStack(stack, copy)) result = java::AdoptEntity(copy);
        if (stack) attach->DeleteLocalRef(stack);
        if (copy) attach->DeleteLocalRef(copy);
        return result;
    }
    bool Matches(int slot, const java::EntityHandle& saved) {
        JniAttach attach; jobject stack = nullptr;
        if (!attach || !saved) return false;
        const bool ok = Read(slot, stack) && java::ItemStacksMatch(stack, static_cast<jobject>(saved.raw()));
        if (stack) attach->DeleteLocalRef(stack);
        return ok;
    }
    bool Available() {
        int physical = -1, effective = -1;
        return input::IsProcessForeground() && !clickgui::IsOpen() && !java::ScreenOpen() &&
            !automlg::IsWorking() && !autocharge::IsWorking() && java::SelectedSlot(physical) &&
            java::EffectiveSelectedSlot(effective) && physical == effective;
    }
    void Begin() {
        auto player = java::PlayerEntity(), world = java::LevelEntity();
        if (!player || !world || !player.IsAlive() || player.IsSpectator()) { Clear(); return; }
        java::ItemFacts chest;
        if (!Facts(-1, chest) || chest.item == java::KnownItem::Elytra) { Clear(); return; }
        source_ = -1;
        for (int i = 0; i < 9; ++i) {
            java::ItemFacts item;
            if (Facts(i, item) && !item.empty && item.item == java::KnownItem::Elytra &&
                (item.maxDamage <= 0 || item.damageValue < item.maxDamage - 1)) { source_ = i; break; }
        }
        if (source_ < 0) { Warn("Put an elytra in the hotbar"); Clear(); return; }
        original_ = Snapshot(-1); originalEmpty_ = chest.empty;
        if (!original_) { Clear(); return; }
        player_ = std::move(player); world_ = std::move(world);
        phase_ = Phase::Equip;
    }
    void Tick() {
        attempted_ = false;
        if (reset_.load()) autoFire_.Reset();
        autoFire_.Tick(requested_.load() && fire_->get());
        const bool down = bind::IsPhysicalBindingDown(key_.keybind());
        if (reset_.exchange(false) || ((phase_ != Phase::Idle || selection_.previous >= 0) && !SameWorld())) {
            selection_.Reset(); Clear(); trigger_.lastDown = down;
        }
        SlotIO io;
        selection_.Restore(io);
        if (phase_ == Phase::Idle) { player_.Reset(); world_.Reset(); }
        trigger_.Tick(requested_.load() && equip_->get(), key_.keybind(), key_.hold(), down,
            input::IsProcessForeground(), java::ScreenOpen() || clickgui::IsOpen());
        if (phase_ == Phase::ConfirmEquip || phase_ == Phase::ConfirmRestore) {
            java::ItemFacts chest;
            const bool read = Facts(-1, chest);
            const bool equipped = phase_ == Phase::ConfirmEquip;
            if (read && (equipped ? chest.item == java::KnownItem::Elytra : Matches(-1, original_))) {
                if (equipped) phase_ = Phase::Worn;
                else Clear();
            } else if (read && !equipped && chest.item != java::KnownItem::Elytra) {
                // A manual swap or durability change after restoring must not
                // make us overwrite the chest slot again.
                Clear();
            } else if (++wait_ >= 10) {
                // A delayed/denied native use is not proof that restoration is
                // complete. Keep the original stack and release intent alive.
                if (!warned_) { Warn("Waiting for equipment swap confirmation"); warned_ = true; }
                // A successful controller result may still be awaiting its
                // inventory update. Repeating it can swap BACK to the elytra.
                // Only retry a refused/failed use after re-reading both slots.
                if (read && !useAccepted_ && !equipped && chest.item == java::KnownItem::Elytra) {
                    phase_ = Phase::Restore;
                } else if (read && !useAccepted_ && equipped && trigger_.desired && Matches(-1, original_)) {
                    // Retry equipping only while still requested. After release
                    // keep observing a late equip, never throw away its undo.
                    phase_ = Phase::Equip;
                }
            }
            return; // Never equip and restore within the same game tick.
        }
        if (phase_ == Phase::Equip && !trigger_.desired) {
            if (equipSubmitted_) phase_ = Phase::ConfirmEquip;
            else Clear();
            return;
        }
        if (phase_ == Phase::Worn && !trigger_.desired) {
            if (originalEmpty_) {
                Warn("No previous chestplate: right-click cannot unequip to an empty chest slot");
                Clear();
            } else phase_ = Phase::Restore;
        }
        if (phase_ == Phase::Idle && trigger_.desired && Available()) {
            if (compat::IsLegacyLunar()) { Warn("This Minecraft version has no elytra"); Clear(); }
            else Begin();
        }
    }
    void Interact() {
        if(compat::UsesModernRuntime() && !rotation::CanSubmitItemUse())return;
        // Equipment changes take priority; never submit a rocket and an equip
        // action together. Auto Fire also works with a manually equipped elytra.
        if (phase_ == Phase::Idle || phase_ == Phase::Worn) {
            autoFire_.Interact(requested_.load() && fire_->get());
            return;
        }
        if (attempted_ || (phase_ != Phase::Equip && phase_ != Phase::Restore) || !SameWorld() || !Available()) return;
        const bool restore = phase_ == Phase::Restore;
        java::ItemFacts chest;
        if (!Facts(-1, chest)) return;
        if (restore) {
            if (chest.item != java::KnownItem::Elytra) { Clear(); return; }
            source_ = -1;
            for (int i = 0; i < 9; ++i) if (Matches(i, original_)) { source_ = i; break; }
            if (source_ < 0) {
                // Transient inventory reads and temporarily moving the armor
                // out of the hotbar must not discard the pending Hold release.
                if (!warned_) { Warn("Waiting for the original chestplate in the hotbar"); warned_ = true; }
                return;
            }
        } else {
            if (!requested_.load() || !equip_->get() || (key_.hold() && !bind::IsPhysicalBindingDown(key_.keybind()))) {
                if (equipSubmitted_) phase_ = Phase::ConfirmEquip;
                else Clear();
                return;
            }
            java::ItemFacts item;
            if (!Matches(-1, original_) || !Facts(source_, item) || item.item != java::KnownItem::Elytra) { Clear(); return; }
        }
        auto expected = Snapshot(source_);
        int old = -1;
        if (!expected || !java::SelectedSlot(old)) return;
        attempted_ = true;
        selection_.Capture(old, source_);
        if (!java::SetSelectedSlot(source_) || !java::SyncSelectedItem()) return;
        // Exactly one native right-click. The controller owns selected-slot sync,
        // use sequencing and the equipment swap. No container packets are sent.
        if (!restore) equipSubmitted_ = true;
        useAccepted_ = java::UseSelectedEquipment(expected);
        phase_ = restore ? Phase::ConfirmRestore : Phase::ConfirmEquip;
        wait_ = 0;
    }
    bind::BindProperty key_;
    Properties::BoolProperty *equip_{}, *fire_{};
    elytrafly::AutoFire autoFire_;
    std::atomic<bool> requested_{false}, reset_{false};
    elytrafly::Trigger trigger_;
    elytrafly::Selection selection_;
    java::EntityHandle original_, player_, world_;
    Phase phase_ = Phase::Idle;
    bool originalEmpty_ = false, attempted_ = false, warned_ = false, equipSubmitted_ = false, useAccepted_ = false;
    int source_ = -1, wait_ = 0;
    event::Subscription tick_, interact_, respawn_;
};
}
std::unique_ptr<Module> MakeElytraFlyModule() { return std::make_unique<ElytraFly>(); }
}
