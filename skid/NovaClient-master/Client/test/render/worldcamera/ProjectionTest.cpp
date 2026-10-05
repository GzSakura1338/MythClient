#include "minecraft/host/render/WorldProjection.h"
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>

using namespace nova;
using namespace nova::minecraft::projection;
void Check(bool ok, const char* message) {
    if (!ok) { std::cerr << message << '\n'; std::exit(1); }
}
int main() {
    constexpr double pi=3.14159265358979323846;
    java::CameraView view;
    view.hasModelView=true;view.windowWidth=2560;view.windowHeight=1440;
    // The captured matrices must win even if angle/bob fields describe a later
    // hand pass. Exercise a fixed world point while moving and turning the camera.
    view.yaw=123;view.pitch=-62;view.bobView=true;view.bob=.4f;view.walkDist=12;
    const rotation::Vec3 chest{2000002.5,66.5,2000012.5};
    for(int frame=0;frame<180;++frame) {
        const double t=frame/179.0,yaw=(-30+60*t)*pi/180,pitch=(-12+24*t)*pi/180;
        view.x=2000000+2*t;view.y=65.62;view.z=2000000+3*t;
        const rotation::Vec3 right{-std::cos(yaw),0,-std::sin(yaw)};
        const rotation::Vec3 up{-std::sin(yaw)*std::sin(pitch),std::cos(pitch),std::cos(yaw)*std::sin(pitch)};
        const rotation::Vec3 forward{-std::sin(yaw)*std::cos(pitch),-std::sin(pitch),std::cos(yaw)*std::cos(pitch)};
        view.modelView[0]=float(right.x);view.modelView[4]=float(right.y);view.modelView[8]=float(right.z);
        view.modelView[1]=float(up.x);view.modelView[5]=float(up.y);view.modelView[9]=float(up.z);
        view.modelView[2]=float(-forward.x);view.modelView[6]=float(-forward.y);view.modelView[10]=float(-forward.z);
        view.modelView[15]=1;
        const double roll=.05*std::sin(t*20),tx=.03*std::sin(t*30),ty=-.05*std::abs(std::cos(t*30));
        const double f=1/std::tan((70+35*t)*pi/360),fx=f*1440/2560;
        // P * (translation * roll), as supplied by the modern world renderer.
        view.projection[0]=float(fx*std::cos(roll));view.projection[4]=float(-fx*std::sin(roll));
        view.projection[1]=float(f*std::sin(roll));view.projection[5]=float(f*std::cos(roll));
        view.projection[10]=-1;view.projection[11]=-1;
        view.projection[12]=float(fx*tx);view.projection[13]=float(f*ty);view.projection[14]=-.1f;
        java::PublishWorldCamera(view);
        java::CameraView captured;
        Check(java::ReadWorldCamera(captured),"world pass snapshot missing");
        ScreenPoint screen;
        Check(Project(MakeBasis(captured,float(t)),chest,{2560,1440},screen),"visible chest rejected");
        const rotation::Vec3 rel{chest.x-view.x,chest.y-view.y,chest.z-view.z};
        const double x=right.x*rel.x+right.y*rel.y+right.z*rel.z;
        const double y=up.x*rel.x+up.y*rel.y+up.z*rel.z;
        const double depth=forward.x*rel.x+forward.y*rel.y+forward.z*rel.z;
        const double expectedX=(1+fx*(x*std::cos(roll)-y*std::sin(roll)+tx)/depth)*1280;
        const double expectedY=(1-f*(x*std::sin(roll)+y*std::cos(roll)+ty)/depth)*720;
        Check(std::abs(screen.x-expectedX)<.002 && std::abs(screen.y-expectedY)<.002,"world projection drift/double bob");
        const rotation::Vec3 behind{view.x-forward.x,view.y-forward.y,view.z-forward.z};
        Check(!Project(MakeBasis(captured,0),behind,{2560,1440},screen),"behind-camera point accepted");
    }
    java::CameraView captured;
    java::PublishWorldCamera(view);
    const float saved=view.projection[0];view.projection[0]=99;
    Check(java::ReadWorldCamera(captured)&&captured.projection[0]==saved,"matrix snapshot aliases source");
    java::InvalidateWorldCamera();
    Check(!java::ReadWorldCamera(captured),"menu frame reused world camera");
    java::PublishWorldCamera(view);
    view.modelView[3]=std::numeric_limits<float>::quiet_NaN();
    java::PublishWorldCamera(view);
    Check(!java::ReadWorldCamera(captured),"failed pass reused preceding camera");
    std::cout<<"World camera: 180 moving/turning/FOV/bob frames and snapshot lifecycle passed\n";
}
