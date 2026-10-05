#pragma once
#include "smtc/backend/Async.h"
#include "smtc/MediaSnapshot.h"
#include "smtc/MediaCommand.h"
namespace nova::smtc::backend {
class WindowsSession {
public:
    explicit WindowsSession(std::shared_ptr<Signal> signal);
    ~WindowsSession();
    MediaSnapshot Refresh(const std::string& source, bool lyricsEnabled = false);
    bool Execute(const MediaCommand& command);
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
}
