#include <algorithm>
#include <format>
#include <memory>
#include <random>
#include <string>
#include <utility>
#include <vector>

#include "minecraft/events/EventBus.h"
#include "minecraft/events/GameEvents.h"
#include "minecraft/modules/Module.h"
#include "minecraft/modules/Registry.h"
#include "minecraft/modules/player/cheststealer/ChestStealer.h"
#include "minecraft/modules/player/invmanager/InvManager.h"
#include "minecraft/java/api/base.h"
#include "minecraft/java/api/game_inventory.h"
#include "minecraft/utils/inventory/InventoryUtils.h"
#include "minecraft/utils/inventory/ChestTitle.h"
#include "minecraft/utils/rotation/Random.h"

// ChestStealer -- a port of Starain's module of the same name.
//
// It opens a chest, decides which of its stacks are worth taking, and moves one
// out per delay tick with a shift-click. "Worth taking" means better than what
// the player already has: a stronger sword, a tougher piece of armour, more
// blocks than the cap allows, and so on -- the same ladder the original walks.
//
// The whole chest is read once per tick. The original asks the menu for a slot
// whenever it needs one, and it needs each of them several times over (once to
// judge, once to compare against its neighbours, once more when emptying); at
// fifty-four slots that is a lot of crossings for an answer that does not change
// within a tick.

namespace nova::module {
namespace {

namespace event = nova::event;

// Shared, exactly as the original's statics are: InvManager asks whether the
// stealer has just acted, and the answer has to come from the stealer's own clock.
inventory::Timer g_timer;

// Deterministic and cheap; the project's rotation code uses the same generator.
struct RandomSource {
    using result_type = unsigned;
    static constexpr result_type min() { return 0u; }
    static constexpr result_type max() { return 0xFFFFFFFFu; }
    result_type operator()() const {
        return static_cast<result_type>(rotation::Random() * 4294967296.0);
    }
};

[[nodiscard]] std::string WholeNumber(double value) { return std::format("{:.0f}", value); }

struct ChestSlot {
    java::ItemFacts facts;
    std::string     name;
};

struct ChestView {
    jobject                menu = nullptr;  // borrowed; the caller owns the local ref
    int                    containerId = -1;
    int                    rows = 0;
    std::vector<ChestSlot> slots;
};

// One read of the whole chest. Every slot keeps a local reference only long enough
// to have its facts taken.
bool ReadChest(jobject menu, ChestView& out) {
    out.menu = menu;
    out.containerId = -1;
    out.rows = 0;
    out.slots.clear();

    if (menu == nullptr || !java::MenuIsChest(menu))
        return false;
    if (!java::MenuContainerId(menu, out.containerId))
        return false;
    if (!java::MenuRowCount(menu, out.rows))
        return false;

    const int count = out.rows * 9;
    if (count <= 0)
        return false;

    JniAttach attach;
    if (!attach)
        return false;

    out.slots.reserve(static_cast<size_t>(count));
    for (int i = 0; i < count; ++i) {
        ChestSlot slot;
        jobject stack = nullptr;
        if (java::MenuSlotStack(menu, i, stack)) {
            if (!java::ReadItemFacts(stack, slot.facts)) {
                attach.env->DeleteLocalRef(stack);
                return false;
            }
            if (!slot.facts.empty)
                java::ItemDisplayName(stack, slot.name);
            attach.env->DeleteLocalRef(stack);
        } else {
            return false;
        }
        out.slots.push_back(std::move(slot));
    }
    return true;
}

class ChestStealerModule final : public Module {
public:
    ChestStealerModule() : Module("ChestStealer", Category::Player, "General", "General") {
        minDelay_ = &addNumber("Min Delay", 0, 1000, 1, 125, {}, WholeNumber);
        delay_ = &addNumber("Delay", 0, 1000, 1, 150, {}, WholeNumber);
        closeDelay_ = &addBool("Close Delay", true);
        cDelay_ = &addNumber("Close Delay Number", 0, 1000, 1, 150, {}, WholeNumber,
                             [this] { return closeDelay_->get(); });
        pickEnderChest_ = &addBool("Ender Chest", false);

        motion_ = event::EventBus::Get().Subscribe<event::MotionEvent>(
            [this](event::MotionEvent& e) { OnMotion(e); }, event::Priority::High);
    }

