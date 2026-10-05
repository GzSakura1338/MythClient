#pragma once
#include "ui/hud/targetesp/TargetRing.h"
#include <algorithm>

namespace nova::module::targetesp {
using ui::targetesp::Point;
using ui::targetesp::Vertex;
struct WorldVertex { Point position; ImVec4 color; };
inline WorldVertex Mix(const WorldVertex& a,const WorldVertex& b,double t) {
    return {{a.position.x+(b.position.x-a.position.x)*t,
             a.position.y+(b.position.y-a.position.y)*t,
             a.position.z+(b.position.z-a.position.z)*t},
            {a.color.x+(b.color.x-a.color.x)*float(t),a.color.y+(b.color.y-a.color.y)*float(t),
             a.color.z+(b.color.z-a.color.z)*float(t),a.color.w+(b.color.w-a.color.w)*float(t)}};
}
template<class Project>
Vertex Boundary(WorldVertex front,WorldVertex back,Vertex projected,Project&& project) {
    for(int i=0;i<32;++i) {
        auto mid=Mix(front,back,.5); ImVec2 p;
        if(project(mid.position,p)) {front=mid;projected={p,mid.color};}
        else back=mid;
    }
    return projected;
}
template<class Project>
void Triangle(const std::array<WorldVertex,3>& vertices,Project&& project,ui::targetesp::Mesh& mesh) {
    std::array<Vertex,3> projected;
    std::array<bool,3> visible;
    for(int i=0;i<3;++i) {
        visible[i]=project(vertices[i].position,projected[i].position);
        projected[i].color=vertices[i].color;
    }
    std::array<Vertex,6> polygon; int count=0;
    for(int i=0;i<3;++i) {
        const int next=(i+1)%3;
        if(visible[i])polygon[count++]=projected[i];
        if(visible[i]!=visible[next]) {
            const int a=visible[i]?i:next,b=visible[i]?next:i;
            polygon[count++]=Boundary(vertices[a],vertices[b],projected[a],project);
        }
    }
    for(int i=1;i+1<count;++i)mesh.triangles.push_back({polygon[0],polygon[i],polygon[i+1]});
}
template<class Project>
void Line(WorldVertex a,WorldVertex b,Project&& project,ui::targetesp::Mesh& mesh) {
    Vertex start{{},a.color},end{{},b.color};
    const bool av=project(a.position,start.position),bv=project(b.position,end.position);
    if(!av&&!bv)return;
    if(!av)start=Boundary(b,a,end,project);
    if(!bv)end=Boundary(a,b,start,project);
    mesh.lines.push_back({start,end});
}
}
