#include "minecraft/utils/inventory/InventoryUtils.h"

#include <algorithm>
#include <chrono>
#include <cmath>

#include "minecraft/java/api/base.h"

namespace nova::inventory {
namespace {

using java::InventoryArmor;
using java::InventoryArmorCount;
using java::InventoryItemCount;
using java::InventoryOffhand;
using java::InventorySlot;
using java::ItemDisplayName;
using java::ReadItemFacts;

constexpr int kHotbarSlots = 9;
constexpr int kInventorySlots = 36;

// The names the original refuses to act on. A server that hands out a clickable
// item expects the click, and clicking it from a module would open a menu or
// leave the game, so anything carrying one of these words is left alone.
constexpr const char* kRefusedNames[] = {
    "Click", "Right", "\xe7\x82\xb9\xe5\x87\xbb",           // 点击
    "Teleport", "\xe4\xbd\xbf\xe7\x94\xa8",                  // 使用
    "\xe4\xbc\xa0\xe9\x80\x81", "\xe5\x86\x8d\xe6\x9d\xa5",  // 传送 / 再来
};

constexpr const char* kMenuNames[] = {
    "\xe9\x95\xbf\xe6\x8c\x89\xe7\x82\xb9\xe5\x87\xbb",  // 长按点击
    "\xe7\x82\xb9\xe5\x87\xbb\xe4\xbd\xbf\xe7\x94\xa8",  // 点击使用
    "\xe7\xa6\xbb\xe5\xbc\x80\xe6\xb8\xb8\xe6\x88\x8f",  // 离开游戏
    "\xe9\x80\x89\xe6\x8b\xa9\xe4\xb8\x80\xe4\xb8\xaa\xe9\x98\x9f\xe4\xbc\x8d",  // 选择一个队伍
    "\xe5\x86\x8d\xe6\x9d\xa5\xe4\xb8\x80\xe5\xb1\x80",  // 再来一局
};

bool Contains(const std::string& text, const char* needle) {
    return text.find(needle) != std::string::npos;
}

// The original walks items and armour together for the "best of" queries, and the
// hotbar/main inventory alone for the slot lookups.
template <typename F>
void ForEachHeld(const Snapshot& snapshot, F&& visit) {
    for (const Entry& entry : snapshot.items)
        visit(entry);
    for (const Entry& entry : snapshot.armor)
        visit(entry);
}

int FindBest(const Snapshot& snapshot, float (*score)(const ItemFacts&), bool wantMax,
             int armorSlot /* -1 for any */) {
    int best = -1;
    float bestValue = wantMax ? 0.0f : 0.0f;
    bool first = true;
    ForEachHeld(snapshot, [&](const Entry& entry) {
        if (entry.facts.empty)
            return;
        if (armorSlot >= 0 && entry.facts.armorSlot != armorSlot)
            return;
        const float value = score(entry.facts);
        if (value <= 0.0f)
            return;
        if (first || (wantMax ? value > bestValue : value < bestValue)) {
            first = false;
            bestValue = value;
            best = entry.slot;
        }
    });
    return best;
}

}  // namespace

long long NowMillis() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
               std::chrono::steady_clock::now().time_since_epoch())
        .count();
}

