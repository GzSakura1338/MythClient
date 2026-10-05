#include "cs2/sdk/hud/HudSnapshot.h"
#include "cs2/sdk/hud/Offsets.h"
#include "cs2/sdk/players/Offsets.h"
#include <windows.h>
#include <algorithm>
#include <array>
#include <cmath>
#include <utility>

namespace nova::cs2::sdk::hud {
namespace {
using Address = std::uintptr_t;
namespace p = players::offsets;
bool Copy(void*, Address address, void* output, std::size_t size) {
    if (address < 0x10000 || size > 4096 || address > 0x00007FFFFFFFFFFFULL - size) return false;
    // A disappearing entity must return failure even with reflective loading;
    // this reader does not rely on the injected image's SEH registration.
    SIZE_T copied = 0;
    return ReadProcessMemory(GetCurrentProcess(), reinterpret_cast<const void*>(address), output, size, &copied) && copied == size;
}
bool String(const players::Memory& m, Address address, std::string& out) {
    std::array<char, 128> text{};
    if (!m.read(address, text)) {
        // A short string may end immediately before an unreadable page.
        for (std::size_t i = 0; i < text.size(); ++i) {
            if (!m.read(address + i, text[i])) return false;
            if (!text[i]) break;
        }
    }
    const auto end = std::find(text.begin(), text.end(), '\0');
    if (end == text.end()) return false;
    out.assign(text.begin(), end);
    for (char& c : out) if (static_cast<unsigned char>(c) < 32 || c == 127) c = ' ';
    return !out.empty();
}
struct Identity { Address entity = 0, slot = 0; std::uint32_t handle = 0; };
bool Resolve(const players::Memory& m, Address system, std::uint32_t handle, Identity& out) {
    if (handle == 0xFFFFFFFF || !(handle & p::indexMask)) return false;
    Address chunk = 0, back = 0; std::uint32_t stored = 0, flags = 0;
    if (!m.read(system + 0x10 + ((handle & p::indexMask) / p::chunkSize) * 8, chunk) || !chunk) return false;
    out.slot = chunk + ((handle & p::indexMask) % p::chunkSize) * p::stride;
    if (!m.read(out.slot, out.entity) || !out.entity || !m.read(out.slot + 0x10, stored) ||
        !m.read(out.slot + 0x30, flags) || stored - ((flags & 1u) << 15) != handle ||
        !m.read(out.entity + p::identity, back) || back != out.slot) return false;
    out.handle = handle;
    return true;
}
bool Controller(const players::Memory& m, Address system, Address entity, Identity& out) {
    Address slot = 0; std::uint32_t stored = 0, flags = 0;
    return m.read(entity + p::identity, slot) && slot && m.read(slot + 0x10, stored) && stored != 0xFFFFFFFF &&
        m.read(slot + 0x30, flags) && Resolve(m, system, stored - ((flags & 1u) << 15), out) && out.entity == entity;
}
bool Same(const players::Memory& m, Address system, const Identity& before) {
    Identity after;
    return Resolve(m, system, before.handle, after) && after.entity == before.entity && after.slot == before.slot;
}
bool Finite(players::Vec3 v) {
    return std::isfinite(v.x) && std::isfinite(v.y) && std::isfinite(v.z) &&
        std::abs(v.x) < 1000000 && std::abs(v.y) < 1000000 && std::abs(v.z) < 1000000;
}
}
bool ReadSnapshot(const players::Memory& m, Address client, Address engine, Snapshot& result) {
    result = {};
    Snapshot next;
    Address network = 0, system = 0, local = 0;
    int signOn = 0;
    Identity controller;
    if (!client || !engine || !m.read(engine + p::networkClient, network) || !network ||
        !m.read(network + p::signOnState, signOn) || signOn != 6 ||
        !m.read(network + offsets::serverCount, next.serverCount) ||
        !m.read(client + p::entitySystem, system) || !system ||
        !m.read(client + p::localController, local) || !local || !Controller(m, system, local, controller)) return false;
    next.session = network;
    Address name = 0;
    if (!m.read(local + p::controllerName, name) || !name || !String(m, name, next.player))
        String(m, local + offsets::playerName, next.player);
    std::uint32_t ping = 0;
    if (m.read(local + offsets::ping, ping) && ping <= 60000) next.ping = static_cast<int>(ping);
    int tick = -1;
    if (m.read(network + offsets::serverTick, tick) && tick >= 0) next.serverTick = tick;
    Address connection = 0, address = 0;
    if (m.read(network + offsets::connection, connection) && connection &&
        m.read(connection + offsets::connectionAddress, address) && address) String(m, address, next.server);

    // Name/ping/ticks remain valid while dead; position/speed require our live Pawn.
    std::uint32_t handle = 0; Identity pawn;
    Address localPawn = 0, node = 0; int health = 0; std::uint8_t life = 0;
    if (m.read(local + p::controllerPawn, handle) && Resolve(m, system, handle, pawn) &&
        m.read(client + p::localPawn, localPawn) && localPawn == pawn.entity &&
        m.read(pawn.entity + p::health, health) && health > 0 &&
        m.read(pawn.entity + p::lifeState, life) && !life) {
        if (m.read(pawn.entity + p::sceneNode, node) && node &&
            m.read(node + p::origin, next.position) && Finite(next.position)) next.hasPosition = true;
        players::Vec3 velocity;
        if (m.read(pawn.entity + offsets::velocity, velocity) && Finite(velocity))
            next.speed = std::hypot(velocity.x, velocity.y);
        std::uint32_t afterHandle = 0; Address afterPawn = 0;
        if (!Same(m, system, pawn) || !m.read(local + p::controllerPawn, afterHandle) || afterHandle != handle ||
            !m.read(client + p::localPawn, afterPawn) || afterPawn != pawn.entity) {
            next.hasPosition = false; next.speed = -1;
        }
    }
    Address after = 0; int count = 0;
    if (!m.read(engine + p::networkClient, after) || after != network ||
        !m.read(network + p::signOnState, signOn) || signOn != 6 ||
        !m.read(network + offsets::serverCount, count) || count != next.serverCount ||
        !m.read(client + p::entitySystem, after) || after != system ||
        !m.read(client + p::localController, after) || after != local || !Same(m, system, controller)) return false;
    result = std::move(next);
    return true;
}
bool Capture(Snapshot& result) {
    result = {};
    if (!players::Available()) return false;
    return ReadSnapshot({nullptr, Copy}, reinterpret_cast<Address>(GetModuleHandleW(L"client.dll")),
        reinterpret_cast<Address>(GetModuleHandleW(L"engine2.dll")), result);
}
void TickRate::Reset() { samples_.clear(); session_ = 0; serverCount_ = -1; }
double TickRate::Update(const Snapshot& snapshot, double now) {
    if (!snapshot.session || snapshot.serverTick < 0 || !std::isfinite(now)) { Reset(); return -1; }
    if (snapshot.session != session_ || snapshot.serverCount != serverCount_ ||
        (!samples_.empty() && (now <= samples_.back().time || now - samples_.back().time > 3 ||
            snapshot.serverTick < samples_.back().tick))) Reset();
    session_ = snapshot.session; serverCount_ = snapshot.serverCount;
    samples_.push_back({now, snapshot.serverTick});
    while (samples_.size() > 2 && (samples_.size() > 9 || now - samples_[1].time >= 2)) samples_.pop_front();
    const double elapsed = now - samples_.front().time;
    if (elapsed < 1) return -1;
    const double rate = (static_cast<double>(snapshot.serverTick) - samples_.front().tick) / elapsed;
    if (rate > 1024) { Reset(); return -1; } // Demo seek/counter reset, not a real TPS sample.
    return rate;
}
}
