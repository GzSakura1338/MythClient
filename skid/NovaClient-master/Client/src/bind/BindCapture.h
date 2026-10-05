#pragma once

#include "bind/InputListener.h"

namespace nova::bind {

class BindProperty;

class BindCapture final : public InputListener {
public:
    static BindCapture& Get();

    BindCapture(const BindCapture&) = delete;
    BindCapture& operator=(const BindCapture&) = delete;

    void Begin(BindProperty* target);
    void Cancel();

    [[nodiscard]] bool listening() const { return target_ != nullptr; }
    [[nodiscard]] const BindProperty* target() const { return target_; }
    void setPressing(bool pressing) { pressing_ = pressing; }

    [[nodiscard]] int inputPriority() const override { return 95; }

    bool onKey(int key, bool state) override;
    bool onMouseButton(int button, bool state) override;

private:
    BindCapture();

    BindProperty* target_ = nullptr;
    bool pressing_ = false;
};

}
