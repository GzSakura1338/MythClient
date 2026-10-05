#pragma once
#include "minecraft/compat/lunar1710/Mapping.h"
#include "minecraft/compat/minecraft263/Minecraft263Access.h"
#include "minecraft/java/api/game_entity.h"

namespace nova::compat::lunar1710 {
using Frame = mc263::access::Frame;
inline jobject Block(Frame& f, jobject world, int x, int y, int z) {
    return f.call(world, names::GetBlock, x, y, z);
}
inline jobject View(Frame& f) { return f.get(f.client(), names::View); }
inline jobject Scaled(Frame& f, mc263::names::Member ctor, jobject client) {
    return f.create(ctor,client,
        f.get<int>(client,{"net.minecraft.client.Minecraft","displayWidth","I"}),
        f.get<int>(client,{"net.minecraft.client.Minecraft","displayHeight","I"}));
}
inline jobject TabInfo(Frame& f, const java::EntityHandle& entity, bool& available) {
    available = false;
    std::string name;
    if (!entity.ScoreboardName(name)) return nullptr;
    auto connection = f.call(f.client(), {"net.minecraft.client.Minecraft","getNetHandler","()Lnet/minecraft/client/network/NetHandlerPlayClient;"});
    auto list = f.get(connection, names::TabList);
    if (!list || !f) return nullptr;
    const int count = f.call<int>(list, {"java.util.List","size","()I"});
    for (int i=0;i<count && f;++i) {
        auto entry = f.call(list, {"java.util.List","get","(I)Ljava/lang/Object;"}, i);
        if (f.text(f.get(entry, names::TabName)) == name) { available = f.check(); return entry; }
        if (entry) f.env->DeleteLocalRef(entry);
    }
    available = f.check();
    return nullptr;
}
}
