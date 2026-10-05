#pragma once
#include "minecraft/modules/player/breaker/BreakerLogic.h"
#include "minecraft/java/api/base.h"
#include <memory>
namespace nova::java {
// One client-thread operation owns its JNI frame and a per-tick block cache.
class BreakerWorld {
  public:
    BreakerWorld();
    ~BreakerWorld();
    bool Ready() const;
    module::breaker::Block Read(BlockPos pos);
    bool Ray(rotation::Vec3 from, rotation::Vec3 target, double range, BlockHitResultInfo &out);
    float Progress();
    bool SetProgress(float value);
    bool Hit(BlockPos pos, rotation::Vec3 from, rotation::Vec3 to, int &face);
    bool Damage(BlockPos pos, int face, bool slowAir);
    bool Instant(BlockPos pos, bool slowAir);
    void Stop();

  private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
bool BreakerDigPacket(jobject packet);
bool BreakerBlockUpdate(jobject packet, BlockPos &pos, bool &air);
bool BreakerTeleport(jobject packet, rotation::Vec3 &out);
bool BreakerBlinkPacket(jobject packet, bool incoming);
void JNICALL BreakerContinueHook(JNIEnv *, jclass, jobject, jboolean);
} // namespace nova::java
