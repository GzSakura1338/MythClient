#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <random>

namespace java {
struct EntityHandle { int id=1; bool IsSame(const EntityHandle& other) const { return id==other.id; } };
}
namespace rotation {
struct Rotation { float yaw=0,pitch=0; };
enum class HitResultType { Entity, Block, Miss };
struct Hit { HitResultType type=HitResultType::Entity; java::EntityHandle entity; } hit;
std::mt19937 engine{123456};
int samples=0,casts=0;
double Gaussian(double mean,double deviation) { ++samples;return std::normal_distribution<double>(mean,deviation)(engine); }
float ClampPitch(float pitch) { return std::clamp(pitch,-90.f,90.f); }
Hit RayCast(const Rotation&,double) { ++casts;return hit; }
}
struct Number { double value=0; double get() const {return value;} } noise;
struct Aura {
    Number* rotationNoise_=&noise;
    rotation::Rotation noiseOffset_;
    int noiseTargetId_=0;
    bool noiseTracking_=false;
    // @PRODUCTION_NOISY_AIM@
};
int main() {
    Aura aura;const java::EntityHandle target{1};
    const rotation::Rotation base{1081.f,0.f};
    auto result=aura.NoisyAim(base,target,1,4,false);
    assert(result.yaw==base.yaw&&result.pitch==base.pitch&&rotation::samples==0&&rotation::casts==0);
    noise.value=1;
    double sum=0,squares=0,cross=0,previous=0;
    constexpr int count=100000;
    for(int i=0;i<count+100;++i) {
        result=aura.NoisyAim(base,target,1,4,i!=0);
        const double delta=result.yaw-base.yaw;
        assert(std::abs(delta)<=3.001&&result.yaw>1000); // No +/-180 wrapping.
        if(i>=100){sum+=delta;squares+=delta*delta;cross+=previous*delta;}
        previous=delta;
    }
    assert(std::abs(sum/count)<.04);
    assert(squares/count>.92&&squares/count<1.06);
    assert(cross/squares>.77&&cross/squares<.82); // Smooth, correlated normal offsets.
    rotation::hit.type=rotation::HitResultType::Block;
    result=aura.NoisyAim(base,target,1,4,true);
    assert(result.yaw==base.yaw&&result.pitch==base.pitch);
    rotation::hit.type=rotation::HitResultType::Entity;rotation::hit.entity.id=2;
    result=aura.NoisyAim(base,target,1,4,true);
    assert(result.yaw==base.yaw&&result.pitch==base.pitch);
    rotation::hit.entity.id=1;
    // Changing target or resuming after a gap must produce the same fresh sequence.
    Aura fresh;aura.noiseOffset_={3,3};aura.noiseTargetId_=2;
    rotation::engine.seed(42);result=aura.NoisyAim(base,target,1,4,true);
    rotation::engine.seed(42);auto expected=fresh.NoisyAim(base,target,1,4,false);
    assert(result.yaw==expected.yaw&&result.pitch==expected.pitch);
    aura.noiseOffset_={-3,-3};rotation::engine.seed(42);
    result=aura.NoisyAim(base,target,1,4,false);
    assert(result.yaw==expected.yaw&&result.pitch==expected.pitch);
    noise.value=5;
    for(int i=0;i<1000;++i){result=aura.NoisyAim({1081,89.9f},target,1,4,true);assert(result.pitch>=-90&&result.pitch<=90);}
    std::puts("PASS: normal noise statistics, correlation, zero passthrough, target/wall safety, reset, continuous yaw and pitch limits");
}
