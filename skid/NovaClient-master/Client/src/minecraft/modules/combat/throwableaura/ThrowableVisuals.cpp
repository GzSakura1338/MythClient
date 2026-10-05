#include "minecraft/modules/combat/throwableaura/ThrowableVisuals.h"
#include "minecraft/compat/lunar189/LunarRuntime.h"
#include "minecraft/java/api/game_chest.h"
#include "minecraft/host/render/WorldProjection.h"
#include "minecraft/host/render/WorldGeometry.h"
#include "ui/hud/throwableaura/ThrowablePreview.h"

namespace nova::module::throwableaura {
namespace lunar = compat::lunar;
void Visuals::Clear() {
    std::lock_guard lock(mutex_);
    world_.Reset(); targetId_ = -1; path_ = {}; updated_ = {};
}
void Visuals::Update(const java::EntityHandle& self, const java::EntityHandle& target,
                     float yaw, float pitch, bool trajectory) {
    auto world = java::LevelEntity();
    int id = -1;
    if (!world || !self || !target || !java::EntityId(target,id)) { Clear(); return; }
    Path path;
    Vec3 eye;
    if (trajectory && self.EyePosition(1.f,eye)) {
        std::vector<java::EntityHandle> entities;
        std::vector<Bounds> boxes;
        // Capture hitboxes once per tick, not once per segment or rendered frame.
        if (java::WorldEntities(self,{eye.x-65,eye.y-65,eye.z-65,eye.x+65,eye.y+65,eye.z+65},entities)) {
            boxes.reserve(entities.size());
            for (const auto& entity : entities) {
                java::Box box;
                if (!entity || entity.IsSame(self) || !entity.IsPickable() || !entity.BoundingBox(box)) continue;
                boxes.push_back({{box.minX-.3,box.minY-.3,box.minZ-.3}, {box.maxX+.3,box.maxY+.3,box.maxZ+.3}});
            }
            path = Predict(eye,yaw,pitch,boxes,
                [&](Vec3 from, Vec3 to, bool& hit, Vec3& location) {
                    java::BlockHitResultInfo block;
                    if (!lunar::WorldClip(from,to,self,block)) return false;
                    hit = block.kind == java::HitKind::Block; location = block.location; return true;
                },
                [](Vec3 point) {
                    std::string name;
                    if (!lunar::BlockRegistryName({int(std::floor(point.x)),int(std::floor(point.y)),int(std::floor(point.z))},name)) return false;
                    return name == "minecraft:water" || name == "minecraft:flowing_water" || name == "water" || name == "flowing_water";
                });
        }
    }
    std::lock_guard lock(mutex_);
    world_ = std::move(world); targetId_ = id; path_ = std::move(path);
    updated_ = std::chrono::steady_clock::now();
}
void Visuals::Render(bool esp, bool trajectory) {
    if ((!esp && !trajectory) || !ImGui::GetCurrentContext()) return;
    auto world = java::LevelEntity();
    if (!world || !java::PlayerEntity()) return;
    Path path;
    int targetId;
    {
        std::lock_guard lock(mutex_);
        if (!world_ || !world_.IsSame(world) ||
            std::chrono::steady_clock::now()-updated_ > std::chrono::milliseconds(250)) return;
        targetId = targetId_;
        if (trajectory) path = path_;
    }
    auto target = java::EntityById(targetId);
    if (!target || !target.IsAlive()) return;
    const auto display = ImGui::GetIO().DisplaySize;
    const float partial = java::FrameTime();
    java::CameraView camera;
    if (display.x <= 1 || display.y <= 1 || !std::isfinite(partial) || !java::ReadCameraView(partial,camera)) return;
    const auto basis = minecraft::projection::MakeBasis(camera,partial);
    auto project = [&](Vec3 worldPoint, ImVec2& point) {
        minecraft::projection::ScreenPoint p;
        if (!minecraft::projection::Project(basis,worldPoint,display,p) || !std::isfinite(p.x) || !std::isfinite(p.y)) return false;
        point = {p.x,p.y}; return true;
    };
    ui::ScreenGeometry targetGeometry, pathGeometry;
    if (esp) {
        java::Box box;
        Vec3 now,before,position;
        if (target.BoundingBox(box) && target.Position(position) && target.RenderPositionPair(now,before)) {
            const auto offset = before.Add((now.x-before.x)*partial,(now.y-before.y)*partial,(now.z-before.z)*partial).Subtract(position);
            box.minX += offset.x; box.maxX += offset.x;
            box.minY += offset.y; box.maxY += offset.y;
            box.minZ += offset.z; box.maxZ += offset.z;
            const std::array<Vec3,8> corners{{{box.minX,box.minY,box.minZ},{box.maxX,box.minY,box.minZ},
                {box.maxX,box.minY,box.maxZ},{box.minX,box.minY,box.maxZ},
                {box.minX,box.maxY,box.minZ},{box.maxX,box.maxY,box.minZ},
                {box.maxX,box.maxY,box.maxZ},{box.minX,box.maxY,box.maxZ}}};
            minecraft::projection::DrawClippedBox<ImVec2>(corners,project,
                [&](auto& polygon) { targetGeometry.faces.push_back(std::move(polygon)); },
                [&](ImVec2 a, ImVec2 b) { targetGeometry.edges.push_back({a,b}); });
        }
    }
    for (std::size_t i=1; i<path.points.size(); ++i)
        minecraft::projection::DrawClippedLine<ImVec2>(path.points[i-1],path.points[i],project,
            [&](ImVec2 a, ImVec2 b) { pathGeometry.edges.push_back({a,b}); });
    ImVec2 impact;
    const bool impactVisible = path.impact && !path.points.empty() && project(path.points.back(),impact);
    ui::throwableaura::Draw(targetGeometry,pathGeometry,impactVisible ? &impact : nullptr);
}
}
