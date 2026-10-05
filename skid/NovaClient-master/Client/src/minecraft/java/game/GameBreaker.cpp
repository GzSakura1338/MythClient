#include "minecraft/java/api/game_breaker.h"
#include "minecraft/java/api/game_freecam.h"
#include "minecraft/modules/player/breaker/Breaker.h"
#include "minecraft/compat/minecraft263/Minecraft263Access.h"
#include "minecraft/compat/lunar1710/Access.h"
#include "minecraft/java/api/game_network.h"
#include "minecraft/java/api/game_place.h"
#include <map>
#include <tuple>

namespace nova::java {
namespace {
using compat::mc263::access::Frame;
using compat::mc263::access::Member;
namespace n17 = compat::lunar1710::names;
#define NOVA_BREAKER_MAPPING
struct Mapping {
#define NOVA_BREAKER(key, owner, name, desc) Member key;
#define NOVA_BREAKER_CLASSES(...) const char *bedClass, *airClass, *liquidClass;
#include "minecraft/compat/mappings/Lunar189.inc"
#undef NOVA_BREAKER
#undef NOVA_BREAKER_CLASSES
};
#define NOVA_BREAKER(key, owner, name, desc) {owner, name, desc},
#define NOVA_BREAKER_CLASSES(...) __VA_ARGS__
constexpr Mapping lunar{
#include "minecraft/compat/mappings/Lunar189.inc"
};
constexpr Mapping forge{
#include "minecraft/compat/mappings/forge1201.inc"
};
constexpr Mapping modern{
#include "minecraft/compat/mappings/minecraft263.inc"
};
#undef NOVA_BREAKER
#undef NOVA_BREAKER_CLASSES
#undef NOVA_BREAKER_MAPPING
const Mapping &Names() { return compat::IsLegacyLunar() ? lunar : compat::UsesModernRuntime() ? modern : forge; }
constexpr Member ordinal{"java.lang.Enum", "ordinal", "()I"};
jobject EnumAt(Frame &f, Member values, int index) {
    auto array = static_cast<jobjectArray>(f.staticCall(values));
    if (!array || !f.check() || index < 0 || index >= f.env->GetArrayLength(array))
        return nullptr;
    auto value = f.env->GetObjectArrayElement(array, index);
    return f.check() ? value : nullptr;
}
} // namespace
struct BreakerWorld::Impl {
    Frame f;
    const Mapping &n = Names();
    bool legacy = compat::IsLegacyLunar();
    jobject client{}, player{}, world{}, controller{};
    bool ready = false;
    std::map<std::tuple<int, int, int>, module::breaker::Block> cache;
    Impl() {
        if (!f)
            return;
        client = f.client();
        player = f.player();
        world = f.get(client, n.level);
        controller = f.get(client, n.controller);
        ready = player && world && controller && f.check();
    }
};
BreakerWorld::BreakerWorld() : impl_(std::make_unique<Impl>()) {}
BreakerWorld::~BreakerWorld() = default;
bool BreakerWorld::Ready() const { return impl_->ready; }
module::breaker::Block BreakerWorld::Read(BlockPos p) {
    auto &r = *impl_;
    const auto key = std::make_tuple(p.x, p.y, p.z);
    if (auto it = r.cache.find(key); it != r.cache.end())
        return it->second;
    module::breaker::Block out;
    if (!r.ready)
        return out;
    Frame f(r.f.env);
    const auto &n = r.n;
    const bool v17 = compat::IsLunar1710();
    auto pos = v17 ? nullptr : f.create(n.pos, p.x, p.y, p.z);
    auto state = v17 ? nullptr : f.call(r.world, n.state, pos);
    auto block = v17 ? compat::lunar1710::Block(f,r.world,p.x,p.y,p.z) : f.call(state, n.block);
    if (!block || !f.check())
        return out;
    out.bed = f.instance(block, n.bedClass);
    out.air = f.instance(block, n.airClass);
    out.liquid = f.instance(block, n.liquidClass);
    out.hardness = v17 ? f.call<float>(block,n17::Hardness,r.world,p.x,p.y,p.z) : f.call<float>(r.legacy ? block : state, n.hardness, r.world, pos);
    out.relative = v17 ? f.call<float>(block,n17::RelativeHardness,r.player,r.world,p.x,p.y,p.z) : f.call<float>(r.legacy ? block : state, n.relative, r.player, r.world, pos);
    if (v17) out.passable = f.call(block,n17::Collision,r.world,p.x,p.y,p.z) == nullptr;
    else if (r.legacy)
        out.passable = f.call(block, n.collision, r.world, pos, state) == nullptr;
    else {
        auto shape = f.call(state, n.collision, r.world, pos);
        out.passable = shape && f.call<bool>(shape, n.empty);
    }
    out.valid = f.check();
    if (!out.valid)
        out = {};
    r.cache.emplace(key, out);
    return out;
}
bool BreakerWorld::Ray(rotation::Vec3 from, rotation::Vec3 target, double range, BlockHitResultInfo &out) {
    const double length = from.DistanceTo(target);
    if (length < 1.e-9)
        return false;
    const auto end = from.Add((target.x - from.x) * range / length, (target.y - from.y) * range / length,
                              (target.z - from.z) * range / length);
    auto self = PlayerEntity();
    return WorldClip(from, end, self, out) && out.kind != HitKind::Miss;
}
float BreakerWorld::Progress() {
    auto &r = *impl_;
    return r.ready ? r.f.get<float>(r.controller, r.n.progress) : 0;
}
bool BreakerWorld::SetProgress(float v) {
    auto &r = *impl_;
    return r.ready && r.f.set(r.controller, r.n.progress, v);
}
bool BreakerWorld::Hit(BlockPos p, rotation::Vec3 from, rotation::Vec3 to, int &face) {
    auto &r = *impl_;
    if (!r.ready)
        return false;
    Frame f(r.f.env);
    const auto &n = r.n;
    const bool v17 = compat::IsLunar1710();
    auto pos = v17 ? nullptr : f.create(n.pos, p.x, p.y, p.z);
    auto state = v17 ? nullptr : f.call(r.world, n.state, pos);
    auto block = v17 ? compat::lunar1710::Block(f,r.world,p.x,p.y,p.z) : f.call(state, n.block);
    auto start = f.create(n.vector, from.x, from.y, from.z), end = f.create(n.vector, to.x, to.y, to.z);
    jobject hit = nullptr;
    if (v17) hit = f.call(block,n17::Clip,r.world,p.x,p.y,p.z,start,end);
    else if (r.legacy)
        hit = f.call(block, n.clip, r.world, pos, start, end);
    else {
        auto shape = f.call(state, n.shape, r.world, pos);
        hit = f.call(shape, n.clip, start, end, pos);
    }
    if (!hit || !f.check())
        return false;
    if (v17) {
        face = f.get<int>(hit,n17::HitSide);
        return f.check() && f.set(r.client,n.hit,hit);
    }
    auto direction = r.legacy ? f.get(hit, n.hitFace) : f.call(hit, n.hitFace);
    face = f.call<int>(direction, ordinal);
    return direction && f.check() && f.set(r.client, n.hit, hit);
}
bool BreakerWorld::Damage(BlockPos p, int face, bool slowAir) {
    if (FreeCamBlocks(FreeCamAction::Dig)) return false;
    auto &r = *impl_;
    if (!r.ready)
        return false;
    Frame f(r.f.env);
    const auto &n = r.n;
    const bool v17=compat::IsLunar1710();
    auto pos = v17 ? nullptr : f.create(n.pos, p.x, p.y, p.z);
    auto direction = v17 ? nullptr : EnumAt(f, n.facing, face);
    if ((!v17 && (!pos || !direction)) || !f.check())
        return false;
    const bool ground = f.get<bool>(r.player, n.ground);
    if (!slowAir && !f.set(r.player, n.ground, true))
        return false;
    const bool damage = v17 ? f.invoke(r.controller,n17::Damage,p.x,p.y,p.z,face) : f.call<bool>(r.controller, n.damage, pos, direction);
    // Restore through an independent frame even if the controller threw.
    if (!slowAir) {
        Frame restore(r.f.env);
        restore.set(r.player, n.ground, ground);
    }
    if (damage && f.check())
        SwingMainHand();
    return f.check();
}
bool BreakerWorld::Instant(BlockPos p, bool slowAir) {
    if (FreeCamBlocks(FreeCamAction::Dig)) return false;
    auto &r = *impl_;
    if (!r.ready)
        return false;
    Frame f(r.f.env);
    const auto &n = r.n;
    const bool v17=compat::IsLunar1710();
    auto pos = v17 ? nullptr : f.create(n.pos,p.x,p.y,p.z);
    auto up = v17 ? nullptr : EnumAt(f,n.facing,1), down = v17 ? nullptr : EnumAt(f,n.facing,0);
    auto start = v17 ? nullptr : EnumAt(f,n.actions,0), stop = v17 ? nullptr : EnumAt(f,n.actions,2);
    const Member dig17{n.dig.owner,"<init>","(IIIII)V"};
    auto first = v17 ? f.create(dig17,0,p.x,p.y,p.z,1) : f.create(n.dig,start,pos,up);
    auto last = v17 ? f.create(dig17,2,p.x,p.y,p.z,1) : f.create(n.dig,stop,pos,up);
    if (!first || !last || !f.check())
        return false;
    SwingMainHand();
    if (!SendPacket(first))
        return false;
    SwingMainHand();
    if (!SendPacket(last))
        return false;
    const bool ground = f.get<bool>(r.player, n.ground);
    if (!slowAir)
        f.set(r.player, n.ground, true);
    if (v17) f.call<bool>(r.controller,n17::Destroy,p.x,p.y,p.z,0);
    else if (r.legacy)
        f.call<bool>(r.controller, n.destroy, pos, down);
    else
        f.call<bool>(r.controller, n.destroy, pos);
    if (!slowAir) {
        Frame restore(r.f.env);
        restore.set(r.player, n.ground, ground);
    }
    return f.check();
}
void BreakerWorld::Stop() {
    auto &r = *impl_;
    if (r.ready)
        r.f.invoke(r.controller, r.n.stop);
}
bool BreakerDigPacket(jobject packet) {
    Frame f;
    const auto &n = Names();
    if (!f || !f.instance(packet, n.dig.owner))
        return false;
    if (compat::IsLunar1710()) {
        const int value=f.call<int>(packet,{n.dig.owner,"getDiggedBlockStatus","()I"});
        return f.check() && (value==0 || value==2);
    }
    auto action = f.call(packet, n.digAction);
    const int value = f.call<int>(action, ordinal);
    return action && f.check() && (value == 0 || value == 2);
}
bool BreakerBlinkPacket(jobject packet, bool incoming) {
    // Rise's default blink selects transaction/keepalive traffic and the play
    // interaction families, leaving chat and tab completion responsive.
    if (incoming)
        return IsPingPacket(packet);
    Frame f;
    if (!f || !packet)
        return false;
    auto type = f.env->GetObjectClass(packet);
    const auto name = f.text(f.call(type, {"java.lang.Class", "getSimpleName", "()Ljava/lang/String;"}));
    if (!f.check())
        return false;
    if (compat::IsLegacyLunar())
        return name != "C01PacketChatMessage" && name != "C14PacketTabComplete";
    return name.find("Chat") == std::string::npos && name.find("Command") == std::string::npos;
}
bool BreakerBlockUpdate(jobject packet, BlockPos &p, bool &air) {
    Frame f;
    const auto &n = Names();
    if (compat::IsLunar1710()) {
        if (!f.instance(packet,n17::BlockUpdateX.owner)) return false;
        p={f.get<int>(packet,n17::BlockUpdateX),f.get<int>(packet,n17::BlockUpdateY),f.get<int>(packet,n17::BlockUpdateZ)};
        auto block=f.get(packet,n17::BlockUpdateBlock);
        air=f.instance(block,n.airClass);
        return block && f.check();
    }
    if (!f || !f.instance(packet, n.updatePos.owner))
        return false;
    auto pos = f.call(packet, n.updatePos), state = f.call(packet, n.updateState),
         block = f.call(state, n.block);
    p = {f.call<int>(pos, n.posX), f.call<int>(pos, n.posY), f.call<int>(pos, n.posZ)};
    air = f.instance(block, n.airClass);
    return block && pos && f.check();
}
bool BreakerTeleport(jobject packet, rotation::Vec3 &out) {
    Frame f;
    const auto &n = Names();
    if (compat::IsLunar1710()) {
        if (!f.instance(packet,n17::TeleportX.owner)) return false;
        out={f.get<double>(packet,n17::TeleportX),f.get<double>(packet,n17::TeleportY),f.get<double>(packet,n17::TeleportZ)};
        return f.check();
    }
    if (!f || !f.instance(packet, n.teleX.owner))
        return false;
    if (compat::UsesModernRuntime()) {
        auto change = f.call(packet, n.teleX), pos = f.call(change, n.teleY);
        out = {f.get<double>(pos, compat::mc263::names::Vec3_Field_x),
               f.get<double>(pos, compat::mc263::names::Vec3_Field_y),
               f.get<double>(pos, compat::mc263::names::Vec3_Field_z)};
    } else
        out = {f.call<double>(packet, n.teleX), f.call<double>(packet, n.teleY),
               f.call<double>(packet, n.teleZ)};
    auto flags = f.call(packet, n.teleFlags);
    auto iterator = f.call(flags, {"java.util.Set", "iterator", "()Ljava/util/Iterator;"});
    rotation::Vec3 self;
    auto entity = PlayerEntity();
    if (!entity.Position(self))
        return false;
    while (iterator && f.call<bool>(iterator, {"java.util.Iterator", "hasNext", "()Z"})) {
        auto flag = f.call(iterator, {"java.util.Iterator", "next", "()Ljava/lang/Object;"});
        auto name = f.call(flag, {"java.lang.Enum", "name", "()Ljava/lang/String;"});
        const auto text = f.text(name);
        if (text == "X")
            out.x += self.x;
        else if (text == "Y")
            out.y += self.y;
        else if (text == "Z")
            out.z += self.z;
        if (flag)
            f.env->DeleteLocalRef(flag);
        if (name)
            f.env->DeleteLocalRef(name);
    }
    return f.check();
}
void JNICALL BreakerContinueHook(JNIEnv *env, jclass, jobject client, jboolean held) {
    if (FreeCamBlocksMouse(false)) return;
    if (!IsCircuitOpen() && module::breaker::OwnsMining())
        return;
    Frame f(env);
    f.invoke(client, Names().continueMining, held);
}
} // namespace nova::java
