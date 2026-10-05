#include "minecraft/modules/combat/antikb/prediction/PredictionVelocity.h"
#include "minecraft/modules/combat/autoblock/HypixelAutoBlock.h"
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

using namespace nova::module;
void Check(bool v, const char* text) { if (!v) { std::cerr << text << '\n'; std::exit(1); } }
struct IO {
    bool ground = false, liquid = false, sprint = true, aura = true, enabled = true;
    bool blocking = true, canReduce = true, ray = true, boost = false, forward = true;
    double y = -.1;
    int hurt = 1, attacks = 0, released = 0, starts = 0, stops = 0, jumps = 0, rotations = 0, computed = 0;
    std::vector<std::string> trace;
    bool OnGround() const { return ground; }
    bool LiquidWeb() const { return liquid; }
    bool Sprinting() const { return sprint; }
    bool AuraTarget() const { return aura; }
    bool AuraEnabled() const { return enabled; }
    bool AuraBlocking() const { return enabled && blocking; }
    bool CanReduce(int) const { return canReduce; }
    bool ShouldBlock() const { return blocking; }
    bool RayPlayer() const { return ray; }
    bool JumpBoost() const { return boost; }
    bool ForwardPressed() const { return forward; }
    double MotionY() const { return y; }
    int HurtTime() const { return hurt; }
    void ComputeRotation(double, double) { ++computed; }
    void Rotate() { ++rotations; }
    void Attack(bool fromAura, bool keep) {
        ++attacks; trace.push_back(fromAura ? "aura" : "ray");
        if (!keep) sprint = false;
    }
    void ReleaseDelay() { ++released; trace.push_back("release"); }
    bool StartBlink() { ++starts; trace.push_back("blink"); return true; }
    void ReleaseBlink() { ++stops; trace.push_back("flush"); }
    void Jump() { ++jumps; }
};
int main() {
    PredictionSettings s;
    {
        PredictionVelocity v;
        Check(!v.ShouldDelayVelocity(s, false, false) && v.received, "fake check needs an own hurt status");
        v.Hurt();
        Check(!v.ShouldDelayVelocity(s, false, false), "Delay off disables hidden air buffer");
        s.delay = true; Check(v.ShouldDelayVelocity(s, false, false), "enabled air buffer holds airborne velocity");
        s.delay = false;
        Check(!v.ShouldDelayVelocity(s, true, false), "ground velocity passes with default settings");
        Check(!v.ShouldDelayVelocity(s, false, true), "liquid/web rejects a new delay");
        v.Applied(s, .4, .42, -.3, false);
        Check(v.hitCount == 3 && v.allowNext && v.ticksSinceVelocity == 0, "smart times uses actual velocity D2I");
        v.Applied(s, 9000, 1, 9000, false);
        Check(v.hitCount == 3, "fake check consumes hurt once");
    }
    {
        s.fakeCheck = false; s.smartTimes = false; s.attackTimes = 1; s.keepSprint = true;
        PredictionVelocity v; IO io;
        v.ShouldDelayVelocity(s, true, false);
        v.Pre(s, io); v.Pre(s, io); v.Pre(s, io); v.Pre(s, io);
        Check(io.attacks == 3 && !v.received && v.reduceTick == 1,
              "reference -1 counter and terminal iteration give three initial AttackTimes=1 attempts");
        v.received = true; io.ray = false;
        v.Pre(s, io);
        Check(!v.received && io.attacks == 3, "a miss consumes the final attempt");
        v.received = true; v.reduceTick = 0; io.ray = true; io.canReduce = false;
        v.Pre(s, io); Check(io.attacks == 3, "aura phase gates reduction");
        v.received = true; io.aura = false; s.cancelAuraAttack = true;
        v.Pre(s, io);
        Check(io.attacks == 4 && v.cancelAura && io.trace.back() == "ray", "no aura uses ray target without phase gate");
        v.Pre(s, io); Check(!v.cancelAura, "cancel aura resets each PRE");
    }
    {
        s = {}; s.fakeCheck = false; s.delay = true; s.airBuffer = false; s.delayTicks = 2;
        PredictionVelocity v; IO io;
        v.BeginDelay(); v.Post(s, io); Check(io.released == 0, "no early timed release");
        v.delayedTicks = 2; s.forceFalling = true; io.y = .2;
        v.Post(s, io); Check(io.released == 0, "force falling holds rising motion");
        io.y = 0; v.Post(s, io);
        Check(io.released == 1 && !v.delayFlag && v.jumpFlag && v.received, "timed delay releases at falling boundary");
        v.BeginDelay(); io.liquid = true; v.Post(s, io);
        Check(io.released == 2, "liquid ends an active timed buffer");
        s.airBuffer = true; io.liquid = false; io.ground = false; v.BeginDelay(); v.delayedTicks = 100;
        v.Post(s, io); Check(io.released == 2, "airbuffer ignores timed duration");
        io.ground = true; v.Post(s, io); Check(io.released == 3, "airbuffer releases on landing");
    }
    for (int mode : {1, 2}) {
        s = {}; s.reduceMode = mode; s.extraAttack = true;
        PredictionVelocity v; IO io;
        v.Post(s, io);
        Check(io.released == 1 && v.extraAttacked && v.velocityAttacked && v.jumpFlag,
              "phase release is intentionally not gated by delayFlag");
        v.Pre(s, io); Check(io.attacks == 0 && v.velocityAttacked,
              "ExtraAttack remains deferred outside Attack reduce mode, as in Leader");
        io.canReduce = false; v.Post(s, io); Check(io.released == 1, "release respects aura phase");
    }
    {
        s = {}; s.reduceMode = 3; s.startReleaseTicks = 1;
        PredictionVelocity v; IO io;
        v.PassedVelocity(); v.Pre(s, io);
        Check(v.blinking && io.starts == 1 && v.knockbackTimer == 1, "blink start uses hurt threshold");
        v.Pre(s, io); Check(!v.blinking && io.stops == 1 && v.knockbackTimer == -1, "release measured from knockback");
        v.PassedVelocity(); io.hurt = 10; v.Pre(s, io); Check(io.starts == 1, "wait for hurt time");
        io.hurt = 1; io.blocking = false; v.Pre(s, io); Check(v.scheduled, "ForceBlocking waits for aura blocking");
        io.blocking = true; v.Pre(s, io); Check(io.starts == 2, "eligible hurt branch precedes scheduled timeout");
        v.Pre(s, io); Check(io.stops == 2, "late blink releases on next PRE");
    }
    {
        s = {}; s.rotate = true; s.fakeCheck = false;
        PredictionVelocity v; IO io;
        v.Applied(s, .4, .42, .1, false);
        for (int i = 0; i < 5; ++i) v.Pre(s, io);
        Check(io.computed == 1 && io.rotations == 3 && v.rotateCounter == 0, "rotation length and cleanup");
        v.jumpFlag = true; io.ground = true; v.Living(io);
        Check(io.jumps == 1 && !v.jumpFlag, "living jump happens once");
        v.jumpFlag = true; io.boost = true; v.Living(io);
        Check(io.jumps == 1 && !v.jumpFlag, "jump boost consumes and suppresses pending jump");
        s.testMode = true; v.ticksSinceVelocity = 2; v.ForgeTick(s);
        Check(v.received && v.stoppedBlock && v.ticksSinceVelocity == 3, "retained Forge event body");
    }
    // Compare phase tables against manually transcribed Leader values.
    HypixelAutoBlock block; HypixelBlockSettings hs;
    for (int sub = 0; sub < 5; ++sub) {
        hs.mode = 3; hs.lagMode = sub;
        const int last = sub == 0 ? 1 : sub == 1 ? 2 : sub == 2 ? 3 : 4;
        Check(block.VelocityCanReduce(hs, 1, 0), "release-when phase");
        Check(block.VelocityCanReduce(hs, 2, last), "release-before phase");
    }
    std::cout << "PASS: Leader Prediction four reduce branches, counters, fake check, delay precedence, blink, jump and rotation\n";
}
