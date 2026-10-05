#pragma once

#include <algorithm>
#include <cmath>

namespace nova::module {

struct PredictionSettings {
    bool reduce = true;
    int reduceMode = 0; // Attack, ReleaseWhenCanAttack, ReleaseBeforeCanAttack, Blink
    int startBlinkHurtTime = 1, startReleaseTicks = 1;
    bool forceBlocking = true, extraAttack = false;
    bool reduceWhenCanAttack = true, cancelAuraAttack = false, onlySprinting = true;
    bool smartTimes = true;
    int attackTimes = 1;
    bool keepSprint = false, testMode = false;
    int stopBlockHurtTime = 2;
    bool jump = true, delay = false;
    int delayTicks = 1;
    bool forceFalling = false, airBuffer = true, groundDelay = false;
    bool rotate = false;
    int rotateTicks = 3;
    bool autoMove = false, fakeCheck = true, debug = false;
};

// Leader Lite B0.4 Velocity.Prediction. JNI and queue ownership live in the
// adapter; the branch order and counters here deliberately match the reference.
class PredictionVelocity {
public:
    bool allowNext = true, received = false, delayFlag = false, jumpFlag = false;
    bool knockback = false, blinking = false, scheduled = false;
    bool extraAttacked = false, velocityAttacked = false, stoppedBlock = false, cancelAura = false;
    int ticksSinceVelocity = -1, rotateCounter = 0, reduceTick = -1, hitCount = 0;
    int knockbackTimer = -1, delayedTicks = 0;
    double knockbackX = 0, knockbackZ = 0;

    void Hurt() { allowNext = false; }
    bool ShouldDelayVelocity(const PredictionSettings& s, bool ground, bool liquidWeb) {
        if (!s.testMode && (!s.delay || (!s.groundDelay && ground))) received = true;
        // Hidden delay options must not hold packets when the master is off.
        return s.delay && !delayFlag && !liquidWeb && !(allowNext && s.fakeCheck) &&
            ((s.airBuffer && !ground) || (s.delay && !ground) ||
             (s.delay && s.groundDelay && !s.airBuffer));
    }
    void BeginDelay() { delayFlag = true; delayedTicks = 0; }
    void PassedVelocity() { knockback = true; knockbackTimer = 0; }
    void Applied(const PredictionSettings& s, double x, double y, double z, bool ground) {
        if (allowNext && s.fakeCheck) return;
        allowNext = true;
        if (s.rotate && y > 0) {
            knockbackX = x; knockbackZ = z;
            if (std::abs(x) > .01 || std::abs(z) > .01) rotateCounter = 1;
        }
        if (s.smartTimes) {
            // Confirmed D2I in Leader's bytecode, NOT S12's raw integer units.
            const auto javaInt = [](double v) {
                if (std::isnan(v)) return 0;
                return static_cast<int>(std::clamp(v, -2147483648.0, 2147483647.0));
            };
            hitCount = std::clamp(static_cast<int>(std::floor(.000643153527 *
                std::hypot(double(javaInt(x)), double(javaInt(z))) + 2.9419087136 + .5)), 1, 10);
        }
        if (s.delay && !s.groundDelay && ground) {
            if (s.jump) jumpFlag = true;
            ticksSinceVelocity = 0;
        }
        if (!s.delay) ticksSinceVelocity = 0;
    }

    // The JAR subscribes this body to Forge TickEvent, while its dispatcher
    // emits leader.events.TickEvent (exact class lookup). Retained for audit;
    // the adapter deliberately does not invent a callback absent in Leader.
    void ForgeTick(const PredictionSettings& s) {
        if (s.testMode && s.reduce && s.reduceMode == 0 && ticksSinceVelocity >= s.stopBlockHurtTime)
            received = stoppedBlock = true;
        if (ticksSinceVelocity >= 0 && ++ticksSinceVelocity >= 10) ticksSinceVelocity = -1;
    }

    template<class IO> void Pre(const PredictionSettings& s, IO& io) {
        cancelAura = false;
        if (rotateCounter > 0 && rotateCounter <= s.rotateTicks) {
            if (rotateCounter == 1) io.ComputeRotation(-knockbackX, -knockbackZ);
            io.Rotate();
            if (++rotateCounter > s.rotateTicks) {
                rotateCounter = 0; knockbackX = knockbackZ = 0;
            }
        }
        if (s.reduce && s.reduceMode == 3) {
            if (knockbackTimer >= 0 && knockbackTimer < 2147483647) ++knockbackTimer;
            if (blinking) {
                if (knockbackTimer >= s.startReleaseTicks) {
                    io.ReleaseBlink();
                    blinking = scheduled = knockback = false; knockbackTimer = -1;
                }
            } else if (knockback && io.HurtTime() <= s.startBlinkHurtTime) {
                if (s.forceBlocking && !io.AuraBlocking()) scheduled = true;
                else if (io.StartBlink()) { blinking = true; scheduled = false; }
            } else if (scheduled) {
                if (knockbackTimer >= s.startReleaseTicks) {
                    scheduled = knockback = false; knockbackTimer = -1;
                } else if (io.AuraBlocking() && io.StartBlink()) {
                    blinking = true; scheduled = false;
                }
            }
        }
        if (s.reduce && s.reduceMode == 0) {
            if (velocityAttacked) {
                if (io.AuraTarget() && io.AuraEnabled() && io.Sprinting()) io.Attack(true, false);
                else extraAttacked = false;
                velocityAttacked = false;
            }
            if (received) {
                if (reduceTick >= (s.smartTimes ? hitCount : s.attackTimes)) {
                    reduceTick = 0; received = stoppedBlock = false;
                }
                if (io.RayPlayer() && (io.Sprinting() || !s.onlySprinting)) {
                    const bool aura = io.AuraTarget();
                    if (!aura || !s.reduceWhenCanAttack || io.CanReduce(0)) {
                        if (s.cancelAuraAttack) cancelAura = true;
                        io.Attack(aura, s.keepSprint);
                    }
                }
                ++reduceTick; // A miss counts; the final iteration still attacks.
            }
        }
    }

    template<class IO> void Post(const PredictionSettings& s, IO& io) {
        const bool release = (delayFlag && (!s.forceFalling || io.MotionY() <= 0) &&
            ((s.delay && (io.LiquidWeb() || (delayedTicks >= s.delayTicks && !s.airBuffer))) ||
             (io.OnGround() && !s.groundDelay && !s.airBuffer) || (s.airBuffer && io.OnGround() && delayFlag))) ||
            (s.reduce && (s.reduceMode == 1 || s.reduceMode == 2) &&
             io.CanReduce(s.reduceMode) && io.ShouldBlock());
        if (!release) return;
        ticksSinceVelocity = 0;
        if (io.AuraTarget() && s.extraAttack && s.reduce && s.reduceMode != 0 && !extraAttacked)
            extraAttacked = velocityAttacked = true;
        if (!s.testMode) received = true;
        io.ReleaseDelay();
        delayFlag = false;
        if (s.jump) jumpFlag = true;
    }

    template<class IO> void Living(IO& io) {
        if (!jumpFlag) return;
        if (io.OnGround() && io.ForwardPressed() && !io.JumpBoost() && !io.LiquidWeb() && io.Sprinting())
            io.Jump();
        jumpFlag = false;
    }
};
}
