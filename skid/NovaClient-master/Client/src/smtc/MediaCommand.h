#pragma once
#include <cstdint>
#include <deque>
#include <cmath>
namespace nova::smtc {
enum class Action { Toggle, Previous, Next, Seek };
struct MediaCommand { Action action{}; std::uint64_t session{}, track{}; double position{}; };
inline bool QueueCommand(std::deque<MediaCommand>& queue, MediaCommand command) {
    if (!command.session || (command.action == Action::Seek && !std::isfinite(command.position))) return false;
    if (command.action == Action::Seek && !queue.empty() && queue.back().action == Action::Seek &&
        queue.back().session == command.session && queue.back().track == command.track) {
        queue.back() = command; return true;
    }
    if (queue.size() >= 16) return false;
    queue.push_back(command); return true;
}
}
