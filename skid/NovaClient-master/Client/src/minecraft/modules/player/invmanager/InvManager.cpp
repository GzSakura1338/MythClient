#include <algorithm>
#include <format>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "minecraft/events/EventBus.h"
#include "minecraft/events/GameEvents.h"
#include "minecraft/modules/Module.h"
#include "minecraft/modules/player/automlg/AutoMLG.h"
#include "minecraft/modules/combat/automace/AutoMace.h"
#include "minecraft/modules/combat/autototem/AutoTotem.h"
#include "minecraft/modules/player/autoreplenish/AutoReplenish.h"
#include "minecraft/modules/player/autocharge/AutoCharge.h"
#include "minecraft/modules/Registry.h"
#include "minecraft/modules/player/cheststealer/ChestStealer.h"
#include "minecraft/modules/player/invmanager/InvManager.h"
#include "minecraft/modules/movement/invmove/InventoryMove.h"
#include "minecraft/compat/Profile.h"
#include "minecraft/modules/player/refill/Refill.h"
#include "minecraft/java/api/game_potion.h"
#include "minecraft/java/api/base.h"
#include "minecraft/java/api/game_inventory.h"
#include "minecraft/utils/inventory/InventoryUtils.h"
#include "minecraft/utils/rotation/Random.h"

// InvManager -- a port of Starain's module of the same name.
//
// It keeps the hotbar arranged: the best sword in one slot, the best pickaxe in
// another, blocks, a bow, buckets, pearls, and whatever the offhand is set to
// hold. With Drop enabled, unwanted or excess items are thrown out according
// to the configured filters and counts.
//
// Every decision is made from one Snapshot of the inventory per tick. The
// original asks the game for a stack each time it wants one, and it wants them
// constantly -- the "best sword" query alone walks the whole inventory, and it is
// asked once per sword in it.

