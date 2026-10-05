#include "bind/InputManager.h"

#include <algorithm>

#include "core/Instance.h"

namespace nova::bind {

InputManager& InputManager::Get() {
    return Instance<InputManager>([] { return new InputManager(); });
}

void InputManager::Register(InputListener* listener) {
    if (listener == nullptr || std::ranges::find(listeners_, listener) != listeners_.end())
        return;

    const int priority = listener->inputPriority();
    const auto at = std::ranges::find_if(listeners_, [priority](const InputListener* other) {
        return other->inputPriority() < priority;
    });
    listeners_.insert(at, listener);
}

void InputManager::Unregister(InputListener* listener) {
    std::erase(listeners_, listener);
}

bool InputManager::onMouseButton(int button, bool state) {
    const std::vector<InputListener*> listeners = listeners_;
    for (InputListener* listener : listeners) {
        if (!listener->onMouseButton(button, state))
            return false;
    }
    return true;
}

bool InputManager::onMouseScroll(float scrollX, float scrollY) {
    const std::vector<InputListener*> listeners = listeners_;
    for (InputListener* listener : listeners) {
        if (!listener->onMouseScroll(scrollX, scrollY))
            return false;
    }
    return true;
}

bool InputManager::onKey(int key, bool state) {
    const std::vector<InputListener*> listeners = listeners_;
    for (InputListener* listener : listeners) {
        if (!listener->onKey(key, state))
            return false;
    }
    return true;
}

bool InputManager::onChar(char c) {
    const std::vector<InputListener*> listeners = listeners_;
    for (InputListener* listener : listeners) {
        if (!listener->onChar(c))
            return false;
    }
    return true;
}

}
