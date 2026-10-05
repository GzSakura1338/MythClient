#include <cassert>
#include <iostream>
#include <limits>
#include "minecraft/modules/combat/backtrack/EspClip.h"

struct Point { double x = 0, y = 0; };
using nova::rotation::Vec3;
std::array<Vec3,8> Box(double nearZ,double farZ) {
    return {{{-0.3,0,nearZ},{0.3,0,nearZ},{0.3,0,farZ},{-0.3,0,farZ},
             {-0.3,1.8,nearZ},{0.3,1.8,nearZ},{0.3,1.8,farZ},{-0.3,1.8,farZ}}};
}
int main() {
    int faces=0,edges=0;
    auto project=[](Vec3 v,Point& p) {
        if(v.z<0.05)return false;
        p={v.x/v.z,v.y/v.z};return true;
    };
    auto face=[&](auto& polygon) {
        ++faces;assert(polygon.size()>=3&&polygon.size()<=5);
        for(const auto& p:polygon)assert(std::isfinite(p.x)&&std::isfinite(p.y));
    };
    auto edge=[&](Point a,Point b) {
        ++edges;assert(std::isfinite(a.x)&&std::isfinite(b.y));
    };
    nova::module::backtrack::DrawClippedBox<Point>(Box(2,3),project,face,edge);
    assert(faces==6&&edges==12);
    faces=edges=0;
    // The old all-eight-points gate discards this partly visible box.
    auto crossing=Box(-0.2,0.4);
    Point p;assert(!project(crossing[0],p)&&project(crossing[2],p));
    nova::module::backtrack::DrawClippedBox<Point>(crossing,project,face,edge);
    assert(faces==5&&edges==8);
    faces=edges=0;
    nova::module::backtrack::DrawClippedBox<Point>(Box(-2,-1),project,face,edge);
    assert(faces==0&&edges==0);
    // Camera rotation/translation: visibility must come from the projector,
    // not an assumed world Z plane.
    auto rotated=[](Vec3 v,Point& p) {
        double depth=v.x+0.15;
        if(depth<0.05)return false;
        p={v.z/depth,v.y/depth};return true;
    };
    nova::module::backtrack::DrawClippedBox<Point>(Box(2,3),rotated,face,edge);
    assert(faces==5&&edges==8);
    auto boundary=nova::module::backtrack::ClipBoundary(Vec3{0.3,1,1},Vec3{0.3,1,-1},Point{0.3,1},project);
    assert(std::fabs(boundary.x-6)<0.00001&&std::fabs(boundary.y-20)<0.00001);
    faces=edges=0;
    auto invalid=Box(2,3);invalid[0].x=std::numeric_limits<double>::quiet_NaN();
    nova::module::backtrack::DrawClippedBox<Point>(invalid,project,face,edge);
    assert(faces==0&&edges==0);
    std::cout<<"PASS: visible, crossing, behind-camera, transformed camera, boundary precision, invalid box\n";
}
