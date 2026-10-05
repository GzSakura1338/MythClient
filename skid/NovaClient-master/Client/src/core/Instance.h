#pragma once

#include <atomic>

namespace nova {

template <typename T, typename F>
T& Instance(F create) {
    static std::atomic<T*> slot{nullptr};
    T* current = slot.load(std::memory_order_acquire);
    if (current == nullptr) {
        T* const created = create();
        if (slot.compare_exchange_strong(current, created, std::memory_order_acq_rel)) {
            current = created;
        }
    }
    return *current;
}

template <typename T>
T& Instance() {
    return Instance<T>([] { return new T(); });
}

}
