#include "minecraft/java/api/game_inventory.h"
#include "minecraft/compat/minecraft263/Minecraft263Access.h"

namespace nova::java {
namespace {
using namespace compat::mc263::access;
struct Mapping { Member copy, matches, count, empty; };
#define NOVA_INVENTORY_STACK_MAPPING
#define NOVA_STACK(key, name, desc) .key={"net.minecraft.world.item.ItemStack", name, desc},
constexpr Mapping forge{
#include "minecraft/compat/mappings/forge1201.inc"
};
constexpr Mapping modern{
#include "minecraft/compat/mappings/minecraft263.inc"
};
#undef NOVA_STACK
#undef NOVA_INVENTORY_STACK_MAPPING
const Mapping& Map() { return compat::UsesModernRuntime() ? modern : forge; }
}
bool CopyItemStack(jobject stack, jobject& out) {
    out = nullptr;
    if (compat::IsLegacyLunar() || !stack) return false;
    Frame f;
    if (!f) return false;
    out = f.keep(f.call(stack, Map().copy));
    return out != nullptr;
}
bool ReadItemIdentity(jobject stack, jobject& out,bool& empty) {
    out=nullptr;empty=true;
    if(compat::IsLegacyLunar()||!stack)return false;
    Frame f;if(!f)return false;
    empty=f.call<bool>(stack,Map().empty);
    if(!f.check())return false;
    if(empty)return true;
    auto copy=f.call(stack,Map().copy);
    if(!copy||!f.set(copy,Map().count,1))return false;
    out=f.keep(copy);return out!=nullptr;
}
bool ItemStacksMatch(jobject left, jobject right) {
    if (compat::IsLegacyLunar() || !left || !right) return false;
    Frame f;
    if (!f) return false;
    const auto member = Map().matches;
    auto method = Method(f.env, member, true);
    auto owner = Class(f.env, member.owner);
    if (!method || !owner || !f.check()) return false;
    const bool same = f.env->CallStaticBooleanMethod(owner, method, left, right) != JNI_FALSE;
    return f.check() && same;
}
}
