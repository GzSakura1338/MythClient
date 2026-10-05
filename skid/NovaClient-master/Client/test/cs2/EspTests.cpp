#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <tlhelp32.h>
#include <unordered_map>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include "cs2/sdk/players/Players.h"
#include "cs2/sdk/players/Offsets.h"
#include "cs2/sdk/players/Transform.h"
#include "cs2/modules/render/esp2d/Projection.h"

#define CHECK(x) do { if (!(x)) { fprintf(stderr, "ESP test failed at %d: %s\n", __LINE__, #x); std::exit(1); } } while (false)
namespace {
using namespace nova::cs2::sdk::players;
using Address = std::uintptr_t;
constexpr Address client = 0x10000000, engine = 0x20000000, system = 0x30000000, chunk = 0x31000000;
constexpr Address own = 0x32000000, controller = 0x32010000, pawn = 0x33000000;
constexpr Address network = 0x34000000, scene = 0x35000000, collision = 0x36000000;
constexpr Address controllerName = 0x37000000, pawnName = 0x37000100, name = 0x37000200;
constexpr unsigned serial = 7u << 15, pawnIndex = 65;
constexpr Address pawnSlot = chunk + pawnIndex * offsets::stride;
struct Fixture {
    std::unordered_map<Address, unsigned char> bytes;
    bool destroyDuringRead = false;
    void write(Address address, const void* value, size_t size) {
        const auto* data = static_cast<const unsigned char*>(value);
        for (size_t i = 0; i < size; ++i) bytes[address + i] = data[i];
    }
    template<class T> void put(Address address, const T& value) { write(address, &value, sizeof(value)); }
    void identity(unsigned index, Address object, Address kind) {
        const Address slot = chunk + index * offsets::stride;
        for (unsigned i = 0; i < offsets::stride; ++i) bytes[slot + i] = 0;
        put(slot, object); put(slot + 0x10, std::uint32_t(serial | index)); put(slot + 0x20, kind);
        put(object + offsets::identity, slot);
    }
    static bool Copy(void* context, Address address, void* output, size_t size) {
        auto& self = *static_cast<Fixture*>(context);
        if (self.destroyDuringRead && address == pawn + offsets::armor)
            self.put(pawnSlot + 0x10, std::uint32_t((serial + (1u << 15)) | pawnIndex));
        auto* target = static_cast<unsigned char*>(output);
        for (size_t i = 0; i < size; ++i) {
            auto it = self.bytes.find(address + i);
            if (it == self.bytes.end()) return false;
            target[i] = it->second;
        }
        return true;
    }
    Memory memory() { return {this, Copy}; }
    Fixture() {
        put(client + offsets::entitySystem, system); put(client + offsets::localController, own);
        put(client + offsets::localPawn, Address(0x33010000)); put(engine + offsets::networkClient, network);
        put(network + offsets::signOnState, 6); put(network + offsets::maxClients, 64);
        put(system + 0x10, chunk); put(own + offsets::team, std::uint8_t(2));
        Matrix matrix{}; matrix[0][0] = matrix[1][2] = .01f; matrix[3][3] = 1;
        put(client + offsets::viewMatrix, matrix);
        put(controllerName, "cs_player_controller"); put(pawnName, "player"); put(name, "Nova Test");
        identity(1, own, controllerName); identity(2, controller, controllerName); identity(pawnIndex, pawn, pawnName);
        put(controller + offsets::controllerPawn, std::uint32_t(serial | pawnIndex));
        put(controller + offsets::controllerName, name);
        put(pawn + offsets::health, 75); put(pawn + offsets::maximumHealth, 100); put(pawn + offsets::armor, 60);
        put(pawn + offsets::team, std::uint8_t(3)); put(pawn + offsets::lifeState, std::uint8_t(0));
        put(pawn + offsets::sceneNode, scene); put(pawn + offsets::collision, collision);
        put(scene + offsets::origin, Vec3{100, 200, 300}); put(scene + offsets::dormant, std::uint8_t(0));
        put(collision + offsets::mins, Vec3{-16, -16, 0}); put(collision + offsets::maxs, Vec3{16, 16, 72});
    }
};
}
void TestEsp() {
    // CS2 uses 0..100 armor; keep the empty track visible and do not round
    // one/two remaining points to zero through Minecraft's 0..20 range.
    struct ArmorDraw {
        int tracks = 0, fills = 0;
        double filledWidth = 0;
        void Rect(double, double, double, double, nova::color::Value) { ++tracks; }
        void Gradient(double, double, double width, double, nova::color::Value, nova::color::Value, bool) {
            ++fills; filledWidth = width;
        }
    } draw;
    using namespace nova::ui::esp2d;
    for (float armor : {0.f, 1.f, 2.f, 25.f, 100.f}) {
        draw = {};
        Render(draw, {20, 20, 120, 220}, BoxMode::None, HealthMode::None, 100, 100, armor, true,
            0xFFFFFFFF, 0xFFFFFFFF, 100, true);
        CHECK(draw.tracks == 1 && draw.fills == (armor > 0 ? 1 : 0));
        CHECK(std::abs(draw.filledWidth - 100.5 * armor / 100) < .001);
    }
    draw = {};
    Render(draw, {20, 20, 120, 220}, BoxMode::None, HealthMode::None, 100, 100, 100, false,
        0xFFFFFFFF, 0xFFFFFFFF, 100, true);
    CHECK(draw.tracks == 0 && draw.fills == 0);
    Render(draw, {20, 20, 120, 220}, BoxMode::None, HealthMode::None, 100, 100, 0, true, 0xFFFFFFFF, 0xFFFFFFFF);
    CHECK(draw.tracks == 0); // Minecraft's existing empty-armor behavior.
    Render(draw, {20, 20, 120, 220}, BoxMode::None, HealthMode::None, 100, 100, 10, true, 0xFFFFFFFF, 0xFFFFFFFF);
    CHECK(draw.tracks == 1 && std::abs(draw.filledWidth - 50.25) < .001);
    for (float armor : {0.f, 1.f, 25.f, 100.f}) {
        draw = {};
        Render(draw, {20, 20, 120, 220}, BoxMode::None, HealthMode::None, 100, 100, armor, true,
            0xFFFFFFFF, 0xFFFFFFFF, 100, true, Layout::Compact);
        CHECK(draw.tracks == 1 && draw.fills == (armor > 0 ? 1 : 0));
        CHECK(std::abs(draw.filledWidth - armor) < .001);
    }
    for (auto box : {BoxMode::Normal, BoxMode::CS}) {
        draw = {};
        Render(draw, {20, 20, 120, 220}, box, HealthMode::None, 100, 100, 0, false,
            0xFFFFFFFF, 0xFFFFFFFF, 100, false, Layout::Compact, true);
        const int outlinedRects = draw.tracks;
        draw = {};
        Render(draw, {20, 20, 120, 220}, box, HealthMode::None, 100, 100, 0, false,
            0xFFFFFFFF, 0xFFFFFFFF, 100, false, Layout::Compact, false);
        CHECK(draw.tracks > 0 && draw.tracks < outlinedRects);
    }
    Snapshot snapshot;
    Fixture valid;
    CHECK(ReadSnapshot(valid.memory(), client, engine, snapshot) && snapshot.players.size() == 1);
    CHECK(snapshot.players[0].name == "Nova Test" && snapshot.players[0].health == 75 && snapshot.players[0].armor == 60);
    CHECK(snapshot.localTeam == 2 && snapshot.players[0].team == 3);
    valid.put(pawn + offsets::maximumHealth, 0);
    CHECK(ReadSnapshot(valid.memory(), client, engine, snapshot) && snapshot.players.size() == 1 && snapshot.players[0].maximum == 100);
    valid.put(pawn + offsets::maximumHealth, 150);
    CHECK(ReadSnapshot(valid.memory(), client, engine, snapshot) && snapshot.players[0].maximum == 150);
    Fixture rotated;
    Transform transform; transform.position = {}; transform.qz = transform.qw = std::sqrt(.5f);
    rotated.put(scene + offsets::nodeToWorld, transform);
    rotated.put(collision + offsets::mins, Vec3{-16, -8, 0}); rotated.put(collision + offsets::maxs, Vec3{16, 8, 72});
    CHECK(ReadSnapshot(rotated.memory(), client, engine, snapshot) && snapshot.players.size() == 1 && snapshot.players[0].transformed);
    CHECK(std::abs(snapshot.players[0].worldCorners[0].x - 8) < .001 && std::abs(snapshot.players[0].worldCorners[0].y + 16) < .001);
    const auto rotatedPlayer = snapshot.players[0];
    transform.scale = 0; rotated.put(scene + offsets::nodeToWorld, transform);
    CHECK(ReadSnapshot(rotated.memory(), client, engine, snapshot) && snapshot.players.size() == 1 && !snapshot.players[0].transformed);
    valid.put(pawnName, "c_cs_player_for_precache");
    CHECK(ReadSnapshot(valid.memory(), client, engine, snapshot) && snapshot.players.size() == 1);
    // A second controller referring to the same pawn must not duplicate ESP.
    constexpr Address duplicate = 0x32020000;
    valid.identity(3, duplicate, controllerName);
    valid.put(duplicate + offsets::controllerPawn, std::uint32_t(serial | pawnIndex));
    CHECK(ReadSnapshot(valid.memory(), client, engine, snapshot) && snapshot.players.size() == 1);
    valid.put(pawn + offsets::team, std::uint8_t(2));
    CHECK(ReadSnapshot(valid.memory(), client, engine, snapshot) && snapshot.players[0].team == snapshot.localTeam);
    valid.put(own + offsets::team, std::uint8_t(1));
    CHECK(ReadSnapshot(valid.memory(), client, engine, snapshot) && snapshot.localTeam == 1 && snapshot.players[0].team == 2);
    valid.put(pawn + offsets::health, 0);
    CHECK(ReadSnapshot(valid.memory(), client, engine, snapshot) && snapshot.players.empty());
    valid.put(pawn + offsets::health, 75); valid.put(scene + offsets::dormant, std::uint8_t(1));
    CHECK(ReadSnapshot(valid.memory(), client, engine, snapshot) && snapshot.players.empty());
    Fixture stale;
    stale.put(pawnSlot + 0x10, std::uint32_t((serial + (1u << 15)) | pawnIndex));
    CHECK(ReadSnapshot(stale.memory(), client, engine, snapshot) && snapshot.players.empty());
    Fixture removed; removed.destroyDuringRead = true;
    CHECK(ReadSnapshot(removed.memory(), client, engine, snapshot) && snapshot.players.empty());
    Fixture invalid; invalid.put(pawn + offsets::sceneNode, Address(0xDEAD0000));
    CHECK(ReadSnapshot(invalid.memory(), client, engine, snapshot) && snapshot.players.empty());
    Fixture disconnected; disconnected.put(network + offsets::signOnState, 0);
    CHECK(!ReadSnapshot(disconnected.memory(), client, engine, snapshot) && snapshot.players.empty());
    disconnected.put(network + offsets::signOnState, 6); disconnected.put(network + offsets::maxClients, 999999);
    CHECK(!ReadSnapshot(disconnected.memory(), client, engine, snapshot));

    Player player; player.mins = {-16, -16, 0}; player.maxs = {16, 16, 72};
    Matrix matrix{}; matrix[0][0] = .02f; matrix[1][2] = .02f; matrix[1][3] = -.72f; matrix[3][3] = 1;
    auto box = nova::cs2::esp2d::Project(player, matrix, 1000, 1000);
    CHECK(box && std::abs(box->left - 340) < .01 && std::abs(box->right - 660) < .01);
    CHECK(std::abs(box->top - 140) < .01 && std::abs(box->bottom - 860) < .01);
    box = nova::cs2::esp2d::Project(rotatedPlayer, matrix, 1000, 1000);
    CHECK(box && std::abs(box->left - 420) < .01 && std::abs(box->right - 580) < .01);
    player.origin.x = -50;
    box = nova::cs2::esp2d::Project(player, matrix, 1000, 1000);
    CHECK(box && std::abs(box->left + 160) < .01 && std::abs(box->right - 160) < .01); // No edge compression.
    player.origin = {};
    matrix[3][3] = -1; CHECK(!nova::cs2::esp2d::Project(player, matrix, 1000, 1000));
    matrix[3][2] = .1f; // Box intersects the camera plane: clip rather than disappear.
    CHECK(nova::cs2::esp2d::Project(player, matrix, 1000, 1000));
    matrix[0][0] = std::numeric_limits<float>::quiet_NaN();
    CHECK(!nova::cs2::esp2d::Project(player, matrix, 1000, 1000));
    puts("PASS: CS2 ESP snapshot, existing entities, deduplication, team/health, invalid pointers, handle reuse/deletion, disconnect, projection and near-plane clipping");
}

