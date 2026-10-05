#include "cs2/sdk/hud/HudSnapshot.h"
#include "cs2/sdk/hud/Offsets.h"
#include "cs2/sdk/players/Offsets.h"
#include <windows.h>
#include <tlhelp32.h>
#include <array>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <limits>
#include <unordered_map>

#define CHECK(x) do { if (!(x)) { std::fprintf(stderr, "HUD test failed at %d: %s\n", __LINE__, #x); std::exit(1); } } while (false)
namespace {
using namespace nova::cs2::sdk;
using Address = std::uintptr_t;
namespace p = players::offsets;
namespace h = hud::offsets;
constexpr Address client = 0x10000000, engine = 0x20000000, network = 0x30000000, system = 0x31000000;
constexpr Address chunk = 0x32000000, controller = 0x40000000, pawn = 0x50000000, scene = 0x60000000;
constexpr Address name = 0x61000000, connection = 0x62000000, server = 0x63000000;
constexpr std::uint32_t controllerHandle = 0x8001, pawnHandle = 0x8002;
struct Fixture {
    std::unordered_map<Address, unsigned char> data;
    Address changing = 0; unsigned reads = 0;
    template<class T> void put(Address a, const T& value) {
        const auto* bytes = reinterpret_cast<const unsigned char*>(&value);
        for (std::size_t i = 0; i < sizeof(T); ++i) data[a + i] = bytes[i];
    }
    void string(Address a, const char* value) {
        std::array<char, 128> text{}; strcpy_s(text.data(), text.size(), value); put(a, text);
    }
    void identity(unsigned index, Address object, std::uint32_t handle) {
        const Address slot = chunk + index * p::stride;
        put(slot, object); put(slot + 0x10, handle); put(slot + 0x30, 0u); put(object + p::identity, slot);
    }
    Fixture() {
        put(engine + p::networkClient, network); put(network + p::signOnState, 6);
        put(network + h::serverCount, 1); put(network + h::serverTick, 1000);
        put(client + p::entitySystem, system); put(system + 0x10, chunk);
        put(client + p::localController, controller); put(client + p::localPawn, pawn);
        identity(1, controller, controllerHandle); identity(2, pawn, pawnHandle);
        put(controller + p::controllerName, name); string(name, "Player UTF8 \xE4\xB8\xAD\xE6\x96\x87");
        string(controller + h::playerName, "Fallback"); put(controller + h::ping, 37u);
        put(controller + p::controllerPawn, pawnHandle);
        put(pawn + p::health, 100); put(pawn + p::lifeState, std::uint8_t{0});
        put(pawn + p::sceneNode, scene); put(scene + p::origin, players::Vec3{10, -25, 30});
        put(pawn + h::velocity, players::Vec3{150, 200, 100});
        put(network + h::connection, connection); put(connection + h::connectionAddress, server);
        string(server, "127.0.0.1:27015");
    }
    players::Memory memory() {
        return {this, [](void* ctx, Address a, void* out, std::size_t n) {
            auto& self = *static_cast<Fixture*>(ctx);
            if (a == self.changing && ++self.reads == 2) return false;
            for (std::size_t i = 0; i < n; ++i) {
                const auto byte = self.data.find(a + i);
                if (byte == self.data.end()) return false;
                static_cast<unsigned char*>(out)[i] = byte->second;
            }
            return true;
        }};
    }
};
}
void TestHud() {
    Fixture f; hud::Snapshot out;
    auto read = [&] { return hud::ReadSnapshot(f.memory(), client, engine, out); };
    CHECK(read() && out.player == "Player UTF8 \xE4\xB8\xAD\xE6\x96\x87" && out.ping == 37);
    CHECK(out.server == "127.0.0.1:27015" && out.serverTick == 1000 && out.session == network);
    CHECK(out.hasPosition && out.position.y == -25 && out.speed == 250);
    f.put(controller + p::controllerName, Address{0}); CHECK(read() && out.player == "Fallback");
    f.put(controller + h::ping, 0u); CHECK(read() && out.ping == 0);
    f.put(controller + h::ping, 0xFFFFFFFFu); CHECK(read() && out.ping == -1);
    f.put(pawn + p::health, 0); CHECK(read() && !out.hasPosition && out.speed < 0 && out.player == "Fallback");
    f.put(pawn + p::health, 100);
    f.put(pawn + h::velocity, players::Vec3{std::numeric_limits<float>::quiet_NaN(), 1, 1});
    CHECK(read() && out.hasPosition && out.speed < 0);
    f.put(chunk + 2 * p::stride + 0x10, pawnHandle + 0x8000);
    CHECK(read() && !out.hasPosition && out.player == "Fallback");
    f.put(network + p::signOnState, 0); CHECK(!read() && out.player.empty() && out.ping == -1);
    f.put(network + p::signOnState, 6); f.changing = network + h::serverCount;
    CHECK(!read() && out.session == 0); f.changing = 0;
    f.put(chunk + p::stride, pawn); CHECK(!read()); // Reused controller identity.

    Fixture valid; valid.put(chunk + p::stride + 0x10, controllerHandle + 0x8000);
    valid.put(chunk + p::stride + 0x30, 1u);
    CHECK(hud::ReadSnapshot(valid.memory(), client, engine, out)); // Identity serial adjustment.
    hud::TickRate ticks;
    CHECK(ticks.Update(out, 0) < 0);
    for (int i = 1; i <= 4; ++i) { out.serverTick += 16; const auto rate = ticks.Update(out, i * .25); if (i == 4) CHECK(rate == 64); }
    out.serverCount++; CHECK(ticks.Update(out, 1.25) < 0); // Same client, new map.
    out.serverTick = 10; CHECK(ticks.Update(out, 1.5) < 0); // Counter moved backward.
    for (int i = 1; i <= 8; ++i) {
        out.serverTick += 16; const auto rate = ticks.Update(out, 1.5 + i * .25); if (i >= 4) CHECK(rate == 64);
    }
    for (int i = 1; i <= 8; ++i) {
        const auto rate = ticks.Update(out, 3.5 + i * .25); if (i == 8) CHECK(rate == 0); // Paused, never fake 64.
    }
    CHECK(ticks.Update(out, 10) < 0); CHECK(ticks.Update({}, 10.25) < 0);
    CHECK(!hud::Capture(out) && out.player.empty()); // No game loaded in this host.
    puts("PASS: HUD player/ping/server/ticks, UTF8/fallback, live movement, death/disconnect/identity guards, tick rate and map reset; no injection");
}

