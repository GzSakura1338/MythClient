#pragma once

#include <vector>

#include "bind/InputListener.h"

namespace nova::bind {

class InputManager {
public:
    static InputManager& Get();

    InputManager(const InputManager&) = delete;
    InputManager& operator=(const InputManager&) = delete;

    void Register(InputListener* listener);
    void Unregister(InputListener* listener);

    bool onMouseButton(int button, bool state);
    bool onMouseScroll(float scrollX, float scrollY);
    bool onKey(int key, bool state);
    bool onChar(char c);

private:
    InputManager() = default;

    std::vector<InputListener*> listeners_;
};

}
