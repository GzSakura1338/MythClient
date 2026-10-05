#include "minecraft/modules/ModuleManager.h"

#include "core/Instance.h"

namespace nova::module {

ModuleManager& ModuleManager::Get() {
    return Instance<ModuleManager>([] { return new ModuleManager(); });
}

Module& ModuleManager::Add(std::unique_ptr<Module> module) {
    Module& reference = *module;
    modules_.push_back(std::move(module));
    return reference;
}

Module* ModuleManager::Find(std::string_view name) const {
    for (const auto& module : modules_) {
        if (module->name() == name)
            return module.get();
    }
    return nullptr;
}

std::vector<Module*> ModuleManager::InCategory(Category category) const {
    std::vector<Module*> found;
    for (const auto& module : modules_) {
        if (module->category() == category)
            found.push_back(module.get());
    }
    return found;
}

void ModuleManager::Seal() { sealed_.store(true, std::memory_order_release); }

void ModuleManager::Tick() {
    if (!sealed_.load(std::memory_order_acquire))
        return;
    for (const auto& module : modules_)
        module->tick();
}

void ModuleManager::RenderOverlays() {
    if (!sealed_.load(std::memory_order_acquire))
        return;
    for (const auto& module : modules_)
        module->renderOverlay();
}

void ModuleManager::SetAllEnabled(bool value) {
    for (const auto& module : modules_)
        module->setEnabled(value);
}

}
