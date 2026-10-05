#include "smtc/SmtcService.h"
#include "smtc/backend/WindowsSession.h"
#include "core/Instance.h"
#include <roapi.h>
#include <mutex>

namespace nova::smtc {
namespace {
struct Service {
    std::mutex mutex;
    std::shared_ptr<backend::Signal> signal = std::make_shared<backend::Signal>();
    HANDLE thread{};
    bool shutdown{};
    std::string source = "Auto";
    bool lyricsEnabled{};
    std::uint64_t sourceRevision{};
    std::deque<MediaCommand> commands;
    std::shared_ptr<const MediaSnapshot> snapshot = std::make_shared<MediaSnapshot>();
    void Publish(MediaSnapshot value) { std::lock_guard lock(mutex); snapshot = std::make_shared<MediaSnapshot>(std::move(value)); }
};
Service& Get() { return Instance<Service>(); }
DWORD WINAPI Worker(void*) {
    auto& service = Get(); auto signal = service.signal;
    const HRESULT initialized = RoInitialize(RO_INIT_MULTITHREADED);
    try {
        backend::Require(initialized);
        std::unique_ptr<backend::WindowsSession> backend;
        while (!signal->stop.load()) {
            try {
                if (!backend) backend = std::make_unique<backend::WindowsSession>(signal);
                std::string source; std::deque<MediaCommand> commands; std::uint64_t revision{}; bool lyricsEnabled{};
                { std::lock_guard lock(service.mutex); source = service.source; revision = service.sourceRevision; lyricsEnabled = service.lyricsEnabled; commands.swap(service.commands); }
                auto snapshot = backend->Refresh(source, lyricsEnabled);
                for (const auto& command : commands) {
                    if (signal->stop.load()) break;
                    { std::lock_guard lock(service.mutex); if (revision != service.sourceRevision) break; }
                    if (!backend->Execute(command)) snapshot.error = "Player could not perform this action";
                }
                { std::lock_guard lock(service.mutex);
                    if (!signal->stop.load() && revision == service.sourceRevision)
                        service.snapshot = std::make_shared<MediaSnapshot>(std::move(snapshot));
                }
            } catch (const std::exception& error) {
                backend.reset();
                MediaSnapshot failed; failed.state = State::Unavailable; failed.error = error.what();
                if (!signal->stop.load()) service.Publish(std::move(failed));
                if (!signal->stop.load()) WaitForSingleObject(signal->wake, 2000);
            }
            // NetEase's missing timeline is sampled, never extrapolated. Poll it four times/sec.
            unsigned interval = 1000;
            { std::lock_guard lock(service.mutex);
                if (_stricmp(service.snapshot->source.c_str(), "cloudmusic.exe") == 0 &&
                    (!service.snapshot->HasDuration() || service.snapshot->measuredTimeline)) interval = 250;
            }
            if (!signal->stop.load() && signal->dirty.load() == 0) WaitForSingleObject(signal->wake, interval);
        }
        backend.reset();
    } catch (const std::exception& error) {
        MediaSnapshot failed; failed.state = State::Unavailable; failed.error = error.what(); service.Publish(std::move(failed));
    } catch (...) { MediaSnapshot failed; failed.state = State::Unavailable; service.Publish(std::move(failed)); }
    if (SUCCEEDED(initialized)) RoUninitialize();
    if (signal->stop.load()) service.Publish({});
    return 0;
}
}
void Start() {
    auto& service = Get(); std::lock_guard lock(service.mutex);
    if (service.shutdown || !service.signal->wake) return;
    if (service.thread) {
        if (WaitForSingleObject(service.thread, 0) != WAIT_OBJECT_0 || service.signal->delegates.load()) return;
        CloseHandle(service.thread); service.thread = nullptr;
    }
    service.signal->stop.store(false); service.signal->dirty.store(7); service.commands.clear();
    MediaSnapshot starting; starting.state = State::Starting; service.snapshot = std::make_shared<MediaSnapshot>(starting);
    service.thread = CreateThread(nullptr, 0, Worker, nullptr, 0, nullptr);
    if (!service.thread) { starting.state = State::Unavailable; service.snapshot = std::make_shared<MediaSnapshot>(starting); }
}
void SelectSource(std::string source) {
    auto& service = Get(); std::lock_guard lock(service.mutex);
    if (service.shutdown) return;
    if (source == service.source) return;
    service.source = std::move(source); ++service.sourceRevision;
    service.commands.clear(); service.signal->Notify(7);
    MediaSnapshot starting; starting.state = State::Starting;
    starting.sources = service.snapshot->sources;
    service.snapshot = std::make_shared<MediaSnapshot>(std::move(starting));
}
void SetLyricsEnabled(bool enabled) {
    auto& service = Get(); std::lock_guard lock(service.mutex);
    if (service.shutdown || enabled == service.lyricsEnabled) return;
    service.lyricsEnabled = enabled; ++service.sourceRevision; service.signal->Notify(4);
}
std::shared_ptr<const MediaSnapshot> ReadSnapshot() { auto& service = Get(); std::lock_guard lock(service.mutex); return service.snapshot; }
bool Enqueue(MediaCommand command) {
    auto& service = Get(); std::lock_guard lock(service.mutex);
    if (service.shutdown || service.signal->stop.load() || service.snapshot->state != State::Ready ||
        command.session != service.snapshot->session || command.track != service.snapshot->track) return false;
    const bool queued = QueueCommand(service.commands, command);
    if (queued) SetEvent(service.signal->wake);
    return queued;
}
void RequestStop() {
    auto& service = Get(); std::lock_guard lock(service.mutex);
    service.signal->stop.store(true); service.commands.clear();
    if (service.signal->wake) SetEvent(service.signal->wake);
}
void BeginShutdown() {
    auto& service = Get(); std::lock_guard lock(service.mutex);
    service.shutdown = true; service.signal->stop.store(true); service.commands.clear();
    if (service.signal->wake) SetEvent(service.signal->wake);
}
bool WaitStopped(unsigned timeoutMs) {
    auto& service = Get(); const auto deadline = GetTickCount64() + timeoutMs;
    for (;;) {
        { std::lock_guard lock(service.mutex);
            if ((!service.thread || WaitForSingleObject(service.thread, 0) == WAIT_OBJECT_0) && !service.signal->delegates.load()) {
                if (service.thread) { CloseHandle(service.thread); service.thread = nullptr; }
                if (service.shutdown && service.signal->wake) {
                    CloseHandle(service.signal->wake); service.signal->wake = nullptr;
                    service.snapshot = std::make_shared<MediaSnapshot>();
                }
                return true;
            }
        }
        if (GetTickCount64() >= deadline) return false;
        Sleep(10);
    }
}
}
