class PredictionProbe : public nova::module::PredictionModuleBase {
public:
    using PredictionModuleBase::BuildPredictionSettings;
    using PredictionModuleBase::ReadPredictionSettings;
};
int main() {
    PredictionProbe module;
    bool enabled = true;
    auto rows = module.BuildPredictionSettings([&] { return enabled; });
    auto row = [&](const char* name) -> Properties::PropDesc& {
        for (auto& r : rows) if (std::string(r.getName()) == name) return r;
        std::abort();
    };
    const auto initial = module.ReadPredictionSettings();
    assert(initial.reduce && initial.reduceMode == 0 && initial.smartTimes && initial.jump);
    assert(!initial.delay && initial.airBuffer && initial.fakeCheck && !initial.rotate);
    assert(!row("Attack Times").isVisible() && !row("Delay Ticks").isVisible());
    assert(row("SmartTimes").isVisible() && !row("StartReleaseTicks").isVisible());
    static_cast<Properties::BoolProperty*>(row("SmartTimes").getProp())->set(false);
    assert(row("Attack Times").isVisible());
    static_cast<Properties::BoolProperty*>(row("Delay").getProp())->set(true);
    assert(row("Delay Till On Ground").isVisible() && !row("Delay Ticks").isVisible());
    static_cast<Properties::BoolProperty*>(row("Delay Till On Ground").getProp())->set(false);
    assert(row("Delay Ticks").isVisible() && row("Ground Delay").isVisible());
    auto* mode = static_cast<Properties::ModeProperty*>(row("ReduceMode").getProp());
    mode->set(3);
    assert(row("StartReleaseTicks").isVisible() && !row("SmartTimes").isVisible());
    static_cast<Properties::NumberProp*>(row("StartReleaseTicks").getProp())->set(0);
    assert(module.ReadPredictionSettings().startReleaseTicks == 0);
    nova::json::Value stored;
    row("ReduceMode").getProp()->save(stored);
    mode->set(0); row("ReduceMode").getProp()->load(stored);
    assert(module.ReadPredictionSettings().reduceMode == 3);
    enabled = false;
    for (auto& r : rows) assert(!r.isVisible());
    std::cout << "PASS: Prediction production properties, defaults, gear visibility, zero thresholds and config serialization\n";
}