namespace nova::module {
namespace {

namespace event = nova::event;

inventory::Timer g_timer;

[[nodiscard]] std::string WholeNumber(double value) { return std::format("{:.0f}", value); }
[[nodiscard]] std::string Milliseconds(double value) { return std::format("{:.0f} ms", value); }

[[nodiscard]] int NextDelay(double minDelay, double delay) {
    const int base = static_cast<int>(delay + rotation::Gaussian(0.0, 50.0));
    return std::max(static_cast<int>(minDelay), base);
}

// The offhand modes, in the order the setting lists them.
enum OffhandMode : int {
    kOffhandNone = 0,
    kOffhandGoldenApple,
    kOffhandProjectile,
    kOffhandFishingRod,
    kOffhandBlock,
};

enum BowPriority : int {
    kPreferCrossbow = 0,
    kPreferPowerBow,
    kPreferPunchBow,
};

// The settings the chest stealer reads out of this module. They are live property
// objects, so a change takes effect whether or not this module is switched on.
Properties::NumberProp* g_maxBlockSize = nullptr;
Properties::NumberProp* g_maxArrowSize = nullptr;
Properties::NumberProp* g_maxProjectileSize = nullptr;
Properties::NumberProp* g_waterBucketCount = nullptr;
Properties::NumberProp* g_lavaBucketCount = nullptr;
Properties::BoolProperty* g_keepProjectile = nullptr;
Module* g_manager = nullptr;
Properties::BoolProperty* g_inventoryOnly = nullptr;

class InvManagerModule final : public Module {
public:
    InvManagerModule() : Module("InvManager", Category::Player, "General", "General") {
        minDelay_ = &addNumber("Min Delay", 0, 1000, 1, 125, {}, Milliseconds);
        delay_ = &addNumber("Delay", 0, 1000, 1, 150, {}, Milliseconds);
        offhandItems_ = &addMode("Offhand Items", kOffhandNone,
                                 {"None", "Golden Apple", "Projectile", "Fishing Rod", "Block"},
                                 {}, [] { return java::InventoryHasOffhand(); });
        autoArmor_ = &addBool("Auto Armor", true);
        drop_ = &addBool("Drop", false);
        inventoryOnly_ = &addBool("Inventory Only", true);
        g_manager=this;g_inventoryOnly=inventoryOnly_;

        switchSword_ = &addBool("Switch Sword", true);
        swordSlot_ = &addNumber("Sword Slot", 1, 9, 1, 1, {}, WholeNumber,
                                [this] { return switchSword_->get(); });

        switchBlock_ = &addBool("Switch Block", true,
                                {}, [this] { return OffhandMode() != kOffhandBlock; });
        blockSlot_ = &addNumber("Block Slot", 1, 9, 1, 2, {}, WholeNumber, [this] {
            return switchBlock_->get() && OffhandMode() != kOffhandBlock;
        });
        maxBlockSize_ = &addNumber("Max Block Size", 64, 512, 64, 256, {}, WholeNumber,
                                   [this] { return switchBlock_->get(); });

        switchPickaxe_ = &addBool("Switch Pickaxe", true);
        pickaxeSlot_ = &addNumber("Pickaxe Slot", 1, 9, 1, 3, {}, WholeNumber,
                                  [this] { return switchPickaxe_->get(); });

        switchAxe_ = &addBool("Switch Axe", true);
        axeSlot_ = &addNumber("Axe Slot", 1, 9, 1, 4, {}, WholeNumber,
                              [this] { return switchAxe_->get(); });

        switchBow_ = &addBool("Switch Bow or Crossbow", true);
        bowSlot_ = &addNumber("Bow Slot", 1, 9, 1, 5, {}, WholeNumber,
                              [this] { return switchBow_->get(); });
        preferBow_ = &addMode("Bow Priority", kPreferCrossbow,
                              {"Crossbow", "Power Bow", "Punch Bow"},
                              {}, [this] { return switchBow_->get(); });
        maxArrowSize_ = &addNumber("Max Arrow Size", 64, 512, 64, 256, {}, WholeNumber,
                                   [this] { return switchBow_->get(); });

        switchWaterBucket_ = &addBool("Switch Water Bucket", true);
        waterBucketSlot_ = &addNumber("Water Bucket Slot", 1, 9, 1, 6, {}, WholeNumber,
                                      [this] { return switchWaterBucket_->get(); });

        switchEnderPearl_ = &addBool("Switch Ender Pearl", true);
        enderPearlSlot_ = &addNumber("Ender Pearl Slot", 1, 9, 1, 7, {}, WholeNumber,
                                     [this] { return switchEnderPearl_->get(); });

        switchFireball_ = &addBool("Switch Fireball", true);
        fireballSlot_ = &addNumber("Fireball Slot", 1, 9, 1, 8, {}, WholeNumber,
                                   [this] { return switchFireball_->get(); });

        switchGoldenApple_ = &addBool("Switch Golden Apple", true, {}, [this] {
            return OffhandMode() != kOffhandGoldenApple;
        });
        goldenAppleSlot_ = &addNumber("Golden Apple Slot", 1, 9, 1, 9, {}, WholeNumber, [this] {
            return switchGoldenApple_->get() && OffhandMode() != kOffhandGoldenApple;
        });

        throwItems_ = &addBool("Throw Items", true);
        waterBucketCount_ = &addNumber("Keep Water Buckets", 0, 5, 1, 1, {}, WholeNumber,
                                       [this] { return throwItems_->get(); });
        lavaBucketCount_ = &addNumber("Keep Lava Buckets", 0, 5, 1, 1, {}, WholeNumber,
                                      [this] { return throwItems_->get(); });

        keepProjectile_ = &addBool("Keep Eggs & Snowballs", true);
        switchProjectile_ = &addBool("Switch Eggs & Snowballs", false, {}, [this] {
            return keepProjectile_->get() && OffhandMode() != kOffhandProjectile;
        });
        projectileSlot_ = &addNumber("Eggs & Snowballs Slot", 1, 9, 1, 9, {}, WholeNumber,
                                     [this] {
                                         return switchProjectile_->get() &&
                                                keepProjectile_->get() &&
                                                OffhandMode() != kOffhandProjectile;
                                     });
        maxProjectileSize_ =
            &addNumber("Max Eggs & Snowballs Size", 16, 256, 16, 64, {}, WholeNumber,
                       [this] { return keepProjectile_->get(); });

        switchRod_ = &addBool("Switch Rod", false, {},
                              [this] { return OffhandMode() != kOffhandFishingRod; });
        rodSlot_ = &addNumber("Rod Slot", 1, 9, 1, 9, {}, WholeNumber, [this] {
            return switchRod_->get() && OffhandMode() != kOffhandFishingRod;
        });

        g_maxBlockSize = maxBlockSize_;
        g_maxArrowSize = maxArrowSize_;
        g_maxProjectileSize = maxProjectileSize_;
        g_waterBucketCount = waterBucketCount_;
        g_lavaBucketCount = lavaBucketCount_;
        g_keepProjectile = keepProjectile_;

        BuildSettings();

        event::EventBus& bus = event::EventBus::Get();
        motion_ = bus.Subscribe<event::MotionEvent>(
            [this](event::MotionEvent& e) { OnMotion(e); });
        packet_ = bus.Subscribe<event::PacketEvent>(
            [this](event::PacketEvent& e) { OnPacket(e); });
    }

    const std::vector<Properties::PropDesc>& configSettings() const override { return configRows_; }

private:
    void BuildSettings() {
        // Only the presentation is grouped. Keep historical flat keys for loading
        // existing configurations and for shared ChestStealer property references.
        configRows_ = settings();
        using Row = Properties::PropDesc;
        const auto row = [this](Properties::AbstractProperty* property, const char* label,
                                std::vector<Row> children = {}) {
            const auto original = *std::find_if(configRows_.begin(), configRows_.end(),
                [property](const Row& candidate) { return candidate.getProp() == property; });
            return Row(label, property, std::move(children), [original] { return original.isVisible(); });
        };
        std::vector<Row> hotbar{
            row(switchSword_, "Sword", {row(swordSlot_, "Slot")}),
            row(switchBlock_, "Blocks", {row(blockSlot_, "Slot")}),
            row(switchPickaxe_, "Pickaxe", {row(pickaxeSlot_, "Slot")}),
            row(switchAxe_, "Axe", {row(axeSlot_, "Slot")}),
            row(switchBow_, "Bow / Crossbow", {row(bowSlot_, "Slot"), row(preferBow_, "Priority")}),
            row(switchWaterBucket_, "Water Bucket", {row(waterBucketSlot_, "Slot")}),
            row(switchEnderPearl_, "Ender Pearl", {row(enderPearlSlot_, "Slot")}),
            row(switchFireball_, "Fireball", {row(fireballSlot_, "Slot")}),
            row(switchGoldenApple_, "Golden Apple", {row(goldenAppleSlot_, "Slot")}),
            row(switchProjectile_, "Eggs & Snowballs", {row(projectileSlot_, "Slot")}),
            row(switchRod_, "Fishing Rod", {row(rodSlot_, "Slot")}),
        };
        std::vector<Row> limits{
            row(keepProjectile_, "Keep Eggs & Snowballs"),
            row(maxProjectileSize_, "Max Eggs & Snowballs"),
            row(maxBlockSize_, "Max Blocks"),
            row(maxArrowSize_, "Max Arrows"),
            row(waterBucketCount_, "Water Buckets"),
            row(lavaBucketCount_, "Lava Buckets"),
        };
        // Enabled / Bind stay at the start, as in other module setting pages.
        std::vector<Row> display{configRows_[0], configRows_[1]};
        display.push_back(row(inventoryOnly_, "Inventory Only"));
        display.push_back(row(delay_, "Delay", {row(minDelay_, "Minimum Delay")}));
        display.push_back(row(autoArmor_, "Auto Armor"));
        display.push_back(row(offhandItems_, "Offhand Items"));
        display.emplace_back("Hotbar Slots", nullptr, std::move(hotbar));
        display.emplace_back("Item Limits", nullptr, std::move(limits));
        display.push_back(row(drop_, "Drop", {row(throwItems_, "Discard Unwanted")}));
        replaceSettings(std::move(display));
    }

