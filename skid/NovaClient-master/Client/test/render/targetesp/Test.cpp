#include "ui/hud/targetesp/TargetRing.h"
#include "minecraft/modules/render/targetesp/RingProjection.h"
#include "imgui_internal.h"
#include <cassert>
#include <iostream>
#include <limits>

using namespace nova::ui::targetesp;
namespace projection=nova::module::targetesp;

int main() {
    const auto initial=Animate(0,.75,1);
    assert(std::abs(initial.pulse-.675f)<1e-6);
    assert(initial.top[0].x==.75 && initial.top[0].y==1);
    assert(std::abs(initial.bottom[0].y-(1-std::sin(1.)/2.7))<1e-7);
    assert(initial.top[64].x==initial.top[0].x && initial.top[64].z==initial.top[0].z);
    assert(std::abs(initial.top[16].z-.75)<1e-6);
    for(int t=0;t<10000;t+=37) {
        const auto ring=Animate(t,1.4,2);
        assert(ring.pulse>=.35f && ring.pulse<=1.65001f);
        assert(ring.top[0].y>=0 && ring.top[0].y<=2);
        assert(std::abs(ring.top[0].y-ring.bottom[0].y)<=1./2.7+1e-6);
        for(const auto& p:ring.top)assert(std::abs(std::hypot(p.x,p.z)-1.4)<1e-6);
        assert(Animate(t,1.4,0).pulse==.35f); // Alpha Factor doesn't erase the source's base opacity.
        assert(Animate(t+1000000,1.4,2).top[0].y==ring.top[0].y);
    }
    Mesh mesh;
    auto project=[](Point p,ImVec2& out){
        if(p.z<.05)return false;
        out={float(320+p.x/p.z*200),float(240-p.y/p.z*200)};
        return true;
    };
    projection::Triangle({projection::WorldVertex{{-.1,0,1},{1,1,1,0}},
                          projection::WorldVertex{{.1,0,1},{1,1,1,1}},
                          projection::WorldVertex{{0,.1,-1},{1,1,1,0}}},project,mesh);
    assert(mesh.triangles.size()==2); // A near-plane crossing is clipped, not dropped.
    bool interpolated=false;
    for(const auto& tri:mesh.triangles)for(const auto& v:tri) {
        assert(std::isfinite(v.position.x) && std::isfinite(v.position.y));
        assert(v.color.w>=0 && v.color.w<=1);
        if(v.color.w>0 && v.color.w<1)interpolated=true;
    }
    assert(interpolated);
    mesh={};
    projection::Triangle({projection::WorldVertex{{0,0,-1},{}},
                          projection::WorldVertex{{1,0,-1},{}},
                          projection::WorldVertex{{0,1,-1},{}}},project,mesh);
    assert(mesh.triangles.empty());
    projection::Line({{0,0,1},{1,1,1,1}},{{1,0,-1},{1,1,1,1}},project,mesh);
    assert(mesh.lines.size()==1);

    ImGui::CreateContext();
    auto& io=ImGui::GetIO();io.DisplaySize={640,480};io.DeltaTime=1.f/60;
    io.IniFilename=nullptr;
    io.Fonts->AddFontDefault();
    unsigned char* pixels;int width,height;
    io.Fonts->GetTexDataAsRGBA32(&pixels,&width,&height);
    ImGui::NewFrame();
    auto* draw=ImGui::GetBackgroundDrawList();
    mesh={};
    mesh.triangles.push_back({Vertex{{-100,-100},{1,0,0,0}},
                              Vertex{{740,-100},{1,0,0,0}},
                              Vertex{{320,580},{1,0,0,1}}});
    Draw(mesh,draw,io.DisplaySize);
    assert(draw->VtxBuffer.Size>0 && draw->IdxBuffer.Size>0);
    bool transparent=false,opaque=false;
    for(const auto& vertex:draw->VtxBuffer) {
        assert(vertex.pos.x>=-.001f && vertex.pos.x<=640.001f);
        assert(vertex.pos.y>=-.001f && vertex.pos.y<=480.001f);
        const auto a=vertex.col>>IM_COL32_A_SHIFT;
        if(a<80)transparent=true;
        if(a>150)opaque=true;
    }
    assert(transparent&&opaque); // Vertex alpha survives clipping and mesh submission.
    const auto count=draw->VtxBuffer.Size;
    mesh.triangles.front()[0].position.x=std::numeric_limits<float>::quiet_NaN();
    Draw(mesh,draw,io.DisplaySize);
    assert(draw->VtxBuffer.Size==count);
    ImGui::Render();ImGui::DestroyContext();
    std::cout<<"TargetESP: source animation, ring closure, near/viewport clipping and alpha mesh checks passed\n";
}
