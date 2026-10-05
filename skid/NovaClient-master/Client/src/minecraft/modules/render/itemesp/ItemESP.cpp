#include <array>
#include <format>
#include <map>
#include <mutex>
#include "minecraft/modules/Module.h"
#include "minecraft/modules/Registry.h"
#include "minecraft/modules/render/itemesp/ItemEspStyle.h"
#include "minecraft/compat/minecraft263/Minecraft263Access.h"
#include "minecraft/events/GameEvents.h"
#include "minecraft/java/api/game_entity.h"
#include "minecraft/host/render/WorldProjection.h"
#include "ui/hud/itemesp/ItemLabel.h"
#include "ui/hud/world/ProjectedBox.h"

namespace nova::module {
namespace {
namespace access = compat::mc263::access;
struct Mapping { access::Member stack, item, count, iron, gold, diamond, emerald; };
#define NOVA_ITEMESP_MAPPING
#define NOVA_ITEMESP(id, owner, name, desc) .id = {owner, name, desc},
constexpr Mapping lunar{
#include "minecraft/compat/mappings/Lunar189.inc"
};
constexpr Mapping forge{
#include "minecraft/compat/mappings/forge1201.inc"
};
constexpr Mapping modern{
#include "minecraft/compat/mappings/minecraft263.inc"
};
#undef NOVA_ITEMESP
#undef NOVA_ITEMESP_MAPPING
const Mapping& Names() { return compat::IsLegacyLunar() ? lunar : compat::UsesModernRuntime() ? modern : forge; }
struct Entry { rotation::Vec3 now, before; int kind; double key; };
ImU32 Color(std::int32_t argb, int alpha = 255) {
    return IM_COL32((argb >> 16) & 255, (argb >> 8) & 255, argb & 255, alpha);
}

class ItemEspModule final : public Module {
public:
    ItemEspModule() : Module("ItemESP", Category::Render, "General", "Overlay") {
        enableOverlay();
        iron_ = &addBool("Render iron", true);
        gold_ = &addBool("Render gold", true);
        textShadow_ = &addBool("Text Shadow", true);
        auto opacity = subNumber("Opacity", 0, 100, 1, 35,
            [](double n) { return std::format("{:.0f}%", n); });
        opacity_ = static_cast<Properties::NumberProp*>(opacity.getProp());
        alpha_ = &addBool("Alpha", true, {std::move(opacity)});
        distance_ = &addNumber("Max distance", 32, 256, 8, 128, {},
            [](double n) { return std::format("{:.0f}", n); });
        respawn_ = event::EventBus::Get().Subscribe<event::RespawnEvent>([this](auto&) { Clear(); });
    }
private:
    void Clear() { std::lock_guard lock(mutex_); entries_.clear(); counts_.clear(); }
    void onDisable() override { Clear(); }
    void onEnable() override { lastTick_ = -1; }
    void onUpdate() override {
        if (!java::PlayerEntity()) { Clear(); lastTick_=-1; return; }
        const int tick = java::PlayerTickCount();
        // Nova publishes tick HEAD, not tick END. Sample after the completed
        // tick on the render update, keeping both interpolation endpoints fresh.
        if (tick != lastTick_) { lastTick_=tick; Gather(); }
    }
    void Gather() {
        std::vector<Entry> fresh;
        std::map<double, std::int64_t> counts;
        const auto self = java::PlayerEntity();
        java::CameraView view;
        if (!self || !java::ReadCameraView(1, view)) { Clear(); return; }
        const rotation::Vec3 origin{view.x, view.y, view.z};
        const double range = distance_->get();
        std::vector<java::EntityHandle> entities;
        if (!java::WorldEntities(self, {origin.x-range, origin.y-range, origin.z-range,
                origin.x+range, origin.y+range, origin.z+range}, entities)) { Clear(); return; }
        access::Frame frame;
        if (!frame) { Clear(); return; }
        const auto& m = Names();
        auto type = access::Class(frame.env, m.stack.owner);
        const std::array<jobject,4> items{frame.stat(m.iron), frame.stat(m.gold), frame.stat(m.diamond), frame.stat(m.emerald)};
        if (!type || !frame) { Clear(); return; }
        for (const auto& entity : entities) {
            if (!frame.env->IsInstanceOf(static_cast<jobject>(entity.raw()), type)) continue;
            int age = 0;
            if (!entity.IsAlive() || !entity.TickCount(age) || age < 3) continue;
            Entry entry{};
            if (!entity.RenderPositionPair(entry.now, entry.before) ||
                !std::isfinite(entry.now.x) || !std::isfinite(entry.now.y) || !std::isfinite(entry.now.z) ||
                origin.DistanceToSqr(entry.now) > range*range) continue;
            access::Frame f(frame.env);
            auto stack = f.call(static_cast<jobject>(entity.raw()), m.stack);
            if (!stack || !f) continue;
            const int count = compat::IsLegacyLunar() ? f.get<int>(stack, m.count) : f.call<int>(stack, m.count);
            auto item = f.call(stack, m.item);
            if (!item || !f || count <= 0) continue;
            int kind = -1;
            for (int i=0;i<4;++i) if (f.env->IsSameObject(item, items[i])) { kind=i; break; }
            if (kind < 0 || (kind == 0 && !iron_->get()) || (kind == 1 && !gold_->get())) continue;
            entry.kind = kind;
            entry.key = itemesp::GroupKey(entry.now.x, entry.now.y, entry.now.z, kind);
            counts[entry.key] += count;
            fresh.push_back(entry);
        }
        std::lock_guard lock(mutex_);
        entries_ = std::move(fresh); counts_ = std::move(counts);
    }
    void onRender() override {
        if (!ImGui::GetCurrentContext() || !java::PlayerEntity()) return;
        const auto display = ImGui::GetIO().DisplaySize;
        const float partial = java::FrameTime();
        java::CameraView view;
        if (!java::ReadCameraView(partial, view)) return;
        const auto basis = minecraft::projection::MakeBasis(view, partial);
        std::vector<Entry> entries;
        std::map<double,std::int64_t> counts;
        { std::lock_guard lock(mutex_); entries=entries_; counts=counts_; }
        auto* draw = ImGui::GetBackgroundDrawList();
        const int boxAlpha = alpha_->get() ?
            static_cast<int>(std::lround(std::clamp(opacity_->get(), 0.0, 100.0) * 2.55)) : 255;
        for (const auto& entry : entries) {
            const auto p = entry.before.Add((entry.now.x-entry.before.x)*partial,
                (entry.now.y-entry.before.y)*partial, (entry.now.z-entry.before.z)*partial);
            const double distance = std::sqrt(p.DistanceToSqr(basis.eye));
            const double r = itemesp::Radius(distance);
            ui::ProjectedBox corners;
            bool visible = true;
            for (int i=0;i<8;++i) {
                minecraft::projection::ScreenPoint point;
                if (!minecraft::projection::Project(basis,
                    p.Add(i&1 ? r : -r, i&2 ? 2*r : 0, i&4 ? r : -r), display, point)) { visible=false; break; }
                corners[i]={point.x,point.y};
            }
            if (!visible) continue;
            ui::DrawFilledBox(draw,corners,Color(itemesp::boxColors[entry.kind],boxAlpha));
            minecraft::projection::ScreenPoint anchor, fontTop;
            const auto textAt = p.Add(0,.3,0);
            const double scale = itemesp::TextScale(distance);
            // Project a world-space font height so Raven's distance scaling survives
            // the switch from a billboard font to Nova's ImGui HUD font.
            if (!minecraft::projection::Project(basis,textAt,display,anchor) || !minecraft::projection::Project(basis,
                textAt.Add(basis.up.x*9*scale,basis.up.y*9*scale,basis.up.z*9*scale),display,fontTop)) continue;
            const float size = std::clamp(std::hypot(fontTop.x-anchor.x,fontTop.y-anchor.y),1.f,128.f);
            ui::DrawItemLabel({anchor.x,anchor.y},size,scale,counts.at(entry.key),
                Color(itemesp::textColors[entry.kind]),textShadow_->get());
        }
    }
    Properties::BoolProperty *iron_, *gold_, *textShadow_, *alpha_;
    Properties::NumberProp *distance_, *opacity_;
    std::mutex mutex_;
    std::vector<Entry> entries_;
    std::map<double,std::int64_t> counts_;
    int lastTick_ = -1;
    event::Subscription respawn_;
};
}
std::unique_ptr<Module> MakeItemEspModule() { return std::make_unique<ItemEspModule>(); }
}