    void onDisable() override { invmove::ReleaseManager(); inventoryOpen_=false; clickOffHand_=false; }
    bool ClickSlot(int id,int slot,int button,java::ClickKind kind) {
        if(invmanager::UsesInventoryMovement()&&!invmove::PrepareManagerClick())return false;
        const bool clicked=java::ClickMenuSlot(id,slot,button,kind);
        if(clicked)invmove::ManagerClicked();
        return clicked;
    }
    void OnPacket(event::PacketEvent& e) {
        if (!isEnabled() || e.type != event::EventType::Outgoing || e.packet == nullptr)
            return;

        if (java::PacketIsContainerClose(e.packet)) {
            inventoryOpen_ = false;
            return;
        }

        if (refill::IsWorking() || autoreplenish::IsWorking()) return;
        if (invmanager::UsesInventoryMovement()) return;

        if (!inventoryOpen_ || inventoryOnly_->get())
            return;

        // While the inventory is being arranged, any action the player takes has to
        // close it first -- the server will not accept the action otherwise.
        int containerId = -1;
        if (java::PacketIsMovePlayer(e.packet)) {
            if (forwardInput_ == 0.0f && strafeInput_ == 0.0f)
                return;
            if (java::PlayerMenuContainerId(containerId))
                java::SendContainerClose(containerId);
            return;
        }

        if (java::PacketIsUseItemOn(e.packet) || java::PacketIsUseItem(e.packet) ||
            java::PacketIsInteract(e.packet) || java::PacketIsPlayerAction(e.packet)) {
            if (java::PlayerMenuContainerId(containerId))
                java::SendContainerClose(containerId);
        }
    }

    // Starain's isItemUseful: does this stack deserve its place in the inventory?
    // The comparisons are against the best the player holds, so exactly one of each
    // kind survives.
    [[nodiscard]] bool IsItemUseful(const inventory::Entry& entry,
                                    const inventory::Snapshot& inventory) const {
        const java::ItemFacts& facts = entry.facts;
        if (facts.empty)
            return false;

        if (inventory::IsGodItem(facts))
            return true;
        if (entry.name.find("\xe7\x82\xb9\xe5\x87\xbb\xe4\xbd\xbf\xe7\x94\xa8") !=
            std::string::npos) {
            return true;  // 点击使用 -- a shop item the server wants kept
        }

        if ((facts.kinds & java::kItemArmor) != 0) {
            const float protection = inventory::Protection(facts);
            if (inventory::CurrentArmorScore(inventory, facts.armorSlot) >= protection)
                return false;
            return !(protection < inventory::BestArmorScore(inventory, facts.armorSlot));
        }
        if ((facts.kinds & java::kItemSword) != 0)
            return inventory::BestSword(inventory) == entry.slot;
        if ((facts.kinds & java::kItemPickaxe) != 0)
            return inventory::BestPickaxe(inventory) == entry.slot;
        if ((facts.kinds & java::kItemAxe) != 0 && !inventory::IsSharpnessAxe(facts))
            return inventory::BestAxe(inventory) == entry.slot;
        if ((facts.kinds & java::kItemShovel) != 0)
            return inventory::BestShovel(inventory) == entry.slot;
        if ((facts.kinds & java::kItemCrossbow) != 0)
            return inventory::BestCrossbow(inventory) == entry.slot;
        if ((facts.kinds & java::kItemBow) != 0 && inventory::IsPunchBow(facts, entry.name))
            return inventory::BestPunchBow(inventory) == entry.slot;
        if ((facts.kinds & java::kItemBow) != 0 && inventory::IsPowerBow(facts, entry.name))
            return inventory::BestPowerBow(inventory) == entry.slot;
        if ((facts.kinds & java::kItemBow) != 0 &&
            inventory::CountItem(inventory, java::KnownItem::Bow) > 1)
            return false;

        if (facts.item == java::KnownItem::WaterBucket &&
            inventory::CountItem(inventory, java::KnownItem::WaterBucket) >
                static_cast<int>(waterBucketCount_->get()))
            return false;
        if (facts.item == java::KnownItem::LavaBucket &&
            inventory::CountItem(inventory, java::KnownItem::LavaBucket) >
                static_cast<int>(lavaBucketCount_->get()))
            return false;
        if ((facts.kinds & java::kItemFishingRod) != 0 &&
            inventory::CountItem(inventory, java::KnownItem::FishingRod) > 1)
            return false;

        const bool projectile = facts.item == java::KnownItem::Snowball ||
                                facts.item == java::KnownItem::Egg;
        if (projectile && !keepProjectile_->get())
            return false;

        if (facts.item == java::KnownItem::GoldenApple ||
            facts.item == java::KnownItem::EnchantedGoldenApple)
            return true;

        return (facts.kinds & java::kItemNameBlock) == 0 &&
               inventory::IsCommonItemUseful(facts);
    }

