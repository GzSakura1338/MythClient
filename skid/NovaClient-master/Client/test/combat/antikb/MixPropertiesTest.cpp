namespace nova::module {
class PredictionModuleBase : public Module {
public:
    PredictionModuleBase() : Module("AntiKB", Category::Combat, "General", "General") {}
    std::vector<Properties::PropDesc> rows;
    void addSettings(std::vector<Properties::PropDesc> value) { rows = std::move(value); }
};
}
// @MIX_PROPERTIES@
class Probe : public nova::module::MixModuleBase {
public:
    using MixModuleBase::AddMixSettings;
    using MixModuleBase::ReadMixSettings;
};
int main() {
    Probe p; bool visible = true;
    p.AddMixSettings([&] { return visible; });
    assert(p.rows.size() == 1 && !p.rows[0].hasProp() && p.rows[0].hasSubProp());
    auto& rows = p.rows[0].getSubProps();
    assert(rows.size() == 8);
    auto initial = p.ReadMixSettings();
    assert(initial.delayTicks == 2 && initial.reduceTicks == 3 && initial.jump && initial.airBuffer && initial.fakeCheck);
    assert(!initial.noBadPackets && !initial.groundDelay && !initial.airPush);
    auto& delay = *static_cast<Properties::NumberProp*>(rows[0].getProp());
    auto& reduce = *static_cast<Properties::NumberProp*>(rows[1].getProp());
    delay.set(5); reduce.set(6); assert(p.ReadMixSettings().delayTicks == 5 && p.ReadMixSettings().reduceTicks == 6);
    assert(!rows[6].isVisible());
    static_cast<Properties::BoolProperty*>(rows[5].getProp())->set(false);
    assert(rows[6].isVisible());
    nova::json::Value saved; reduce.save(saved); reduce.set(1); reduce.load(saved);
    assert(p.ReadMixSettings().reduceTicks == 6);
    visible = false; assert(!p.rows[0].isVisible());
    std::cout << "PASS: Mix production settings, defaults, nested gear, visibility and value round trip\n";
}
