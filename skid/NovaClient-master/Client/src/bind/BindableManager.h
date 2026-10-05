#pragma once

#include <vector>

#include "bind/IBindable.h"
#include "bind/InputListener.h"

namespace nova::bind {

class BindableManager final : public InputListener {
public:
    static BindableManager& Get();

    BindableManager(const BindableManager&) = delete;
    BindableManager& operator=(const BindableManager&) = delete;

    void Register(IBindable* bindable);
    void Unregister(IBindable* bindable);

    [[nodiscard]] int inputPriority() const override { return 80; }

    bool onKey(int key, bool state) override;
    bool onMouseButton(int button, bool state) override;

private:
    BindableManager();

    void ProcessInput(int mergedCode, bool state);

    std::vector<IBindable*> bindables_;
};

}
