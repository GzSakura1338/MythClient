#pragma once

#include <format>
#include "minecraft/modules/Module.h"
#include "minecraft/modules/combat/autoblock/AutoBlock.h"

namespace nova::module {

// Shared property definitions preserve KillAura config keys and mode visibility.
class AutoBlockModuleBase : public Module {
protected:
    explicit AutoBlockModuleBase(std::string name)
        : Module(std::move(name), Category::Combat, "General", "General") {}

    std::vector<Properties::PropDesc> BuildBlockSettings() {
        auto WholeNumber = [](double value) { return std::format("{:.0f}", value); };
        auto Hundredths = [](double value) { return std::format("{:.2f}", value); };
        std::vector<Properties::PropDesc> blockSettings;
        auto blockMode = subMode("Mode", static_cast<int>(AutoBlockMode::Lag), {"Lag", "Vanilla", "Grim", "Hypixel", "Slinky", "Lag New"});
        blockMode_ = static_cast<Properties::ModeProperty*>(blockMode.getProp());
        blockSettings.push_back(std::move(blockMode));
        auto lagMode = [this] { return blockMode_->get() == static_cast<int>(AutoBlockMode::Lag); };
        auto number = [&](const char* name, double min, double max, double step, double value,
                          Properties::NumberProp*& target) {
            auto row = subNumber(name, min, max, step, value, WholeNumber);
            row = Properties::PropDesc(row.getName(), row.getProp(), {}, lagMode);
            target = static_cast<Properties::NumberProp*>(row.getProp());
            blockSettings.push_back(std::move(row));
        };
        auto boolean = [&](const char* name, bool value, Properties::BoolProperty*& target, bool common = false) {
            auto row = subBool(name, value);
            if (!common) row = Properties::PropDesc(row.getName(), row.getProp(), {}, lagMode);
            else row = Properties::PropDesc(row.getName(), row.getProp(), {},
                [this] { return blockMode_->get() != static_cast<int>(AutoBlockMode::Hypixel) &&
                                blockMode_->get() != static_cast<int>(AutoBlockMode::Slinky); });
            target = static_cast<Properties::BoolProperty*>(row.getProp());
            blockSettings.push_back(std::move(row));
        };
        auto blockRange = subNumber("Range", 2, 6, 0.1, 4, Hundredths);
        blockRange_ = static_cast<Properties::NumberProp*>(blockRange.getProp());
        blockSettings.emplace_back(blockRange.getName(), blockRange.getProp(), std::vector<Properties::PropDesc>{},
            [this] { return blockMode_->get() != static_cast<int>(AutoBlockMode::Hypixel) &&
                            blockMode_->get() != static_cast<int>(AutoBlockMode::Slinky); });
        auto blockHoldTicks = subNumber("Hold ticks", 1, 10, 1, 1, WholeNumber);
        blockHoldTicks_ = static_cast<Properties::NumberProp*>(blockHoldTicks.getProp());
        blockSettings.emplace_back(blockHoldTicks.getName(), blockHoldTicks.getProp(),
            std::vector<Properties::PropDesc>{},
            [this] { return blockMode_->get() == static_cast<int>(AutoBlockMode::Vanilla) ||
                            blockMode_->get() == static_cast<int>(AutoBlockMode::Grim); });
        auto grimMode = [this] {
            return blockMode_->get() == static_cast<int>(AutoBlockMode::Grim);
        };
        auto blinkTicks = subNumber("Blink ticks", 1, 6, 1, 3, WholeNumber);
        blockBlinkTicks_ = static_cast<Properties::NumberProp*>(blinkTicks.getProp());
        blockSettings.emplace_back(blinkTicks.getName(), blinkTicks.getProp(),
            std::vector<Properties::PropDesc>{}, grimMode);
        auto moveTicks = subNumber("Move ticks", 0, 6, 1, 2, WholeNumber);
        blockMoveTicks_ = static_cast<Properties::NumberProp*>(moveTicks.getProp());
        blockSettings.emplace_back(moveTicks.getName(), moveTicks.getProp(),
            std::vector<Properties::PropDesc>{}, grimMode);
        auto releaseOnAttack = subBool("Release on attack", false);
        blockReleaseOnAttack_ = static_cast<Properties::BoolProperty*>(releaseOnAttack.getProp());
        blockSettings.emplace_back(releaseOnAttack.getName(), releaseOnAttack.getProp(),
            std::vector<Properties::PropDesc>{}, grimMode);
        number("Maximum Hurt Time (ms)", 50, 500, 50, 200, blockHurt_);
        number("Maximum Hold Time (ms)", 50, 500, 50, 150, blockHold_);
        number("Lag Chance (%)", 0, 100, 5, 100, blockChance_);
        number("Lag Max Duration (ms)", 50, 500, 50, 200, blockLag_);
        boolean("Prevent delaying attacks", true, blockPrevent_);
        boolean("Block again immediately", true, blockAgain_);
        auto animation = subBool("Force block animation", true);
        blockAnimation_ = static_cast<Properties::BoolProperty*>(animation.getProp());
        blockSettings.push_back(std::move(animation));
        boolean("Require Left mouse", true, blockLeft_);
        boolean("Require right mouse", false, blockRight_, true);
        boolean("Damaged", false, blockDamaged_);
        auto hypixel = [this] { return blockMode_->get() == static_cast<int>(AutoBlockMode::Hypixel); };
        auto old = [this, hypixel] { return hypixel() && hypixelMode_->get() == 0; };
        auto custom = [this, hypixel] { return hypixel() && hypixelMode_->get() == 2; };
        auto lag = [this, hypixel] { return hypixel() && hypixelMode_->get() == 3; };
        auto mode = subMode("HypixelMode", 0, {"OldHypixel", "Without NoSlow", "Custom", "Lag"});
        hypixelMode_ = static_cast<Properties::ModeProperty*>(mode.getProp());
        blockSettings.emplace_back(mode.getName(), mode.getProp(), std::vector<Properties::PropDesc>{}, hypixel);
        auto sub = subMode("LagMode", 1, {"2Tick", "3Tick", "4Tick", "3Tick + 2Tick", "5Tick"});
        hypixelLagMode_ = static_cast<Properties::ModeProperty*>(sub.getProp());
        blockSettings.emplace_back(sub.getName(), sub.getProp(), std::vector<Properties::PropDesc>{}, lag);
        auto hb = [&](const char* name, bool value, Properties::BoolProperty*& prop, auto visible) {
            auto row = subBool(name, value);
            prop = static_cast<Properties::BoolProperty*>(row.getProp());
            blockSettings.emplace_back(row.getName(), row.getProp(), std::vector<Properties::PropDesc>{}, visible);
        };
        auto hn = [&](const char* name, double min, double max, double value,
                      Properties::NumberProp*& prop, auto visible) {
            auto row = subNumber(name, min, max, 1, value, WholeNumber);
            prop = static_cast<Properties::NumberProp*>(row.getProp());
            blockSettings.emplace_back(row.getName(), row.getProp(), std::vector<Properties::PropDesc>{}, visible);
        };
        hb("NoSwap", true, hypixelNoSwap_, old);
        hb("MoreAttack", false, hypixelMoreAttack_, old);
        hn("MoreAttackDelay", 0, 3, 1, hypixelMoreDelay_, [this, old] { return old() && hypixelMoreAttack_->get(); });
        hn("MaxTick", 1, 5, 3, hypixelMaxTick_, custom);
        const char* tickNames[] = {"StartBlinkTick", "StopBlinkTick", "SwapTick", "SwitchBackTick",
                                  "StopBlockTick", "AttackTick", "StartBlockTick"};
        const int defaults[] = {0, 2, 2, 2, 2, 0, 0};
        for (int i = 0; i < 7; ++i)
            hn(tickNames[i], 0, 5, defaults[i], hypixelTicks_[i], custom);
        hb("PostBlock", false, hypixelPostBlock_, custom);
        hb("AlwaysRenderBlocking", true, hypixelRender_, lag);
        hb("C09Instead", true, hypixelC09_, [this, lag] { return lag() && hypixelLagMode_->get() == 1; });
        hb("FullC09(Will Cause Damage Less)", false, hypixelFullC09_,
            [this, lag] { return lag() && hypixelLagMode_->get() >= 2; });
        hn("AutoBlock Aps", 1, 20, 10, hypixelAps_, hypixel);
        hb("AutoBlock Require Press", false, hypixelRequirePress_, hypixel);
        auto range = subNumber("AutoBlock Range", 3, 8, 0.1, 6, Hundredths);
        hypixelRange_ = static_cast<Properties::NumberProp*>(range.getProp());
        blockSettings.emplace_back(range.getName(), range.getProp(), std::vector<Properties::PropDesc>{}, hypixel);
        auto slinky = [this] { return blockMode_->get() == static_cast<int>(AutoBlockMode::Slinky); };
        const char* names[] = {"Slinky Range", "Max Hurt Time (ms)", "Max Hold (ms)",
            "Slinky Lag Chance (%)", "Slinky Lag Max Duration (ms)", "Queue Size", "Damage Window (ms)"};
        const double mins[] = {1,50,50,0,0,1,500}, maxs[] = {6,1000,500,100,500,10,5000};
        const double steps[] = {.1,5,5,5,10,1,100}, slinkyDefaults[] = {3.5,200,150,0,100,3,2000};
        for (int i=0;i<7;++i) {
            auto row = subNumber(names[i],mins[i],maxs[i],steps[i],slinkyDefaults[i],
                i == 0 ? std::function<std::string(double)>(Hundredths) : WholeNumber);
            slinkyNumbers_[i] = static_cast<Properties::NumberProp*>(row.getProp());
            blockSettings.emplace_back(row.getName(),row.getProp(),std::vector<Properties::PropDesc>{},slinky);
        }
        const char* buttons[] = {"Block Immediately", "Prevent Delaying Attacks", "Block Again Immediately",
            "Left Click", "Right Click", "Damaged Only", "Ignore Mobs", "Ignore Teammates", "Ignore Bots", "Debug"};
        const bool values[] = {true,true,true,false,false,false,true,false,false,false};
        for (int i=0;i<10;++i) hb(buttons[i],values[i],slinkyButtons_[i],slinky);
        auto lagNew = [this] { return blockMode_->get() == static_cast<int>(AutoBlockMode::LagNew); };
        auto aps = subMode("APS Mode", 0, {"7APS", "3APS", "5APS", "10APS", "14APS"});
        lagNewAps_ = static_cast<Properties::ModeProperty*>(aps.getProp());
        blockSettings.emplace_back(aps.getName(), aps.getProp(), std::vector<Properties::PropDesc>{}, lagNew);
        hb("Manual Left Click", false, lagNewManualClick_, lagNew);
        return blockSettings;
    }