    void OnMotion(event::MotionEvent& e) {
        if (e.type != event::EventType::Pre)
            return;

        forwardInput_ = e.type == event::EventType::Pre ? forwardInput_ : 0.0f;
        // The move input is what decides whether the player is walking; it arrives
        // from a different hook, so it is cached rather than read here.
        if (!isEnabled() || refill::IsWorking() || autoreplenish::IsWorking() || autototem::IsWorking() || automace::IsWorking() || automlg::IsWorking() || autocharge::IsWorking())
            return;

        inventory::Snapshot inventory;
        if (!inventory::Scan(inventory))
            return;

        if (inventory::ShouldDisableFeatures(inventory))
            return;

        const bool moving = forwardInput_ != 0.0f || strafeInput_ != 0.0f;
        if (moving)
            noMoveTicks_ = 0;
        else
            ++noMoveTicks_;

        const bool allowMove = !inventoryOnly_->get();
        jobject screen = java::CurrentScreen();
        const bool screenIsInventory = java::ScreenIsInventoryScreen(screen);
        if (screen != nullptr) {
            JniAttach attach;
            if (attach)
                attach.env->DeleteLocalRef(screen);
        }

        // Stand down while the chest stealer is mid-action, and while the player is
        // walking with the inventory open unless that was asked for.
        if (cheststealer::IsWorking() ||
            (inventoryOnly_->get() ? !screenIsInventory
                                   : (!allowMove && noMoveTicks_ <= 1))) {
            clickOffHand_ = false;
            return;
        }

        jobject current = java::CurrentScreen();
        if (current != nullptr) {
            jobject menu = nullptr;
            const bool isContainer = java::ScreenIsContainerScreen(current);
            if (isContainer && java::ScreenMenu(current, menu)) {
                int menuId = -1;
                int playerMenuId = -1;
                const bool haveIds = java::MenuContainerId(menu, menuId) &&
                                     java::PlayerMenuContainerId(playerMenuId);
                JniAttach attach;
                if (attach)
                    attach.env->DeleteLocalRef(menu);
                if (haveIds && menuId != playerMenuId) {
                    JniAttach attach2;
                    if (attach2)
                        attach2.env->DeleteLocalRef(current);
                    return;
                }
            }
            JniAttach attach;
            if (attach)
                attach.env->DeleteLocalRef(current);
        }

        const int nextDelay = NextDelay(minDelay_->get(), delay_->get());
        Arrange(inventory, nextDelay);
    }

    void Arrange(const inventory::Snapshot& inventory, int nextDelay) {
        int playerMenuId = -1;
        if (!java::PlayerMenuContainerId(playerMenuId))
            return;

        Repeat(inventory, nextDelay, playerMenuId);
    }

    int OffhandMode() const {
        return java::InventoryHasOffhand() && !autototem::OwnsOffhand() ? offhandItems_->get() : kOffhandNone;
    }

    void Repeat(const inventory::Snapshot& inventory, int nextDelay, int playerMenuId) {
        if (autoArmor_->get())
            AutoArmor(inventory, nextDelay, playerMenuId);

        if (java::InventoryHasOffhand() && clickOffHand_ && g_timer.Delay(nextDelay)) {
            if(!ClickSlot(playerMenuId, 45, 0, java::ClickKind::Pickup))return;
            inventoryOpen_ = true;
            clickOffHand_ = false;
            g_timer.Reset();
        }

        Offhand(inventory, nextDelay, playerMenuId);
        Hotbar(inventory, nextDelay, playerMenuId);
        Discard(inventory, nextDelay, playerMenuId);
    }

