#include "minecraft/modules/combat/legitcrystalaura/PlacementEsp.h"
#include "minecraft/host/render/WorldProjection.h"
#include "ui/hud/world/ProjectedBox.h"
#include "ui/clickgui/panel/ClientSettings.h"
#include "ui/clickgui/panel/Theme.h"

namespace nova::module::crystal {
void PlacementEsp::Draw() {
    if(!ImGui::GetCurrentContext())return;
    const auto selected=Read();auto world=java::LevelEntity();
    if(!selected.world||!world||!selected.world->IsSame(world)||
       std::chrono::steady_clock::now()-selected.updated>std::chrono::milliseconds(250))return;
    java::CameraView camera;const float partial=java::FrameTime();
    const auto display=ImGui::GetIO().DisplaySize;
    if(!std::isfinite(partial)||display.x<=1||display.y<=1||!java::ReadCameraView(partial,camera))return;
    const auto basis=minecraft::projection::MakeBasis(camera,partial);
    ui::ProjectedBox points;bool visible[8]{};
    for(int i=0;i<8;++i) {
        const rotation::Vec3 corner{selected.block.x+(i&1?1.003:-.003),
            selected.block.y+(i&2?1.003:-.003),selected.block.z+(i&4?1.003:-.003)};
        minecraft::projection::ScreenPoint p;
        visible[i]=minecraft::projection::Project(basis,corner,display,p)&&std::isfinite(p.x)&&std::isfinite(p.y);
        if(visible[i])points[i]={p.x,p.y};
    }
    const auto tint=menu::panel::ThemeColors(menu::panel::ClientSettings::Get().themeValue()).iconHighlightColor;
    auto color=ImGui::ColorConvertU32ToFloat4(tint);color.w=.85f;
    const auto line=ImGui::ColorConvertFloat4ToU32(color);color.w=.08f;
    auto* draw=ImGui::GetBackgroundDrawList();
    constexpr int faces[6][4]={{0,1,5,4},{2,6,7,3},{0,2,3,1},{4,5,7,6},{0,4,6,2},{1,3,7,5}};
    for(const auto& face:faces) {
        if(!visible[face[0]]||!visible[face[1]]||!visible[face[2]]||!visible[face[3]])continue;
        const ImVec2 polygon[]{points[face[0]],points[face[1]],points[face[2]],points[face[3]]};
        draw->AddConvexPolyFilled(polygon,4,ImGui::ColorConvertFloat4ToU32(color));
    }
    // Keep visible edges when part of the block crosses the camera near plane.
    for(int i=0;i<8;++i)for(int bit:{1,2,4})
        if(!(i&bit)&&visible[i]&&visible[i|bit])draw->AddLine(points[i],points[i|bit],line,1.5f);
}
}