    ~ChestStealerModule() override { ForgetScreen(); }

private:
    void Trace(const char* status) {
        if (lastStatus_ == status)
            return;
        lastStatus_ = status;
        LogTo("ChestStealer: %s", status);
    }

    void onDisable() override { ForgetScreen(); }

    // Starain's lastTickScreen identity check: the frame a container opens is spent
    // only starting its clock, not emptying the chest.
    void RememberScreen(jobject screen) {
        JniAttach attach;
        if (!attach)
            return;
        if (lastScreen_)
            attach.env->DeleteGlobalRef(lastScreen_);
        lastScreen_ = screen ? attach.env->NewGlobalRef(screen) : nullptr;
    }

    [[nodiscard]] bool SameAsLastScreen(jobject screen) const {
        if (lastScreen_ == nullptr || screen == nullptr)
            return false;
        JniAttach attach;
        if (!attach)
            return false;
        return attach.env->IsSameObject(lastScreen_, screen) == JNI_TRUE;
    }

    void ForgetScreen() {
        if (lastScreen_ == nullptr)
            return;
        jobject ref = lastScreen_;
        lastScreen_ = nullptr;
        JniAttach attach;
        if (attach)
            attach.env->DeleteGlobalRef(ref);
    }

    [[nodiscard]] int NextDelay() const {
        const int base = static_cast<int>(delay_->get() +
                                          rotation::Gaussian(0.0, 50.0));
        return std::max(static_cast<int>(minDelay_->get()), base);
    }

    // The ladder from Starain's isItemUseful: keep it if it beats what we carry.
    [[nodiscard]] bool IsItemUseful(const ChestSlot& slot,
                                    const inventory::Snapshot& inventory) const {
        const java::ItemFacts& facts = slot.facts;
        if (facts.empty)
            return false;

        if (inventory::IsGodItem(facts) || inventory::IsSharpnessAxe(facts))
            return true;

        if ((facts.kinds & java::kItemArmor) != 0) {
            const float protection = inventory::Protection(facts);
            const float best = inventory::BestArmorScore(inventory, facts.armorSlot);
            return !(protection <= best);
        }
        if ((facts.kinds & java::kItemSword) != 0)
            return !(inventory::SwordDamage(facts) <= inventory::BestSwordDamage(inventory));
        if ((facts.kinds & java::kItemPickaxe) != 0)
            return !(inventory::ToolScore(facts) <= inventory::BestPickaxeScore(inventory));
        if ((facts.kinds & java::kItemAxe) != 0)
            return !(inventory::ToolScore(facts) <= inventory::BestAxeScore(inventory));
        if ((facts.kinds & java::kItemShovel) != 0)
            return !(inventory::ToolScore(facts) <= inventory::BestShovelScore(inventory));
        if ((facts.kinds & java::kItemCrossbow) != 0)
            return !(inventory::CrossbowScore(facts) <=
                     inventory::BestCrossbowScore(inventory));
        if ((facts.kinds & java::kItemBow) != 0 &&
            inventory::IsPunchBow(facts, slot.name))
            return !(inventory::PunchBowScore(facts) <=
                     inventory::BestPunchBowScore(inventory));
        if ((facts.kinds & java::kItemBow) != 0 &&
            inventory::IsPowerBow(facts, slot.name))
            return !(inventory::PowerBowScore(facts) <=
                     inventory::BestPowerBowScore(inventory));

        if (facts.item == java::KnownItem::Compass)
            return !inventory::HasItem(inventory, java::KnownItem::Compass);

        if (facts.item == java::KnownItem::WaterBucket &&
            inventory::CountItem(inventory, java::KnownItem::WaterBucket) >=
                invmanager::WaterBucketCount())
            return false;
        if (facts.item == java::KnownItem::LavaBucket &&
            inventory::CountItem(inventory, java::KnownItem::LavaBucket) >=
                invmanager::LavaBucketCount())
            return false;

        if ((facts.kinds & java::kItemBlock) != 0 &&
            inventory::IsValidBuildingBlock(facts, slot.name) &&
            inventory::CountBuildingBlocks(inventory) + facts.count >=
                invmanager::MaxBlockSize())
            return false;

        if (facts.item == java::KnownItem::Arrow &&
            inventory::CountItem(inventory, java::KnownItem::Arrow) + facts.count >=
                invmanager::MaxArrowSize())
            return false;

        if ((facts.kinds & java::kItemFishingRod) != 0 &&
            inventory::CountItem(inventory, java::KnownItem::FishingRod) >= 1)
            return false;

        const bool notProjectile = facts.item != java::KnownItem::Snowball &&
                                   facts.item != java::KnownItem::Egg;
        const int heldProjectiles = inventory::CountItem(inventory, java::KnownItem::Snowball) +
                                    inventory::CountItem(inventory, java::KnownItem::Egg);
        if (notProjectile ||
            (heldProjectiles + facts.count < invmanager::MaxProjectileSize() &&
             invmanager::ShouldKeepProjectile())) {
            return (facts.kinds & java::kItemNameBlock) == 0 &&
                   inventory::IsCommonItemUseful(facts);
        }
        return false;
    }