// Optional diagnostic: read memory only, never inject or call game code.
int ProbeEsp(unsigned long pid) {
    HANDLE process = OpenProcess(PROCESS_VM_READ | PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
    if (!process) return 1;
    HANDLE modules = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, pid);
    Address clientBase = 0, engineBase = 0;
    MODULEENTRY32W entry{}; entry.dwSize = sizeof(entry);
    if (modules != INVALID_HANDLE_VALUE) {
        for (BOOL more = Module32FirstW(modules, &entry); more; more = Module32NextW(modules, &entry)) {
            if (!_wcsicmp(entry.szModule, L"client.dll") && entry.modBaseSize == offsets::clientSize) clientBase = reinterpret_cast<Address>(entry.modBaseAddr);
            if (!_wcsicmp(entry.szModule, L"engine2.dll") && entry.modBaseSize == offsets::engineSize) engineBase = reinterpret_cast<Address>(entry.modBaseAddr);
        }
        CloseHandle(modules);
    }
    Memory memory{process, [](void* handle, Address address, void* out, size_t size) {
        SIZE_T read = 0; return ReadProcessMemory(static_cast<HANDLE>(handle), reinterpret_cast<const void*>(address), out, size, &read) && read == size;
    }};
    int build = 0, signOn = 0, maxClients = 0; Address net = 0;
    if (!clientBase || !engineBase || !memory.read(engineBase + offsets::buildNumber, build) || build != offsets::build) { CloseHandle(process); return 1; }
    memory.read(engineBase + offsets::networkClient, net);
    memory.read(net + offsets::signOnState, signOn); memory.read(net + offsets::maxClients, maxClients);
    Snapshot snapshot;
    const bool captured = ReadSnapshot(memory, clientBase, engineBase, snapshot);
    printf("READ ONLY: build=%d signOn=%d maxClients=%d snapshot=%s players=%zu\n", build, signOn, maxClients, captured ? "ready" : "not in playable session", snapshot.players.size());
    if (captured) {
        Address entities = 0, firstChunk = 0;
        memory.read(clientBase + offsets::entitySystem, entities); memory.read(entities + 0x10, firstChunk);
        for (int i = 1; i <= maxClients; ++i) {
            const Address slot = firstChunk + i * offsets::stride;
            Address object = 0, kind = 0; std::uint32_t identityHandle = 0, handle = 0;
            std::array<char, 64> designer{};
            if (!memory.read(slot, object) || !object) continue;
            memory.read(slot + 0x20, kind); memory.read(kind, designer); memory.read(slot + 0x10, identityHandle);
            memory.read(object + offsets::controllerPawn, handle);
            Address pawnChunk = 0, pawnObject = 0, pawnKind = 0, node = 0;
            std::uint32_t pawnIdentity = 0, flags = 0; int health = 0; std::uint8_t dormant = 0, life = 0;
            memory.read(entities + 0x10 + ((handle & offsets::indexMask) / offsets::chunkSize) * sizeof(Address), pawnChunk);
            const Address pslot = pawnChunk + ((handle & offsets::indexMask) % offsets::chunkSize) * offsets::stride;
            memory.read(pslot, pawnObject); memory.read(pslot + 0x10, pawnIdentity); memory.read(pslot + 0x30, flags);
            memory.read(pslot + 0x20, pawnKind); std::array<char, 64> pawnDesigner{}; memory.read(pawnKind, pawnDesigner);
            memory.read(pawnObject + offsets::health, health); memory.read(pawnObject + offsets::lifeState, life);
            memory.read(pawnObject + offsets::sceneNode, node); memory.read(node + offsets::dormant, dormant);
            int pawnArmor = -1, controllerArmor = -1;
            const bool pawnArmorRead = memory.read(pawnObject + offsets::armor, pawnArmor);
            const bool controllerArmorRead = memory.read(object + offsets::controllerArmor, controllerArmor);
            printf("slot=%d class=%.63s id=%u pawn=%u pawnClass=%.63s pawnId=%u flags=%u health=%d life=%u dormant=%u\n",
                i, designer.data(), identityHandle, handle, pawnDesigner.data(), pawnIdentity, flags, health, unsigned(life), unsigned(dormant));
            printf("  armor: pawn=%d (read=%d), controller=%d (read=%d)\n", pawnArmor, pawnArmorRead, controllerArmor, controllerArmorRead);
        }
    }
    for (const auto& player : snapshot.players) printf("player handle=%u health=%d/%d armor=%d team=%d localTeam=%d bounds=(%.1f,%.1f,%.1f)-(%.1f,%.1f,%.1f)\n",
        player.handle, player.health, player.maximum, player.armor, player.team, snapshot.localTeam, player.mins.x, player.mins.y, player.mins.z, player.maxs.x, player.maxs.y, player.maxs.z);
    CloseHandle(process); return 0;
}
