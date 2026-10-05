#pragma once

#include <algorithm>
#include <cstdint>
#include <deque>

namespace nova::module::autoblock {

class BlockCoverage {
public:
    void Start(std::uint64_t now, bool blocking) {
        changes_.clear();
        changes_.push_back({now, blocking});
        started_ = now;
    }

    void Record(std::uint64_t now, bool blocking) {
        if (changes_.empty()) return;
        Trim(now);
        if (changes_.back().blocking == blocking) return;
        if (changes_.back().time == now) changes_.back().blocking = blocking;
        else changes_.push_back({now, blocking});
    }

    float Percent(std::uint64_t now) {
        if (changes_.empty() || now <= started_) return 0;
        Trim(now);
        const auto begin = std::max(started_, now > 5000 ? now - 5000 : 0);
        std::uint64_t blocked = 0;
        for (std::size_t i = 0; i < changes_.size(); ++i) {
            const auto left = std::max(begin, changes_[i].time);
            const auto right = i + 1 < changes_.size() ? changes_[i + 1].time : now;
            if (changes_[i].blocking && right > left) blocked += right - left;
        }
        return std::clamp(100.0f * static_cast<float>(blocked) /
            static_cast<float>(now - begin), 0.0f, 100.0f);
    }

private:
    void Trim(std::uint64_t now) {
        const auto cutoff = now > 5000 ? now - 5000 : 0;
        while (changes_.size() > 1 && changes_[1].time <= cutoff) changes_.pop_front();
    }

    struct Change { std::uint64_t time; bool blocking; };
    std::deque<Change> changes_;
    std::uint64_t started_ = 0;
};

}
