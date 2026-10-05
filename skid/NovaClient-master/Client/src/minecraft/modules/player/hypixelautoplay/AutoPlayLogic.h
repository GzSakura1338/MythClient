#pragma once
#include <algorithm>
#include <string>
#include <string_view>
#include <vector>

namespace nova::module::autoplay {
inline bool IsReplayPrompt(std::string_view text) {
    return text.find("play again?") != std::string_view::npos;
}
inline bool IsPlayCommand(std::string_view command) {
    // Rise's /play prefix and no-dot check; require an actual command boundary
    // and reject control characters rather than executing a lookalike command.
    if (!command.starts_with("/play ") || command.size() <= 6 || command.find('.') != command.npos)
        return false;
    for (unsigned char c : command) if (c < 32 || c == 127) return false;
    return true;
}

class ReplayQueue {
public:
    void Clear() { pending_.clear(); }
    void Add(const std::vector<std::string>& commands, long long now, int delayMs) {
        for (const auto& command : commands) {
            if (!IsPlayCommand(command) || pending_.size() >= 64) continue;
            // Repeated server prompts must not restart the countdown or send twice.
            if (std::any_of(pending_.begin(), pending_.end(), [&](const Pending& p) { return p.command == command; })) continue;
            pending_.push_back({command, now + std::clamp(delayMs, 0, 30000)});
        }
    }
    std::vector<std::string> TakeReady(long long now) {
        std::vector<std::string> ready;
        for (auto it = pending_.begin(); it != pending_.end();) {
            if (now < it->due) { ++it; continue; }
            ready.push_back(std::move(it->command));
            it = pending_.erase(it);
        }
        return ready;
    }
private:
    struct Pending { std::string command; long long due; };
    std::vector<Pending> pending_;
};
}
