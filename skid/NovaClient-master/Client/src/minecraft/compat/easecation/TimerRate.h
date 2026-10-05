#pragma once
#include <atomic>
#include <cmath>

namespace nova::compat::easecation {
class TimerRate {
    std::atomic<float> speed_{1.0f};
public:
    bool Set(float speed) {
        if (!std::isfinite(speed) || speed <= 0.0f) return false;
        speed_.store(speed, std::memory_order_relaxed);
        return true;
    }
    float Millis(float vanilla) const {
        return vanilla / speed_.load(std::memory_order_relaxed);
    }
    void Reset() { speed_.store(1.0f, std::memory_order_relaxed); }
};
}
