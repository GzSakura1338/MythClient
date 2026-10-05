#pragma once
#include "minecraft/java/api/game_entity.h"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <optional>
#include <unordered_map>

// Raven BS Displace, excluding arrow rendering. IO owns game/JNI access.
namespace nova::module::displace {
using rotation::Vec3;
using java::Box;
inline constexpr double Pi = 3.14159265358979323846;
struct Settings {
    bool dynamic = false, findVoid = false, blink = false, ignoreTeammates = true;
    bool requireKnockback = false, whitelist = false;
    float yawOffset = 90;
    double delay = 0;
    bool left = true;
};
struct Target {
    int id = 0;
    Vec3 position;
    Box box;
    double width = .6, eyeHeight = 1.62;
};
inline float Wrap(float angle) {
    angle = std::fmod(angle,360.f);
    if (angle >= 180) angle -= 360;
    if (angle < -180) angle += 360;
    return angle;
}
inline float FixYaw(float wanted,float previous,float sensitivity) {
    // Java remainder and Math.round (ties toward +infinity), including float
    // intermediates from RotationUtils.fixRotation / RotationHelper.unwrapYaw.
    wanted = previous + (std::fmod(std::fmod(wanted-previous+180.f,360.f)+360.f,360.f)-180.f);
    const float base=sensitivity*.6f+.2f;
    const double gcd=static_cast<double>(base*base*base)*1.2;
    return previous+static_cast<float>(std::floor((wanted-previous)/gcd+.5)*gcd);
}
inline double Direction(float yaw,float forward,float strafe) {
    if(forward<0)yaw+=180;
    const float factor=forward<0?-.5f:forward>0?.5f:1.f;
    if(strafe>0)yaw-=90*factor;
    if(strafe<0)yaw+=90*factor;
    return yaw;
}
inline double WrapDouble(double x) {
    x=std::fmod(x,360.);if(x>=180)x-=360;if(x< -180)x+=360;return x;
}
inline void FixInput(float& forward,float& strafe,float yaw,float playerYaw,bool sneak) {
    if(forward==0&&strafe==0)return;
    const double angle=WrapDouble(Direction(playerYaw,forward,strafe));
    float best=std::numeric_limits<float>::max(),bf=0,bs=0;
    const float multiplier=sneak?.3f:1.f;
    for(float f=-1;f<=1;++f)for(float s=-1;s<=1;++s) {
        if(f==0&&s==0)continue;
        const float pf=f*multiplier,ps=s*multiplier;
        // Deliberately not circular distance: match Raven's boundary behavior.
        const double difference=std::abs(angle-WrapDouble(Direction(yaw,pf,ps)));
        if(difference<best){best=static_cast<float>(difference);bf=pf;bs=ps;}
    }
    forward=bf;strafe=bs;
}
template<class IO> bool VoidColumn(IO& io,double x,double y,double z) {
    const int bx=static_cast<int>(std::floor(x)),bz=static_cast<int>(std::floor(z));
    const int start=static_cast<int>(std::floor(y))-1,end=std::max(0,start-10);
    for(int by=start;by>=end;--by)if(!io.Air(bx,by,bz))return false;
    return true;
}
inline Box Offset(Box b,double x,double z) {
    b.minX+=x;b.maxX+=x;b.minZ+=z;b.maxZ+=z;return b;
}
struct State {
    bool displacing=false,active=false,hasKB=false,compensate=false,left=false,wasDisplacing=false;
    bool releaseBlinkNextTick=false;
    std::optional<float> yaw;
    std::uint64_t tick=0;
    std::unordered_map<int,std::uint64_t> windows;
    void ClearActive() { active=displacing=compensate=wasDisplacing=false;yaw.reset(); }
    bool Window(int id,double delay) {
        auto it=windows.find(id);
        if(it==windows.end()||tick-it->second>=10){windows[id]=tick;return true;}
        return delay<=0||tick-it->second>=static_cast<std::uint64_t>(std::ceil(delay/50.));
    }
    void Side(const Target& t,const Vec3& self,double x,double z) {
        left=(t.position.x-self.x)*(z-self.z)-(t.position.z-self.z)*(x-self.x)<0;
    }
    template<class IO> std::optional<float> StaticVoid(IO& io,const Target& t) {
        double best=std::numeric_limits<double>::max(),bx=0,bz=0;
        const auto self=io.SelfPosition();
        for(int ring=1;ring<=12;++ring) {
            const double radius=ring*.5;bool found=false;
            for(int i=0;i<32;++i) {
                const double angle=Pi*2*i/32.,x=t.position.x+std::cos(angle)*radius,z=t.position.z+std::sin(angle)*radius;
                if(!VoidColumn(io,x,t.position.y,z))continue;
                const double dx=x-self.x,dz=z-self.z,score=radius*radius*1000+dx*dx+dz*dz;
                if(score<best){best=score;bx=x;bz=z;found=true;}
            }
            if(found)break;
        }
        if(best==std::numeric_limits<double>::max())return {};
        Side(t,self,bx,bz);
        const double dx=bx-t.position.x,dz=bz-t.position.z,dist=std::sqrt(dx*dx+dz*dz);
        if(dist<.001)return {};
        const double radius=std::min(dist,std::max(.35,t.width*.5+.15));
        const auto eye=io.Eye();
        return static_cast<float>(std::atan2(t.position.z+dz/dist*radius-eye.z,t.position.x+dx/dist*radius-eye.x)*180/Pi)-90.f;
    }
    template<class IO> double Score(IO& io,const Target& t,double fx,double fz) {
        double score=0,checked=0;int consecutive=0;
        Box box=t.box;box.minX+=.03;box.minZ+=.03;box.maxX-=.03;box.maxZ-=.03;
        for(int step=1;step<=12;++step) {
            const double forward=step*.5;bool clear=true;
            for(double f=checked+.25;f<=forward+1e-4;f+=.25)
                if(io.Collision(t.id,Offset(box,fx*f,fz*f))){clear=false;break;}
            if(!clear)break;
            checked=forward;bool center=false;
            for(int side=-1;side<=1;++side) {
                const double offset=side*.45;
                if(VoidColumn(io,t.position.x+fx*forward-fz*offset,t.position.y,t.position.z+fz*forward+fx*offset)) {
                    score+=(side==0?1.4:1.)*(6.5-forward);center|=side==0;
                }
            }
            if(center){++consecutive;score+=consecutive*2.;}else consecutive=0;
        }
        return score;
    }
    template<class IO> std::optional<float> DynamicVoid(IO& io,const Target& t) {
        double best=0,bx=0,bz=0;
        for(int i=0;i<32;++i) {
            const double angle=Pi*2*i/32.,fx=std::cos(angle),fz=std::sin(angle),score=Score(io,t,fx,fz);
            if(score>best){best=score;bx=fx;bz=fz;}
        }
        if(best<=0)return {};
        Side(t,io.SelfPosition(),t.position.x+bx,t.position.z+bz);
        return static_cast<float>(std::atan2(bz,bx)*180/Pi-90);
    }
    template<class IO> void Rotation(IO& io,const Settings& s) {
        if(!io.Ready()){ClearActive();return;}
        ++tick;
        std::erase_if(windows,[&](const auto& entry){return !io.Trackable(entry.first);});
        const bool kb=io.Knockback();
        // Preserve the source's OR, even when one condition is disabled.
        if((s.requireKnockback||s.whitelist)&&!((!s.requireKnockback||kb)||(!s.whitelist||io.Whitelist()))) {
            ClearActive();return;
        }
        auto target=io.Attacking()?io.Closest(s.ignoreTeammates):std::optional<Target>{};
        active=target.has_value()&&(kb||io.Movement());
        if(!active){ClearActive();return;}
        yaw=s.dynamic?DynamicVoid(io,*target):s.findVoid?StaticVoid(io,*target):std::optional<float>{};
        if(!yaw&&!s.dynamic){left=s.left;yaw=io.ServerYaw()+(left?-s.yawOffset:s.yawOffset);}
        if(!yaw){ClearActive();return;}
        hasKB=kb;displacing=!displacing;
        if(displacing&&!Window(target->id,s.delay)) {
            displacing=compensate=wasDisplacing=false;yaw.reset();return;
        }
        if(!displacing&&wasDisplacing)io.AttackKeyTick();
        wasDisplacing=displacing;
    }
    void Input(float& forward,float& strafe,bool movement) {
        if(!active){compensate=false;return;}
        if(compensate&&!displacing){compensate=false;strafe=left?-1.f:1.f;return;}
        if(!displacing||hasKB||!movement)return;
        forward=1;compensate=true;
    }
};
}
