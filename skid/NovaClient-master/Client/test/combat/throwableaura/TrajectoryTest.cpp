#include "minecraft/modules/combat/throwableaura/Trajectory.h"
#include "minecraft/host/render/WorldGeometry.h"
#include <cassert>
#include <iostream>
#include <limits>

using namespace nova::module::throwableaura;
bool Close(double a,double b){return std::abs(a-b)<1e-8;}
int main() {
    auto air=[](Vec3){return false;};
    auto miss=[](Vec3,Vec3,bool& hit,Vec3&){hit=false;return true;};
    auto path=Predict({0,2,0},0,0,{},miss,air);
    assert(path.points.size()>3 && path.points.size()<=81 && !path.impact);
    assert(Close(path.points[0].x,-.16) && Close(path.points[0].y,1.9));
    assert(Close(path.points[1].z,1.5) && Close(path.points[1].y,1.9));
    assert(Close(path.points[2].z,2.985) && Close(path.points[2].y,1.87));
    auto water=Predict({0,2,0},0,0,{},miss,[](Vec3){return true;});
    assert(Close(water.points[2].z,2.7));
    auto west=Predict({0,2,0},90,0,{},miss,air);
    assert(Close(west.points[1].x,-1.5) && Close(west.points[0].z,-.16));
    auto up=Predict({0,2,0},0,-90,{},miss,air);assert(Close(up.points[1].y,3.4));
    auto wall=[](Vec3 from,Vec3 to,bool& hit,Vec3& location) {
        hit=from.z<=1 && to.z>=1;
        if(hit)location=from.Add((to.x-from.x)/(to.z-from.z),(to.y-from.y)/(to.z-from.z),1-from.z);
        return true;
    };
    auto blocked=Predict({0,2,0},0,0,{{{-.5,1,.8},{.5,3,.9}}},wall,air);
    assert(blocked.impact && blocked.points.size()==2 && Close(blocked.points.back().z,.8));
    auto behind=Predict({0,2,0},0,0,{{{-.5,1,1.2},{.5,3,1.3}}},wall,air);
    assert(behind.impact && Close(behind.points.back().z,1));
    auto fail=Predict({0,2,0},0,0,{},[](Vec3,Vec3,bool&,Vec3&){return false;},air);
    assert(fail.points.size()==1 && !fail.impact);
    assert(Predict({NAN,0,0},0,0,{},miss,air).points.empty());
    double t=0;
    assert(Intersect({{-1,-1,-1},{1,1,1}},{0,0,0},{0,0,4},t) && t==0);
    assert(!Intersect({{-1,-1,2},{1,1,3}},{2,0,0},{2,0,4},t));
    assert(Intersect({{-1,-1,2},{1,1,3}},{0,0,4},{0,0,0},t) && t==.25);

    struct Point{double x=0,y=0;};int edges=0;
    auto project=[](Vec3 p,Point& out){if(p.z<.1)return false;out={p.x/p.z,p.y/p.z};return true;};
    auto line=[&](Point a,Point b){assert(std::isfinite(a.x) && std::isfinite(b.x));++edges;};
    using nova::minecraft::projection::DrawClippedLine;
    DrawClippedLine<Point>({1,0,-1},{1,0,2},project,line);assert(edges==1);
    DrawClippedLine<Point>({1,0,-1},{1,0,-2},project,line);assert(edges==1);
    DrawClippedLine<Point>({NAN,0,1},{1,0,2},project,line);assert(edges==1);
    std::cout<<"PASS: projectile launch, drag/gravity, water, nearest block/entity collision, bounded work, failed reads and near-plane clipping\n";
}
