#pragma once

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <functional>
#include <memory>
#include <mutex>
#include <immintrin.h>
#include <type_traits>
#include <utility>
#include <vector>

namespace nova::event {

class Event {
public:
    virtual ~Event() = default;
};

class CancellableEvent : public Event {
public:
    [[nodiscard]] bool IsCancelled() const { return cancelled_; }
    void SetCancelled(bool cancelled) { cancelled_ = cancelled; }
    void Cancel() { cancelled_ = true; }

private:
    bool cancelled_ = false;
};

namespace Priority {
inline constexpr int Highest = 0;
inline constexpr int High = 5;
inline constexpr int Medium = 10;
inline constexpr int Low = 15;
inline constexpr int Lowest = 20;
}

enum class EventType {
    Pre,
    Post,
    Incoming,
    Outgoing,
};

using TypeId = std::uint32_t;

inline constexpr TypeId InvalidTypeId = static_cast<TypeId>(-1);

TypeId AllocateTypeId();

template <typename E>
TypeId TypeIdOf() {
    static std::atomic<TypeId> slot{InvalidTypeId};
    TypeId id = slot.load(std::memory_order_acquire);
    if (id == InvalidTypeId) {
        const TypeId allocated = AllocateTypeId();
        if (!slot.compare_exchange_strong(id, allocated, std::memory_order_acq_rel)) return id;
        id = allocated;
    }
    return id;
}

class EventBus;
class Subscription;

class EventBus {
public:
    static EventBus& Get();

    EventBus(const EventBus&) = delete;
    EventBus& operator=(const EventBus&) = delete;

    void Seal();

    template <typename E, typename F>
    [[nodiscard]] Subscription Subscribe(F&& handler, int priority = Priority::Medium);

    template <typename E>
    [[nodiscard]] bool Post(E& event);

    template <typename E>
    [[nodiscard]] bool HasListeners() {
        if (!sealed_.load(std::memory_order_acquire)) return false;
        const auto list = Snapshot(TypeIdOf<E>());
        if (!list) return false;
        for (const auto& listener : *list) {
            if (listener->alive.load(std::memory_order_acquire)) return true;
        }
        return false;
    }

private:
    friend class Subscription;

    struct Listener {
        std::uint64_t id = 0;
        int priority = Priority::Medium;
        std::function<void(Event&)> handler;
        std::atomic<bool> alive{true};
    };

    EventBus() = default;

    using List = std::vector<std::shared_ptr<Listener>>;
    using ListSnapshot = std::shared_ptr<const List>;
    // Protect only publication/copying of native pointers. Never hold this lock
    // during a handler: JNI calls can re-enter the bus or wait for other threads.
    struct PublicationLock {
        void lock() { while (flag.test_and_set(std::memory_order_acquire)) _mm_pause(); }
        void unlock() { flag.clear(std::memory_order_release); }
        std::atomic_flag flag = ATOMIC_FLAG_INIT;
    };
    ListSnapshot Snapshot(TypeId type);
    void Add(TypeId type, const std::shared_ptr<Listener>& listener);
    void Unsubscribe(TypeId type, std::uint64_t id);
    PublicationLock lock_;
    std::deque<ListSnapshot> lists_;
    std::atomic<std::uint64_t> nextId_{1};
    std::atomic<bool> sealed_{false};
};

class Subscription {
public:
    Subscription() = default;
    ~Subscription() { Reset(); }

    Subscription(const Subscription&) = delete;
    Subscription& operator=(const Subscription&) = delete;

    Subscription(Subscription&& other) noexcept
        : bus_(other.bus_), type_(other.type_), id_(other.id_) {
        other.bus_ = nullptr;
    }

    Subscription& operator=(Subscription&& other) noexcept {
        if (this != &other) {
            Reset();
            bus_ = other.bus_;
            type_ = other.type_;
            id_ = other.id_;
            other.bus_ = nullptr;
        }
        return *this;
    }

    void Reset() {
        if (bus_) {
            bus_->Unsubscribe(type_, id_);
            bus_ = nullptr;
        }
    }

    [[nodiscard]] bool IsValid() const { return bus_ != nullptr; }

private:
    friend class EventBus;

    Subscription(EventBus* bus, TypeId type, std::uint64_t id)
        : bus_(bus), type_(type), id_(id) {}

    EventBus* bus_ = nullptr;
    TypeId type_ = 0;
    std::uint64_t id_ = 0;
};

template <typename E, typename F>
Subscription EventBus::Subscribe(F&& handler, int priority) {
    static_assert(std::is_base_of_v<Event, E>,
                  "an event must derive from nova::event::Event");
    static_assert(std::is_invocable_v<std::decay_t<F>&, E&>,
                  "the handler has to be callable with the type it subscribes to");

    const std::uint64_t id = nextId_.fetch_add(1, std::memory_order_relaxed);
    std::function<void(Event&)> erased =
        [fn = std::decay_t<F>(std::forward<F>(handler))](Event& event) mutable {
            fn(static_cast<E&>(event));
        };

    auto listener = std::make_shared<Listener>();
    listener->id = id;
    listener->priority = priority;
    listener->handler = std::move(erased);
    Add(TypeIdOf<E>(), listener);

    return Subscription(this, TypeIdOf<E>(), id);
}

template <typename E>
bool EventBus::Post(E& event) {
    static_assert(std::is_base_of_v<Event, E>,
                  "an event must derive from nova::event::Event");

    if (sealed_.load(std::memory_order_acquire)) {
        const auto list = Snapshot(TypeIdOf<E>());
        if (list) {
            for (const auto& listener : *list) {
                if (listener->alive.load(std::memory_order_acquire))
                    listener->handler(event);
            }
        }
    }

    if constexpr (std::is_base_of_v<CancellableEvent, E>)
        return event.IsCancelled();
    else
        return false;
}

}
