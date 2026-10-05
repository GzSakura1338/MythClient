#pragma once
#include "smtc/Lyrics.h"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace nova::smtc {
using Clock = std::chrono::steady_clock;
enum class State { Disabled, Starting, NoSession, Ready, Unavailable };
struct Artwork { int width{}, height{}; std::uint64_t version{}; std::vector<std::uint32_t> rgba; };
struct Source { std::string id, name; };
struct MediaSnapshot {
    State state = State::Disabled;
    std::uint64_t session{}, track{};
    std::string source, title, artist, album, error;
    std::vector<Source> sources;
    bool playing{}, canPlay{}, canPause{}, canToggle{}, canPrevious{}, canNext{}, canSeek{};
    bool measuredTimeline{}; // Read-only player fallback; no wall-clock extrapolation.
    std::string songId; // Verified native NetEase identity; empty for other/unknown sources.
    std::shared_ptr<const Lyrics> lyrics;
    double start{}, end{}, minSeek{}, maxSeek{}, position{}, rate = 1;
    Clock::time_point sampled = Clock::now();
    std::shared_ptr<const Artwork> artwork;
    bool HasDuration() const { return std::isfinite(start) && std::isfinite(end) && end > start; }
    double Position(Clock::time_point now = Clock::now()) const {
        double value = position;
        if (playing && std::isfinite(rate)) value += std::max(0.0, std::chrono::duration<double>(now - sampled).count()) * rate;
        if (!std::isfinite(value)) value = start;
        return HasDuration() ? std::clamp(value, start, end) : std::max(start, value);
    }
};
}
