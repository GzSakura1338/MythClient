#include "cs2/hooks/RadarHook.h"
#include "cs2/sdk/radar/Radar.h"
#include "cs2/modules/render/radaresp/RadarESP.h"
#include "MinHook.h"
#include <windows.h>
#include <array>
#include <atomic>
#include <thread>
#include <cstdio>
#include <cstdlib>

#define CHECK(x) do { if (!(x)) { fprintf(stderr, "Radar test failed at %d: %s\n", __LINE__, #x); std::exit(1); } } while(false)
namespace {
std::atomic<unsigned> observed{0};
std::atomic<bool> block{false}, entered{false}, releaseCall{false};
bool throwFault = false;
#pragma optimize("", off)
__declspec(noinline) void __fastcall GameRadar(void* hud) {
    auto* flags = static_cast<unsigned char*>(hud) + nova::cs2::sdk::radar::displayFlags;
    observed.store(*flags);
    *flags |= 0x40; // A different HUD flag changed by the original must survive.
    if (throwFault) RaiseException(0xE0424242, 0, 0, nullptr);
    if (block.load()) {
        entered.store(true);
        while (!releaseCall.load()) Sleep(1);
    }
}
#pragma optimize("", on)
using Update = void (__fastcall*)(void*);
Update volatile invoke = GameRadar;
bool Fault(void* hud) {
    __try { invoke(hud); return false; }
    __except(GetExceptionCode() == 0xE0424242 ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) { return true; }
}
}
void TestRadar() {
    using namespace nova::cs2;
    CHECK(MH_Initialize() == MH_OK);
    CHECK(!hooks::radar::Install(nullptr));
    CHECK(hooks::radar::Install(reinterpret_cast<void*>(&GameRadar)));
    std::array<unsigned char, sdk::radar::displayFlags + 1> hud{};
    auto& flags = hud.back();
    modules::RadarESP module;
    flags = 0x20; invoke(hud.data()); CHECK(observed == 0x20 && flags == 0x60);
    module.setEnabled(true); module.tick();
    flags = 0x20; invoke(hud.data()); CHECK(observed == 0x21 && flags == 0x60);
    flags = 0x21; invoke(hud.data()); CHECK(observed == 0x21 && flags == 0x61);
    throwFault = true; flags = 0x20;
    CHECK(Fault(hud.data()) && flags == 0x60); // Restoration during exception unwind.
    throwFault = false;
    module.setEnabled(false); module.tick();
    flags = 0x20; invoke(hud.data()); CHECK(observed == 0x20);
    module.setEnabled(true); module.tick();
    block = true; entered = false; releaseCall = false; flags = 0x20;
    std::thread pending([&] { invoke(hud.data()); });
    for (int i = 0; i < 1000 && !entered; ++i) Sleep(1);
    CHECK(entered);
    CHECK(!hooks::radar::Stop(10)); // Cannot release trampoline while original runs.
    releaseCall = true; pending.join(); CHECK(flags == 0x60);
    CHECK(hooks::radar::Stop(1000)); hooks::radar::Finalize();
    module.shutdown(); block = false;
    CHECK(hooks::radar::Install(reinterpret_cast<void*>(&GameRadar)));
    flags = 0x20; invoke(hud.data()); CHECK(observed == 0x20);
    CHECK(hooks::radar::Stop(1000)); hooks::radar::Finalize();
    CHECK(MH_Uninitialize() == MH_OK);
    puts("PASS: native radar hook, module toggle, original flags, exception restore, callback drain, reinstall; no game injection");
}