    // Nothing in the chest beats what we already have.
    [[nodiscard]] bool IsChestEmpty(const ChestView& chest,
                                    const inventory::Snapshot& inventory) const {
        for (const ChestSlot& slot : chest.slots) {
            if (!slot.facts.empty && IsItemUseful(slot, inventory) &&
                IsBestItemInChest(chest, slot))
                return false;
        }
        return true;
    }

    // Starain's isBestItemInChest: a piece is only taken when nothing else in the
    // same chest beats it, so a chest holding two swords takes the better one.
    [[nodiscard]] bool IsBestItemInChest(const ChestView& chest,
                                         const ChestSlot& candidate) const {
        const java::ItemFacts& facts = candidate.facts;

        if (inventory::IsGodItem(facts) || inventory::IsSharpnessAxe(facts))
            return true;

        for (const ChestSlot& other : chest.slots) {
            const java::ItemFacts& against = other.facts;
            if (against.empty)
                continue;

            if ((facts.kinds & java::kItemArmor) != 0 &&
                (against.kinds & java::kItemArmor) != 0) {
                if (facts.armorSlot == against.armorSlot &&
                    inventory::Protection(against) > inventory::Protection(facts))
                    return false;
            } else if ((facts.kinds & java::kItemSword) != 0 &&
                       (against.kinds & java::kItemSword) != 0) {
                if (inventory::SwordDamage(against) > inventory::SwordDamage(facts))
                    return false;
            } else if ((facts.kinds & java::kItemPickaxe) != 0 &&
                       (against.kinds & java::kItemPickaxe) != 0) {
                if (inventory::ToolScore(against) > inventory::ToolScore(facts))
                    return false;
            } else if ((facts.kinds & java::kItemAxe) != 0 &&
                       (against.kinds & java::kItemAxe) != 0) {
                if (inventory::ToolScore(against) > inventory::ToolScore(facts))
                    return false;
            } else if ((facts.kinds & java::kItemShovel) != 0 &&
                       (against.kinds & java::kItemShovel) != 0) {
                if (inventory::ToolScore(against) > inventory::ToolScore(facts))
                    return false;
            }
        }
        return true;
    }

    [[nodiscard]] bool TitleMatches(const std::string& title) {
        java::TranslatedText("container.chest", chestTitle_);
        java::TranslatedText("container.chestDouble", largeChestTitle_);
        java::TranslatedText("container.enderchest", enderChestTitle_);
        return inventory::MatchesChestTitle(title, chestTitle_, largeChestTitle_,
                                            enderChestTitle_, pickEnderChest_->get());
    }

