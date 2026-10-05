#pragma once
#include "config/PropertyCodec.h"
#include "ui/hud/nametags/Appearance.h"

namespace nova::minecraft {
inline json::Value MigrateNameTagsSettings(json::Value settings) {
    if (!settings.isObject()) return settings;
    ui::nametags::Appearance original,classic;
    const auto* mode=settings.Find("Mode");
    if (mode && mode->isObject()) mode=mode->Find("value");
    const bool isClassic=mode && ((mode->isString() && mode->asString()=="Classic") ||
        (mode->isNumber() && mode->asNumber(0)==1));
    if (!settings.Find("Default Appearance") && !settings.Find("Classic Appearance")) {
        auto& appearance=isClassic?classic:original;
        Properties::NumberProp range{8,512,1,48};
        std::vector<Properties::PropDesc> legacy{{"Range",&range}};
        for (auto& row : appearance.Rows()) legacy.push_back(std::move(row));
        config::LoadRows(settings,legacy);
    }
    if (!settings.Find("Default Appearance"))
        settings["Default Appearance"]=config::SaveRows(original.ConfigRows());
    if (!settings.Find("Classic Appearance"))
        settings["Classic Appearance"]=config::SaveRows(classic.ConfigRows());
    return settings;
}
}
