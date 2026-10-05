#include "minecraft/java/api/game_potion.h"
#include "minecraft/java/api/game_itemuse.h"
#include <cmath>
#include "minecraft/java/api/game_freecam.h"
#include "minecraft/java/api/game_inventory.h"
#include "minecraft/java/api/game_mlg.h"
#include "minecraft/java/api/game_slotspoof.h"
#include "minecraft/java/api/game_rotation.h"
#include "minecraft/java/api/game_place.h"
#include "minecraft/java/api/game_click.h"
#include "minecraft/utils/rotation/RotationManager.h"
#include "minecraft/compat/minecraft263/Minecraft263Access.h"

namespace nova::java {
namespace {
using compat::mc263::access::Frame;
using compat::mc263::access::Member;
struct Mapping {
    const char* potionItem{};
    Member stackItem{}, stackCount{}, damage{}, isSplash{}, effects{}, effectKind{};
    Member componentGet{}, contentsType{}, controller{}, world{}, sync{}, use{}, hand{}, consumesAction{};
};
#define NOVA_AUTOPOT_MAPPING
#define NOVA_AUTOPOT_CLASS(name) .potionItem = name,
#define NOVA_AUTOPOT(key, owner, name, desc) .key = {owner, name, desc},
constexpr Mapping lunar{
#include "minecraft/compat/mappings/Lunar189.inc"
};
constexpr Mapping forge{
#include "minecraft/compat/mappings/forge1201.inc"
};
constexpr Mapping modern{
#include "minecraft/compat/mappings/minecraft263.inc"
};
#undef NOVA_AUTOPOT
#undef NOVA_AUTOPOT_CLASS
#undef NOVA_AUTOPOT_MAPPING
const Mapping& Names() { return compat::IsLegacyLunar() ? lunar : compat::UsesModernRuntime() ? modern : forge; }
constexpr Member iterator{"java.lang.Iterable", "iterator", "()Ljava/util/Iterator;"};
constexpr Member hasNext{"java.util.Iterator", "hasNext", "()Z"};
constexpr Member next{"java.util.Iterator", "next", "()Ljava/lang/Object;"};

bool Healing(Frame& f, jobject stack) {
    if (!stack || !f.check()) return false;
    const auto& n = Names();
    const bool legacy = compat::IsLegacyLunar();
    const int count = legacy ? f.get<int>(stack, n.stackCount) : f.call<int>(stack, n.stackCount);
    if (count <= 0 || !f.check()) return false;
    auto item = f.call(stack, n.stackItem);
    if (!f.instance(item, n.potionItem)) return false;
    jobject effects = nullptr;
    if (legacy) {
        const int damage = f.call<int>(stack, n.damage);
        const auto checkSplash = compat::mc263::access::Method(f.env, n.isSplash, true);
        if (!checkSplash || !f.check()) return false;
        const bool splash = f.env->CallStaticBooleanMethod(
            compat::mc263::access::Class(f.env, n.isSplash.owner), checkSplash, static_cast<jint>(damage));
        if (!f.check() || !splash) return false;
        effects = f.call(item, n.effects, stack);
    } else if (compat::UsesModernRuntime()) {
        auto type = f.stat(n.contentsType);
        auto contents = type ? f.call(stack, n.componentGet, type) : nullptr;
        effects = f.call(contents, n.effects);
    } else {
        effects = f.staticCall(n.effects, stack);
    }
    auto it = f.call(effects, iterator);
    // Check the actual potion effects, never the stack's display/custom name.
    while (it && f.call<bool>(it, hasNext)) {
        auto effect = f.call(it, next);
        if (!effect || !f.check()) return false;
        bool healing = false;
        if (legacy) healing = f.call<int>(effect, n.effectKind) == 6; // Potion.heal
        else {
            auto id = f.call(effect, n.effectKind);
            healing = f.text(id) == "effect.minecraft.instant_health";
            if (id) f.env->DeleteLocalRef(id);
        }
        f.env->DeleteLocalRef(effect);
        if (!f.check()) return false;
        if (healing) return true;
    }
    return false;
}
}

bool HotbarHasHealingSplashPotion(int slot) {
    if (slot < 0 || slot >= 9) return false;
    return InventoryHasHealingSplashPotion(slot);
}

bool InventoryHasHealingSplashPotion(int slot) {
    if (slot < 0 || slot >= 36) return false;
    Frame f;
    jobject stack = nullptr;
    return f && InventorySlot(slot, stack) && Healing(f, stack);
}

bool SyncSelectedItem() {
    Frame f;
    const auto& n = Names();
    return f && f.invoke(f.get(f.client(), n.controller), n.sync);
}

namespace {
bool UseStack(Frame& f, jobject stack, bool swing=true, bool animate=true, bool* attempted=nullptr, const void* owner=nullptr) {
    if(attempted)*attempted=false;
    if (FreeCamBlocks(FreeCamAction::Place)) return false;
    const auto& n = Names();
    auto client = f.client();
    auto player = f.player();
    auto controller = f.get(client, n.controller);
    if (!player || !controller || !f.check()) return false;
    // Reserve before prediction, even when vanilla returns PASS (it can still
    // send USE_ITEM). A second automated consumer retries on the next tick.
    float cameraYaw=0, cameraPitch=0;
    const bool modern=compat::UsesModernRuntime();
    if(modern && (!PlayerRotation(cameraYaw,cameraPitch) ||
       !(owner ? rotation::SubmitItemUse(owner) : rotation::SubmitItemUse())))return false;
    struct RestoreCamera {
        bool restore; float yaw,pitch;
        ~RestoreCamera(){if(restore)SetPlayerRotation(yaw,pitch);}
    } restore{modern,cameraYaw,cameraPitch};
    if(modern){
        const auto angle=rotation::GetRotation();
        if(!SetPlayerRotation(angle.yaw,angle.pitch))return false;
    }
    bool used = false;
    if (compat::IsLegacyLunar()) {
        auto world = f.get(client, n.world);
        if (world) {
            if(attempted)*attempted=true;
            used = f.call<bool>(controller, n.use, player, world, stack);
        }
    } else {
        auto hand = f.stat(n.hand);
        if (hand) {
            if(attempted)*attempted=true;
            used = f.call<bool>(f.call(controller, n.use, player, hand), n.consumesAction);
        }
    }
    if (!f.check() || !used) return false;
    // Vanilla owns the slot synchronization/use packet and stack consumption.
    // Do not append a second use packet after the controller consumes the stack.
    if (animate) {
        if(swing)SwingHand();
        else SendSwingPacket();
    }
    return true;
}
}

bool UseSelectedHealingPotion() {
    Frame f;
    if (!f || compat::ScreenOpen()) return false;
    int slot = -1;
    jobject stack = nullptr;
    if (!SelectedSlot(slot) || slot < 0 || slot >= 9 || !InventorySlot(slot, stack) || !Healing(f, stack))
        return false;
    return UseStack(f, stack);
}

bool UseSelectedItem(KnownItem expected) {
    bool attempted=false;
    return UseSelectedItem(expected, attempted);
}

bool UseSelectedItem(KnownItem expected, bool& attempted) {
    attempted=false;
    Frame f;
    if (!f || compat::ScreenOpen() || expected == KnownItem::Nothing) return false;
    int slot = -1;
    jobject stack = nullptr;
    ItemFacts facts;
    if (!SelectedSlot(slot) || slot < 0 || slot >= 9 || !InventorySlot(slot, stack) ||
        !stack || !ReadItemFacts(stack, facts) || facts.empty || facts.count <= 0 || facts.item != expected)
        return false;
    return UseStack(f, stack, true, true, &attempted);
}

bool UseSelectedEquipment(const EntityHandle& expected) {
    Frame f;
    int slot = -1, effective = -1;
    jobject stack = nullptr;
    ItemFacts facts;
    if (!f || !expected || compat::IsLegacyLunar() || compat::ScreenOpen() ||
        !SelectedSlot(slot) || !EffectiveSelectedSlot(effective) || effective != slot ||
        slot < 0 || slot > 8 || !InventorySlot(slot, stack) ||
        !ItemStacksMatch(stack, static_cast<jobject>(expected.raw())) ||
        !ReadItemFacts(stack, facts) || facts.empty ||
        (facts.item != KnownItem::Elytra && facts.armorSlot != kArmorChest)) return false;
    // Vanilla equipment use swaps the hand stack and chest slot; no container
    // click or synthetic mouse state, and no extra swing for an equip action.
    return UseStack(f, stack, true, false);
}

bool UseMlgBucket(MlgBucket expected, float yaw, float pitch) {
    Frame f;int slot=-1;jobject stack=nullptr;
    if(!f || compat::ScreenOpen() || expected==MlgBucket::None || !EffectiveSelectedSlot(slot) ||
       MlgBucketAt(slot)!=expected || !InventorySlot(slot,stack))return false;
    // Bucket ray casts read the player's angles directly on several versions.
    // Use the already published rotation only for the synchronous vanilla use.
    float cameraYaw=0, cameraPitch=0;
    if(!PlayerRotation(cameraYaw,cameraPitch) || !SetPlayerRotation(yaw,pitch))return false;
    struct Restore {
        float yaw,pitch;
        ~Restore(){SetPlayerRotation(yaw,pitch);}
    } restore{cameraYaw,cameraPitch};
    return UseStack(f,stack,false);
}

bool UseSilentItem(KnownItem expected,float yaw,float pitch) {
    bool attempted=false;
    return UseSilentItem(expected,yaw,pitch,nullptr,attempted);
}

bool UseSilentItem(KnownItem expected,float yaw,float pitch,const void* owner,bool& attempted) {
    attempted=false;
    if(!std::isfinite(yaw)||!std::isfinite(pitch)||pitch< -90||pitch>90)return false;
    Frame f;int slot=-1;jobject stack=nullptr;ItemFacts facts;
    if(!f || compat::ScreenOpen() || expected==KnownItem::Nothing ||
       !EffectiveSelectedSlot(slot) || !InventorySlot(slot,stack) ||
       !ReadItemFacts(stack,facts) || facts.empty || facts.count<=0 || facts.item!=expected)return false;
    float cameraYaw=0,cameraPitch=0;
    if(!PlayerRotation(cameraYaw,cameraPitch)||!SetPlayerRotation(yaw,pitch))return false;
    struct Restore {float yaw,pitch;~Restore(){SetPlayerRotation(yaw,pitch);}} restore{cameraYaw,cameraPitch};
    return UseStack(f,stack,true,true,&attempted,owner);
}
}
