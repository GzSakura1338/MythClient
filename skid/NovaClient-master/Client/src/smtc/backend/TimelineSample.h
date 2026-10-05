#pragma once
#include "smtc/MediaSnapshot.h"

namespace nova::smtc::backend {
struct TimelineSample {
    std::uint64_t generation{}, object{};
    std::string playId, songId;
    double position{}, duration{};
    bool SameTrack(const TimelineSample& other) const {
        return generation == other.generation && object == other.object &&
            playId == other.playId && songId == other.songId;
    }
    bool Valid() const {
        return generation && object && !songId.empty() && playId.starts_with(songId + "_") &&
            std::isfinite(position) && std::isfinite(duration) && duration > 0 && duration <= 86400 &&
            position >= 0 && position <= duration + 0.5;
    }
};
// The progress publisher and current track are updated on different threads.
// Discard transitional samples, including a new track still carrying old progress.
class TimelineGate {
    TimelineSample previous_;
    Clock::time_point since_{};
public:
    void Reset() { previous_ = {}; since_ = {}; }
    bool Accept(const TimelineSample& sample, Clock::time_point now) {
        if (!sample.Valid()) { Reset(); return false; }
        if (!sample.SameTrack(previous_) || sample.duration != previous_.duration) {
            previous_ = sample; since_ = now; return false;
        }
        return now - since_ >= std::chrono::seconds(1);
    }
};
}
