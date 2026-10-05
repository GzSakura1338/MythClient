#include "bind/BindProperty.h"

#include "bind/BindableManager.h"

namespace nova::bind {

BindProperty::BindProperty() : BindProperty(0, false) {}

BindProperty::BindProperty(int keybind, bool hold, bool keyOnly) : keybind_(keybind), hold_(hold && !keyOnly), keyOnly_(keyOnly) {
    BindableManager::Get().Register(this);
}

BindProperty::~BindProperty() {
    BindableManager::Get().Unregister(this);
}

void BindProperty::setHold(bool hold) {
    if (keyOnly_) return;
    holdPressed_ = false;
    setState(false);
    hold_ = hold;
}

void BindProperty::setKeybind(int keybind) {
    if (keyOnly_ && (keybind < -5 || keybind > 255)) return;
    if (keybind_ == keybind) return;
    if (holdPressed_) onActive(0);
    keybind_ = keybind;
    if (keyConsumer_) keyConsumer_(keybind);
}

void BindProperty::setState(bool state) {
    if (state_ == state)
        return;
    state_ = state;
    if (consumer_)
        consumer_(state_);
}

void BindProperty::syncState(bool state) { state_ = state; }

void BindProperty::onActive(int action) {
    if (keyOnly_) return;
    if (hold_) {
        // Release must work even when a menu now blocks activation, and must
        // never invert an already disabled state back to enabled.
        if (action == 0) {
            holdPressed_ = false;
            setState(false);
            return;
        }
        if (holdPressed_ || (gate_ && !gate_())) return;
        holdPressed_ = true;
        setState(true);
        return;
    }
    if (action == 0) return;
    if (gate_ && !gate_())
        return;
    setState(!isActive());
}

void BindProperty::reconcileHold(bool physicalDown) {
    if (hold_ && holdPressed_ && !physicalDown) onActive(0);
}

void BindProperty::save(Properties::Json& out) const {
    out = Properties::Json::Object();
    out["keycode"] = keybind_;
    out["requireHold"] = hold_;
}

void BindProperty::load(const Properties::Json& in) {
    if (!in.isObject())
        return;

    if (const Properties::Json* keycode = in.Find("keycode"))
        setKeybind(static_cast<int>(keycode->asNumber(keybind_)));
    if (const Properties::Json* requireHold = in.Find("requireHold"))
        setHold(requireHold->asBool(hold_));
}

}
