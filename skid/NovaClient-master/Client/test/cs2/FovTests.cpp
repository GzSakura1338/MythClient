#include "cs2/hooks/FovHook.h"
#include "cs2/modules/render/fovcontrol/FovControl.h"
#include "cs2/modules/ModuleManager.h"
#include "cs2/sdk/camera/Offsets.h"
#include "cs2/sdk/players/Offsets.h"
#include "MinHook.h"
#include <windows.h>
#include <atomic>
#include <thread>
#include <unordered_map>
#include <limits>
#include <cstdio>
#include <cstdlib>

#define CHECK(x) do { if (!(x)) { std::fprintf(stderr, "FOV test failed at %d: %s\n", __LINE__, #x); std::exit(1); } } while (false)
namespace {
using namespace nova::cs2;
void Near(float a, float b, float tolerance = .0002f) { CHECK(std::abs(a - b) < tolerance); }
void ProjectionTests() {
    Near(fov::Adjust(90, 90, false, {110, true}), 110);
    Near(fov::Adjust(100, 100, false, {110, true}), 110); // Non-default native base.
    Near(fov::Adjust(40, 90, true, {110, false}), 110);
    Near(fov::Adjust(90, 90, false, {0, true}), 30);
    Near(fov::Adjust(90, 90, false, {500, true}), 150);
    Near(fov::Adjust(90, 90, false, {110.3f, true}), 110.3f);
    Near(fov::Adjust(90, 90, false, {std::numeric_limits<float>::quiet_NaN(), true}), 90);
    // Holding/swapping weapons and native unscoping interpolation must not
    // change the unscoped result, even when the configured FOV equals base.
    for (float native : {40.f, 65.f, 80.f, 90.f, 100.f, 110.f}) {
        Near(fov::Adjust(native, 90, false, {110, true}), 110);
        Near(fov::Adjust(native, 90, false, {90, true}), 90);
        Near(fov::Adjust(native, 100, false, {110, true}), 110);
    }
    constexpr double halfRadians = std::numbers::pi / 360.;
    float previous = 0;
    for (int native = 10; native <= 90; ++native) {
        const float output = fov::Adjust(static_cast<float>(native), 90, true, {120, true});
        CHECK(output > previous); previous = output;
        const double originalZoom = std::tan(90 * halfRadians) / std::tan(native * halfRadians);
        const double adjustedZoom = std::tan(120 * halfRadians) / std::tan(output * halfRadians);
        CHECK(std::abs(originalZoom - adjustedZoom) < .00001);
        Near(fov::Adjust(static_cast<float>(native), 90, true, {90, true}), static_cast<float>(native));
    }
    Near(fov::Adjust(40, 0, true, {110, true}), 40);
    Near(fov::Adjust(0, 90, false, {110, false}), 0);
    Near(fov::Adjust(180, 90, false, {110, true}), 180);
    CHECK(std::isnan(fov::Adjust(std::numeric_limits<float>::quiet_NaN(), 90, false, {})));
    CHECK(std::isinf(fov::Adjust(std::numeric_limits<float>::infinity(), 90, false, {})));
}
struct Memory {
    std::unordered_map<std::uintptr_t, unsigned char> bytes;
    std::uintptr_t changingAddress = 0;
    unsigned changingReads = 0;
    std::uintptr_t watchedAddress = 0;
    unsigned watchedReads = 0;
    std::uintptr_t transitionAt = 0, transitionAddress = 0;
    int transitionValue = 0;
    template<class T> void Put(std::uintptr_t address, T value) {
        const auto* data = reinterpret_cast<const unsigned char*>(&value);
        for (unsigned i = 0; i < sizeof(T); ++i) bytes[address + i] = data[i];
    }
    sdk::players::Memory Reader() {
        return {this, [](void* context, std::uintptr_t address, void* out, std::size_t size) {
            auto& self = *static_cast<Memory*>(context);
            if (address == self.watchedAddress) ++self.watchedReads;
            if (address == self.transitionAt) self.Put(self.transitionAddress, self.transitionValue);
            if (address == self.changingAddress && ++self.changingReads == 2) return false;
            for (std::size_t i = 0; i < size; ++i) {
                auto found = self.bytes.find(address + i); if (found == self.bytes.end()) return false;
                static_cast<unsigned char*>(out)[i] = found->second;
            }
            return true;
        }};
    }
};
void OwnershipTests() {
    namespace p = sdk::players::offsets; namespace c = sdk::camera::offsets;
    constexpr std::uintptr_t client = 0x10000000, engine = 0x20000000, network = 0x30000000, world = 0x40000000;
    constexpr std::uintptr_t chunk = 0x50000000, controller = 0x60000000, pawn = 0x70000000, camera = 0x71000000;
    constexpr std::uint32_t handle = 0x8002;
    const auto slot = chunk + (handle & p::indexMask) * p::stride;
    Memory m;
    m.Put(client + p::entitySystem, world); m.Put(engine + p::networkClient, network); m.Put(network + p::signOnState, 6);
    m.Put(client + p::localController, controller); m.Put(client + p::localPawn, pawn);
    m.Put(controller + p::controllerPawn, handle); m.Put(world + 0x10, chunk);
    m.Put(slot, pawn); m.Put(slot + 0x10, handle); m.Put(slot + 0x30, 0u); m.Put(pawn + p::identity, slot);
    m.Put(pawn + p::health, 100); m.Put(pawn + p::lifeState, std::uint8_t{0}); m.Put(pawn + p::team, std::uint8_t{2});
    m.Put(pawn + c::cameraServices, camera); m.Put(camera + c::serviceOwner, pawn);
    auto valid = [&] { return sdk::camera::IsLocalCamera(m.Reader(), client, engine, camera); };
    CHECK(valid()); CHECK(!sdk::camera::IsLocalCamera(m.Reader(), client, engine, camera + 8));
    // The render path must reject an invalid session/Pawn BEFORE touching its
    // camera pointer, including when the old global Pawn is still non-null.
    sdk::players::Matrix matrix{}; matrix[1][1] = 2; matrix[3][2] = 1;
    m.Put(client + p::viewMatrix, matrix);
    float scale = 99;
    auto projection = [&] { scale = 99; return sdk::camera::ReadVerticalProjection(m.Reader(), client, engine, scale); };
    CHECK(projection()); Near(scale, 2);
    m.watchedAddress = pawn + c::cameraServices;
    auto rejectedBeforeCamera = [&] {
        m.watchedReads = 0;
        CHECK(!projection() && scale == 0 && m.watchedReads == 0);
    };
    m.Put(network + p::signOnState, 3); rejectedBeforeCamera(); m.Put(network + p::signOnState, 6);
    m.Put(slot, pawn + 8); rejectedBeforeCamera(); m.Put(slot, pawn);
    m.Put(slot + 0x10, handle + 0x8000); rejectedBeforeCamera(); m.Put(slot + 0x10, handle);
    m.Put(pawn + p::identity, slot + 8); rejectedBeforeCamera(); m.Put(pawn + p::identity, slot);
    m.Put(pawn + p::health, 0); rejectedBeforeCamera(); m.Put(pawn + p::health, 100);
    m.Put(pawn + p::lifeState, std::uint8_t{1}); rejectedBeforeCamera(); m.Put(pawn + p::lifeState, std::uint8_t{0});
    m.bytes.erase(pawn + c::cameraServices); CHECK(!projection() && scale == 0); m.Put(pawn + c::cameraServices, camera);
    m.Put(camera + c::serviceOwner, pawn + 8); CHECK(!projection() && scale == 0); m.Put(camera + c::serviceOwner, pawn);
    m.changingAddress = client + p::localPawn; CHECK(!projection() && scale == 0);
    m.changingAddress = 0; m.changingReads = 0;
    // Disconnect/death during the matrix read must invalidate the result too.
    m.transitionAt = client + p::viewMatrix;
    m.transitionAddress = network + p::signOnState; m.transitionValue = 3;
    CHECK(!projection() && scale == 0); m.Put(network + p::signOnState, 6);
    m.transitionAddress = pawn + p::health; m.transitionValue = 0;
    CHECK(!projection() && scale == 0); m.Put(pawn + p::health, 100); m.transitionAt = 0;
    matrix[1][1] = std::numeric_limits<float>::quiet_NaN(); m.Put(client + p::viewMatrix, matrix);
    CHECK(!projection() && scale == 0);
    matrix[1][1] = 2; matrix[3][2] = 0; m.Put(client + p::viewMatrix, matrix);
    CHECK(!projection() && scale == 0);
    matrix[3][2] = 1; m.Put(client + p::viewMatrix, matrix); CHECK(projection()); Near(scale, 2);
    bool scoped = true;
    auto readScope = [&] { return sdk::camera::IsLocalCamera(m.Reader(), client, engine, camera, &scoped); };
    CHECK(!readScope() && scoped); // Missing scope data must not invent unscoped state.
    m.Put(pawn + c::isScoped, std::uint8_t{0}); CHECK(readScope() && !scoped);
    m.Put(pawn + c::isScoped, std::uint8_t{1}); CHECK(readScope() && scoped);
    m.Put(pawn + c::isScoped, std::uint8_t{2}); CHECK(!readScope() && scoped);
    m.Put(pawn + c::isScoped, std::uint8_t{0});
    m.changingAddress = client + p::localPawn; CHECK(!readScope() && scoped);
    m.changingAddress = 0; m.changingReads = 0;
    CHECK(readScope() && !scoped);
    m.Put(camera + c::serviceOwner, pawn + 8); CHECK(!valid()); m.Put(camera + c::serviceOwner, pawn);
    m.Put(pawn + p::health, 0); CHECK(!valid()); m.Put(pawn + p::health, 100);
    m.Put(pawn + p::lifeState, std::uint8_t{1}); CHECK(!valid()); m.Put(pawn + p::lifeState, std::uint8_t{0});
    m.Put(pawn + p::team, std::uint8_t{1}); CHECK(!valid()); m.Put(pawn + p::team, std::uint8_t{3}); CHECK(valid());
    m.Put(network + p::signOnState, 3); CHECK(!valid()); m.Put(network + p::signOnState, 6);
    m.Put(controller + p::controllerPawn, handle + 0x8000); CHECK(!valid()); m.Put(controller + p::controllerPawn, handle);
    m.Put(slot + 0x10, handle + 0x8000); m.Put(slot + 0x30, 1u); CHECK(valid()); // Serial adjustment.
    m.Put(pawn + p::identity, slot + 8); CHECK(!valid()); m.Put(pawn + p::identity, slot);
    m.changingAddress = client + p::localPawn; CHECK(!valid()); m.changingAddress = 0;
    m.bytes.erase(pawn + c::cameraServices); CHECK(!valid());
    CHECK(!sdk::camera::ResolveBinding().target); // No CS2 DLL loaded into test process.
}
void PlayerViewTests() {
    namespace p = sdk::players::offsets; namespace c = sdk::camera::offsets;
    constexpr std::uintptr_t client = 0x10000000, engine = 0x20000000, network = 0x30000000;
    constexpr std::uintptr_t world = 0x40000000, chunk = 0x50000000, controller = 0x60000000;
    constexpr std::uintptr_t self = 0x70000000, target = 0x71000000, observer = 0x72000000;
    constexpr std::uintptr_t selfCamera = 0x73000000, targetCamera = 0x74000000;
    constexpr std::uint32_t selfHandle = 0x8002, targetHandle = 0x8003;
    Memory m;
    m.Put(engine + p::networkClient, network); m.Put(network + p::signOnState, 6);
    m.Put(client + p::entitySystem, world); m.Put(world + 0x10, chunk);
    m.Put(client + p::localController, controller); m.Put(controller + c::activePawn, selfHandle);
    m.Put(client + c::cinematicCamera, std::uintptr_t{0});
    const auto player = [&](std::uintptr_t pawn, std::uint32_t handle, std::uintptr_t camera) {
        const auto slot = chunk + (handle & p::indexMask) * p::stride;
        m.Put(slot, pawn); m.Put(slot + 0x10, handle); m.Put(slot + 0x30, 0u);
        m.Put(pawn + p::identity, slot); m.Put(pawn, client + c::playerVtable);
        m.Put(pawn + p::health, 100); m.Put(pawn + p::lifeState, std::uint8_t{0});
        m.Put(pawn + c::cameraServices, camera); m.Put(camera + c::serviceOwner, pawn);
        m.Put(camera + c::viewEntity, 0xFFFFFFFFu); m.Put(pawn + c::isScoped, std::uint8_t{0});
    };
    player(self, selfHandle, selfCamera); player(target, targetHandle, targetCamera);
    m.Put(self + c::observerServices, observer); m.Put(observer + c::serviceOwner, self);
    m.Put(observer + c::observerMode, std::uint8_t{0}); m.Put(observer + c::observerTarget, targetHandle);
    sdk::camera::PlayerView view;
    auto read = [&] { return sdk::camera::ReadPlayerView(m.Reader(), client, engine, view); };
    CHECK(read() && view.pawn == self && !view.spectating);
    // End-of-match cameras can coexist with a living local Pawn.
    m.Put(client + c::cinematicCamera, std::uintptr_t{0x75000000}); CHECK(!read() && !view.pawn);
    m.Put(client + c::cinematicCamera, std::uintptr_t{0});
    m.Put(selfCamera + c::viewEntity, std::uint32_t{0x8004}); CHECK(!read());
    m.Put(selfCamera + c::viewEntity, 0xFFFFFFFFu);
    // A dead/local observer Pawn must not prevent spectating a living target.
    m.Put(self + p::health, 0); m.Put(self + p::lifeState, std::uint8_t{1});
    m.Put(self, std::uintptr_t{0x12345000});
    m.Put(target + c::isScoped, std::uint8_t{1});
    for (std::uint8_t mode : {2, 3}) {
        m.Put(observer + c::observerMode, mode);
        CHECK(read() && view.pawn == target && view.service == targetCamera && view.spectating && view.scoped);
    }
    for (std::uint8_t mode : {0, 1, 4, 5}) {
        m.Put(observer + c::observerMode, mode); CHECK(!read());
    }
    m.Put(observer + c::observerMode, std::uint8_t{2});
    m.Put(target + p::health, 0); CHECK(!read()); m.Put(target + p::health, 100);
    m.Put(observer + c::observerTarget, targetHandle + 0x8000); CHECK(!read());
    m.Put(observer + c::observerTarget, targetHandle);
    m.Put(target, std::uintptr_t{0x12345000}); CHECK(!read()); m.Put(target, client + c::playerVtable);
    m.Put(client + c::cinematicCamera, std::uintptr_t{0x75000000}); CHECK(!read());
    // An ordinary active ICamera must not suppress player FOV. The native
    // override checks CDataDrivenCamera, including derived camera classes.
    constexpr std::uintptr_t cameraTable = client + 0x1000, locator = client + 0x2000;
    m.Put(std::uintptr_t{0x75000000}, cameraTable); m.Put(cameraTable - 8, locator);
    m.Put(locator, 1u); m.Put(locator + 20, 0x2000u); m.Put(locator + 16, 0x3000u);
    m.Put(client + 0x3008, 2u); m.Put(client + 0x300C, 0x4000u);
    m.Put(client + 0x4000, 0x5000u); m.Put(client + 0x4004, 0x5010u);
    m.Put(client + 0x5000, 0x6000u); m.Put(client + 0x5010, 0x6010u);
    CHECK(read());
    m.Put(client + 0x5010, c::dataDrivenCameraType); CHECK(!read());
    m.Put(client + 0x5010, 0x6010u); CHECK(read());
    m.Put(client + 0x3008, 65u); CHECK(!read()); m.Put(client + 0x3008, 2u);
    m.Put(client + c::cinematicCamera, std::uintptr_t{0}); CHECK(read());
    m.transitionAt = target + c::isScoped; m.transitionAddress = observer + c::observerTarget;
    m.transitionValue = selfHandle; CHECK(!read()); m.transitionAt = 0;
    m.Put(observer + c::observerTarget, targetHandle);
    m.Put(network + p::signOnState, 3); CHECK(!read()); m.Put(network + p::signOnState, 6);
    CHECK(read());
}
struct Camera {
    float native = 90, base = 90; bool local = true, scoped = false; unsigned evaluations = 0;
    float rendered = 0;
    sdk::weapons::Angle raw{10, 20, 0}, angles;
    bool firstPerson = true;
};
std::atomic<bool> block{false}, entered{false}, released{false};
bool throwOriginal = false, throwCapture = false;
#pragma optimize("", off)
__declspec(noinline) std::intptr_t __fastcall GameFov(void* context, void* view) {
    auto& camera = *static_cast<Camera*>(context);
    CHECK(view == context);
    ++camera.evaluations;
    if (throwOriginal) RaiseException(0xE0424242, 0, 0, nullptr);
    if (block.load()) { entered = true; while (!released.load()) Sleep(1); }
    // Simulate the scoped-camera overwrite AFTER an early world-FOV getter.
    camera.rendered = 110;
    camera.rendered = camera.native;
    camera.angles = {camera.raw.pitch + 4, camera.raw.yaw + 2, 3};
    return 12345;
}
#pragma optimize("", on)
using Resolve = std::intptr_t (__fastcall*)(void*, void*);
Resolve volatile callView = GameFov;
float invoke(Camera* camera) { CHECK(callView(camera, camera) == 12345); return camera->rendered; }
bool CaptureView(void* context, void* view, sdk::camera::View& value) {
    if (throwCapture) RaiseException(0xE0424242, 0, 0, nullptr);
    auto& camera = *static_cast<Camera*>(context);
    CHECK(view == context);
    value = {&camera.rendered, &camera.angles, {camera.base, camera.scoped}, camera.raw, camera.firstPerson};
    return camera.local;
}
bool Fault(Camera* camera) {
    __try { invoke(camera); return false; }
    __except(GetExceptionCode() == 0xE0424242 ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) { return true; }
}
void HookTests() {
    CHECK(MH_Initialize() == MH_OK);
    const sdk::camera::Binding binding{reinterpret_cast<void*>(&GameFov), CaptureView};
    CHECK(!hooks::fov::Install({})); CHECK(hooks::fov::Install(binding));
    Camera camera;
    Near(invoke(&camera), 90); CHECK(camera.evaluations == 1);
    modules::ModuleManager manager;
    auto& module = manager.Create<modules::FovControl>(); manager.Seal();
    CHECK(module.category() == "render" && !module.requestedEnabled() && module.settings().size() == 5);
    Properties::NumberProp* degrees = nullptr; Properties::BoolProperty *zoom = nullptr, *shake = nullptr;
    for (const auto& row : module.settings()) {
        if (std::string_view(row.getName()) == "FOV") degrees = dynamic_cast<Properties::NumberProp*>(row.getProp());
        if (std::string_view(row.getName()) == "Preserve Zoom") zoom = dynamic_cast<Properties::BoolProperty*>(row.getProp());
        if (std::string_view(row.getName()) == "Remove View Shake") shake = dynamic_cast<Properties::BoolProperty*>(row.getProp());
    }
    CHECK(degrees && zoom && degrees->get() == 90 && zoom->get());
    module.setEnabled(true); degrees->set(110); manager.Tick(); Near(invoke(&camera), 110);
    CHECK(camera.native == 90 && camera.base == 90 && camera.evaluations == 2);
    CHECK(shake && shake->get()); Near(camera.angles.pitch, 10); Near(camera.angles.yaw, 20); Near(camera.angles.roll, 0);
    camera.firstPerson = false; invoke(&camera); Near(camera.angles.pitch, 14); camera.firstPerson = true;
    shake->set(false); manager.Tick(); invoke(&camera); Near(camera.angles.pitch, 14);
    shake->set(true); manager.Tick(); invoke(&camera); Near(camera.angles.pitch, 10);
    degrees->set(110.3); manager.Tick(); Near(invoke(&camera), 110.3f); degrees->set(110); manager.Tick();
    for (float native : {80.f, 100.f, 40.f}) {
        camera.native = native; Near(invoke(&camera), 110);
    }
    camera.base = 100; Near(invoke(&camera), 110); camera.base = 90;
    camera.scoped = true; const auto zoomed = invoke(&camera); CHECK(zoomed > 40 && zoomed < 110);
    camera.scoped = false; Near(invoke(&camera), 110); // Unscope before native FOV settles.
    camera.native = 75; Near(invoke(&camera), 110);
    camera.native = 40; camera.scoped = true; Near(invoke(&camera), zoomed);
    zoom->set(false); manager.Tick(); Near(invoke(&camera), 110);
    camera.local = false; Near(invoke(&camera), 40); camera.local = true;
    auto saved = nova::json::Value::Object(); manager.Save(saved);
    degrees->set(70); zoom->set(true); module.setEnabled(false); manager.Tick(); Near(invoke(&camera), 40);
    manager.Load(saved); manager.Tick(); CHECK(degrees->get() == 110 && !zoom->get()); Near(invoke(&camera), 110);
    zoom->set(true); manager.Tick(); Near(invoke(&camera), zoomed);
    throwOriginal = true; CHECK(Fault(&camera)); throwOriginal = false;
    block = true; entered = false; released = false;
    float pendingResult = 0;
    std::thread pending([&] { pendingResult = invoke(&camera); });
    for (int i = 0; i < 1000 && !entered; ++i) Sleep(1);
    CHECK(entered && !hooks::fov::Stop(10)); released = true; pending.join(); block = false;
    Near(pendingResult, 40); CHECK(hooks::fov::Stop(1000)); hooks::fov::Finalize(); manager.Shutdown();
    CHECK(hooks::fov::Install(binding)); Near(invoke(&camera), 40); // Reinstall is disabled.
    hooks::fov::Publish({120, false}, true); Near(invoke(&camera), 120);
    throwCapture = true; Near(invoke(&camera), 40); throwCapture = false;
    CHECK(!hooks::fov::Available()); Near(invoke(&camera), 40);
    CHECK(hooks::fov::Stop(1000)); hooks::fov::Finalize();
    CHECK(MH_Uninitialize() == MH_OK);
}
}
void TestFov() {
    ProjectionTests(); OwnershipTests(); PlayerViewTests(); HookTests();
    puts("PASS: final-view FOV after scoped overwrite, fractional FOV, view shake, scope/ownership guards, zoom transitions, native ABI/config/side effects/fault/drain/reinstall; no game injection");
}