    void AutoArmor(const inventory::Snapshot& inventory, int nextDelay, int playerMenuId) {
        // Without Drop, keep replaced armor in the inventory. The armor list
        // runs feet, legs, chest, head; menu slots run in the opposite order.
        for (const inventory::Entry& entry : inventory.armor) {
            if (entry.facts.empty || (entry.facts.kinds & java::kItemArmor) == 0)
                continue;
            if (!g_timer.Delay(nextDelay))
                return;
            if (inventory::BestArmorScore(inventory, entry.facts.armorSlot) >
                inventory::Protection(entry.facts)) {
                const bool drop = drop_->get();
                if (!drop && inventory::FirstEmptyInventorySlot(inventory) == -1 &&
                    inventory::FirstEmptyHotbarSlot(inventory) == -1)
                    continue;
                if (ClickSlot(playerMenuId, 4 + (4 - entry.slot), drop ? 1 : 0,
                                       drop ? java::ClickKind::Throw : java::ClickKind::QuickMove)) {
                    inventoryOpen_ = true;
                    g_timer.Reset();
                    // Wait for a fresh snapshot before equipping the replacement.
                    return;
                }
            }
        }

        // Then shift-click the best spare into the empty armour slot.
        for (const inventory::Entry& entry : inventory.items) {
            const java::ItemFacts& facts = entry.facts;
            if (facts.empty || (facts.kinds & java::kItemArmor) == 0)
                continue;
            // Shift-click only into an empty armor slot. A full inventory must
            // not turn a failed unequip into repeatedly shuffling the new piece.
            const int armorIndex = 3 - facts.armorSlot;
            if (armorIndex < 0 || armorIndex >= static_cast<int>(inventory.armor.size()) ||
                !inventory.armor[armorIndex].facts.empty)
                continue;
            const float score = inventory::Protection(facts);
            const bool isBest = inventory::BestArmorScore(inventory, facts.armorSlot) == score;
            const bool isBetter = inventory::CurrentArmorScore(inventory, facts.armorSlot) < score;
            if (!isBest || !isBetter || !g_timer.Delay(nextDelay))
                continue;
            // Hotbar slots sit 36..44 in the player menu; the rest are already there.
            const int slot = entry.slot < 9 ? entry.slot + 36 : entry.slot;
            if(!ClickSlot(playerMenuId, slot, 0, java::ClickKind::QuickMove))return;
            inventoryOpen_ = true;
            g_timer.Reset();
        }
    }

    // The offhand strategies. Each ends by swapping the chosen stack into slot 45,
    // which the click at the end of the caller's flow completes.
    void Offhand(const inventory::Snapshot& inventory, int nextDelay, int playerMenuId) {
        const int mode = OffhandMode();

        if (mode == kOffhandGoldenApple) {
            if (inventory.offhand.empty())
                return;
            const java::ItemFacts& offHand = inventory.offhand[0].facts;
            const int egap = inventory::FindItem(inventory, java::KnownItem::EnchantedGoldenApple);
            const int gap = inventory::FindItem(inventory, java::KnownItem::GoldenApple);

            if (offHand.item == java::KnownItem::EnchantedGoldenApple) {
                if (offHand.count < offHand.maxStack && egap != -1 && g_timer.Delay(nextDelay)) {
                    if(!ClickSlot(playerMenuId, egap < 9 ? egap + 36 : egap, 0,
                                        java::ClickKind::Pickup))return;
                    inventoryOpen_ = true;
                    clickOffHand_ = true;
                    g_timer.Reset();
                }
            } else if (offHand.item == java::KnownItem::GoldenApple) {
                if (egap != -1) {
                    if (g_timer.Delay(nextDelay))
                        SwapOffHand(egap, playerMenuId);
                } else if (offHand.count < offHand.maxStack && gap != -1 &&
                           g_timer.Delay(nextDelay)) {
                    if(!ClickSlot(playerMenuId, gap < 9 ? gap + 36 : gap, 0,
                                        java::ClickKind::Pickup))return;
                    inventoryOpen_ = true;
                    clickOffHand_ = true;
                    g_timer.Reset();
                }
            } else if (egap != -1) {
                if (g_timer.Delay(nextDelay))
                    SwapOffHand(egap, playerMenuId);
            } else if (gap != -1 && g_timer.Delay(nextDelay)) {
                SwapOffHand(gap, playerMenuId);
            }
            return;
        }

        if (mode == kOffhandProjectile) {
            const int best = inventory::BestProjectile(inventory);
            if (best == -1)
                return;
            const inventory::Entry* entry = inventory::At(inventory, best);
            if (entry == nullptr)
                return;
            const java::ItemFacts& offHand =
                inventory.offhand.empty() ? java::ItemFacts{} : inventory.offhand[0].facts;
            const bool offHandIsProjectile = offHand.item == java::KnownItem::Egg ||
                                             offHand.item == java::KnownItem::Snowball;
            const bool shouldSwap = !offHandIsProjectile || offHand.count < entry->facts.count;
            if (shouldSwap && g_timer.Delay(nextDelay))
                SwapOffHand(best, playerMenuId);
            return;
        }

        if (mode == kOffhandFishingRod) {
            const int rod = inventory::FindFishingRod(inventory);
            const java::ItemFacts& offHand =
                inventory.offhand.empty() ? java::ItemFacts{} : inventory.offhand[0].facts;
            if (rod != -1 && offHand.item != java::KnownItem::FishingRod &&
                g_timer.Delay(nextDelay))
                SwapOffHand(rod, playerMenuId);
            return;
        }

        if (mode == kOffhandBlock) {
            const int best = inventory::BestBlock(inventory);
            if (best == -1)
                return;
            const inventory::Entry* entry = inventory::At(inventory, best);
            if (entry == nullptr)
                return;
            const java::ItemFacts& offHand =
                inventory.offhand.empty() ? java::ItemFacts{} : inventory.offhand[0].facts;
            const bool offHandIsBlock = inventory::IsValidBuildingBlock(offHand, std::string());
            const bool shouldSwap = !offHandIsBlock || offHand.count < entry->facts.count;
            if (shouldSwap && g_timer.Delay(nextDelay))
                SwapOffHand(best, playerMenuId);
        }
    }

