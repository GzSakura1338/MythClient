#pragma once

namespace nova::bind {

class InputListener {
public:
    virtual ~InputListener() = default;

    [[nodiscard]] virtual int inputPriority() const { return 10; }

    virtual bool onMouseButton(int button, bool state) {
        (void)button;
        (void)state;
        return true;
    }
    virtual bool onMouseScroll(float scrollX, float scrollY) {
        (void)scrollX;
        (void)scrollY;
        return true;
    }
    virtual bool onKey(int key, bool state) {
        (void)key;
        (void)state;
        return true;
    }
    virtual bool onChar(char c) {
        (void)c;
        return true;
    }
};

}