bool Scan(Snapshot& out) {
    out.items.clear();
    out.armor.clear();
    out.offhand.clear();

    JniAttach attach;
    if (!attach)
        return false;
    JNIEnv* env = attach.env;

    const int itemCount = InventoryItemCount();
    if (itemCount <= 0)
        return false;

    // Each slot's stack is released as soon as its facts are read, so the whole
    // walk never holds more than one local reference at a time.
    out.items.reserve(static_cast<size_t>(itemCount));
    for (int i = 0; i < itemCount; ++i) {
        jobject stack = nullptr;
        Entry entry;
        entry.slot = i;
        if (InventorySlot(i, stack)) {
            ReadItemFacts(stack, entry.facts);
            if (!entry.facts.empty)
                ItemDisplayName(stack, entry.name);
            env->DeleteLocalRef(stack);
        } else {
            entry.facts.empty = true;
        }
        out.items.push_back(std::move(entry));
    }

    const int armorCount = InventoryArmorCount();
    out.armor.reserve(static_cast<size_t>(armorCount > 0 ? armorCount : 0));
    for (int i = 0; i < armorCount; ++i) {
        jobject stack = nullptr;
        Entry entry;
        entry.slot = i;
        if (InventoryArmor(i, stack)) {
            ReadItemFacts(stack, entry.facts);
            if (!entry.facts.empty)
                ItemDisplayName(stack, entry.name);
            env->DeleteLocalRef(stack);
        } else {
            entry.facts.empty = true;
        }
        out.armor.push_back(std::move(entry));
    }

    jobject offhand = nullptr;
    if (InventoryOffhand(0, offhand)) {
        Entry entry;
        entry.slot = 0;
        ReadItemFacts(offhand, entry.facts);
        if (!entry.facts.empty)
            ItemDisplayName(offhand, entry.name);
        env->DeleteLocalRef(offhand);
        out.offhand.push_back(std::move(entry));
    } else {
        Entry entry;
        entry.slot = 0;
        entry.facts.empty = true;
        out.offhand.push_back(std::move(entry));
    }

    return true;
}

float Protection(const ItemFacts& facts) {
    if (facts.empty)
        return 0.0f;
    int value = 0;
    switch (facts.material) {
        case KnownMaterial::Leather:   value += 100; break;
        case KnownMaterial::Chain:     value += 200; break;
        case KnownMaterial::Iron:      value += 400; break;
        case KnownMaterial::Gold:      value += 300; break;
        case KnownMaterial::Diamond:   value += 500; break;
        case KnownMaterial::Netherite: value += 600; break;
        case KnownMaterial::Nothing:   break;
    }
    value += facts.protection;
    return static_cast<float>(value);
}

float SwordDamage(const ItemFacts& facts) {
    if (facts.empty)
        return 0.0f;
    float value = 0.0f;
    if ((facts.kinds & kItemSword) != 0)
        value += facts.swordDamage + 1.0f;
    if (facts.sharpness > 0)
        value += facts.sharpnessBonus;
    return value;
}

float AxeDamage(const ItemFacts& facts) {
    if (facts.empty)
        return 0.0f;
    float value = 0.0f;
    if ((facts.kinds & kItemAxe) != 0 && IsSharpnessAxe(facts)) {
        switch (facts.item) {
            case KnownItem::WoodenAxe:  value += 4.0f; break;
            case KnownItem::StoneAxe:   value += 5.0f; break;
            case KnownItem::IronAxe:    value += 6.0f; break;
            case KnownItem::GoldenAxe:  value += 4.0f; break;
            case KnownItem::DiamondAxe: value += 7.0f; break;
            default: break;
        }
    }
    if (facts.sharpness > 0)
        value += facts.sharpnessBonus;
    return value;
}

float ToolScore(const ItemFacts& facts) {
    if (facts.empty)
        return 0.0f;
    // A god item and a sharpness axe are weapon-shaped tools and are scored by the
    // weapon rules instead, so they contribute nothing here.
    if (IsGodItem(facts) || IsSharpnessAxe(facts))
        return 0.0f;

    if ((facts.kinds & (kItemPickaxe | kItemAxe | kItemShovel)) == 0)
        return 0.0f;

    float value = facts.destroySpeed;
    if (facts.efficiency > 0)
        value += static_cast<float>(facts.efficiency) * 0.0075f;
    return value;
}

float CrossbowScore(const ItemFacts& facts) {
    if (facts.empty || (facts.kinds & kItemCrossbow) == 0)
        return 0.0f;
    return static_cast<float>(facts.quickCharge + facts.multishot + facts.piercing);
}

