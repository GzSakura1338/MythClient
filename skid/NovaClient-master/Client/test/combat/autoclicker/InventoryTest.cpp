#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <format>
#include <functional>
#include <map>
#include <memory>
#include <random>
#include <string>
#include <vector>

using ULONGLONG = unsigned long long;
constexpr int VK_LBUTTON = 1, VK_SHIFT = 16;
namespace fixture {
ULONGLONG now = 1000;
bool left = true, cachedLeft = true, shift = false, foreground = true;
bool inventory = true, gui = false;
int slot = 3, attacks = 0;
bool lunar = false, pulsePending = false;
int pulses = 0;
int missTime = 0;
struct Click { int slot; bool shift; };
std::vector<Click> clicks;
std::vector<char> packets;
}
short GetAsyncKeyState(int key) {
    return (key == VK_LBUTTON ? fixture::left : fixture::shift) ? short(0x8000) : 0;
}
ULONGLONG NowMillis() { return fixture::now; }
namespace Properties {
struct BoolProperty { bool value; bool get() const { return value; } };
struct NumberProp { double value; double get() const { return value; } };
}
namespace nova {
namespace compat { bool IsLegacyLunar() { return fixture::lunar; } }
namespace glhook {
bool QueueAttackMousePulse() {
    assert(fixture::missTime == 0);
    if (fixture::pulsePending) return false;
    fixture::pulsePending = true;
    ++fixture::pulses;
    return true;
}
void CancelAttackMousePulse() { fixture::pulsePending = false; }
}
namespace event {
enum class EventType { Pre, Post };
struct MotionEvent { EventType type; };
struct Subscription { std::shared_ptr<std::function<void(MotionEvent&)>> fn; };
struct EventBus {
    std::weak_ptr<std::function<void(MotionEvent&)>> listener;
    static EventBus& Get() { static EventBus bus; return bus; }
    template<class E, class F> Subscription Subscribe(F f) {
        auto fn = std::make_shared<std::function<void(MotionEvent&)>>(f);
        listener = fn;
        return {fn};
    }
    void Post(EventType type) {
        MotionEvent e{type};
        if (auto fn = listener.lock()) (*fn)(e);
    }
};
}
namespace input {
enum class MouseButton { Left };
bool IsMouseButtonDown(MouseButton) { return fixture::cachedLeft; }
bool IsKeyDown(int) { return fixture::shift; }
bool IsProcessForeground() { return fixture::foreground; }
}
namespace clickgui { bool IsOpen() { return fixture::gui; } }
namespace java {
bool InventoryScreenOpen() { return fixture::inventory; }
bool ScreenOpen() { return fixture::inventory; }
bool PlayerUsingItem() { return false; }
bool PlayerInCreative() { return false; }
bool MouseGrabbed() { return true; }
bool HoldingWeapon() { return true; }
bool PlayerCanEdit() { return true; }
bool TargetedBlockBreakable() { return false; }
void SetAttackDown(bool) {}
void ClearMissTime() { fixture::missTime = 0; }
void ClickAttack() { assert(fixture::missTime == 0); ++fixture::attacks; }
bool ClickHoveredSlot(bool quick) {
    if (!fixture::inventory || fixture::slot < 0) return false;
    fixture::clicks.push_back({fixture::slot, quick});
    fixture::packets.push_back('C');
    return true;
}
}
namespace module {
enum class Category { Combat };
class Module {
public:
    Module(const char*, Category, const char*, const char*) {}
    virtual ~Module() = default;
    bool isEnabled() const { return enabled; }
    void Enable(bool value) { enabled = value; if (value) onEnable(); else onDisable(); }
    void Render() { if (enabled) onUpdate(); }
    void Bool(const char* key, bool value) { booleans.at(key).value = value; }
    void Number(const char* key, double value) { numbers.at(key).value = value; }
protected:
    virtual void onEnable() {}
    virtual void onDisable() {}
    virtual void onUpdate() {}
    Properties::BoolProperty& addBool(const char* key, bool value = false) {
        return booleans.emplace(key, Properties::BoolProperty{value}).first->second;
    }
    Properties::NumberProp& addNumber(const char* key, double, double, double, double value,
        std::vector<int>, std::function<std::string(double)>) {
        return numbers.emplace(key, Properties::NumberProp{value}).first->second;
    }
private:
    bool enabled = false;
    std::map<std::string, Properties::BoolProperty> booleans;
    std::map<std::string, Properties::NumberProp> numbers;
};
// @PRODUCTION_AUTOCLICKER_CLASS@
}
}

void Pre() { nova::event::EventBus::Get().Post(nova::event::EventType::Pre); }
void Post() { nova::event::EventBus::Get().Post(nova::event::EventType::Post); }