// Read-only validation of the same snapshot path against a running game.
int ProbeHud(unsigned long pid) {
    HANDLE process = OpenProcess(PROCESS_VM_READ | PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
    if (!process) return 1;
    HANDLE modules = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, pid);
    Address clientBase = 0, engineBase = 0;
    MODULEENTRY32W entry{}; entry.dwSize = sizeof(entry);
    if (modules != INVALID_HANDLE_VALUE) {
        for (BOOL more = Module32FirstW(modules, &entry); more; more = Module32NextW(modules, &entry)) {
            if (!_wcsicmp(entry.szModule, L"client.dll") && entry.modBaseSize == p::clientSize) clientBase = reinterpret_cast<Address>(entry.modBaseAddr);
            if (!_wcsicmp(entry.szModule, L"engine2.dll") && entry.modBaseSize == p::engineSize) engineBase = reinterpret_cast<Address>(entry.modBaseAddr);
        }
        CloseHandle(modules);
    }
    players::Memory memory{process, [](void* handle, Address a, void* out, std::size_t size) {
        SIZE_T read = 0; return ReadProcessMemory(static_cast<HANDLE>(handle), reinterpret_cast<const void*>(a), out, size, &read) && read == size;
    }};
    int build = 0;
    if (!clientBase || !engineBase || !memory.read(engineBase + p::buildNumber, build) || build != p::build) {
        CloseHandle(process); return 1;
    }
    hud::TickRate ticks;
    for (int i = 0; i < 9; ++i) {
        hud::Snapshot snapshot;
        const bool ready = hud::ReadSnapshot(memory, clientBase, engineBase, snapshot);
        const auto tps = ticks.Update(snapshot, GetTickCount64() / 1000.);
        printf("READ ONLY: ready=%d player=%s ping=%d server=%s tick=%d TPS=%.1f speed=%.1f position=%d\n",
            ready, snapshot.player.c_str(), snapshot.ping, snapshot.server.c_str(), snapshot.serverTick, tps, snapshot.speed, snapshot.hasPosition);
        if (!ready) break;
        Sleep(250);
    }
    CloseHandle(process); return 0;
}