    void Hotbar(const inventory::Snapshot& inventory, int nextDelay, int playerMenuId) {
        // Golden apples into their chosen slot.
        if (switchGoldenApple_->get() && OffhandMode() != kOffhandGoldenApple) {
            const int target = static_cast<int>(goldenAppleSlot_->get()) - 1;
            const int egap = inventory::FindItem(inventory, java::KnownItem::EnchantedGoldenApple);
            const int gap = inventory::FindItem(inventory, java::KnownItem::GoldenApple);
            const int best = egap != -1 ? egap : gap;
            if (best != -1) {
                const inventory::Entry* current = inventory::At(inventory, target);
                const inventory::Entry* candidate = inventory::At(inventory, best);
                if (current != nullptr && candidate != nullptr) {
                    const bool sameKind = current->facts.item == candidate->facts.item;
                    if (!sameKind ||
                        (current->facts.count < candidate->facts.count && sameKind)) {
                        SwapItem(target, best, inventory, nextDelay, playerMenuId);
                    }
                }
            }
        }

        if (switchBlock_->get()) {
            const int slot = static_cast<int>(blockSlot_->get()) - 1;
            const int best = inventory::BestBlock(inventory);
            const inventory::Entry* current = inventory::At(inventory, slot);
            const inventory::Entry* candidate = best != -1 ? inventory::At(inventory, best) : nullptr;
            if (candidate != nullptr && current != nullptr &&
                (candidate->facts.count > current->facts.count ||
                 !inventory::IsValidBuildingBlock(current->facts, current->name)) &&
                OffhandMode() != kOffhandBlock) {
                SwapItem(slot, best, inventory, nextDelay, playerMenuId);
            }
            if (inventory::CountBuildingBlocks(inventory) > static_cast<int>(maxBlockSize_->get()))
                ThrowItem(inventory::WorstBlock(inventory), inventory, nextDelay, playerMenuId);
        }

        if (switchSword_->get()) {
            const int slot = static_cast<int>(swordSlot_->get()) - 1;
            int best = inventory::BestSword(inventory);
            const int shapeAxe = inventory::BestShapeAxe(inventory);
            const inventory::Entry* shape = shapeAxe != -1 ? inventory::At(inventory, shapeAxe) : nullptr;
            const inventory::Entry* sword = best != -1 ? inventory::At(inventory, best) : nullptr;
            if (shape != nullptr &&
                inventory::AxeDamage(shape->facts) >
                    (sword != nullptr ? inventory::SwordDamage(sword->facts) : 0.0f)) {
                best = shapeAxe;
            }
            const inventory::Entry* current = inventory::At(inventory, slot);
            const inventory::Entry* candidate = best != -1 ? inventory::At(inventory, best) : nullptr;
            if (candidate != nullptr && current != nullptr) {
                const float currentDamage = (current->facts.kinds & java::kItemSword) != 0
                                                ? inventory::SwordDamage(current->facts)
                                                : inventory::AxeDamage(current->facts);
                const float bestDamage = (candidate->facts.kinds & java::kItemSword) != 0
                                             ? inventory::SwordDamage(candidate->facts)
                                             : inventory::AxeDamage(candidate->facts);
                if (bestDamage > currentDamage)
                    SwapItem(slot, best, inventory, nextDelay, playerMenuId);
            }
        }

        if (switchPickaxe_->get()) {
            const int slot = static_cast<int>(pickaxeSlot_->get()) - 1;
            const int best = inventory::BestPickaxe(inventory);
            const inventory::Entry* current = inventory::At(inventory, slot);
            const inventory::Entry* candidate = best != -1 ? inventory::At(inventory, best) : nullptr;
            if (candidate != nullptr && current != nullptr &&
                (inventory::ToolScore(candidate->facts) >
                     inventory::ToolScore(current->facts) ||
                 (current->facts.kinds & java::kItemPickaxe) == 0)) {
                SwapItem(slot, best, inventory, nextDelay, playerMenuId);
            }
        }

        if (switchAxe_->get()) {
            const int slot = static_cast<int>(axeSlot_->get()) - 1;
            const int best = inventory::BestAxe(inventory);
            const inventory::Entry* current = inventory::At(inventory, slot);
            const inventory::Entry* candidate = best != -1 ? inventory::At(inventory, best) : nullptr;
            if (candidate != nullptr && current != nullptr &&
                (inventory::ToolScore(candidate->facts) >
                     inventory::ToolScore(current->facts) ||
                 (current->facts.kinds & java::kItemAxe) == 0)) {
                SwapItem(slot, best, inventory, nextDelay, playerMenuId);
            }
        }

        if (switchRod_->get() && OffhandMode() != kOffhandFishingRod) {
            const int slot = static_cast<int>(rodSlot_->get()) - 1;
            const int rod = inventory::FindFishingRod(inventory);
            const inventory::Entry* current = inventory::At(inventory, slot);
            if (current != nullptr && (current->facts.kinds & java::kItemFishingRod) == 0 &&
                rod != -1) {
                SwapItem(slot, rod, inventory, nextDelay, playerMenuId);
            }
        }

        if (switchBow_->get()) {
            const int slot = static_cast<int>(bowSlot_->get()) - 1;
            const inventory::Entry* current = inventory::At(inventory, slot);

            int best = -1;
            // The preferred kind first; if the player has none, fall back through
            // the other two in the original's order.
            const int first = preferBow_->get();
            const int order[3] = {first, (first + 1) % 3, (first + 2) % 3};
            for (int i = 0; i < 3 && best == -1; ++i) {
                switch (order[i]) {
                    case kPreferCrossbow: best = inventory::BestCrossbow(inventory); break;
                    case kPreferPowerBow: best = inventory::BestPowerBow(inventory); break;
                    case kPreferPunchBow: best = inventory::BestPunchBow(inventory); break;
                    default: break;
                }
            }

            const inventory::Entry* candidate = best != -1 ? inventory::At(inventory, best) : nullptr;
            if (candidate != nullptr && current != nullptr) {
                const int kind = order[0];
                float bestScore = 0.0f;
                float currentScore = 0.0f;
                switch (kind) {
                    case kPreferCrossbow:
                        bestScore = inventory::CrossbowScore(candidate->facts);
                        currentScore = inventory::CrossbowScore(current->facts);
                        break;
                    case kPreferPowerBow:
                        bestScore = inventory::PowerBowScore(candidate->facts);
                        currentScore = inventory::PowerBowScore(current->facts);
                        break;
                    default:
                        bestScore = inventory::PunchBowScore(candidate->facts);
                        currentScore = inventory::PunchBowScore(current->facts);
                        break;
                }
                if (bestScore > currentScore)
                    SwapItem(slot, best, inventory, nextDelay, playerMenuId);
            }

            if (inventory::CountItem(inventory, java::KnownItem::Arrow) >
                static_cast<int>(maxArrowSize_->get())) {
                ThrowItem(inventory::WorstArrow(inventory), inventory, nextDelay, playerMenuId);
            }
        }

        if (switchEnderPearl_->get())
            SwapItemFor(static_cast<int>(enderPearlSlot_->get()) - 1, java::KnownItem::EnderPearl,
                        inventory, nextDelay, playerMenuId);
        if (switchWaterBucket_->get())
            SwapItemFor(static_cast<int>(waterBucketSlot_->get()) - 1, java::KnownItem::WaterBucket,
                        inventory, nextDelay, playerMenuId);
        if (switchFireball_->get())
            SwapItemFor(static_cast<int>(fireballSlot_->get()) - 1, java::KnownItem::FireCharge,
                        inventory, nextDelay, playerMenuId);

        if (keepProjectile_->get()) {
            const int held = inventory::CountItem(inventory, java::KnownItem::Egg) +
                             inventory::CountItem(inventory, java::KnownItem::Snowball);
            if (held > static_cast<int>(maxProjectileSize_->get()))
                ThrowItem(inventory::WorstProjectile(inventory), inventory, nextDelay,
                          playerMenuId);

            if (switchProjectile_->get() && OffhandMode() != kOffhandProjectile) {
                const int slot = static_cast<int>(projectileSlot_->get()) - 1;
                if (inventory::CountItem(inventory, java::KnownItem::Egg) > 0)
                    SwapItemFor(slot, java::KnownItem::Egg, inventory, nextDelay, playerMenuId);
                else if (inventory::CountItem(inventory, java::KnownItem::Snowball) > 0)
                    SwapItemFor(slot, java::KnownItem::Snowball, inventory, nextDelay,
                                playerMenuId);
            }
        }
    }

