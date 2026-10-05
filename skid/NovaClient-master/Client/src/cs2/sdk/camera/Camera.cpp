#include "cs2/sdk/camera/Camera.h"
#include "cs2/sdk/camera/Offsets.h"
#include "cs2/sdk/players/Offsets.h"
#include "cs2/sdk/players/EntityAccess.h"
#include "core/Instance.h"
#include <atomic>
#include <windows.h>
#include <cmath>
#include <cstring>

namespace nova::cs2::sdk::camera {
namespace {
using Address = std::uintptr_t;
namespace p = players::offsets;
struct Diagnostics { std::atomic<const char*> status{"not sampled"}; };
void Stage(const char* status) { Instance<Diagnostics>().status.store(status, std::memory_order_relaxed); }
bool PlayerCameraAllowed(const players::Memory& m, Address client, Address camera) {
    if (!camera) return true;
    // OverrideView only replaces player projection for CDataDrivenCamera.
    // A non-null ICamera alone is not evidence of a cinematic. Inspect MSVC
    // RTTI through bounded reads instead of invoking methods on this object.
    const auto inside = [client](Address address, std::size_t size) {
        return address >= client && size <= p::clientSize && address - client <= p::clientSize - size;
    };
    Address table = 0, locator = 0;
    std::uint32_t signature = 0, hierarchy = 0, self = 0, count = 0, bases = 0;
    if (!m.read(camera, table) || !inside(table, 8) || table < client + 8 ||
        !m.read(table - 8, locator) || !inside(locator, 24) ||
        !m.read(locator, signature) || signature != 1 ||
        !m.read(locator + 20, self) || client + self != locator ||
        !m.read(locator + 16, hierarchy) || !inside(client + hierarchy, 16) ||
        !m.read(client + hierarchy + 8, count) || !count || count > 64 ||
        !m.read(client + hierarchy + 12, bases) || !inside(client + bases, count * 4)) return false;
    for (std::uint32_t i = 0; i < count; ++i) {
        std::uint32_t descriptor = 0, type = 0;
        if (!m.read(client + bases + i * 4, descriptor) || !inside(client + descriptor, 4) ||
            !m.read(client + descriptor, type) || !inside(client + type, 16)) return false;
        if (type == offsets::dataDrivenCameraType) return false;
    }
    return true;
}
bool Copy(void*, Address address, void* output, std::size_t size) {
    if (address < 0x10000 || address > 0x00007FFFFFFFFFFFULL - size) return false;
    // Render callbacks can race entity destruction even after ownership checks.
    SIZE_T copied = 0;
    return ReadProcessMemory(GetCurrentProcess(), reinterpret_cast<const void*>(address),
        output, size, &copied) && copied == size;
}
bool CaptureFov(const PlayerView& player, NativeFov& value) {
    const auto client = reinterpret_cast<Address>(GetModuleHandleW(L"client.dll"));
    const auto engine = reinterpret_cast<Address>(GetModuleHandleW(L"engine2.dll"));
    const auto camera = player.service;
    const players::Memory memory{nullptr, Copy};
    Address table = 0, world = 0, base = 0;
    if (!client || !engine ||
        !memory.read(camera, table) || !table ||
        !memory.read(table + offsets::worldFovVirtual, world) || world != client + offsets::worldFov ||
        !memory.read(table + offsets::baseFovVirtual, base) || base != client + offsets::baseFov) return false;
    // Called only inside the camera callback, after native world-FOV evaluation.
    // The validated getter honors controller FOV and game-rules defaults.
    const float baseFov = reinterpret_cast<float (__fastcall*)(void*)>(base)(reinterpret_cast<void*>(camera));
    if (!std::isfinite(baseFov) || baseFov <= 0 || baseFov >= 180) return false;
    value = {baseFov, player.scoped};
    return true;
}
struct LocalCamera { Address pawn = 0, service = 0; std::uint32_t handle = 0; };
bool ReadLocalCamera(const players::Memory& m, Address client, Address engine, Address expectedService,
    LocalCamera& result, bool* scoped = nullptr) {
    Address network = 0, world = 0, controller = 0, pawn = 0, chunk = 0, entity = 0, back = 0, camera = 0, owner = 0;
    std::uint32_t handle = 0, stored = 0, flags = 0;
    int signOn = 0, health = 0; std::uint8_t life = 0, team = 0;
    if (!client || !engine ||
        !m.read(engine + p::networkClient, network) || !network ||
        !m.read(network + p::signOnState, signOn) || signOn != 6 ||
        !m.read(client + p::entitySystem, world) || !world ||
        !m.read(client + p::localController, controller) || !controller ||
        !m.read(client + p::localPawn, pawn) || !pawn ||
        !m.read(controller + p::controllerPawn, handle) || handle == 0xFFFFFFFF || !(handle & p::indexMask) ||
        !m.read(world + 0x10 + ((handle & p::indexMask) / p::chunkSize) * 8, chunk) || !chunk) return false;
    const auto slot = chunk + ((handle & p::indexMask) % p::chunkSize) * p::stride;
    if (!m.read(slot, entity) || entity != pawn || !m.read(slot + 0x10, stored) ||
        !m.read(slot + 0x30, flags) || stored - ((flags & 1u) << 15) != handle ||
        !m.read(pawn + p::identity, back) || back != slot ||
        !m.read(pawn + p::health, health) || health <= 0 || health > 10000 ||
        !m.read(pawn + p::lifeState, life) || life ||
        !m.read(pawn + p::team, team) || (team != 2 && team != 3) ||
        !m.read(pawn + offsets::cameraServices, camera) || !camera ||
        (expectedService && camera != expectedService) ||
        !m.read(camera + offsets::serviceOwner, owner) || owner != pawn) return false;
    std::uint8_t scope = 0;
    if (scoped && (!m.read(pawn + offsets::isScoped, scope) || scope > 1)) return false;
    // Do not retain pointers across callbacks or accept a changing local Pawn.
    Address check = 0; std::uint32_t checkHandle = 0;
    const bool valid = m.read(client + p::localPawn, check) && check == pawn &&
        m.read(client + p::localController, check) && check == controller &&
        m.read(controller + p::controllerPawn, checkHandle) && checkHandle == handle &&
        m.read(client + p::entitySystem, check) && check == world &&
        m.read(engine + p::networkClient, check) && check == network &&
        m.read(network + p::signOnState, signOn) && signOn == 6;
    if (valid) result = {pawn, camera, handle};
    if (valid && scoped) *scoped = scope != 0;
    return valid;
}
bool CaptureView(void* modeObject, void* viewObject, View& value) {
    value = {};
    const auto client = reinterpret_cast<Address>(GetModuleHandleW(L"client.dll"));
    const auto engine = reinterpret_cast<Address>(GetModuleHandleW(L"engine2.dll"));
    const players::Memory m{nullptr, Copy};
    const auto mode = reinterpret_cast<Address>(modeObject), view = reinterpret_cast<Address>(viewObject);
    Address table = 0, input = 0; int slot = -1; PlayerView local;
    Stage("client mode/slot");
    if (!view || !client || !engine || !m.read(mode, table) || table != client + offsets::modeVtable ||
        !m.read(mode + offsets::modeSlot, slot) || slot != 0) return false;
    if (!ReadPlayerView(m, client, engine, local)) return false;
    Stage("native FOV getter");
    if (!CaptureFov(local, value.native)) return false;
    float fov = 0, aspect = 0; weapons::Angle rendered;
    Stage("final view values");
    if (!m.read(view + offsets::viewFov, fov) || !std::isfinite(fov) || fov <= 0 || fov >= 180 ||
        !m.read(view + offsets::viewAngles, rendered) ||
        !m.read(view + offsets::viewAspect, aspect) || !std::isfinite(aspect) || aspect <= 0) return false;
    std::uint8_t thirdPerson = 1;
    const auto finite = [](weapons::Angle a) {
        return std::isfinite(a.pitch) && std::isfinite(a.yaw) && std::isfinite(a.roll) &&
            std::abs(a.pitch) <= 89 && std::abs(a.yaw) <= 360 && std::abs(a.roll) <= 180;
    };
    value.unshakenValid = !local.spectating && m.read(client + offsets::input, input) && input &&
        m.read(input + offsets::thirdPerson, thirdPerson) && thirdPerson == 0 &&
        m.read(local.pawn + offsets::inputAngles, value.unshaken) && finite(value.unshaken) && finite(rendered);
    PlayerView after;
    if (!ReadPlayerView(m, client, engine, after) || after != local) return false;
    value.fov = reinterpret_cast<float*>(view + offsets::viewFov);
    value.angles = reinterpret_cast<weapons::Angle*>(view + offsets::viewAngles);
    value.pawn = local.pawn; value.pawnHandle = local.handle;
    value.aspect = aspect;
    Stage("ready");
    return true;
}
}
const char* LastViewStatus() { return Instance<Diagnostics>().status.load(std::memory_order_relaxed); }
bool ReadPlayerView(const players::Memory& m, Address client, Address engine, PlayerView& result) {
    result = {};
    Address network = 0, world = 0, controller = 0, active = 0, cinematic = 0;
    std::uint32_t activeHandle = 0;
    int signOn = 0;
    Stage("session");
    if (!client || !engine || !m.read(engine + p::networkClient, network) || !network ||
        !m.read(network + p::signOnState, signOn) || signOn != 6) return false;
    Stage("cinematic camera type");
    if (!m.read(client + offsets::cinematicCamera, cinematic) || !PlayerCameraAllowed(m, client, cinematic)) return false;
    Stage("active pawn handle");
    if (!m.read(client + p::entitySystem, world) || !world ||
        !m.read(client + p::localController, controller) || !controller ||
        !m.read(controller + offsets::activePawn, activeHandle) ||
        !players::ResolveEntity(m, world, activeHandle, active)) return false;

    Address observer = 0, activeCamera = 0, owner = 0;
    std::uint8_t mode = 0;
    std::uint32_t target = activeHandle, view = 0;
    Stage("observer/camera ownership");
    if (!m.read(active + offsets::observerServices, observer) ||
        (observer && (!m.read(observer + offsets::serviceOwner, owner) || owner != active ||
            !m.read(observer + offsets::observerMode, mode))) ||
        !m.read(active + offsets::cameraServices, activeCamera) || !activeCamera ||
        !m.read(activeCamera + offsets::serviceOwner, owner) || owner != active ||
        !m.read(activeCamera + offsets::viewEntity, view)) return false;
    // None / in-eye / chase only. Fixed, roaming and death cameras are native.
    Stage("observer mode/target");
    if (mode != 0 && mode != 2 && mode != 3) return false;
    if (mode && (!m.read(observer + offsets::observerTarget, target) || target == activeHandle)) return false;
    // A point_camera or other entity override must never inherit player FOV.
    Stage("view entity override");
    if (view != 0xFFFFFFFFu && view != activeHandle && view != target) return false;

    PlayerView value;
    value.active = active; value.handle = target; value.spectating = mode != 0;
    Address table = 0;
    int health = 0; std::uint8_t life = 1, scope = 0;
    Stage("player type/life/camera");
    if (!players::ResolveEntity(m, world, target, value.pawn) ||
        !m.read(value.pawn, table) || table != client + offsets::playerVtable ||
        !m.read(value.pawn + p::health, health) || health <= 0 || health > 10000 ||
        !m.read(value.pawn + p::lifeState, life) || life ||
        !m.read(value.pawn + offsets::cameraServices, value.service) || !value.service ||
        !m.read(value.service + offsets::serviceOwner, owner) || owner != value.pawn ||
        !m.read(value.pawn + offsets::isScoped, scope) || scope > 1) return false;
    value.scoped = scope != 0;
    Address check = 0; std::uint32_t handleCheck = 0; std::uint8_t modeCheck = 0;
    Stage("view changed during read");
    if (!m.read(client + offsets::cinematicCamera, check) || check != cinematic ||
        !PlayerCameraAllowed(m, client, check) ||
        !m.read(client + p::localController, check) || check != controller ||
        !m.read(controller + offsets::activePawn, handleCheck) || handleCheck != activeHandle ||
        !m.read(active + offsets::observerServices, check) || check != observer ||
        (observer && (!m.read(observer + offsets::observerMode, modeCheck) || modeCheck != mode)) ||
        (mode && (!m.read(observer + offsets::observerTarget, handleCheck) || handleCheck != target)) ||
        !m.read(activeCamera + offsets::viewEntity, handleCheck) || handleCheck != view ||
        !players::ResolveEntity(m, world, target, check) || check != value.pawn ||
        !m.read(client + p::entitySystem, check) || check != world ||
        !m.read(engine + p::networkClient, check) || check != network ||
        !m.read(network + p::signOnState, signOn) || signOn != 6) return false;
    result = value;
    Stage("ready");
    return true;
}
bool IsLocalCamera(const players::Memory& m, Address client, Address engine, Address service, bool* scoped) {
    LocalCamera local;
    return service && ReadLocalCamera(m, client, engine, service, local, scoped);
}
bool ReadVerticalProjection(const players::Memory& memory, Address client, Address engine, float& scale) {
    scale = 0;
    LocalCamera local;
    players::Matrix matrix;
    if (!ReadLocalCamera(memory, client, engine, 0, local) ||
        !memory.read(client + p::viewMatrix, matrix)) return false;
    for (const auto& row : matrix) for (float value : row) if (!std::isfinite(value)) return false;
    // Rotation doesn't change row lengths; dividing by the perspective row
    // removes a common homogeneous scale without reading a fixed FOV value.
    const double vertical = std::hypot(matrix[1][0], matrix[1][1], matrix[1][2]);
    const double perspective = std::hypot(matrix[3][0], matrix[3][1], matrix[3][2]);
    if (vertical <= 0 || perspective <= .000001 || !std::isfinite(vertical / perspective)) return false;
    LocalCamera after;
    if (!ReadLocalCamera(memory, client, engine, local.service, after) || after.pawn != local.pawn) return false;
    const auto projection = static_cast<float>(vertical / perspective);
    if (!std::isfinite(projection) || projection <= 0) return false;
    scale = projection;
    return true;
}
bool CaptureVerticalProjection(float& scale) {
    scale = 0;
    if (!players::Available()) return false;
    const auto client = reinterpret_cast<Address>(GetModuleHandleW(L"client.dll"));
    const auto engine = reinterpret_cast<Address>(GetModuleHandleW(L"engine2.dll"));
    return ReadVerticalProjection({nullptr, Copy}, client, engine, scale);
}
Binding ResolveBinding() {
    if (!players::Available()) return {};
    auto* client = reinterpret_cast<unsigned char*>(GetModuleHandleW(L"client.dll"));
    if (!client) return {};
    constexpr unsigned char worldEntry[]{0x40,0x53,0x48,0x83,0xEC,0x50,0x48,0x8B,0xD9,0xE8,0x32,0x75,0x42,0x00};
    constexpr unsigned char baseEntry[]{0x48,0x89,0x5C,0x24,0x08,0x57,0x48,0x83,0xEC,0x20,0x48,0x83,0x79,0x38,0x00};
    constexpr unsigned char viewEntry[]{0x4C,0x8B,0xDC,0x49,0x89,0x4B,0x08,0x53,0x55,0x56,0x57,0x41,0x54,0x41,0x55};
    __try {
        if (std::memcmp(client + offsets::worldFov, worldEntry, sizeof(worldEntry)) ||
            std::memcmp(client + offsets::baseFov, baseEntry, sizeof(baseEntry)) ||
            std::memcmp(client + offsets::setupView, viewEntry, sizeof(viewEntry))) return {};
    } __except(EXCEPTION_EXECUTE_HANDLER) { return {}; }
    return {client + offsets::setupView, CaptureView};
}
}
