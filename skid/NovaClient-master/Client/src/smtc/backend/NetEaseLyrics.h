#pragma once
#include "smtc/backend/Async.h"
#include "smtc/MediaSnapshot.h"

namespace nova::smtc::backend {
class NetEaseLyrics {
public:
    std::shared_ptr<const Lyrics> Get(const std::string& song, const std::shared_ptr<Signal>& signal);
    void Reset();
private:
    std::string song_;
    std::shared_ptr<const Lyrics> lyrics_;
    Clock::time_point retry_{};
};
}