float PunchBowScore(const ItemFacts& facts) {
    if (facts.empty || (facts.kinds & kItemBow) == 0)
        return 0.0f;
    float value = 10.0f;
    value += static_cast<float>(facts.punch);
    value += static_cast<float>(facts.infinity);
    value += static_cast<float>(facts.flame);
    value += static_cast<float>(facts.power) / 10.0f;
    if (facts.maxDamage > 0)
        value += static_cast<float>(facts.damageValue) / static_cast<float>(facts.maxDamage);
    return value;
}

float PowerBowScore(const ItemFacts& facts) {
    if (facts.empty || (facts.kinds & kItemBow) == 0)
        return 0.0f;
    float value = 10.0f;
    value += static_cast<float>(facts.punch) / 10.0f;
    value += static_cast<float>(facts.infinity);
    value += static_cast<float>(facts.flame);
    value += static_cast<float>(facts.power);
    if (facts.maxDamage > 0)
        value += static_cast<float>(facts.damageValue) / static_cast<float>(facts.maxDamage);
    return value;
}

bool IsGodItem(const ItemFacts& facts) {
    if (facts.empty)
        return false;
    if ((facts.kinds & kItemAxe) != 0 && facts.item == KnownItem::GoldenAxe && facts.sharpness > 100)
        return true;
    if (facts.item == KnownItem::SlimeBall && facts.knockback > 1)
        return true;
    return facts.item == KnownItem::TotemOfUndying || facts.item == KnownItem::EndCrystal;
}

bool IsGodAxe(const ItemFacts& facts) {
    if (facts.empty)
        return false;
    return facts.item == KnownItem::GoldenAxe && facts.sharpness > 100;
}

bool IsSharpnessAxe(const ItemFacts& facts) {
    if (facts.empty || (facts.kinds & kItemAxe) == 0)
        return false;
    return facts.sharpness >= 8 && facts.sharpness < 50;
}

bool IsPunchBow(const ItemFacts& facts, const std::string& name) {
    return PunchBowScore(facts) > 10.0f && IsNameValid(facts, name);
}

bool IsPowerBow(const ItemFacts& facts, const std::string& name) {
    return PowerBowScore(facts) > 10.0f && IsNameValid(facts, name);
}

bool IsEnchantedGApple(const ItemFacts& facts) {
    return !facts.empty && facts.item == KnownItem::EnchantedGoldenApple;
}

bool IsEndCrystal(const ItemFacts& facts) {
    return !facts.empty && facts.item == KnownItem::EndCrystal;
}

bool IsKBBall(const ItemFacts& facts) {
    return !facts.empty && facts.item == KnownItem::SlimeBall && facts.knockback > 1;
}

bool IsKBStick(const ItemFacts& facts) {
    return !facts.empty && facts.item == KnownItem::Stick && facts.knockback > 1;
}

bool IsGoldenHead(const ItemFacts& facts) {
    return !facts.empty && (facts.kinds & kItemSkullBlock) != 0;
}

bool IsNameValid(const ItemFacts& facts, const std::string& name) {
    if (!facts.empty && (facts.kinds & kItemHead) != 0)
        return false;
    for (const char* refused : kRefusedNames) {
        if (Contains(name, refused))
            return false;
    }
    return true;
}

bool IsValidBuildingBlock(const ItemFacts& facts, const std::string& name) {
    if (facts.empty || (facts.kinds & kItemBlock) == 0 || facts.count <= 1)
        return false;
    if (!IsNameValid(facts, name))
        return false;
    if (Contains(name, "Click") || Contains(name, "\xe7\x82\xb9\xe5\x87\xbb"))
        return false;
    // A block that renames itself is a server shop item, not a building block.
    if ((facts.kinds & kItemNameBlock) != 0)
        return false;
    if ((facts.blockKinds & (kBlockFlower | kBlockBush | kBlockFungus | kBlockCrop | kBlockSlab)) != 0)
        return false;
    return !facts.blacklistedBlock;
}

