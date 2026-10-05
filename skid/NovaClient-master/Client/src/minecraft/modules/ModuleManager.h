#pragma once

#include <atomic>
#include <cstddef>
#include <memory>
#include <string_view>
#include <utility>
#include <vector>

#include "minecraft/modules/Category.h"
#include "minecraft/modules/Module.h"

namespace nova::module {

class ModuleManager {
public:
    static ModuleManager& Get();

    ModuleManager(const ModuleManager&) = delete;
    ModuleManager& operator=(const ModuleManager&) = delete;

    Module& Add(std::unique_ptr<Module> module);

    template <typename T, typename... A>
    T& Create(A&&... args) {
        auto module = std::make_unique<T>(std::forward<A>(args)...);
        T& reference = *module;
        Add(std::move(module));
        return reference;
    }

    [[nodiscard]] const std::vector<std::unique_ptr<Module>>& All() const { return modules_; }
    [[nodiscard]] std::size_t Count() const { return modules_.size(); }
    [[nodiscard]] bool Empty() const { return modules_.empty(); }

    [[nodiscard]] Module* Find(std::string_view name) const;
    [[nodiscard]] std::vector<Module*> InCategory(Category category) const;

    void Seal();

    void Tick();

    void RenderOverlays();

    void SetAllEnabled(bool value);

private:
    ModuleManager() = default;

    std::atomic<bool> sealed_{false};
    std::vector<std::unique_ptr<Module>> modules_;
};

void RegisterBuiltinModules();

}
