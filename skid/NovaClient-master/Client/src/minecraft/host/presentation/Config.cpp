#include "config/Config.h"
#include "config/HostConfig.h"
#include "config/PropertyCodec.h"
#include "minecraft/host/presentation/Config.h"
#include "minecraft/host/presentation/NameTagsConfig.h"
#include "minecraft/compat/Profile.h"

#include <cctype>
#include <cstdlib>
#include <system_error>
#include <utility>

#include "minecraft/modules/Module.h"
#include "minecraft/modules/ModuleManager.h"

namespace nova::config {
namespace {

constexpr const char* kValueKey = "value";

nova::json::Value MigrateAimAssistSettings(nova::json::Value settings) {
    if (!settings.isObject()) return settings;
    if (auto* mode = settings.Find("Mode")) {
        auto* value = mode->isObject() ? mode->Find(kValueKey) : mode;
        if (value && value->isString() && value->asString() == "Linear") {
            if (mode->isObject()) settings["Mode"][kValueKey] = "Normal";
            else settings["Mode"] = "Normal";
        }
    }
    const auto* mode = settings.Find("Mode");
    const auto* value = mode && mode->isObject() ? mode->Find(kValueKey) : mode;
    const bool silent = value && ((value->isString() && value->asString() == "Silent") ||
                                 (value->isNumber() && value->asNumber(0) == 1));
    // Old Silent configs saved Normal's hidden axis values too. Their active
    // Rotation speed initializes both shared axes; new configs omit that key.
    const auto* rotationSpeed = settings.Find("Rotation speed");
    const auto* speed = rotationSpeed && rotationSpeed->isNumber() ? rotationSpeed : settings.Find("Turn speed");
    if (speed && speed->isNumber()) {
        const auto oldSpeed = *speed;
        const bool replaceHiddenAxes = silent && rotationSpeed && rotationSpeed->isNumber();
        if (replaceHiddenAxes || !settings.Find("Horizontal speed")) settings["Horizontal speed"] = oldSpeed;
        if (replaceHiddenAxes || !settings.Find("Vertical speed")) settings["Vertical speed"] = oldSpeed;
    }
    return settings;
}

nova::json::Value BuildModule(const module::Module& module) {
    nova::json::Value out = nova::json::Value::Object();
    out["enabled"] = module.isEnabled();
    out["hiddenInArrayList"] = module.hiddenInArrayList();

    std::vector<Properties::PropDesc> rows;
    for (const Properties::PropDesc& row : module.configSettings()) {
        if (row.getProp() != module.enabledProperty())
            rows.push_back(row);
    }
    out["settings"] = SaveRows(rows);
    return out;
}

void ApplyModule(const nova::json::Value& in, module::Module& module) {
    if (!in.isObject())
        return;

    if (const nova::json::Value* settings = in.Find("settings")) {
        if (module.name() == "AimAssist")
            LoadRows(MigrateAimAssistSettings(*settings), module.configSettings());
        else if (module.name() == "NameTags")
            LoadRows(minecraft::MigrateNameTagsSettings(*settings), module.configSettings());
        else
            LoadRows(*settings, module.configSettings());
    }
    if (const nova::json::Value* enabled = in.Find("enabled"))
        module.setEnabled(enabled->asBool(module.isEnabled()));
    // Carry the previous hide preference over when loading older configs.
    const nova::json::Value* hidden = in.Find("hiddenInArrayList");
    if (hidden == nullptr) hidden = in.Find("hiddenInClickGui");
    if (hidden != nullptr)
        module.setHiddenInArrayList(hidden->asBool(module.hiddenInArrayList()));
}

std::filesystem::path MinecraftConfigDirectory() {
    const char* suffix = nova::compat::Current().id == NovaClient_Forge1201
        ? "" : nova::compat::Current().key;
    const char* profile = std::getenv("USERPROFILE");
    if (profile != nullptr && *profile != '\0')
        return std::filesystem::path(profile) / ".nova" / "configs" / suffix;

    std::error_code error;
    const std::filesystem::path home = std::filesystem::current_path(error);
    if (error)
        return std::filesystem::path(".nova") / "configs" / suffix;
    return home / ".nova" / "configs" / suffix;
}

void SaveMinecraft(json::Value& root) {
    nova::json::Value modules = nova::json::Value::Object();
    for (const auto& module : module::ModuleManager::Get().All())
        modules[module->name()] = BuildModule(*module);
    root["modules"] = std::move(modules);

}

void LoadMinecraft(const json::Value& root) {
    const nova::json::Value* modules = root.Find("modules");
    if (modules == nullptr || !modules->isObject())
        return;

    for (const auto& module : module::ModuleManager::Get().All()) {
        const nova::json::Value* entry = modules->Find(module->name());
        if (entry != nullptr)
            ApplyModule(*entry, *module);
    }

}
}
}

namespace nova::minecraft {
void InstallConfig() {
    config::InstallHostConfig({config::MinecraftConfigDirectory(),&config::SaveMinecraft,&config::LoadMinecraft});
}
}
