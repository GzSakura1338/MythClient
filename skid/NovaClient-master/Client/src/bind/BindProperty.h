#pragma once

#include <functional>

#include "bind/IBindable.h"
#include "config/settings/Properties.h"

namespace nova::bind {

class BindProperty final : public Properties::AbstractProperty, public IBindable {
public:
    BindProperty();
    BindProperty(int keybind, bool hold, bool keyOnly = false);
    ~BindProperty() override;

    BindProperty(const BindProperty&) = delete;
    BindProperty& operator=(const BindProperty&) = delete;

    [[nodiscard]] int keybind() const override { return keybind_; }
    void setKeybind(int keybind);
    [[nodiscard]] bool keyOnly() const { return keyOnly_; }
    void setKeyChangeConsumer(std::function<void(int)> consumer) { keyConsumer_ = std::move(consumer); }

    [[nodiscard]] bool hold() const { return hold_; }
    void setHold(bool hold);

    [[nodiscard]] bool isActive() const override { return state_; }
    void setState(bool state);
    void syncState(bool state);
    // Reconcile only a hold started by input; GUI/config toggles stay explicit.
    void reconcileHold(bool physicalDown);

    void onActive(int action) override;
    [[nodiscard]] bool shouldActiveWhenRelease() const override { return hold_; }

    void setStateChangeConsumer(std::function<void(bool)> consumer) {
        consumer_ = std::move(consumer);
    }
    void setGate(std::function<bool()> gate) { gate_ = std::move(gate); }

    void save(Properties::Json& out) const override;
    void load(const Properties::Json& in) override;

private:
    int keybind_ = 0;
    bool hold_ = false;
    bool state_ = false;
    bool holdPressed_ = false;
    bool keyOnly_ = false;
    std::function<void(int)> keyConsumer_;
    std::function<void(bool)> consumer_;
    std::function<bool()> gate_;
};

}
