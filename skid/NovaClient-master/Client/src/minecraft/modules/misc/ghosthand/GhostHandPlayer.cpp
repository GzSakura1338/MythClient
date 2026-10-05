#include "minecraft/modules/misc/ghosthand/GhostHandPlayer.h"
#include "minecraft/modules/misc/ghosthand/RayTraversal.h"
#include "minecraft/modules/misc/antibot/AntiBot.h"
#include "minecraft/modules/misc/teams/Teams.h"
#include "minecraft/compat/lunar1710/Access.h"
#include "minecraft/java/api/game_click.h"
#include "minecraft/java/api/game_freecam.h"
#include "minecraft/java/api/game_inventory.h"
#include "minecraft/java/api/game_place.h"
#include "ui/clickgui/ClickGui.h"
#include <algorithm>
#include <limits>

namespace nova::module::ghosthand {
namespace {
namespace access = compat::mc263::access;
using access::Frame;
using access::Member;
using java::BlockPos;
using rotation::Vec3;
struct Mapping {
#define NOVA_BREAKER_MAPPING
#define NOVA_BREAKER(key, owner, name, desc) Member key;
#define NOVA_BREAKER_CLASSES(...) const char *bedClass, *airClass, *liquidClass;
#include "minecraft/compat/mappings/Lunar189.inc"
#undef NOVA_BREAKER
#undef NOVA_BREAKER_CLASSES
#undef NOVA_BREAKER_MAPPING
#define NOVA_GHOSTHAND_MAPPING
#define NOVA_GHOSTHAND(key, owner, name, desc) Member key;
#define NOVA_GHOSTHAND_CLASSES(...) const char *toolClass, *hoeClass, *shearsClass, *bucketClass;
#include "minecraft/compat/mappings/Lunar189.inc"
#undef NOVA_GHOSTHAND
#undef NOVA_GHOSTHAND_CLASSES
#undef NOVA_GHOSTHAND_MAPPING
};
#define NOVA_BREAKER_MAPPING
#define NOVA_BREAKER(key, owner, name, desc) .key = {owner, name, desc},
#define NOVA_BREAKER_CLASSES(bed, air, liquid) .bedClass = bed, .airClass = air, .liquidClass = liquid,
constexpr Mapping legacyBlocks{
#include "minecraft/compat/mappings/Lunar189.inc"
};
constexpr Mapping forgeBlocks{
#include "minecraft/compat/mappings/forge1201.inc"
};
constexpr Mapping modernBlocks{
#include "minecraft/compat/mappings/minecraft263.inc"
};
#undef NOVA_BREAKER
#undef NOVA_BREAKER_CLASSES
#undef NOVA_BREAKER_MAPPING
#define NOVA_GHOSTHAND_MAPPING
#define NOVA_GHOSTHAND(key, owner, name, desc) m.key = {owner, name, desc};
#define NOVA_GHOSTHAND_CLASSES(tool, hoe, shears, bucket) m.toolClass = tool; m.hoeClass = hoe; m.shearsClass = shears; m.bucketClass = bucket;
constexpr Mapping legacy = [] { auto m = legacyBlocks;
#include "minecraft/compat/mappings/Lunar189.inc"
return m; }();
constexpr Mapping forge = [] { auto m = forgeBlocks;
#include "minecraft/compat/mappings/forge1201.inc"
return m; }();
constexpr Mapping modern = [] { auto m = modernBlocks;
#include "minecraft/compat/mappings/minecraft263.inc"
return m; }();
#undef NOVA_GHOSTHAND
#undef NOVA_GHOSTHAND_CLASSES
#undef NOVA_GHOSTHAND_MAPPING

struct World {
    Frame f;
    const Mapping& n = compat::IsLegacyLunar() ? legacy : compat::UsesModernRuntime() ? modern : forge;
    const bool old = compat::IsLegacyLunar(), v17 = compat::IsLunar1710();
    jobject client{}, player{}, world{}, controller{};
    World() {
        if (!f) return;
        client = f.client(); player = f.player();
        world = f.get(client, n.level); controller = f.get(client, n.controller);
    }
    bool Ready() { return f && client && player && world && controller; }
    Vec3 Vector(Frame& scope, jobject v) {
        return {scope.get<double>(v, n.vx), scope.get<double>(v, n.vy), scope.get<double>(v, n.vz)};
    }
    jobject Block(Frame& scope, BlockPos p) {
        if (v17) return compat::lunar1710::Block(scope, world, p.x, p.y, p.z);
        return scope.call(scope.call(world, n.state, scope.create(n.pos, p.x, p.y, p.z)), n.block);
    }
    bool Bed(BlockPos p) {
        Frame scope(f.env);
        const bool result = scope.instance(Block(scope, p), n.bedClass);
        if (!scope) f.ok = false;
        return result;
    }
    bool Preferred(BlockPos p, const PlayerSettings& s) {
        if (!s.bed && !s.adjacent) return false;
        if (Bed(p)) return s.bed;
        if (!s.adjacent) return false;
        for (const BlockPos d : {BlockPos{0,-1,0}, {0,1,0}, {0,0,-1}, {0,0,1}, {-1,0,0}, {1,0,0}})
            if (Bed({p.x+d.x, p.y+d.y, p.z+d.z})) return true;
        return false;
    }
    jobject Clip(BlockPos p, Vec3 start, Vec3 end) {
        Frame scope(f.env);
        const auto pos = v17 ? nullptr : scope.create(n.pos, p.x, p.y, p.z);
        const auto state = v17 ? nullptr : scope.call(world, n.state, pos);
        const auto block = v17 ? Block(scope, p) : scope.call(state, n.block);
        if (!block || !scope) { f.ok = false; return nullptr; }
        if (old) {
            bool collides;
            if (v17) {
                const int metadata = scope.call<int>(world, {"net.minecraft.world.World", "getBlockMetadata", "(III)I"}, p.x, p.y, p.z);
                collides = scope.call<bool>(block, {n.canCollide.owner, n.canCollide.name, "(IZ)Z"}, metadata, JNI_FALSE);
            } else collides = scope.call<bool>(block, n.canCollide, state, JNI_FALSE);
            if (!scope) f.ok = false;
            if (!collides) return nullptr;
        }
        const auto from = scope.create(n.vector, start.x, start.y, start.z);
        const auto to = scope.create(n.vector, end.x, end.y, end.z);
        jobject hit;
        if (v17) hit = scope.call(block, compat::lunar1710::names::Clip, world, p.x, p.y, p.z, from, to);
        else if (old) hit = scope.call(block, n.clip, world, pos, from, to);
        else hit = scope.call(scope.call(state, n.shape, world, pos), n.clip, from, to, pos);
        if (!scope) { f.ok = false; return nullptr; }
        return scope.keep(hit);
    }
    bool HeldAllowed(const PlayerSettings& s) {
        int slot;
        jobject stack = nullptr;
        java::ItemFacts facts;
        if (!java::SelectedSlot(slot) || !java::InventorySlot(slot, stack) || !java::ReadItemFacts(stack, facts)) return false;
        if (facts.empty) return s.fists;
        if (facts.kinds & java::kItemSword) return !s.notSword && s.sword;
        const auto item = f.call(stack, n.stackItem);
        if (!item || !f) return false;
        // 26.3 represents hoes with plain Item rather than the removed HoeItem
        // subclass. The untranslated vanilla item key retains the tool kind.
        const bool hoe = compat::IsMinecraft263()
            ? f.text(f.call(item, compat::mc263::names::Item_getDescriptionId)).ends_with("_hoe")
            : f.instance(item, n.hoeClass);
        if ((facts.kinds & (java::kItemAxe | java::kItemPickaxe | java::kItemShovel)) ||
            (*n.toolClass && f.instance(item, n.toolClass)) || hoe || f.instance(item, n.shearsClass))
            return s.tool;
        if (f.instance(item, n.bucketClass)) return s.bucket;
        if (facts.item == java::KnownItem::FlintAndSteel) return s.flintSteel;
        if (facts.item == java::KnownItem::CobwebBlock) return s.cobweb;
        return s.other;
    }
};

bool Allowed(const java::EntityHandle& entity, const PlayerSettings& s) {
    if (!entity.IsPlayer()) return s.nonPlayer;
    if (antibot::IsBot(entity)) return s.bots;
    if (teams::IsTeammate(entity)) return s.friendlies;
    return s.enemies;
}
}

void PickPlayer(const PlayerSettings& s, float partialTicks) {
    // Do not let a detached camera interact on behalf of the real player.
    if (java::FreeCamActive() || java::ScreenOpen() || clickgui::IsOpen()) return;
    if (s.requireLeft && !(GetAsyncKeyState(VK_LBUTTON) & 0x8000)) return;
    if (s.requireRight && !(GetAsyncKeyState(VK_RBUTTON) & 0x8000)) return;
    World w;
    if (!w.Ready() || !w.HeldAllowed(s)) return;
    auto& f = w.f;
    const auto& n = w.n;
    const auto rawView = w.v17 ? compat::lunar1710::View(f) : f.call(w.client, n.view);
    auto view = java::AdoptEntity(rawView);
    Vec3 eyes;
    if (!view || !view.EyePosition(partialTicks, eyes)) return;
    const auto lookObject = f.call(rawView, n.look, partialTicks);
    if (!lookObject || !f) return;
    const auto look = w.Vector(f, lookObject);
    double reach = compat::UsesModernRuntime() ? f.call<double>(w.player, n.reach) : f.call<float>(w.controller, n.reach);
    if (!compat::UsesModernRuntime() && f.call<bool>(w.controller, n.extended)) reach = 6.0;
    if (!f || !std::isfinite(reach) || reach <= 0) return;
    const auto end = eyes.Add(look.x * reach, look.y * reach, look.z * reach);
    jobject first = nullptr, preferred = nullptr;
    Traverse(eyes, end, [&](int x, int y, int z) {
        auto hit = w.Clip({x,y,z}, eyes, end);
        if (!f) return true;
        if (!hit) return false;
        if (w.Preferred({x,y,z}, s)) { preferred = hit; return true; }
        if (!first) first = hit;
        else f.env->DeleteLocalRef(hit);
        return false;
    });
    const auto hit = preferred ? preferred : first;
    if (!hit || !f || (!s.everything && !preferred)) return;
    if (!preferred) {
        const auto hitVec = w.old ? f.get(hit, n.hitLocation) : f.call(hit, n.hitLocation);
        const auto point = w.Vector(f, hitVec);
        const double blockDistance = eyes.DistanceTo(point);
        java::Box scan{std::min(eyes.x,point.x)-1, std::min(eyes.y,point.y)-1, std::min(eyes.z,point.z)-1,
                       std::max(eyes.x,point.x)+1, std::max(eyes.y,point.y)+1, std::max(eyes.z,point.z)+1};
        std::vector<java::EntityHandle> candidates;
        if (!f || !java::WorldEntities(view, scan, candidates)) return;
        const auto vehicle = w.old ? f.get(rawView, n.vehicle) : f.call(rawView, n.vehicle);
        bool riderInteract = false;
        if (vehicle && access::ValidMember(n.riderInteract)) {
            // Forge adds this method; vanilla/Lunar may not provide it.
            Frame optional(f.env);
            const auto method = access::Method(optional.env, n.riderInteract);
            if (method) riderInteract = optional.env->CallBooleanMethod(rawView, method) == JNI_TRUE;
            optional.check();
        }
        const java::EntityHandle* closest = nullptr;
        double nearest = std::numeric_limits<double>::max();
        for (const auto& e : candidates) {
            if (e.IsSame(view) || e.IsSpectator() || !e.IsPickable()) continue;
            java::Box box;
            double border = 0;
            if (!e.BoundingBox(box) || !e.PickRadius(border)) continue;
            box = java::BoxInflate(box, border);
            // Raven 1.8 AxisAlignedBB.isVecInside excludes all six boundaries.
            const bool inside = eyes.x > box.minX && eyes.x < box.maxX &&
                                eyes.y > box.minY && eyes.y < box.maxY &&
                                eyes.z > box.minZ && eyes.z < box.maxZ;
            bool intersects = false;
            Vec3 intercept;
            if (!inside && (!java::BoxClip(box, eyes, point, intersects, intercept) || !intersects)) continue;
            const double distance = inside ? 0.0 : eyes.DistanceTo(intercept);
            if (distance >= blockDistance) continue;
            if (vehicle && !riderInteract && f.env->IsSameObject(vehicle, static_cast<jobject>(e.raw()))) continue;
            if (distance < nearest) { nearest = distance; closest = &e; }
        }
        // Decide using only the nearest obstruction, exactly as Raven does.
        if (!closest || !Allowed(*closest, s)) return;
    }
    // Resolve all fields before mutating any of them. A missing mapping leaves
    // the vanilla hit intact rather than producing a partially updated target.
    const auto hitField = access::Field(f.env, n.hit);
    const auto pointedField = access::Field(f.env, n.pointed);
    jobject renderer = nullptr;
    jfieldID rendererField = nullptr;
    if (w.old) {
        renderer = f.get(w.client, n.renderer);
        rendererField = access::Field(f.env, n.rendererPointed);
    }
    if (!f.check() || !hitField || !pointedField || (w.old && (!renderer || !rendererField))) return;
    f.env->SetObjectField(w.client, hitField, hit);
    f.env->SetObjectField(w.client, pointedField, nullptr);
    if (renderer) f.env->SetObjectField(renderer, rendererField, nullptr);
    f.check();
}
}