bool IsCommonItemUseful(const ItemFacts& facts) {
    if (facts.empty)
        return true;

    if ((facts.kinds & kItemBlock) != 0) {
        if (facts.item == KnownItem::EnchantingTableBlock)
            return false;
        return facts.item != KnownItem::CobwebBlock;
    }

    if ((facts.kinds & (kItemBook | kItemBottle | kItemFirework)) != 0)
        return false;

    switch (facts.item) {
        case KnownItem::WheatSeeds:
        case KnownItem::BeetrootSeeds:
        case KnownItem::MelonSeeds:
        case KnownItem::PumpkinSeeds:
        case KnownItem::FlintAndSteel:
            return false;
        default:
            break;
    }
    return true;
}

const Entry* At(const Snapshot& snapshot, int slot) {
    for (const Entry& entry : snapshot.items) {
        if (entry.slot == slot)
            return &entry;
    }
    return nullptr;
}

float BestArmorScore(const Snapshot& snapshot, int armorSlot) {
    float best = 0.0f;
    ForEachHeld(snapshot, [&](const Entry& entry) {
        if (entry.facts.empty || (entry.facts.kinds & kItemArmor) == 0)
            return;
        if (entry.facts.armorSlot != armorSlot)
            return;
        best = std::max(best, Protection(entry.facts));
    });
    return best;
}

float CurrentArmorScore(const Snapshot& snapshot, int armorSlot) {
    // The armour list runs feet, legs, chest, head -- the reverse of the slot order
    // the rules use.
    const int index = 3 - armorSlot;
    if (index < 0 || index >= static_cast<int>(snapshot.armor.size()))
        return 0.0f;
    return Protection(snapshot.armor[static_cast<size_t>(index)].facts);
}

namespace {

struct MaxQuery {
    float (*score)(const ItemFacts&);
    unsigned requireKinds;
    bool     rejectGod;
    bool     rejectSharpnessAxe;
};

float MaxOver(const Snapshot& snapshot, const MaxQuery& query) {
    float best = 0.0f;
    ForEachHeld(snapshot, [&](const Entry& entry) {
        const ItemFacts& facts = entry.facts;
        if (facts.empty)
            return;
        if (query.requireKinds != 0 && (facts.kinds & query.requireKinds) == 0)
            return;
        if (query.rejectGod && IsGodItem(facts))
            return;
        if (query.rejectSharpnessAxe && IsSharpnessAxe(facts))
            return;
        best = std::max(best, query.score(facts));
    });
    return best;
}

}  // namespace

float BestSwordDamage(const Snapshot& snapshot) {
    return MaxOver(snapshot, {SwordDamage, kItemSword, false, false});
}

float BestPickaxeScore(const Snapshot& snapshot) {
    return MaxOver(snapshot, {ToolScore, kItemPickaxe, false, false});
}

float BestAxeScore(const Snapshot& snapshot) {
    return MaxOver(snapshot, {ToolScore, kItemAxe, false, true});
}

float BestShovelScore(const Snapshot& snapshot) {
    return MaxOver(snapshot, {ToolScore, kItemShovel, false, false});
}

float BestCrossbowScore(const Snapshot& snapshot) {
    return MaxOver(snapshot, {CrossbowScore, kItemCrossbow, false, false});
}

float BestPunchBowScore(const Snapshot& snapshot) {
    return MaxOver(snapshot, {PunchBowScore, kItemBow, false, false});
}

float BestPowerBowScore(const Snapshot& snapshot) {
    return MaxOver(snapshot, {PowerBowScore, kItemBow, false, false});
}

