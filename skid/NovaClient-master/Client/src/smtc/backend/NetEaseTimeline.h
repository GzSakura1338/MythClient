#pragma once
#include "smtc/MediaSnapshot.h"
#include <memory>

namespace nova::smtc::backend {
// Worker-thread only. Read-only compatibility adapter for NetEase's empty SMTC timeline.
class NetEaseTimeline {
public:
    NetEaseTimeline();
    ~NetEaseTimeline();
    bool Apply(MediaSnapshot& snapshot);
    void Reset();
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
}
