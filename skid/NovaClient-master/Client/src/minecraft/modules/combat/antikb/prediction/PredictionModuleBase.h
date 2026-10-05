#pragma once
#include <format>
#include "minecraft/modules/Module.h"
#include "minecraft/modules/combat/antikb/prediction/PredictionVelocity.h"

namespace nova::module {
class PredictionModuleBase : public Module {
protected:
    PredictionModuleBase() : Module("AntiKB", Category::Combat, "General", "General") {}
    std::vector<Properties::PropDesc> BuildPredictionSettings(Properties::PropDesc::Visibility visible) {
        std::vector<Properties::PropDesc> rows;
        auto b = [&](const char* name, bool value, Properties::BoolProperty*& p, auto show) {
            auto row = subBool(name, value); p = static_cast<Properties::BoolProperty*>(row.getProp());
            rows.emplace_back(row.getName(), row.getProp(), std::vector<Properties::PropDesc>{}, show);
        };
        auto n = [&](const char* name, int min, int max, int value, Properties::NumberProp*& p, auto show) {
            auto row = subNumber(name, min, max, 1, value, [](double v) { return std::format("{:.0f}", v); });
            p = static_cast<Properties::NumberProp*>(row.getProp());
            rows.emplace_back(row.getName(), row.getProp(), std::vector<Properties::PropDesc>{}, show);
        };
        b("Reduce", true, reduce_, visible);
        auto reducing = [this, visible] { return visible() && reduce_->get(); };
        auto mode = subMode("ReduceMode", 0, {"Attack", "ReleaseWhenCanAttack", "ReleaseBeforeCanAttack", "Blink"});
        reduceMode_ = static_cast<Properties::ModeProperty*>(mode.getProp());
        rows.emplace_back(mode.getName(), mode.getProp(), std::vector<Properties::PropDesc>{}, reducing);
        auto attack = [this, reducing] { return reducing() && reduceMode_->get() == 0; };
        auto blink = [this, reducing] { return reducing() && reduceMode_->get() == 3; };
        n("StartBlinkHurtTime", 0, 10, 1, startHurt_, blink);
        n("StartReleaseTicks", 0, 5, 1, startRelease_, blink);
        b("ForceBlocking", true, forceBlocking_, blink);
        b("ExtraAttack", false, extra_, [this, reducing] { return reducing() && reduceMode_->get() != 0; });
        b("Reduce When Can Attack", true, whenAttack_, attack);
        b("CancelKillAuraAttack", false, cancel_, attack);
        b("Only Sprinting", true, sprinting_, attack);
        b("SmartTimes", true, smart_, attack);
        n("Attack Times", 1, 5, 1, times_, [this, attack] { return attack() && !smart_->get(); });
        b("KeepSprint", false, keep_, attack);
        b("TestMode", false, test_, attack);
        n("StopBlockHurtTime", 0, 10, 2, stopHurt_, [this, attack] { return attack() && test_->get(); });
        b("Jump", true, jump_, visible);
        b("Delay", false, delay_, visible);
        auto delayed = [this, visible] { return visible() && delay_->get(); };
        auto timed = [this, delayed] { return delayed() && !air_->get(); };
        n("Delay Ticks", 1, 5, 1, delayTicks_, timed);
        b("Force Delay Rising To Falling", false, falling_, timed);
        b("Delay Till On Ground", true, air_, delayed);
        b("Ground Delay", false, ground_, timed);
        b("Rotate", false, rotate_, visible);
        auto rotating = [this, visible] { return visible() && rotate_->get(); };
        n("Rotate Ticks", 1, 12, 3, rotateTicks_, rotating);
        b("Auto Move", false, autoMove_, rotating);
        b("Fake Check", true, fake_, visible);
        b("Debug", false, debug_, visible);
        return rows;
    }
    PredictionSettings ReadPredictionSettings() const {
        PredictionSettings s;
        s.reduce = reduce_->get(); s.reduceMode = reduceMode_->get();
        s.startBlinkHurtTime = int(startHurt_->get()); s.startReleaseTicks = int(startRelease_->get());
        s.forceBlocking = forceBlocking_->get(); s.extraAttack = extra_->get();
        s.reduceWhenCanAttack = whenAttack_->get(); s.cancelAuraAttack = cancel_->get();
        s.onlySprinting = sprinting_->get(); s.smartTimes = smart_->get(); s.attackTimes = int(times_->get());
        s.keepSprint = keep_->get(); s.testMode = test_->get(); s.stopBlockHurtTime = int(stopHurt_->get());
        s.jump = jump_->get(); s.delay = delay_->get(); s.delayTicks = int(delayTicks_->get());
        s.forceFalling = falling_->get(); s.airBuffer = air_->get(); s.groundDelay = ground_->get();
        s.rotate = rotate_->get(); s.rotateTicks = int(rotateTicks_->get()); s.autoMove = autoMove_->get();
        s.fakeCheck = fake_->get(); s.debug = debug_->get();
        return s;
    }
private:
    Properties::ModeProperty* reduceMode_{};
    Properties::BoolProperty *reduce_{}, *forceBlocking_{}, *extra_{}, *whenAttack_{}, *cancel_{},
        *sprinting_{}, *smart_{}, *keep_{}, *test_{}, *jump_{}, *delay_{}, *falling_{}, *air_{},
        *ground_{}, *rotate_{}, *autoMove_{}, *fake_{}, *debug_{};
    Properties::NumberProp *startHurt_{}, *startRelease_{}, *times_{}, *stopHurt_{}, *delayTicks_{}, *rotateTicks_{};
};
}