    void OnMotion(event::MotionEvent& e) {
        if (e.type != event::EventType::Pre)
            return;

        jobject screen = java::CurrentScreen();
        if (screen != nullptr && isEnabled() && java::ScreenIsContainerScreen(screen)) {
            if (SameAsLastScreen(screen)) {
                Step(screen);
            } else {
                g_timer.Reset();
                lastStatus_.clear();
                Trace("container opened; waiting for next motion tick");
            }
        }

        RememberScreen(screen);
        if (screen != nullptr) {
            JniAttach attach;
            if (attach)
                attach.env->DeleteLocalRef(screen);
        }
    }

    void Step(jobject screen) {
        jobject menu = nullptr;
        if (!java::ScreenMenu(screen, menu)) {
            Trace("screen menu unavailable");
            return;
        }

        ChestView chest;
        const bool readable = ReadChest(menu, chest);
        if (readable) {
            std::string title;
            if (java::ScreenTitle(screen, title) && TitleMatches(title)) {
                inventory::Snapshot inventory;
                if (inventory::Scan(inventory)) {
                    const int nextDelay = NextDelay();
                    if (IsChestEmpty(chest, inventory) && g_timer.Delay(nextDelay)) {
                        if (closeDelay_->get() &&
                            g_timer.Delay(static_cast<long long>(cDelay_->get()))) {
                            java::CloseContainer();
                            g_timer.Reset();
                        }
                        Trace("no useful items in chest");
                    } else {
                        TakeOne(chest, inventory, nextDelay);
                    }
                } else {
                    Trace("player inventory read failed");
                }
            } else {
                if (lastStatus_ != "chest title rejected")
                    LogTo("ChestStealer: title='%s', expected='%s'/'%s'/'%s'", title.c_str(),
                          chestTitle_.c_str(), largeChestTitle_.c_str(), enderChestTitle_.c_str());
                Trace("chest title rejected");
            }
        } else {
            Trace("chest menu or slots unreadable");
        }

        JniAttach attach;
        if (attach)
            attach.env->DeleteLocalRef(menu);
    }

    // One stack per delay: the slots are shuffled so the order does not look like a
    // scan, and the first acceptable one is shift-clicked out.
    void TakeOne(const ChestView& chest, const inventory::Snapshot& inventory, int nextDelay) {
        std::vector<int> order(chest.slots.size());
        for (size_t i = 0; i < order.size(); ++i)
            order[i] = static_cast<int>(i);
        std::shuffle(order.begin(), order.end(), RandomSource{});

        for (int index : order) {
            const ChestSlot& slot = chest.slots[static_cast<size_t>(index)];
            if (!IsItemUseful(slot, inventory) || !IsBestItemInChest(chest, slot))
                continue;
            if (!g_timer.Delay(nextDelay))
                return;

            if (java::ClickMenuSlot(chest.containerId, index, 0, java::ClickKind::QuickMove)) {
                Trace("quick move submitted");
                g_timer.Reset();
            } else {
                Trace("quick move failed");
            }
            return;
        }
    }

    Properties::NumberProp* minDelay_ = nullptr;
    Properties::NumberProp* delay_ = nullptr;
    Properties::BoolProperty* closeDelay_ = nullptr;
    Properties::NumberProp* cDelay_ = nullptr;
    Properties::BoolProperty* pickEnderChest_ = nullptr;

    event::Subscription motion_;

    jobject lastScreen_ = nullptr;
    std::string lastStatus_;
    std::string chestTitle_;
    std::string largeChestTitle_;
    std::string enderChestTitle_;
};

}

namespace cheststealer {

bool IsWorking() { return !g_timer.Delay(3); }

}

std::unique_ptr<Module> MakeChestStealerModule() {
    return std::make_unique<ChestStealerModule>();
}

}
