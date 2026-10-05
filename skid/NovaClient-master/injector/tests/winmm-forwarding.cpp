#include <windows.h>
#include <mmsystem.h>
#include <cstdio>
#include <cstdlib>
#include <thread>
#include <vector>
#include "winmm-exports.h"

int wmain(int argc, wchar_t** argv) {
    if (argc != 2) return 2;
    HMODULE proxy = LoadLibraryW(argv[1]);
    wchar_t path[MAX_PATH];
    GetSystemDirectoryW(path, MAX_PATH);
    wcscat_s(path, L"\\winmm.dll");
    HMODULE system = LoadLibraryW(path);
    if (!proxy || !system || proxy == system) return 3;
    for (unsigned i = 0; i < NOVA_WINMM_EXPORT_COUNT; ++i)
        if (!GetProcAddress(proxy, exportNames[i]) || !GetProcAddress(system, exportNames[i])) return 4;
    auto timer = reinterpret_cast<decltype(&timeGetTime)>(GetProcAddress(proxy, "timeGetTime"));
    std::vector<std::thread> threads;
    for (int i = 0; i < 16; ++i) threads.emplace_back([timer]() {
        for (int n = 0; n < 1000; ++n) if (!timer()) std::abort();
    });
    for (auto& thread : threads) thread.join();
    auto begin = reinterpret_cast<decltype(&timeBeginPeriod)>(GetProcAddress(proxy, "timeBeginPeriod"));
    auto end = reinterpret_cast<decltype(&timeEndPeriod)>(GetProcAddress(proxy, "timeEndPeriod"));
    if (begin(1) != TIMERR_NOERROR || end(1) != TIMERR_NOERROR) return 5;
    auto openProxy = reinterpret_cast<decltype(&waveOutOpen)>(GetProcAddress(proxy, "waveOutOpen"));
    auto openSystem = reinterpret_cast<decltype(&waveOutOpen)>(GetProcAddress(system, "waveOutOpen"));
    WAVEFORMATEX format{WAVE_FORMAT_PCM, 2, 44100, 176400, 4, 16, 0};
    // Six arguments exercise register preservation plus the untouched stack args.
    MMRESULT expected = openSystem(nullptr, 0xffff1234, &format, 0, 0, WAVE_FORMAT_QUERY);
    if (openProxy(nullptr, 0xffff1234, &format, 0, 0, WAVE_FORMAT_QUERY) != expected) return 6;
    std::puts("PASS all 181 exports, concurrent first-call resolution, timer calls and six-argument forwarding");
    FreeLibrary(proxy);
    return 0;
}
