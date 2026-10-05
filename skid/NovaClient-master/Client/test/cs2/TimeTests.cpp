#include "cs2/hooks/TimeHook.h"
#include "cs2/modules/render/time/Time.h"
#include "cs2/modules/ModuleManager.h"
#include "cs2/sdk/players/Offsets.h"
#include "MinHook.h"
#include <windows.h>
#include <intrin.h>
#include <atomic>
#include <thread>
#include <limits>
#include <unordered_map>
#include <cstdio>
#include <cstdlib>

#define CHECK(x) do { if (!(x)) { std::fprintf(stderr, "Time test failed at %d: %s\n", __LINE__, #x); std::exit(1); } } while (false)
namespace {
using namespace nova::cs2;
using namespace sdk::environment;
void Near(float a, float b) { CHECK(std::abs(a - b) < .0001f); }
void PresetTests() {
    Near(Scale({}), 1.25f); Near(Scale({Period::Night}), .2f);
    Near(Scale({Period::Day, 100}), 3); Near(Scale({Period::Night, 1, 0}), .05f);
    const float nan = std::numeric_limits<float>::quiet_NaN();
    Near(Scale({Period::Day, nan}), 1.25f); Near(Scale({Period::Night, 1, nan}), .2f);
    Near(Adjust(2, .2f), .4f); Near(Adjust(2, 1.25f), 2.5f);
    Near(Adjust(2, nan), 2); Near(Adjust(0, .2f), 0);
    CHECK(std::isnan(Adjust(nan, .2f)));
    const auto maximum = (std::numeric_limits<float>::max)(); Near(Adjust(maximum, 3), maximum);
    CHECK(!ResolveBinding().target);
}
struct Memory {
    std::unordered_map<std::uintptr_t, unsigned char> bytes;
    bool disconnect = false; unsigned signOnReads = 0;
    static constexpr std::uintptr_t client = 0x10000000, engine = 0x20000000, network = 0x30000000;
    template<class T> void Put(std::uintptr_t address, T value) {
        const auto* data = reinterpret_cast<const unsigned char*>(&value);
        for (unsigned i = 0; i < sizeof(T); ++i) bytes[address + i] = data[i];
    }
    sdk::players::Memory Reader() {
        return {this, [](void* context, std::uintptr_t address, void* out, std::size_t size) {
            auto& m = *static_cast<Memory*>(context);
            if (address == network + sdk::players::offsets::signOnState && ++m.signOnReads == 2 && m.disconnect)
                m.Put(address, 3);
            for (std::size_t i = 0; i < size; ++i) {
                const auto found = m.bytes.find(address + i);
                if (found == m.bytes.end()) return false;
                static_cast<unsigned char*>(out)[i] = found->second;
            }
            return true;
        }};
    }
};
void SessionTests() {
    namespace p = sdk::players::offsets;
    Memory m;
    m.Put(Memory::engine + p::networkClient, Memory::network);
    m.Put(Memory::client + p::entitySystem, std::uintptr_t{0x40000000});
    auto valid = [&] { m.signOnReads = 0; return InGame(m.Reader(), Memory::client, Memory::engine); };
    for (int signOn : {0, 1, 3, 5, 6}) {
        m.Put(Memory::network + p::signOnState, signOn); CHECK(valid() == (signOn == 6));
    }
    m.disconnect = true; CHECK(!valid()); m.disconnect = false;
    m.Put(Memory::network + p::signOnState, 6);
    m.Put(Memory::client + p::entitySystem, std::uintptr_t{0}); CHECK(!valid());
}
std::atomic<bool> block{false}, entered{false}, released{false};
bool inGame = true, throwNative = false, throwSession = false;
void* returnAddress = nullptr;
unsigned evaluations = 0;
#pragma optimize("", off)
__declspec(noinline) float __cdecl GamePower(float base, float exponent) {
    returnAddress = _ReturnAddress();
    ++evaluations;
    if (throwNative) RaiseException(0xE0424242, 0, 0, nullptr);
    if (block.load()) { entered = true; while (!released.load()) Sleep(1); }
    return std::pow(base, exponent);
}
using Power = float (__cdecl*)(float, float);
Power volatile callPower = GamePower;
__declspec(noinline) float Invoke(float base = 2, float exponent = 1) {
    const float value = callPower(base, exponent); return value;
}
__declspec(noinline) float OtherCaller() {
    const float value = callPower(2, 1); return value + 1;
}
#pragma optimize("", on)
bool Session() {
    if (throwSession) RaiseException(0xE0424242, 0, 0, nullptr);
    return inGame;
}
bool Fault() {
    __try { Invoke(); return false; }
    __except(GetExceptionCode() == 0xE0424242 ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) { return true; }
}
void HookTests() {
    Near(Invoke(), 2);
    const Binding binding{reinterpret_cast<void*>(&GamePower), returnAddress, Session};
    CHECK(MH_Initialize() == MH_OK);
    CHECK(!hooks::time::Install({})); CHECK(hooks::time::Install(binding));
    Near(Invoke(), 2);
    modules::ModuleManager manager;
    auto& module = manager.Create<modules::Time>(); manager.Seal();
    CHECK(module.category() == "render" && !module.requestedEnabled());
    Properties::ModeProperty* period = nullptr; Properties::NumberProp *day = nullptr, *night = nullptr;
    for (const auto& row : module.settings()) {
        const std::string_view name = row.getName();
        if (name == "Time") period = dynamic_cast<Properties::ModeProperty*>(row.getProp());
        if (name == "Day Brightness") day = dynamic_cast<Properties::NumberProp*>(row.getProp());
        if (name == "Night Brightness") night = dynamic_cast<Properties::NumberProp*>(row.getProp());
    }
    CHECK(period && day && night);
    module.setEnabled(true); manager.Tick(); Near(Invoke(), 2.5f); CHECK(module.suffix() == "Day");
    period->set(1); manager.Tick(); Near(Invoke(), .4f); CHECK(module.suffix() == "Night");
    for (unsigned i = 0; i < 100; ++i) Near(Invoke(), .4f); // No accumulating darkening.
    Near(OtherCaller(), 3); Near(Invoke(3, 1), 3); // Never affect other native math.
    inGame = false; Near(Invoke(), 2); inGame = true; Near(Invoke(), .4f);
    night->set(.35); manager.Tick(); Near(Invoke(), .7f);
    auto saved = nova::json::Value::Object(); manager.Save(saved);
    module.setEnabled(false); manager.Tick(); Near(Invoke(), 2);
    period->set(0); night->set(.5); manager.Load(saved); manager.Tick();
    CHECK(module.requestedEnabled() && period->get() == 1); Near(Invoke(), .7f);
    throwNative = true; CHECK(Fault()); throwNative = false;
    block = true; entered = false; released = false;
    float pendingResult = 0;
    std::thread pending([&] { pendingResult = Invoke(); });
    for (int i = 0; i < 1000 && !entered; ++i) Sleep(1);
    CHECK(entered && !hooks::time::Stop(10)); released = true; pending.join(); block = false;
    Near(pendingResult, 2); CHECK(hooks::time::Stop(1000)); hooks::time::Finalize();
    Near(Invoke(), 2); manager.Shutdown();
    CHECK(hooks::time::Install(binding)); Near(Invoke(), 2);
    hooks::time::Publish({Period::Night}, true); Near(Invoke(), .4f);
    throwSession = true; Near(Invoke(), 2); throwSession = false;
    CHECK(!hooks::time::Available()); Near(Invoke(), 2);
    CHECK(hooks::time::Stop(1000)); hooks::time::Finalize(); CHECK(MH_Uninitialize() == MH_OK);
}
}
void TestTime() {
    PresetTests(); SessionTests(); HookTests();
    puts("PASS: Time day/night exposure, caller isolation, session/disconnect guard, config, disable/fault/drain/reinstall; no game injection");
}