int main() {
    using namespace fixture;
    nova::module::AutoClickerModule module;
    module.Bool("Inventory", true);
    module.Enable(true);
    module.Render(); Pre(); // Start delay begins on the game tick.
    now = 1099; Pre(); assert(clicks.empty());
    now = 1100; module.Render(); Post(); assert(clicks.empty());
    slot = 7; shift = true; Pre();
    assert(clicks.size() == 1 && clicks.back().slot == 7 && clicks.back().shift);
    now = 5000; module.Render(); Post(); assert(clicks.size() == 1);
    Pre(); assert(clicks.size() == 2); // Long stall produces one click only.
    Pre(); assert(clicks.size() == 2);

    // Fresh physical state must override the cached render-frame input.
    left = false; now = 6000; Pre(); assert(clicks.size() == 2);
    left = true; Pre(); now += 99; Pre(); assert(clicks.size() == 2);
    ++now; Pre(); assert(clicks.size() == 3);

    for (int condition = 0; condition < 5; ++condition) {
        const auto before = clicks.size();
        now += 1000;
        if (condition == 0) inventory = false;
        if (condition == 1) gui = true;
        if (condition == 2) foreground = false;
        if (condition == 3) module.Bool("Inventory", false);
        if (condition == 4) module.Enable(false);
        module.Render(); Pre(); Post(); assert(clicks.size() == before);
        inventory = true; gui = false; foreground = true;
        module.Bool("Inventory", true);
        if (condition == 4) module.Enable(true);
        Pre(); now += 99; Pre(); assert(clicks.size() == before);
        ++now; Pre(); assert(clicks.size() == before + 1);
    }

    // No retained slot: resolve the live hover when the action is due.
    now += 1000; slot = -1; Pre(); const auto before = clicks.size();
    now += 1000; slot = 11; shift = false; Pre();
    assert(clicks.size() == before + 1 && clicks.back().slot == 11 && !clicks.back().shift);

    // Replay variable render cadence and transactions through the Post state
    // transitions: movement clears pending clicks, transactions examine them.
    packets.clear();
    for (int tick = 0; tick < 200; ++tick) {
        now += tick % 17 == 0 ? 2000 : 50;
        for (int frame = 0; frame < tick % 5; ++frame) {
            module.Render(); packets.push_back('T');
        }
        Pre(); packets.push_back('M'); Post();
        module.Render(); packets.push_back('T');
    }
    bool flying = false, pending = false;
    int flags = 0, emitted = 0;
    for (char packet : packets) {
        if (packet == 'M') { flying = true; pending = false; }
        if (packet == 'C') { ++emitted; if (flying) pending = true; }
        if (packet == 'T') { if (flying && pending) ++flags; flying = pending = false; }
    }
    assert(flags == 0 && emitted > 0 && emitted <= 200);

    // Combat clicks remain on the existing render path.
    inventory = false; now += 1000; module.Render();
    const int previous = attacks;
    missTime = 10; // Air swing immediately before acquiring an entity.
    now += 1000; module.Render(); assert(attacks == previous + 1);
    lunar = true;
    const int direct = attacks;
    missTime = 10;
    now += 1000; module.Render(); assert(pulses == 1 && attacks == direct);
    now += 60000; module.Render(); assert(pulses == 1); // One outstanding pulse.
    pulsePending = false;
    missTime = 10; // A subsequent air swing must not reintroduce the pause.
    now += 60000; module.Render(); assert(pulses == 2 && attacks == direct);
    module.Render(); assert(pulses == 2); // No catch-up burst after a stall.
    left = false; module.Render(); assert(!pulsePending);
    left = true; module.Render(); now += 1000; module.Render(); assert(pulsePending);
    foreground = false; module.Render(); assert(!pulsePending);
    foreground = true; module.Render(); now += 1000; module.Render(); assert(pulsePending);
    gui = true; module.Render(); assert(!pulsePending);
    gui = false; module.Render(); now += 1000; module.Render(); assert(pulsePending);
    module.Enable(false); assert(!pulsePending);
    lunar=false;left=true;foreground=true;gui=false;inventory=false;
    module.Bool("Gaussian random",true);
    for(double cps:{1.0,5.0,10.5,20.0}) {
        module.Number("Min CPS",cps);module.Number("Max CPS",cps);
        module.Enable(true);module.Render();const int count=attacks;
        const auto delay=static_cast<ULONGLONG>(std::lround(1000.0/cps));
        now+=delay-1;module.Render();assert(attacks==count);
        ++now;module.Render();assert(attacks==count+1);module.Enable(false);
    }
    module.Number("Min CPS",16);module.Number("Max CPS",8); // Inverted endpoints.
    module.Enable(true);module.Render();
    ULONGLONG last=now;int center=0,edges=0,samples=0;
    while(samples<2000) {
        const int count=attacks;++now;module.Render();
        if(attacks!=count) {
            const auto delay=now-last;last=now;
            assert(delay>=62&&delay<=125);
            const double cps=1000.0/delay;
            if(cps>=10&&cps<=14)++center;else ++edges;
            ++samples;
        }
    }
    assert(center>edges*3);module.Enable(false);
    std::puts("PASS: inventory phase/order, live input, Lunar pulse routing, bounded catch-up and cancellation");
}
