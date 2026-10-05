#pragma once
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <windows.foundation.h>
#include <wrl.h>
#include <atomic>
#include <memory>
#include <stdexcept>
#include <string>

namespace nova::smtc::backend {
using Microsoft::WRL::ComPtr;
struct Signal {
    HANDLE wake = CreateEventW(nullptr, FALSE, FALSE, nullptr);
    std::atomic<bool> stop{false};
    std::atomic<unsigned> dirty{7}, delegates{0};
    std::atomic<std::uint64_t> mediaRevision{0};
    ~Signal() { if (wake) CloseHandle(wake); }
    void Notify(unsigned mask) { dirty.fetch_or(mask); if (mask & 2) ++mediaRevision; SetEvent(wake); }
};
struct DelegateLease {
    std::shared_ptr<Signal> signal;
    explicit DelegateLease(std::shared_ptr<Signal> s) : signal(std::move(s)) { ++signal->delegates; }
    ~DelegateLease() { --signal->delegates; }
};
inline void Require(HRESULT hr) {
    if (FAILED(hr)) throw std::runtime_error("SMTC HRESULT " + std::to_string(static_cast<unsigned long>(hr)));
}
// No completion delegate is registered. Cancellation cannot schedule DLL code.
inline bool Await(IUnknown* operation, const std::shared_ptr<Signal>& signal, unsigned timeout = 3000) {
    if (!operation) return false;
    ComPtr<ABI::Windows::Foundation::IAsyncInfo> info;
    Require(operation->QueryInterface(IID_PPV_ARGS(&info)));
    const auto deadline = GetTickCount64() + timeout;
    for (;;) {
        ABI::Windows::Foundation::AsyncStatus status{};
        Require(info->get_Status(&status));
        if (status != ABI::Windows::Foundation::AsyncStatus::Started)
            return status == ABI::Windows::Foundation::AsyncStatus::Completed && !signal->stop.load();
        if (signal->stop.load() || GetTickCount64() >= deadline) { info->Cancel(); return false; }
        WaitForSingleObject(signal->wake, 20);
    }
}
}
