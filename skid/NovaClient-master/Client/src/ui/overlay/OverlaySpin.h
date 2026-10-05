#pragma once

#include <atomic>
#include <immintrin.h>

namespace nova::ui {

// The list a module fills on the game thread and draws from on the render thread --
// separate threads here, because Ixeris moves the frame loop off the game thread, so
// the two cannot simply share a vector.
//
// A spin lock over std::atomic_flag rather than a std::mutex. The rest of the client
// synchronises with bare atomics and no mutex has ever been used in it; the one this
// started as faulted inside the C++ runtime on first lock, which is what a
// standard-library mutex does when the runtime behind it has not been set up the way
// a normally-loaded DLL expects. An atomic flag is a compiler intrinsic and needs
// none of that.
//
// The pause is _mm_pause rather than YieldProcessor so this header does not have to
// drag windows.h in behind it: windows.h defines DrawText as a macro and has to
// arrive after the properties and before the text helper, which is an ordering
// nothing shared should be able to disturb. It is the same instruction.
//
// The critical sections are a list copy, so spinning is measured in instructions.
class SpinLock {
public:
    void lock() {
        while (flag_.test_and_set(std::memory_order_acquire))
            _mm_pause();
    }
    void unlock() { flag_.clear(std::memory_order_release); }

private:
    std::atomic_flag flag_ = ATOMIC_FLAG_INIT;
};

class ScopedSpin {
public:
    explicit ScopedSpin(SpinLock& lock) : lock_(lock) { lock_.lock(); }
    ~ScopedSpin() { lock_.unlock(); }
    ScopedSpin(const ScopedSpin&) = delete;
    ScopedSpin& operator=(const ScopedSpin&) = delete;

private:
    SpinLock& lock_;
};

}
