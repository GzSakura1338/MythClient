#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <mutex>
#include <condition_variable>
#include <cstdio>

// Run against both the launcher-bundled and system MSVC runtimes.
int main() {
    SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX);
    std::mutex mutex;
    std::recursive_mutex recursive;
    std::condition_variable condition;
    for (int i = 0; i < 1000; ++i) {
        std::lock_guard lock(mutex);
        std::lock_guard first(recursive);
        std::lock_guard second(recursive);
        condition.notify_all();
    }
    std::puts("Mutex compatibility PASS");
}
