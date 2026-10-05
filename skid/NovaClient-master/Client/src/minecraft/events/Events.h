#pragma once

#include "minecraft/events/EventBus.h"

namespace nova::event {

class KeyboardEvent : public CancellableEvent {
public:
    KeyboardEvent(int key, int scancode, bool down, bool repeat)
        : key(key), scancode(scancode), down(down), repeat(repeat) {}

    int key;
    int scancode;
    bool down;
    bool repeat;
};

class MouseButtonEvent : public CancellableEvent {
public:
    MouseButtonEvent(int button, bool down) : button(button), down(down) {}

    int button;
    bool down;
};

}
