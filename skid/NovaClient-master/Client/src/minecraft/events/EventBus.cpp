#include "minecraft/events/EventBus.h"

#include <algorithm>

#include "core/Instance.h"

namespace nova::event {

TypeId AllocateTypeId() {
    static std::atomic<TypeId> next{0};
    return next.fetch_add(1, std::memory_order_relaxed);
}

EventBus& EventBus::Get() {
    return Instance<EventBus>([] { return new EventBus(); });
}

void EventBus::Seal() { sealed_.store(true, std::memory_order_release); }

EventBus::ListSnapshot EventBus::Snapshot(TypeId type) {
    std::lock_guard guard(lock_);
    return type < lists_.size() ? lists_[type] : nullptr;
}

void EventBus::Add(TypeId type, const std::shared_ptr<Listener>& listener) {
    ListSnapshot retired;
    {
        std::lock_guard guard(lock_);
        if (type >= lists_.size()) lists_.resize(type + 1);
        retired = lists_[type];
        auto next = retired ? std::make_shared<List>(*retired) : std::make_shared<List>();
        std::erase_if(*next, [](const auto& entry) { return !entry->alive.load(std::memory_order_acquire); });
        const auto at = std::upper_bound(next->begin(), next->end(), listener->priority,
            [](int priority, const auto& entry) { return priority < entry->priority; });
        next->insert(at, listener);
        lists_[type] = std::move(next);
    }
    // Destroy retired handlers outside the lock too: a captured Subscription
    // can unsubscribe from this bus when its handler is finally released.
}

void EventBus::Unsubscribe(TypeId type, std::uint64_t id) {
    const auto list = Snapshot(type);
    if (!list) return;
    for (const auto& listener : *list) {
        if (listener->id == id) {
            listener->alive.store(false, std::memory_order_release);
            return;
        }
    }
}

}
