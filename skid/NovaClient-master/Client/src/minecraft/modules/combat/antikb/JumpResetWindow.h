#pragma once

#include <cstdint>

namespace nova::module::antikb {

class JumpResetWindow {
public:
    void Reset() { pending_ = false; lastJumpTick_ = -1; }

    void Arm(std::uint64_t now, int tick, bool eligible) {
        pending_ = eligible;
        receivedAt_ = now;
        receivedTick_ = tick;
    }

    bool Consume(std::uint64_t now, int tick, bool eligible, int cooldownTicks) {
        const bool pending = pending_;
        pending_ = false;
        if (!pending || !eligible || now < receivedAt_ || now - receivedAt_ > 150 ||
            tick < receivedTick_ || tick - receivedTick_ > 1) return false;
        if (lastJumpTick_ >= 0 && (tick <= lastJumpTick_ || tick - lastJumpTick_ < cooldownTicks))
            return false;
        lastJumpTick_ = tick;
        return true;
    }

private:
    bool pending_ = false;
    std::uint64_t receivedAt_ = 0;
    int receivedTick_ = 0;
    int lastJumpTick_ = -1;
};

}