    void Discard(const inventory::Snapshot& inventory, int nextDelay, int playerMenuId) {
        if (!drop_->get() || !throwItems_->get())
            return;

        std::vector<int> order;
        order.reserve(inventory.items.size());
        for (const inventory::Entry& entry : inventory.items)
            order.push_back(entry.slot);
        std::shuffle(order.begin(), order.end(), RandomSource{});

        for (int slot : order) {
            const inventory::Entry* entry = inventory::At(inventory, slot);
            if (entry == nullptr || entry->facts.empty)
                continue;
            // Healing potions remain useful after the one-shot Refill releases
            // ownership; otherwise the next InvManager tick would discard them.
            if (!IsItemUseful(*entry, inventory) && !java::InventoryHasHealingSplashPotion(slot))
                ThrowItem(slot, inventory, nextDelay, playerMenuId);
        }
    }

    // Swap the stack in `from` into hotbar slot `target`.
    void SwapItem(int target, int from, const inventory::Snapshot& inventory, int nextDelay,
                  int playerMenuId) {
        const inventory::Entry* current = inventory::At(inventory, target);
        if (current == nullptr || from == target)
            return;
        if (!inventory::IsNameValid(current->facts, current->name))
            return;
        if (!g_timer.Delay(nextDelay))
            return;
        if(!ClickSlot(playerMenuId, from < 9 ? from + 36 : from, target,
                            java::ClickKind::Swap))return;
        inventoryOpen_ = true;
        g_timer.Reset();
    }

    // Swap a named item into `target`, if it is not already there or is thinner.
    void SwapItemFor(int target, java::KnownItem item, const inventory::Snapshot& inventory,
                     int nextDelay, int playerMenuId) {
        const inventory::Entry* current = inventory::At(inventory, target);
        if (current == nullptr)
            return;
        if (!inventory::IsNameValid(current->facts, current->name))
            return;
        if (!g_timer.Delay(nextDelay))
            return;

        const int from = inventory::FindItem(inventory, item);
        if (from == -1)
            return;
        const inventory::Entry* candidate = inventory::At(inventory, from);
        if (candidate == nullptr)
            return;

        const bool sameKind = current->facts.item == item;
        if (!sameKind || (sameKind && current->facts.count < candidate->facts.count)) {
            if(!ClickSlot(playerMenuId, from < 9 ? from + 36 : from, target,
                                java::ClickKind::Swap))return;
            inventoryOpen_ = true;
            g_timer.Reset();
        }
    }

