#pragma once
#include "cs2/modules/Module.h"
#include "ui/hud/arraylist/ArrayListHud.h"

namespace nova::cs2::modules {
class ModuleManager {
public:
    static ModuleManager& Get();
    ModuleManager() = default;
    ~ModuleManager() { Shutdown(); }
    ModuleManager(const ModuleManager&) = delete;
    ModuleManager& operator=(const ModuleManager&) = delete;
    Module& Add(std::unique_ptr<Module> module);
    template<class T, class... Args> T& Create(Args&&... args) {
        auto module = std::make_unique<T>(std::forward<Args>(args)...);
        auto& result = *module; Add(std::move(module)); return result;
    }
    const std::vector<std::unique_ptr<Module>>& All() const { return modules_; }
    Module* Find(std::string_view name) const;
    std::vector<Module*> InCategory(std::string_view category) const;
    ui::FeatureCatalog Catalog();
    std::vector<ui::arraylist::Entry> Entries() const;
    void Seal() { sealed_ = true; }
    void Tick();
    void RenderOverlays();
    void SetAllEnabled(bool enabled);
    void Save(json::Value& root) const;
    void Load(const json::Value& root);
    void Shutdown();
private:
    std::vector<std::unique_ptr<Module>> modules_;
    bool sealed_ = false, stopped_ = false;
};
void RegisterBuiltinModules(bool preview);
}