namespace {

// The "best of" queries that hand back a slot. Each walks the same held set and
// keeps the highest score, ties going to the first seen -- the same order the
// original's stream max() would report.
int BestScoring(const Snapshot& snapshot, float (*score)(const ItemFacts&),
                bool (*accept)(const Snapshot&, const Entry&)) {
    int bestSlot = -1;
    float bestValue = 0.0f;
    ForEachHeld(snapshot, [&](const Entry& entry) {
        if (entry.facts.empty)
            return;
        if (accept && !accept(snapshot, entry))
            return;
        const float value = score(entry.facts);
        if (bestSlot == -1 || value > bestValue) {
            bestValue = value;
            bestSlot = entry.slot;
        }
    });
    return bestSlot;
}

bool AnySword(const Snapshot&, const Entry& entry) {
    return (entry.facts.kinds & kItemSword) != 0;
}
bool AnyPickaxe(const Snapshot&, const Entry& entry) {
    return (entry.facts.kinds & kItemPickaxe) != 0;
}
bool PlainAxe(const Snapshot&, const Entry& entry) {
    return (entry.facts.kinds & kItemAxe) != 0 && !IsSharpnessAxe(entry.facts);
}
bool ShapeAxe(const Snapshot&, const Entry& entry) {
    return (entry.facts.kinds & kItemAxe) != 0 && IsSharpnessAxe(entry.facts) &&
           !IsGodAxe(entry.facts);
}
bool AnyShovel(const Snapshot&, const Entry& entry) {
    return (entry.facts.kinds & kItemShovel) != 0;
}
bool AnyCrossbow(const Snapshot&, const Entry& entry) {
    return (entry.facts.kinds & kItemCrossbow) != 0;
}
bool AnyBow(const Snapshot&, const Entry& entry) {
    return (entry.facts.kinds & kItemBow) != 0;
}
bool IsRod(const Snapshot&, const Entry& entry) {
    return (entry.facts.kinds & kItemFishingRod) != 0;
}
bool IsArrow(const Snapshot&, const Entry& entry) {
    return (entry.facts.kinds & kItemArrow) != 0;
}

float AxeDamageScore(const ItemFacts& facts) { return AxeDamage(facts); }

}  // namespace

int BestSword(const Snapshot& snapshot) {
    return BestScoring(snapshot, SwordDamage, AnySword);
}

int BestPickaxe(const Snapshot& snapshot) {
    return BestScoring(snapshot, ToolScore, AnyPickaxe);
}

int BestAxe(const Snapshot& snapshot) {
    return BestScoring(snapshot, ToolScore, PlainAxe);
}

int BestShapeAxe(const Snapshot& snapshot) {
    return BestScoring(snapshot, AxeDamageScore, ShapeAxe);
}

int BestShovel(const Snapshot& snapshot) {
    return BestScoring(snapshot, ToolScore, AnyShovel);
}

int BestCrossbow(const Snapshot& snapshot) {
    return BestScoring(snapshot, CrossbowScore, AnyCrossbow);
}

int BestPunchBow(const Snapshot& snapshot) {
    return BestScoring(snapshot, PunchBowScore, AnyBow);
}

int BestPowerBow(const Snapshot& snapshot) {
    return BestScoring(snapshot, PowerBowScore, AnyBow);
}

int FindFishingRod(const Snapshot& snapshot) {
    return BestScoring(snapshot, [](const ItemFacts&) { return 1.0f; }, IsRod);
}

namespace {

float Count(const ItemFacts& facts) { return static_cast<float>(facts.count); }

bool UsableBlock(const Snapshot&, const Entry& entry) {
    return (entry.facts.kinds & kItemBlock) != 0 &&
           IsValidBuildingBlock(entry.facts, entry.name) && IsNameValid(entry.facts, entry.name);
}

bool Projectile(const Snapshot&, const Entry& entry) {
    return entry.facts.item == KnownItem::Egg || entry.facts.item == KnownItem::Snowball;
}

}  // namespace

int BestBlock(const Snapshot& snapshot) {
    return BestScoring(snapshot, Count, UsableBlock);
}

