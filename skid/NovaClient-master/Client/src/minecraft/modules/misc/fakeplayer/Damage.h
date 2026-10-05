#pragma once
#include <algorithm>
#include <cmath>
namespace nova::module::fakeplayer {
inline double Melee(double attack,double strength) {
    if(!std::isfinite(attack)||!std::isfinite(strength))return 0;
    strength=std::clamp(strength,0.,1.);return std::max(0.,attack)*(.2+.8*strength*strength);
}
inline double Explosion(double distance,double exposure,double radius,int difficulty) {
    if(!std::isfinite(distance)||!std::isfinite(exposure)||!std::isfinite(radius)||radius<=0||
       distance<0||distance>radius*2||difficulty==0)return 0;
    const double impact=(1-distance/(radius*2))*std::clamp(exposure,0.,1.);
    double raw=(impact*impact+impact)*7*radius+1;
    if(difficulty==1)raw=std::min(raw,raw*.5+1);
    if(difficulty==3)raw*=1.5;return raw;
}
inline double Mitigate(double raw,double armor,double toughness,int resistance,int protection) {
    if(!std::isfinite(raw)||!std::isfinite(armor)||!std::isfinite(toughness)||raw<=0)return 0;
    armor=std::clamp(armor,0.,30.);toughness=std::max(0.,toughness);
    raw*=1-std::clamp(armor-raw/(2+toughness/4),armor*.2,20.)/25;
    if(resistance>=0)raw*=std::max(0.,1-.2*(resistance+1));
    return std::max(0.,raw*(1-std::clamp(protection,0,20)/25.));
}
struct HitState {
    float health=20,absorption=0;
    int immunity=0;
    double lastDamage=0;
    void Tick(){if(immunity>0)--immunity;}
    bool Hit(double damage,bool reset,float initial) {
        if(!std::isfinite(damage)||damage<=0)return false;
        if(immunity>10) {
            if(damage<=lastDamage)return false;
            const double previous=lastDamage;lastDamage=damage;damage-=previous;
        } else {lastDamage=damage;immunity=20;}
        const double absorbed=std::min(double(absorption),damage);
        absorption-=static_cast<float>(absorbed);health=std::max(0.f,health-static_cast<float>(damage-absorbed));
        if(health<=0&&reset){health=std::clamp(initial,1.f,20.f);absorption=0;}
        return true;
    }
};
}
