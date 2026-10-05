#include "bind/BindableManager.h"
#include "bind/BrowserCapture.h"

#include <algorithm>

#include "bind/InputManager.h"
#include "core/Instance.h"

namespace nova::bind {

BindableManager& BindableManager::Get() {
    return Instance<BindableManager>([] { return new BindableManager(); });
}

BindableManager::BindableManager() {
    InputManager::Get().Register(this);
}

void BindableManager::Register(IBindable* bindable) {
    if (bindable == nullptr || std::ranges::find(bindables_, bindable) != bindables_.end())
        return;
    bindables_.push_back(bindable);
}

void BindableManager::Unregister(IBindable* bindable) {
    std::erase(bindables_, bindable);
}

bool BindableManager::onKey(int key, bool state) {
    ProcessInput(key, state);
    return true;
}

bool BindableManager::onMouseButton(int button, bool state) {
    ProcessInput(-1 - button, state);
    return true;
}

void BindableManager::ProcessInput(int mergedCode, bool state) {
    if(state && browserCapture.load())return;
    const std::vector<IBindable*> bindables = bindables_;
    for (IBindable* bindable : bindables) {
        if (bindable->keybind() != mergedCode)
            continue;
        if (!state && !bindable->shouldActiveWhenRelease())
            continue;
        bindable->onActive(state ? 1 : 0);
    }
}

}
