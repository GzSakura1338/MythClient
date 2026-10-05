#include "ui/notifications/Notifications.h"

#include <chrono>
#include <deque>
#include <mutex>
#include <utility>

#include "core/Instance.h"

namespace nova::notifications {
namespace {
using Clock = std::chrono::steady_clock;
struct Pending {
    Message message;
    Clock::time_point posted;
};
struct Queue {
    std::mutex mutex;
    std::deque<Pending> pending;
};
}

void Post(std::string text, Type type, Source source) {
    if (text.empty()) return;
    // Bound both memory use and text measurement work, preserving UTF-8 boundaries.
    if (text.size() > 512) {
        std::size_t end = 512;
        while (end > 0 && (static_cast<unsigned char>(text[end]) & 0xC0) == 0x80) --end;
        text.resize(end);
        text += "...";
    }
    for (char& c : text)
        if (c == '\n' || c == '\r' || c == '\t') c = ' ';
    Queue& queue = Instance<Queue>();
    std::lock_guard lock(queue.mutex);
    if (queue.pending.size() >= 32) queue.pending.pop_front();
    queue.pending.push_back({{std::move(text), type, source}, Clock::now()});
}

std::vector<Message> Drain() {
    Queue& queue = Instance<Queue>();
    std::lock_guard lock(queue.mutex);
    std::vector<Message> result;
    const auto now = Clock::now();
    for (auto& pending : queue.pending)
        if (now - pending.posted < std::chrono::seconds(10))
            result.push_back(std::move(pending.message));
    queue.pending.clear();
    return result;
}

}
