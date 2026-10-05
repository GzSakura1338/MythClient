#include "cs2/sdk/chams/Chams.h"
#include "cs2/sdk/chams/MaterialText.h"
#include "cs2/sdk/players/EntityAccess.h"
#include "core/Instance.h"
#include <windows.h>
#include <cstring>
#include <cstdio>
#include <atomic>

namespace nova::cs2::sdk::chams {
namespace {
using Address = std::uintptr_t;
struct Diagnostics { std::atomic<const char*> status{"not sampled"}; };
void Stage(const char* status) { Instance<Diagnostics>().status.store(status, std::memory_order_relaxed); }
struct Kv { std::uint64_t flags = 0, value = 0; };
struct Format { const char* name; std::uint64_t low, high; };
struct Native {
    void* (*construct)(Kv*, unsigned char, unsigned char) = nullptr;
    bool (*load)(Kv*, void*, const char*, const Format*, const char*, unsigned) = nullptr;
    void* (*create)(void*, void**, const char*, Kv*, void*, unsigned char) = nullptr;
    std::uintptr_t (*purge)(Kv*, unsigned) = nullptr;
    void (*destroy)(void*, void*) = nullptr;
    void* utils = nullptr;
    Address client = 0, engine = 0;
};
Native& Get() { return Instance<Native>(); }
bool Copy(void*, Address address, void* out, std::size_t size) {
    if (address < 0x10000 || address > 0x00007FFFFFFFFFFFULL - size) return false;
    __try { std::memcpy(out, reinterpret_cast<void*>(address), size); return true; }
    __except(EXCEPTION_EXECUTE_HANDLER) { return false; }
}
bool Enemy(Address scene) { auto& n = Get(); return IsEnemy({nullptr, Copy}, n.client, n.engine, scene); }
void Release(Materials& materials) {
    auto& n = Get();
    for (auto& material : materials.slots) {
        if (!material.binding) continue;
        auto* binding = material.binding;
        const bool owned = material.owned;
        material = {}; // Never decrement the same handle twice.
        auto* refs = reinterpret_cast<volatile LONG*>(static_cast<unsigned char*>(binding) + 32);
        if ((!owned && *refs == 0) || (owned && InterlockedDecrement(refs) == 0))
            n.destroy(n.utils, binding); // Engine queues destruction; no direct material free.
    }
}
bool CreateOne(Material& material, const char* text, const char* name) {
    auto& n = Get();
    Kv kv;
    const Format format{"generic", 0x41B818518343427EULL, 0xB5F447C23C0CDF8CULL};
    n.construct(&kv, 1, 8);
    bool valid = false;
    __try {
        if (n.load(&kv, nullptr, text, &format, "", 0)) {
            n.create(nullptr, &material.binding, name, &kv, nullptr, 1);
            if (material.binding) {
                // Retain the binding while hooks may submit its material.
                InterlockedIncrement(reinterpret_cast<volatile LONG*>(static_cast<unsigned char*>(material.binding) + 32));
                material.owned = true;
                material.data = *static_cast<void**>(material.binding);
                valid = material.data != nullptr;
            }
        }
    } __finally { n.purge(&kv, 0); }
    return valid;
}
bool Create(Materials& materials) {
    const char* texts[]{szVMatBufferWhiteVisible, szVMatBufferWhiteInvisible,
        szVMatBufferIlluminateVisible, szVMatBufferIlluminateInvisible};
    const auto stamp = GetTickCount64();
    for (unsigned i = 0; i < 4; ++i) {
        char name[128];
        std::snprintf(name, sizeof(name), "materials/nova/chams_%lu_%llu_%u.vmat", GetCurrentProcessId(), stamp, i);
        if (!CreateOne(materials.slots[i], texts[i], name)) return false;
    }
    return true;
}
bool ResolveNative(Native& n, unsigned char* client, unsigned char* tier, unsigned char* material, unsigned char* resource) {
    using Factory = void* (*)(const char*, int*);
    const auto factory = reinterpret_cast<Factory>(GetProcAddress(reinterpret_cast<HMODULE>(resource), "CreateInterface"));
    n.load = reinterpret_cast<decltype(n.load)>(GetProcAddress(reinterpret_cast<HMODULE>(tier),
        "?LoadKV3@@YA_NPEAVKeyValues3@@PEAVCUtlString@@PEBDAEBUKV3ID_t@@2I@Z"));
    if (!factory || reinterpret_cast<void*>(n.load) != tier + 0x124810) return false;
    void* system = factory("ResourceSystem013", nullptr);
    if (!system || *static_cast<void**>(system) != resource + 0x65348) return false;
    using Query = void* (*)(void*, const char*);
    auto query = reinterpret_cast<Query>((*static_cast<void***>(system))[2]);
    if (reinterpret_cast<void*>(query) != resource + 0x14BA0) return false;
    n.utils = query(system, "ResourceHandleUtils001");
    if (!n.utils || *static_cast<void**>(n.utils) != resource + 0x65868) return false;
    n.destroy = reinterpret_cast<decltype(n.destroy)>((*static_cast<void***>(n.utils))[2]);
    if (reinterpret_cast<void*>(n.destroy) != resource + 0x17AC0) return false;
    n.construct = reinterpret_cast<decltype(n.construct)>(client + 0x19AEB90);
    n.purge = reinterpret_cast<decltype(n.purge)>(tier + 0x21B620);
    n.create = reinterpret_cast<decltype(n.create)>(material + 0x3ABF0);
    n.client = reinterpret_cast<Address>(client);
    n.engine = reinterpret_cast<Address>(GetModuleHandleW(L"engine2.dll"));
    return true;
}
}
const char* LastSelectionStatus() { return Instance<Diagnostics>().status.load(std::memory_order_relaxed); }
bool IsEnemy(const players::Memory& m, Address client, Address engine, Address scene) {
    namespace p = players::offsets;
    Address network = 0, world = 0, local = 0, controller = 0, pawn = 0, table = 0, resolved = 0, check = 0;
    std::uint32_t owner = 0, localHandle = 0, checkHandle = 0;
    int signOn = 0, health = 0; std::uint8_t team = 0, localTeam = 0, life = 1;
    Stage("session");
    if (!scene || !client || !engine ||
        !m.read(engine + p::networkClient, network) || !network ||
        !m.read(network + p::signOnState, signOn) || signOn != 6 ||
        !m.read(client + p::entitySystem, world) || !world) return false;
    Stage("local pawn handle");
    if (!m.read(client + p::localPawn, local) || !local ||
        !m.read(client + p::localController, controller) || !controller ||
        !m.read(controller + p::controllerPawn, localHandle) ||
        !players::ResolveEntity(m, world, localHandle, resolved) || resolved != local) return false;
    Stage("local player type/team");
    if (!m.read(local, table) || table != client + pawnVtable ||
        !m.read(local + p::team, localTeam) || (localTeam != 2 && localTeam != 3)) return false;
    Stage("scene owner handle");
    if (!m.read(scene + sceneOwner, owner) || !players::ResolveEntity(m, world, owner, pawn)) return false;
    Stage("self/team/nonplayer/dead");
    if (pawn == local || !m.read(pawn, table) || table != client + pawnVtable ||
        !m.read(pawn + p::health, health) || health <= 0 || health > 10000 ||
        !m.read(pawn + p::lifeState, life) || life != 0 ||
        !m.read(pawn + p::team, team) || (team != 2 && team != 3) || team == localTeam) return false;
    Stage("identity recheck");
    const bool valid = m.read(scene + sceneOwner, checkHandle) && checkHandle == owner &&
        players::ResolveEntity(m, world, owner, resolved) && resolved == pawn &&
        m.read(client + p::entitySystem, check) && check == world &&
        m.read(client + p::localPawn, check) && check == local &&
        m.read(engine + p::networkClient, check) && check == network &&
        m.read(network + p::signOnState, signOn) && signOn == 6;
    if (valid) Stage("enemy selected");
    return valid;
}
Binding ResolveBinding() {
    if (!players::Available() ||
        !players::MatchesModule(L"scenesystem.dll", 9928704, "0C35299F59E4BB906BBAF1C71DF019B1839B7027BA4FBC6D039761DE0A1F88DE") ||
        !players::MatchesModule(L"materialsystem2.dll", 1597440, "EE3214CD25113DA36DE5721A02ADB92D303DB510BD5FAE4E3E5FA9FDD2779C48") ||
        !players::MatchesModule(L"tier0.dll", 4198400, "4E0DCB0AF3F6953F37DDAED0F4E67A56D031F1E84964A262148F8A6F80547791") ||
        !players::MatchesModule(L"resourcesystem.dll", 614400, "4FA4023773091593ED1B47085973A6E079B909093D4323A303DAF310D3DAB515")) return {};
    auto* client = reinterpret_cast<unsigned char*>(GetModuleHandleW(L"client.dll"));
    auto* tier = reinterpret_cast<unsigned char*>(GetModuleHandleW(L"tier0.dll"));
    auto* material = reinterpret_cast<unsigned char*>(GetModuleHandleW(L"materialsystem2.dll"));
    auto* resource = reinterpret_cast<unsigned char*>(GetModuleHandleW(L"resourcesystem.dll"));
    auto* scene = reinterpret_cast<unsigned char*>(GetModuleHandleW(L"scenesystem.dll"));
    constexpr unsigned char drawEntry[]{0x48,0x8B,0xC4,0x48,0x89,0x50,0x10,0x48,0x89,0x48,0x08,0x53,0x56,0x41,0x57};
    Native n;
    __try {
        if (std::memcmp(scene + 0x59FC0, drawEntry, sizeof(drawEntry)) || !ResolveNative(n, client, tier, material, resource)) return {};
    } __except(EXCEPTION_EXECUTE_HANDLER) { return {}; }
    Get() = n;
    return {scene + 0x59FC0, Enemy, Create, Release};
}
}
