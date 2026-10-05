#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>

namespace nova::module::crystal {
inline bool Above(bool enabled,double feetY,double crystalY) {
    return std::isfinite(feetY)&&std::isfinite(crystalY)&&(!enabled||feetY>=crystalY);
}
// Vanilla power-6 crystal explosion, followed by player difficulty and defense.
inline double Damage(double distance,double exposure,int difficulty,double armor,
                     double toughness,int resistance,int protection,double power=6) {
    if(!std::isfinite(distance)||!std::isfinite(exposure)||!std::isfinite(power)||power<=0||distance<0||distance>power*2||difficulty==0)return 0;
    const double impact=(1-distance/(power*2))*std::clamp(exposure,0.,1.);
    double damage=(impact*impact+impact)*(power*7)+1;
    if(difficulty==1)damage=std::min(damage,damage*.5+1);
    if(difficulty==3)damage*=1.5;
    const double effective=std::clamp(armor-damage/(2+toughness/4),armor*.2,20.);
    damage*=1-effective/25;
    if(resistance>=0)damage*=std::max(0.,1-.2*(resistance+1));
    return std::max(0.,damage*(1-std::clamp(protection,0,20)/25.));
}
inline bool Safe(double damage,double health,double absorption,double maximum,double remaining) {
    return std::isfinite(damage)&&std::isfinite(health)&&std::isfinite(absorption)&&
        damage<=maximum&&health+absorption-damage>=remaining;
}
// Acknowledgment window is independent of the user's delay, including delay=0.
class Confirmation {
public:
    enum class Kind { None, Place, Break, Base };
    void Start(Kind kind,std::int64_t now,std::int64_t wait=200){kind_=kind;deadline_=now+std::clamp<std::int64_t>(wait,200,500);}
    bool Waiting(std::int64_t now)const{return kind_!=Kind::None&&now<deadline_;}
    Kind Action()const{return kind_;}
    void Reset(){kind_=Kind::None;deadline_=0;}
private:
    Kind kind_=Kind::None;
    std::int64_t deadline_=0;
};
}
