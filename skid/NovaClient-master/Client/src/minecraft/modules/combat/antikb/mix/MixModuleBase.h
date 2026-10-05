#pragma once
#include "minecraft/modules/combat/antikb/prediction/PredictionModuleBase.h"
#include "minecraft/modules/combat/antikb/mix/MixVelocity.h"

namespace nova::module {
class MixModuleBase : public PredictionModuleBase {
protected:
    void AddMixSettings(Properties::PropDesc::Visibility visible) {
        auto number = [&](const char* name, int lo, int hi, int value, Properties::NumberProp*& out) {
            auto row = subNumber(name, lo, hi, 1, value, [](double v) { return std::format("{:.0f}", v); });
            out = static_cast<Properties::NumberProp*>(row.getProp()); return row;
        };
        auto boolean = [&](const char* name, bool value, Properties::BoolProperty*& out) {
            auto row = subBool(name, value); out = static_cast<Properties::BoolProperty*>(row.getProp()); return row;
        };
        std::vector<Properties::PropDesc> rows;
        rows.push_back(number("Delay Ticks", 1, 5, 2, delay_));
        rows.push_back(number("Reduce Ticks", 1, 6, 3, reduce_));
        rows.push_back(boolean("No Bad Packets", false, bad_));
        rows.push_back(boolean("Jump", true, jump_));
        rows.push_back(boolean("Air Push", false, airPush_));
        rows.push_back(boolean("Delay Till On Ground", true, air_));
        auto ground = boolean("Delay On Ground", false, ground_);
        rows.emplace_back(ground.getName(), ground.getProp(), std::vector<Properties::PropDesc>{},
                          [this] { return !air_->get(); });
        rows.push_back(boolean("Fake Check", true, fake_));
        // Separate namespace: Prediction has identically named serialized settings.
        addSettings({Properties::PropDesc("Mix", nullptr, std::move(rows), std::move(visible))});
    }
    mix::Settings ReadMixSettings() const {
        return {int(delay_->get()), int(reduce_->get()), bad_->get(), jump_->get(), airPush_->get(),
                air_->get(), ground_->get(), fake_->get()};
    }
private:
    Properties::NumberProp *delay_{}, *reduce_{};
    Properties::BoolProperty *bad_{}, *jump_{}, *airPush_{}, *air_{}, *ground_{}, *fake_{};
};
}
