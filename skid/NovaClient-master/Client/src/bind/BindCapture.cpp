#include "bind/BindCapture.h"

#include "bind/BindProperty.h"
#include "bind/InputManager.h"
#include "bind/UnloadKey.h"
#include "core/Instance.h"

namespace nova::bind {

BindCapture& BindCapture::Get() {
    return Instance<BindCapture>([] { return new BindCapture(); });
}

BindCapture::BindCapture() {
    InputManager::Get().Register(this);
}

void BindCapture::Begin(BindProperty* target) {
    unload::EndCapture();
    target_ = target;
    unload::BeginCapture(target);
}

void BindCapture::Cancel() {
    unload::EndCapture();
    target_ = nullptr;
    pressing_ = false;
}

bool BindCapture::onKey(int key, bool state) {
    if (target_ == nullptr)
        return true;

    if (state) {
        target_->setKeybind(key);
        Cancel();
    }
    return false;
}

bool BindCapture::onMouseButton(int button, bool state) {
    if (target_ == nullptr)
        return true;

    if (state) {
        if (button != 0)
            target_->setKeybind(-1 - button);
        if (!pressing_)
            Cancel();
    }
    return false;
}

}
