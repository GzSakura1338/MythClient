#include "cs2/modules/ModuleManager.h"
#include "cs2/modules/misc/team/Team.h"
#include "bind/InputManager.h"
#include "bind/BindCapture.h"
#include "bind/UnloadKey.h"
#include "ui/clickgui/panel/ClientSettings.h"
#include "ui/clickgui/ClickGui.h"
#include <cstdio>
#include <cstdlib>
#include <stdexcept>

#define CHECK(x) do { if (!(x)) { fprintf(stderr, "Module test failed at %d: %s\n", __LINE__, #x); std::exit(1); } } while (false)
namespace {
using namespace nova::cs2::modules;
class ProbeMode final : public Mode {
public:
    ProbeMode(const char* name, std::vector<std::string>& events) : Mode(name), events_(events) {}
    Properties::NumberProp amount{0, 10, 1, 3, "{:.0f}"};
    std::vector<Properties::PropDesc> settings() override { return {{"Amount", &amount}}; }
    void onEnable() override { events_.push_back(name() + "+"); }
    void onDisable() override { events_.push_back(name() + "-"); }
    void onUpdate() override { events_.push_back(name() + "*"); }
    void onRender() override { events_.push_back(name() + "#"); }
private:
    std::vector<std::string>& events_;
};
class Probe final : public Module {
public:
    explicit Probe(std::vector<std::string>& events) : Module("Probe", "misc"), events_(events) {
        std::vector<std::unique_ptr<Mode>> modes;
        auto first = std::make_unique<ProbeMode>("A", events); a = first.get();
        auto second = std::make_unique<ProbeMode>("B", events); b = second.get();
        modes.push_back(std::move(first)); modes.push_back(std::move(second)); setModes(std::move(modes));
    }
    ProbeMode *a, *b;
private:
    void onEnable() override { events_.push_back("M+"); }
    void onDisable() override { events_.push_back("M-"); }
    void onUpdate() override { events_.push_back("M*"); }
    void onRender() override { events_.push_back("M#"); }
    std::vector<std::string>& events_;
};
}
void TestUnloadBinding() {
    using namespace nova::bind;
    BindProperty* setting = nullptr;
    for (const auto& row : nova::menu::panel::ClientSettings::Get().rows())
        if (std::string_view(row.getName()) == "Unload Key") setting = dynamic_cast<BindProperty*>(row.getProp());
    CHECK(setting && setting->keyOnly() && setting->keybind() == VK_DELETE);
    setting->setHold(true); CHECK(!setting->hold());
    CHECK(unload::Input(VK_DELETE, true));
    CHECK(!unload::Input(VK_DELETE, true));
    unload::Input(VK_DELETE, false);
    auto& capture = BindCapture::Get();
    capture.Begin(setting);
    CHECK(!unload::Input(VK_DELETE, true));
    capture.onKey(VK_DELETE, true);
    CHECK(!capture.listening() && !unload::Input(VK_DELETE, true));
    unload::Input(VK_DELETE, false);
    CHECK(unload::Input(VK_DELETE, true));
    unload::Input(VK_DELETE, false);
    capture.Begin(setting); capture.onKey(VK_F9, true);
    CHECK(setting->keybind() == VK_F9 && !unload::Input(VK_F9, true));
    unload::Input(VK_F9, false);
    CHECK(!unload::Input(VK_DELETE, true) && unload::Input(VK_F9, true));
    unload::Input(VK_F9, false);
    Properties::Json saved; setting->save(saved);
    setting->setKeybind(-5); unload::Input(-5, false);
    CHECK(unload::Input(-5, true)); unload::Input(-5, false);
    setting->load(saved); CHECK(setting->keybind() == VK_F9 && unload::Key() == VK_F9);
    unload::Input(VK_F9, false);
    CHECK(!unload::Observe(unload::state.load(), true, false)); // Out of foreground.
    CHECK(!unload::Input(VK_F9, true)); // Focus returning while still held.
    unload::Input(VK_F9, false); CHECK(unload::Input(VK_F9, true));
    setting->setKeybind(0); CHECK(!unload::Input(VK_F9, true));
    setting->setKeybind(VK_DELETE); unload::Input(VK_DELETE, false);
    puts("PASS: unload default, keyboard/mouse rebind, capture suppression, release latch, focus, config and clear");
}
void TestModules() {
    {
        ModuleManager teams;
        auto& team = teams.Create<Team>();
        teams.Seal(); teams.Tick();
        CHECK(teams.InCategory("misc").size() == 1 && !team.ShouldIgnore(2, 2));
        team.setEnabled(true); teams.Tick();
        CHECK(team.ShouldIgnore(2, 2) && team.ShouldIgnore(3, 3));
        CHECK(!team.ShouldIgnore(2, 3) && !team.ShouldIgnore(3, 2));
        CHECK(!team.ShouldIgnore(0, 0) && !team.ShouldIgnore(1, 1) && !team.ShouldIgnore(1, 2));
        auto config = nova::json::Value::Object(); teams.Save(config);
        team.setEnabled(false); teams.Tick(); CHECK(!team.ShouldIgnore(2, 2));
        teams.Load(config); teams.Tick(); CHECK(team.ShouldIgnore(2, 2));
        auto old = nova::json::Value::Object();
        old["modules"]["2DESP"]["settings"]["Targets"]["Player Teammates"] = true;
        teams.Load(old); teams.Tick(); CHECK(!team.ShouldIgnore(2, 2));
        old["modules"]["2DESP"]["settings"]["Targets"]["Player Teammates"] = false;
        teams.Load(old); teams.Tick(); CHECK(team.ShouldIgnore(2, 2));
        old["modules"]["Team"]["enabled"] = false;
        teams.Load(old); teams.Tick(); CHECK(!team.ShouldIgnore(2, 2));
        team.keybind().setKeybind(VK_F10);
        nova::bind::InputManager::Get().onKey(VK_F10, true); teams.Tick();
        CHECK(team.ShouldIgnore(2, 2));
        nova::bind::InputManager::Get().onKey(VK_F10, false);
        teams.Shutdown(); CHECK(!team.ShouldIgnore(2, 2));
        puts("PASS: Team T/CT filtering, spectator guard, toggle/bind, config migration/precedence and shutdown");
    }
    std::vector<std::string> events;
    ModuleManager manager;
    auto& probe = manager.Create<Probe>(events);
    bool rejected = false;
    try { manager.Create<Probe>(events); } catch (const std::invalid_argument&) { rejected = true; }
    CHECK(rejected);
    manager.Seal(); manager.Tick(); CHECK(events.empty());
    rejected = false;
    try { manager.Create<Probe>(events); } catch (const std::logic_error&) { rejected = true; }
    CHECK(rejected);
    CHECK(manager.Find("Probe") == &probe && manager.InCategory("misc").size() == 1);
    probe.setEnabled(true); manager.Tick(); manager.RenderOverlays();
    CHECK((events == std::vector<std::string>{"M+", "A+", "M*", "A*", "M#", "A#"}));
    events.clear(); probe.modeProperty()->set(1); manager.Tick();
    CHECK((events == std::vector<std::string>{"A-", "B+", "M*", "B*"}));
    probe.a->amount.set(7); probe.b->amount.set(9); probe.hiddenInArrayListProperty().set(true);
    probe.keybind().setKeybind(-5);
    nova::json::Value saved = nova::json::Value::Object(); manager.Save(saved);
    probe.setEnabled(false); probe.modeProperty()->set(0); probe.a->amount.set(0); probe.b->amount.set(0);
    probe.hiddenInArrayListProperty().set(false); probe.keybind().setKeybind(0);
    manager.Load(saved);
    CHECK(probe.requestedEnabled() && probe.keybind().keybind() == -5 && probe.hiddenInArrayList());
    CHECK(probe.modeProperty()->get() == 1 && probe.a->amount.get() == 7 && probe.b->amount.get() == 9);
    CHECK(manager.Entries()[0].suffix == "B" && manager.Entries()[0].hidden);
    events.clear(); probe.setEnabled(false); manager.Tick();
    CHECK((events == std::vector<std::string>{"B-", "M-"}));
    // Mouse5 toggle, menu activation gate, and hold release through shared bindings.
    nova::bind::InputManager::Get().onMouseButton(4, true); CHECK(probe.requestedEnabled());
    nova::bind::InputManager::Get().onMouseButton(4, false); CHECK(probe.requestedEnabled());
    nova::bind::InputManager::Get().onMouseButton(4, true); CHECK(!probe.requestedEnabled());
    nova::clickgui::SetOpen(true);
    nova::bind::InputManager::Get().onMouseButton(4, true); CHECK(!probe.requestedEnabled());
    nova::clickgui::SetOpen(false);
    probe.keybind().setHold(true);
    nova::bind::InputManager::Get().onMouseButton(4, true); CHECK(probe.requestedEnabled());
    nova::clickgui::SetOpen(true);
    nova::bind::InputManager::Get().onMouseButton(4, false); CHECK(!probe.requestedEnabled());
    nova::clickgui::SetOpen(false);
    auto legacy = nova::json::Value::Object();
    legacy["features"]["Probe"]["Enabled"] = true;
    legacy["features"]["Probe"]["Mode"] = "A";
    manager.Load(legacy); manager.Tick(); CHECK(probe.isEnabled() && probe.suffix() == "A");
    events.clear(); manager.Shutdown(); manager.Shutdown(); manager.Tick(); manager.RenderOverlays();
    CHECK((events == std::vector<std::string>{"A-", "M-"}));
    puts("PASS: module registry, lifecycle, submode transitions, all-mode config, legacy config, mouse/hold binds, menu gate, shutdown");
}
