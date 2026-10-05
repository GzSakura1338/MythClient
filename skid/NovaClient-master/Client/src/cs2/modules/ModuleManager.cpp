#include "cs2/modules/ModuleManager.h"
#include "config/PropertyCodec.h"
#include "core/Instance.h"
#include <stdexcept>

namespace nova::cs2::modules {
ModuleManager& ModuleManager::Get() { return Instance<ModuleManager>(); }
Module& ModuleManager::Add(std::unique_ptr<Module> module) {
    if (sealed_ || stopped_) throw std::logic_error("CS2 module registry is sealed");
    if (!module || module->name().empty() || Find(module->name())) throw std::invalid_argument("Invalid or duplicate CS2 module");
    auto& result = *module; modules_.push_back(std::move(module)); return result;
}
Module* ModuleManager::Find(std::string_view name) const {
    for (const auto& module : modules_) if (module->name() == name) return module.get();
    return nullptr;
}
std::vector<Module*> ModuleManager::InCategory(std::string_view category) const {
    std::vector<Module*> result;
    for (const auto& module : modules_) if (module->category() == category) result.push_back(module.get());
    return result;
}
ui::FeatureCatalog ModuleManager::Catalog() {
    ui::FeatureCatalog result;
    result.categories = {
        // Classic and Panel use separate icon fonts and Unicode mappings.
        {"combat", "COMBAT", u8"\ue66f", ui::IconFace::Fatal, "Combat", u8"\uf05b", 0},
        {"player", "PLAYER", u8"\ue6df", ui::IconFace::Fatal, "Player", u8"\uf007", 2},
        {"movement", "MOVEMENT", u8"\ue86b", ui::IconFace::Frost, "Movement", u8"\uf8cc", 1},
        {"render", "RENDER", u8"\ue620", ui::IconFace::Fatal, "Render", u8"\uf03e", 3},
        {"misc", "MISC", u8"\ue665", ui::IconFace::Fatal, "Miscellaneous", u8"\uf1de", 4}};
    for (const auto& module : modules_) result.features.push_back(module->feature());
    // Keep all five categories, including empty ones, matching Minecraft.
    // The shared panel adds CLIENT / Settings separately from module categories.
    return result;
}
std::vector<ui::arraylist::Entry> ModuleManager::Entries() const {
    std::vector<ui::arraylist::Entry> result;
    for (const auto& module : modules_)
        result.push_back({module->name(), module->name(), module->suffix(), module->isEnabled(), module->hiddenInArrayList()});
    return result;
}
void ModuleManager::Tick() {
    if (sealed_ && !stopped_) for (const auto& module : modules_) module->tick();
}
void ModuleManager::RenderOverlays() {
    if (sealed_ && !stopped_) for (const auto& module : modules_) module->renderOverlay();
}
void ModuleManager::SetAllEnabled(bool enabled) {
    if (!stopped_) for (const auto& module : modules_) module->setEnabled(enabled);
}
void ModuleManager::Save(json::Value& root) const {
    auto data = json::Value::Object();
    for (const auto& module : modules_) {
        auto entry = json::Value::Object();
        entry["enabled"] = module->persistEnabled() && module->requestedEnabled();
        entry["hiddenInArrayList"] = module->hiddenInArrayList();
        std::vector<Properties::PropDesc> rows;
        for (const auto& row : module->settings()) if (std::string_view(row.getName()) != "Enabled") rows.push_back(row);
        entry["settings"] = config::SaveRows(rows);
        data[module->name()] = std::move(entry);
    }
    root["modules"] = std::move(data);
}
void ModuleManager::Load(const json::Value& root) {
    const auto* data = root.Find("modules");
    const bool legacy = !data;
    if (!data) data = root.Find("features"); // Read the first CS2 skeleton's configs.
    if (!data || !data->isObject() || stopped_) return;
    // Migrate the old ESP-specific teammate checkbox only when this config
    // has no explicit Team module. New configs have one central switch.
    if (!data->Find("Team")) {
        auto* team = Find("Team");
        const auto* esp = data->Find("2DESP");
        const auto* rows = esp ? (legacy ? esp : esp->Find("settings")) : nullptr;
        const auto* targets = rows ? rows->Find("Targets") : nullptr;
        const auto* teammates = targets ? targets->Find("Player Teammates") : nullptr;
        if (team && teammates) team->setEnabled(!teammates->asBool(true));
    }
    for (const auto& module : modules_) if (const auto* entry = data->Find(module->name())) {
        const bool enabled = module->requestedEnabled();
        if (legacy) {
            config::LoadRows(*entry, module->settings());
            const auto* value = entry->Find("Enabled");
            module->setEnabled(value ? value->asBool(enabled) : enabled);
        } else {
            if (const auto* rows = entry->Find("settings")) config::LoadRows(*rows, module->settings());
            const auto* value = entry->Find("enabled");
            module->setEnabled(value ? value->asBool(enabled) : enabled);
            if (const auto* hidden = entry->Find("hiddenInArrayList")) module->hiddenInArrayListProperty().load(*hidden);
        }
        // Preserve renamed Aimbot options and its old nested recoil setting.
        // Explicit current-format values take precedence.
        if (module->name() == "Aimbot") {
            const auto* rows = legacy ? entry : entry->Find("settings");
            const auto* mode = rows ? rows->Find("Mode") : nullptr;
            const auto* details = mode ? mode->Find("sub") : nullptr;
            const auto* recoil = details ? details->Find("Recoil Compensation") : nullptr;
            if (rows) {
                auto migrated = json::Value::Object();
                if (!rows->Find("Recoil Compensation") && recoil) migrated["Recoil Compensation"] = *recoil;
                if (const auto* point = rows->Find("Aim Point"); point && !rows->Find("Aim Priority"))
                    migrated["Aim Priority"] = *point;
                config::LoadRows(migrated, module->settings());
            }
        }
        if (!module->persistEnabled()) module->setEnabled(false);
    }
}
void ModuleManager::Shutdown() {
    if (stopped_) return;
    stopped_ = true;
    for (auto it = modules_.rbegin(); it != modules_.rend(); ++it) (*it)->shutdown();
}
}
