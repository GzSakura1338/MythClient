#include <Windows.h>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include <cstdio>
#include <stdexcept>
#include <thread>
#include "../../../shared/Bootstrap.h"

std::mutex probeMutex;
extern "C" decltype(&_Mtx_lock) __imp__Mtx_lock;

extern "C" void InitializeBase(void* parameter) {
    auto* boot = static_cast<NovaBootstrap*>(parameter);
    HMODULE importedRuntime = nullptr;
    if (!GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
        reinterpret_cast<LPCSTR>(__imp__Mtx_lock), &importedRuntime)) {
        std::snprintf(boot->message, sizeof(boot->message), "Runtime address lookup failed: %lu", GetLastError());
        return;
    }
    const bool system = importedRuntime != GetModuleHandleA("msvcp140.dll");
    if (system != (boot->client == NovaClient_EaseCation || boot->client == NovaClient_Forge1201)) {
        GetModuleFileNameA(importedRuntime, boot->message, sizeof(boot->message));
        return;
    }
    if (system) {
        std::unique_lock lock(probeMutex);
        std::recursive_mutex recursive;
        std::lock_guard first(recursive);
        std::lock_guard second(recursive);
        std::condition_variable condition;
        if (condition.wait_for(lock, std::chrono::milliseconds(1)) != std::cv_status::timeout) return;
        condition.notify_all();
        lock.unlock();
        unsigned lockedCount = 0;
        auto increment = [&] {
            for (unsigned i = 0; i < 1000; ++i) {
                std::lock_guard guard(probeMutex);
                ++lockedCount;
            }
        };
        std::thread worker(increment);
        increment();
        worker.join();
        if (lockedCount != 2000) return;
        // A failed operation must unwind a Nova-owned lock correctly with the
        // imported runtime, not terminate or leave the mutex owned.
        bool caught = false;
        try {
            std::lock_guard guard(probeMutex);
            throw std::runtime_error("runtime compatibility probe");
        } catch (const std::runtime_error&) { caught = true; }
        if (!caught || !probeMutex.try_lock()) return;
        probeMutex.unlock();
    }
    boot->state = NovaBoot_Ready;
}
