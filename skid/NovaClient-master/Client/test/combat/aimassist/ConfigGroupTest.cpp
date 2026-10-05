#include <cassert>
#include <cstdio>
#include "config/settings/Properties.h"
#include "config/PropertyCodec.h"
using nova::config::SaveRows;
using nova::config::LoadRows;

constexpr const char* kValueKey = "value";
constexpr const char* kSubKey = "sub";
// @PRODUCTION_CONFIG_ROWS@

int main() {
    Properties::ModeProperty mode(0, {"Normal", "Silent"});
    Properties::ModeProperty aim(0, {"Single", "Switch"});
    Properties::ModeProperty part(0, {"Head", "Chest", "Legs"});
    Properties::NumberProp delay(0, 1000, 50, 250);
    std::vector<Properties::PropDesc> rows;
    rows.emplace_back("Mode", &mode, std::vector<Properties::PropDesc>{
        {"Aim", &aim}, {"Switch delay", &delay, {}, [&] { return aim.get() == 1; }}, {"Aim part", &part}});
    auto legacy = nova::json::Value::Object();
    legacy["Mode"] = "Silent";
    LoadRows(legacy, rows);
    assert(mode.get() == 1 && aim.get() == 0 && delay.get() == 250);
    assert(rows[0].hasSubProp() && !rows[0].getSubProps()[1].isVisible());
    aim.set(1); delay.set(500); part.set(2);
    assert(rows[0].getSubProps()[1].isVisible());
    const auto saved = SaveRows(rows);
    mode.set(0); aim.set(0); delay.set(250); part.set(0);
    LoadRows(saved, rows);
    assert(mode.get() == 1 && aim.get() == 1 && delay.get() == 500 && part.get() == 2);
    assert(saved.Find("Mode")->Find("sub")->Find("Aim")->asString() == "Switch");
    Properties::NumberProp horizontal(0, 720, 10, 180), vertical(0, 720, 10, 0);
    rows.emplace_back("Horizontal speed", &horizontal);
    rows.emplace_back("Vertical speed", &vertical);
    assert(rows[1].isVisible() && rows[2].isVisible());
    mode.set(0); horizontal.set(0); vertical.set(240);
    const auto axisSaved = SaveRows(rows);
    mode.set(0); horizontal.set(180); vertical.set(0);
    LoadRows(axisSaved, rows);
    assert(mode.get() == 0 && horizontal.get() == 0 && vertical.get() == 240);
    assert(rows[1].isVisible() && rows[2].isVisible());
    auto oldSettings = nova::json::Value::Object();
    oldSettings["Mode"] = "Normal";
    oldSettings["Turn speed"] = 580;
    LoadRows(MigrateAimAssistSettings(oldSettings), rows);
    assert(mode.get() == 0 && horizontal.get() == 580 && vertical.get() == 580);
    oldSettings["Mode"] = "Linear";
    oldSettings["Horizontal speed"] = 0;
    oldSettings["Vertical speed"] = 240;
    mode.set(1);
    LoadRows(MigrateAimAssistSettings(oldSettings), rows);
    assert(mode.get() == 0 && horizontal.get() == 0 && vertical.get() == 240);
    oldSettings["Mode"] = nova::json::Value::Object();
    oldSettings["Mode"]["value"] = "Linear";
    oldSettings["Mode"]["sub"]["Aim"] = "Switch";
    mode.set(1); aim.set(0);
    LoadRows(MigrateAimAssistSettings(oldSettings), rows);
    assert(mode.get() == 0 && aim.get() == 1 && horizontal.get() == 0);
    oldSettings["Mode"]["value"] = "Silent";
    LoadRows(MigrateAimAssistSettings(oldSettings), rows);
    assert(mode.get() == 1 && rows[1].isVisible() && rows[2].isVisible());
    Properties::BoolProperty smooth(false);
    Properties::NumberProp amount(0, 100, 1, 50);
    rows.emplace_back("Smooth", &smooth, std::vector<Properties::PropDesc>{{"Smooth amount", &amount}},
                      [&] { return mode.get() == 0; });
    auto smoothing = nova::json::Value::Object();
    smoothing["Smooth"] = true; // Legacy scalar can acquire a gear.
    LoadRows(smoothing, rows);
    assert(smooth.get() && amount.get() == 50 && !rows.back().isVisible());
    mode.set(0); amount.set(73);
    const auto smoothSaved = SaveRows(rows);
    smooth.set(false); amount.set(0);
    LoadRows(smoothSaved, rows);
    assert(smooth.get() && amount.get() == 73 && rows.back().isVisible());
    Properties::NumberProp backSpeed(10,720,10,180);
    rows.emplace_back("Rotation back speed", &backSpeed);
    auto silentLegacy = nova::json::Value::Object();
    silentLegacy["Mode"] = "Silent"; silentLegacy["Turn speed"] = 580;
    LoadRows(MigrateAimAssistSettings(silentLegacy), rows);
    assert(horizontal.get() == 580 && vertical.get() == 580 && backSpeed.get() == 180);
    silentLegacy["Rotation speed"] = 240; silentLegacy["Rotation back speed"] = 60;
    silentLegacy["Horizontal speed"] = 180; silentLegacy["Vertical speed"] = 0;
    LoadRows(MigrateAimAssistSettings(silentLegacy), rows);
    assert(horizontal.get() == 240 && vertical.get() == 240 && backSpeed.get() == 60);
    silentLegacy["Mode"] = nova::json::Value::Object();
    silentLegacy["Mode"]["value"] = "Silent";
    LoadRows(MigrateAimAssistSettings(silentLegacy), rows);
    assert(horizontal.get() == 240 && vertical.get() == 240);
    horizontal.set(0); vertical.set(360);
    const auto speedsSaved = SaveRows(rows);
    assert(!speedsSaved.Find("Turn speed") && !speedsSaved.Find("Rotation speed"));
    horizontal.set(180); vertical.set(180); backSpeed.set(180);
    LoadRows(MigrateAimAssistSettings(speedsSaved), rows);
    assert(horizontal.get() == 0 && vertical.get() == 360 && backSpeed.get() == 60);
    silentLegacy["Mode"] = "Normal";
    LoadRows(MigrateAimAssistSettings(silentLegacy), rows);
    assert(horizontal.get() == 180 && vertical.get() == 0); // Preserve active Normal settings.
    // BridgeAssist's gears are containers without their own toggle/property.
    Properties::NumberProp edge(0, .3, .01, 0);
    Properties::BoolProperty require(false);
    std::vector<Properties::PropDesc> groups{
        {"Sneaking", nullptr, {{"Edge offset", &edge}}},
        {"Conditions", nullptr, {{"Sneak key pressed", &require}}}};
    edge.set(.23); require.set(true);
    const auto groupSaved = SaveRows(groups);
    edge.set(0); require.set(false);
    LoadRows(groupSaved, groups);
    assert(std::abs(edge.get()-.23)<1e-6 && require.get());
    assert(groupSaved.Find("Sneaking")->Find("Edge offset"));
    std::puts("AimAssist gear: legacy mode, nested config and visibility checks passed");
}