int WorstBlock(const Snapshot& snapshot) {
    int worstSlot = -1;
    float worstValue = 0.0f;
    ForEachHeld(snapshot, [&](const Entry& entry) {
        if (!UsableBlock(snapshot, entry))
            return;
        const float value = static_cast<float>(entry.facts.count);
        if (worstSlot == -1 || value < worstValue) {
            worstValue = value;
            worstSlot = entry.slot;
        }
    });
    return worstSlot;
}

int BestProjectile(const Snapshot& snapshot) {
    int bestSlot = -1;
    int bestCount = 0;
    ForEachHeld(snapshot, [&](const Entry& entry) {
        if (!Projectile(snapshot, entry) || !IsNameValid(entry.facts, entry.name))
            return;
        if (bestSlot == -1 || entry.facts.count > bestCount) {
            bestCount = entry.facts.count;
            bestSlot = entry.slot;
        }
    });
    return bestSlot;
}

int WorstProjectile(const Snapshot& snapshot) {
    int worstSlot = -1;
    int worstCount = 0;
    ForEachHeld(snapshot, [&](const Entry& entry) {
        if (!Projectile(snapshot, entry))
            return;
        if (worstSlot == -1 || entry.facts.count < worstCount) {
            worstCount = entry.facts.count;
            worstSlot = entry.slot;
        }
    });
    return worstSlot;
}

int WorstArrow(const Snapshot& snapshot) {
    int worstSlot = -1;
    int worstCount = 0;
    ForEachHeld(snapshot, [&](const Entry& entry) {
        if (!IsArrow(snapshot, entry) || !IsNameValid(entry.facts, entry.name))
            return;
        if (worstSlot == -1 || entry.facts.count < worstCount) {
            worstCount = entry.facts.count;
            worstSlot = entry.slot;
        }
    });
    return worstSlot;
}

namespace {

// The lookups below only ever consider the player's own rows, exactly as the
// original's getItemSlot / getItemStackSlot do.
int FindInItems(const Snapshot& snapshot, KnownItem item) {
    for (const Entry& entry : snapshot.items) {
        if (!entry.facts.empty && entry.facts.item == item)
            return entry.slot;
    }
    return -1;
}

}  // namespace

int FindItem(const Snapshot& snapshot, KnownItem item) {
    return FindInItems(snapshot, item);
}

int CountItem(const Snapshot& snapshot, KnownItem item) {
    int total = 0;
    ForEachHeld(snapshot, [&](const Entry& entry) {
        if (!entry.facts.empty && entry.facts.item == item)
            total += entry.facts.count;
    });
    return total;
}

bool HasItem(const Snapshot& snapshot, KnownItem item) {
    bool found = false;
    ForEachHeld(snapshot, [&](const Entry& entry) {
        if (!entry.facts.empty && entry.facts.item == item)
            found = true;
    });
    return found;
}

int CountBuildingBlocks(const Snapshot& snapshot) {
    int total = 0;
    ForEachHeld(snapshot, [&](const Entry& entry) {
        if (UsableBlock(snapshot, entry) && IsNameValid(entry.facts, entry.name))
            total += entry.facts.count;
    });
    return total;
}

int FirstEmptyInventorySlot(const Snapshot& snapshot) {
    for (const Entry& entry : snapshot.items) {
        if (entry.slot >= kHotbarSlots && entry.facts.empty)
            return entry.slot;
    }
    return -1;
}

int FirstEmptyHotbarSlot(const Snapshot& snapshot) {
    for (const Entry& entry : snapshot.items) {
        if (entry.slot < kHotbarSlots && entry.facts.empty)
            return entry.slot;
    }
    return -1;
}

bool ShouldDisableFeatures(const Snapshot& snapshot) {
    bool found = false;
    ForEachHeld(snapshot, [&](const Entry& entry) {
        if (entry.facts.empty || entry.name.empty())
            return;
        for (const char* menu : kMenuNames) {
            if (Contains(entry.name, menu))
                found = true;
        }
    });
    return found;
}

}