    // Button 40 is the offhand in the player menu's swap protocol.
    void SwapOffHand(int from, int playerMenuId) {
        if(!ClickSlot(playerMenuId, from < 9 ? from + 36 : from, 40, java::ClickKind::Swap))return;
        inventoryOpen_ = true;
        g_timer.Reset();
    }

    void ThrowItem(int slot, const inventory::Snapshot& inventory, int nextDelay,
                   int playerMenuId) {
        // Shared by unwanted-item and excess block/arrow/projectile cleanup.
        if (!drop_->get() || slot == -1)
            return;
        const inventory::Entry* entry = inventory::At(inventory, slot);
        if (entry == nullptr)
            return;
        if (!inventory::IsNameValid(entry->facts, entry->name))
            return;
        if (!g_timer.Delay(nextDelay))
            return;
        if(!ClickSlot(playerMenuId, slot < 9 ? slot + 36 : slot, 1, java::ClickKind::Throw))return;
        inventoryOpen_ = true;
        g_timer.Reset();
    }

    struct RandomSource {
        using result_type = unsigned;
        static constexpr result_type min() { return 0u; }
        static constexpr result_type max() { return 0xFFFFFFFFu; }
        result_type operator()() const {
            return static_cast<result_type>(rotation::Random() * 4294967296.0);
        }
    };

public:
    // The move input arrives on its own event; the walk test reads it from here.
    void SetMoveInput(float forward, float strafe) {
        forwardInput_ = forward;
        strafeInput_ = strafe;
    }

private:
    std::vector<Properties::PropDesc> configRows_;
    Properties::NumberProp* minDelay_ = nullptr;
    Properties::NumberProp* delay_ = nullptr;
    Properties::ModeProperty* offhandItems_ = nullptr;
    Properties::BoolProperty* autoArmor_ = nullptr;
    Properties::BoolProperty* drop_ = nullptr;
    Properties::BoolProperty* inventoryOnly_ = nullptr;

    Properties::BoolProperty* switchSword_ = nullptr;
    Properties::NumberProp* swordSlot_ = nullptr;
    Properties::BoolProperty* switchBlock_ = nullptr;
    Properties::NumberProp* blockSlot_ = nullptr;
    Properties::NumberProp* maxBlockSize_ = nullptr;
    Properties::BoolProperty* switchPickaxe_ = nullptr;
    Properties::NumberProp* pickaxeSlot_ = nullptr;
    Properties::BoolProperty* switchAxe_ = nullptr;
    Properties::NumberProp* axeSlot_ = nullptr;
    Properties::BoolProperty* switchBow_ = nullptr;
    Properties::NumberProp* bowSlot_ = nullptr;
    Properties::ModeProperty* preferBow_ = nullptr;
    Properties::NumberProp* maxArrowSize_ = nullptr;
    Properties::BoolProperty* switchWaterBucket_ = nullptr;
    Properties::NumberProp* waterBucketSlot_ = nullptr;
    Properties::BoolProperty* switchEnderPearl_ = nullptr;
    Properties::NumberProp* enderPearlSlot_ = nullptr;
    Properties::BoolProperty* switchFireball_ = nullptr;
    Properties::NumberProp* fireballSlot_ = nullptr;
    Properties::BoolProperty* switchGoldenApple_ = nullptr;
    Properties::NumberProp* goldenAppleSlot_ = nullptr;
    Properties::BoolProperty* throwItems_ = nullptr;
    Properties::NumberProp* waterBucketCount_ = nullptr;
    Properties::NumberProp* lavaBucketCount_ = nullptr;
    Properties::BoolProperty* keepProjectile_ = nullptr;
    Properties::BoolProperty* switchProjectile_ = nullptr;
    Properties::NumberProp* projectileSlot_ = nullptr;
    Properties::NumberProp* maxProjectileSize_ = nullptr;
    Properties::BoolProperty* switchRod_ = nullptr;
    Properties::NumberProp* rodSlot_ = nullptr;

    event::Subscription motion_;
    event::Subscription packet_;

    bool  clickOffHand_ = false;
    bool  inventoryOpen_ = false;
    int   noMoveTicks_ = 0;
    float forwardInput_ = 0.0f;
    float strafeInput_ = 0.0f;
};

}

namespace invmanager {
bool UsesInventoryMovement(){return compat::IsLunar189()&&g_manager&&g_manager->isEnabled()&&g_inventoryOnly&&!g_inventoryOnly->get()&&!refill::IsWorking();}

int MaxBlockSize() {
    return g_maxBlockSize ? static_cast<int>(g_maxBlockSize->get()) : 256;
}

int MaxArrowSize() {
    return g_maxArrowSize ? static_cast<int>(g_maxArrowSize->get()) : 256;
}

int MaxProjectileSize() {
    return g_maxProjectileSize ? static_cast<int>(g_maxProjectileSize->get()) : 64;
}

int WaterBucketCount() {
    return g_waterBucketCount ? static_cast<int>(g_waterBucketCount->get()) : 1;
}

int LavaBucketCount() {
    return g_lavaBucketCount ? static_cast<int>(g_lavaBucketCount->get()) : 1;
}

bool ShouldKeepProjectile() {
    return g_keepProjectile ? g_keepProjectile->get() : true;
}

}

std::unique_ptr<Module> MakeInvManagerModule() {
    return std::make_unique<InvManagerModule>();
}

}
