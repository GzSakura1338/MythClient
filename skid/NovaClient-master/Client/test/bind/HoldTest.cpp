#include "bind/BindProperty.h"
#include "bind/BindableManager.h"
#include <cassert>
#include <iostream>

// Registration is inert in this native test; exercise the real property and
// serialization without a window/input-manager singleton.
namespace nova::bind {
BindableManager::BindableManager() = default;
BindableManager& BindableManager::Get() { static BindableManager manager; return manager; }
void BindableManager::Register(IBindable*) {}
void BindableManager::Unregister(IBindable*) {}
bool BindableManager::onKey(int, bool) { return true; }
bool BindableManager::onMouseButton(int, bool) { return true; }
}
int main() {
    for (int key : {65, -5}) {
        nova::bind::BindProperty bind(key, true);
        bool allowed = true, enabled = false;
        int changes = 0;
        bind.setGate([&] { return allowed; });
        bind.setStateChangeConsumer([&](bool v) { enabled=v; ++changes; });
        bind.onActive(1); assert(enabled);
        bind.onActive(1); assert(enabled && changes==1);
        allowed=false; bind.onActive(0); assert(!enabled);
        bind.onActive(0); assert(!enabled && changes==2);
        bind.onActive(1); allowed=true; bind.onActive(0);
        assert(!enabled); // Press blocked in GUI, release after GUI closes.
        bind.onActive(1); bind.syncState(false); enabled=false;
        bind.onActive(0); assert(!enabled); // Module disabled itself during hold.
        bind.onActive(1); bind.reconcileHold(true); assert(enabled);
        bind.reconcileHold(false); assert(!enabled); // Lost key-up / focus.
        bind.reconcileHold(true); assert(!enabled); // No auto-enable on refocus.
        bind.onActive(1); bind.setKeybind(66); assert(!enabled);
        bind.setState(true); bind.reconcileHold(false); assert(enabled); // GUI toggle.
        bind.setHold(false); assert(!enabled);
        bind.onActive(1); assert(enabled);
        bind.onActive(0); assert(enabled);
        bind.onActive(1); assert(!enabled); // Toggle mode unchanged.
    }
    std::cout << "PASS: hold press/release, blocked input, repeats, self-disable, lost release, rebind and toggle\n";
}