    AutoBlockSettings ReadBlockSettings() const {
        AutoBlockSettings settings;
        settings.mode = static_cast<AutoBlockMode>(blockMode_->get());
        settings.lagNewManualClick = lagNewManualClick_->get();
        settings.lagNewApsMode = lagNewAps_->get();
        settings.holdTicks = static_cast<int>(blockHoldTicks_->get());
        settings.blinkTicks = static_cast<int>(blockBlinkTicks_->get());
        settings.moveTicks = static_cast<int>(blockMoveTicks_->get());
        settings.releaseOnAttack = blockReleaseOnAttack_->get();
        settings.range = blockRange_->get();
        settings.hurtMs = static_cast<int>(blockHurt_->get());
        settings.holdMs = static_cast<int>(blockHold_->get());
        settings.lagChance = static_cast<int>(blockChance_->get());
        settings.lagMs = static_cast<int>(blockLag_->get());
        settings.preventDelayAttacks = blockPrevent_->get();
        settings.blockAgain = blockAgain_->get();
        settings.animation = blockAnimation_->get();
        settings.requireLeft = blockLeft_->get();
        settings.requireRight = blockRight_->get();
        settings.damaged = blockDamaged_->get();
        auto& h = settings.hypixel;
        h.mode = hypixelMode_->get(); h.lagMode = hypixelLagMode_->get();
        h.noSwap = hypixelNoSwap_->get(); h.moreAttack = hypixelMoreAttack_->get();
        h.moreAttackDelay = static_cast<int>(hypixelMoreDelay_->get());
        h.maxTick = static_cast<int>(hypixelMaxTick_->get());
        h.startBlinkTick = static_cast<int>(hypixelTicks_[0]->get());
        h.stopBlinkTick = static_cast<int>(hypixelTicks_[1]->get());
        h.swapTick = static_cast<int>(hypixelTicks_[2]->get());
        h.switchBackTick = static_cast<int>(hypixelTicks_[3]->get());
        h.stopBlockTick = static_cast<int>(hypixelTicks_[4]->get());
        h.attackTick = static_cast<int>(hypixelTicks_[5]->get());
        h.startBlockTick = static_cast<int>(hypixelTicks_[6]->get());
        h.postBlock = hypixelPostBlock_->get(); h.alwaysRenderBlocking = hypixelRender_->get();
        h.c09Instead = hypixelC09_->get(); h.fullC09 = hypixelFullC09_->get();
        h.aps = static_cast<int>(hypixelAps_->get()); h.range = hypixelRange_->get();
        if (settings.mode == AutoBlockMode::Hypixel) settings.requireRight = hypixelRequirePress_->get();
        auto& s = settings.slinky;
        s.range = slinkyNumbers_[0]->get(); s.hurtMs = static_cast<int>(slinkyNumbers_[1]->get());
        s.holdMs = static_cast<int>(slinkyNumbers_[2]->get()); s.lagChance = static_cast<int>(slinkyNumbers_[3]->get());
        s.lagMs = static_cast<int>(slinkyNumbers_[4]->get()); s.queueSize = static_cast<int>(slinkyNumbers_[5]->get());
        s.damageWindow = static_cast<int>(slinkyNumbers_[6]->get());
        s.immediately = slinkyButtons_[0]->get(); s.preventDelayAttacks = slinkyButtons_[1]->get();
        s.blockAgain = slinkyButtons_[2]->get(); s.requireLeft = slinkyButtons_[3]->get();
        s.requireRight = slinkyButtons_[4]->get(); s.damagedOnly = slinkyButtons_[5]->get();
        s.ignoreMobs = slinkyButtons_[6]->get(); s.ignoreTeammates = slinkyButtons_[7]->get();
        s.ignoreBots = slinkyButtons_[8]->get(); s.debug = slinkyButtons_[9]->get();
        return settings;
    }

private:
    Properties::BoolProperty* lagNewManualClick_ = nullptr;
    Properties::ModeProperty* lagNewAps_ = nullptr;
    Properties::NumberProp* slinkyNumbers_[7]{};
    Properties::BoolProperty* slinkyButtons_[10]{};
    Properties::ModeProperty* hypixelMode_ = nullptr;
    Properties::ModeProperty* hypixelLagMode_ = nullptr;
    Properties::BoolProperty *hypixelNoSwap_ = nullptr, *hypixelMoreAttack_ = nullptr,
        *hypixelPostBlock_ = nullptr, *hypixelRender_ = nullptr, *hypixelC09_ = nullptr, *hypixelFullC09_ = nullptr,
        *hypixelRequirePress_ = nullptr;
    Properties::NumberProp *hypixelMoreDelay_ = nullptr, *hypixelMaxTick_ = nullptr,
        *hypixelAps_ = nullptr, *hypixelRange_ = nullptr;
    Properties::NumberProp* hypixelTicks_[7]{};
    Properties::NumberProp* blockRange_ = nullptr;
    Properties::NumberProp* blockHurt_ = nullptr;
    Properties::NumberProp* blockHold_ = nullptr;
    Properties::NumberProp* blockChance_ = nullptr;
    Properties::NumberProp* blockLag_ = nullptr;
    Properties::BoolProperty* blockPrevent_ = nullptr;
    Properties::BoolProperty* blockAgain_ = nullptr;
    Properties::BoolProperty* blockAnimation_ = nullptr;
    Properties::BoolProperty* blockLeft_ = nullptr;
    Properties::BoolProperty* blockRight_ = nullptr;
    Properties::BoolProperty* blockDamaged_ = nullptr;
    Properties::ModeProperty* blockMode_ = nullptr;
    Properties::NumberProp* blockHoldTicks_ = nullptr;
    Properties::NumberProp* blockBlinkTicks_ = nullptr;
    Properties::NumberProp* blockMoveTicks_ = nullptr;
    Properties::BoolProperty* blockReleaseOnAttack_ = nullptr;
};

}
