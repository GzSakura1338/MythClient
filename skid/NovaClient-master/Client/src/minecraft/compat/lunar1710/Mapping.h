#pragma once
#include "minecraft/compat/Profile.h"
#include "minecraft/compat/minecraft263/Minecraft263.h"
#include "minecraft/compat/easecation/EaseCation.h"
#include "minecraft/compat/neoforge12110/NeoForge12110.h"
#include "minecraft/compat/lunar12111/Mapping.h"
#include <string_view>

namespace nova::compat::lunar1710 {
namespace names {
#define NOVA_LUNAR1710_MEMBER(id, owner, name, desc) inline constexpr mc263::names::Member id{owner,name,desc};
#include "minecraft/compat/mappings/Lunar1710.inc"
#undef NOVA_LUNAR1710_MEMBER
}
// Translation is limited to verified symbols. Callers must handle ABI changes
// (for example BlockPos -> III) explicitly; this does not adapt varargs.
template<class Member> Member Translate(Member member) {
    if (IsLunar12111()) {
        auto mapped=lunar12111::Translate({member.owner,member.name,member.descriptor});
        return {mapped.owner,mapped.name,mapped.descriptor};
    }
    if (IsEaseCation()) {
        auto mapped=easecation::Translate({member.owner,member.name,member.descriptor});
        return {mapped.owner,mapped.name,mapped.descriptor};
    }
    if (IsNeoForge12110()) {
        auto mapped=neoforge12110::Translate({member.owner,member.name,member.descriptor});
        return {mapped.owner,mapped.name,mapped.descriptor};
    }
    if (!IsLunar1710() || !member.owner || !member.name || !member.descriptor) return member;
    struct Alias { Member from, to; };
    static constexpr Alias aliases[] = {
#define NOVA_LUNAR1710_ALIAS(oc, on, od, nc, nn, nd) {{oc,on,od},{nc,nn,nd}},
#include "minecraft/compat/mappings/Lunar1710.inc"
#undef NOVA_LUNAR1710_ALIAS
    };
    for (const auto& entry : aliases)
        if (std::string_view(member.owner) == entry.from.owner && std::string_view(member.name) == entry.from.name &&
            std::string_view(member.descriptor) == entry.from.descriptor) return entry.to;
    return member;
}
}
