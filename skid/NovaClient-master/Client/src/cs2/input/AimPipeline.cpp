#include "cs2/input/AimPipeline.h"

namespace nova::cs2::input {
void AimPipeline::Configure(recoil::Settings recoil, bool rcs, aim::Settings aim, bool assist) {
    recoil = cs2::recoil::Normalize(recoil); aim = cs2::aim::Normalize(aim);
    if (recoil != recoilSettings_ || rcs != recoilEnabled_) {
        recoil_.Reset(); recoil_.Configure(recoil); results_ = {}; next_ = 0;
        aim_.Reset();
    }
    if (aim != aimSettings_ || assist != aimEnabled_) ResetAim();
    recoilSettings_ = recoil; aimSettings_ = aim; recoilEnabled_ = rcs; aimEnabled_ = assist;
}
void AimPipeline::Reset() { recoil_.Reset(); aim_.Reset(); results_ = {}; next_ = 0; visualOffset_ = {}; }
void AimPipeline::ResetAim() { aim_.Reset(); visualOffset_ = {}; }
sdk::weapons::Angle AimPipeline::Process(sdk::weapons::Sample sample, const sdk::aim::Snapshot& snapshot) {
    struct Provided { const sdk::aim::Snapshot& snapshot; } provided{snapshot};
    return Process(sample, &provided, [](void* data, const sdk::weapons::Sample&, sdk::weapons::Angle, sdk::aim::Snapshot& out) {
        out = static_cast<Provided*>(data)->snapshot; return true;
    });
}
sdk::weapons::Angle AimPipeline::Process(sdk::weapons::Sample sample, void* context, CaptureTargets capture) {
    // Undo only our output on repeated samples before either controller sees it.
    // A changed raw input for a replay remains authoritative.
    for (auto& saved : results_) {
        if (!saved.valid || saved.source.weapon.identity != sample.weapon.identity ||
            saved.source.frame != sample.frame || saved.source.ordinal != sample.ordinal || saved.source.time != sample.time) continue;
        if (sample.view.pitch == saved.output.pitch && sample.view.yaw == saved.output.yaw && sample.view.roll == saved.output.roll)
            sample.view = saved.source.view;
        for (auto& future : results_)
            if (future.valid && (future.source.frame > sample.frame ||
                (future.source.frame == sample.frame && future.source.ordinal >= sample.ordinal))) future.valid = false;
        break;
    }
    auto result = recoilEnabled_ ? recoil_.Process(sample) : sample.view;
    const auto applied = recoilEnabled_ ? recoil_.Contribution() : sdk::weapons::Angle{};
    visualOffset_ = {-applied.pitch, -applied.yaw, 0};
    if (aimEnabled_) {
        auto compensation = recoilEnabled_ ? recoil_.Contribution() : sdk::weapons::Angle{};
        if (aimSettings_.compensateRecoil)
            compensation = {-sample.punch.pitch, -sample.punch.yaw, 0};
        const auto current = aimSettings_.compensateRecoil ? sample.view : result;
        const auto reference = aim_.Reference(sample, aimSettings_, current, compensation);
        sdk::aim::Snapshot snapshot;
        if (!capture || !capture(context, sample, reference, snapshot)) { ResetAim(); snapshot = {}; }
        // Absolute recoil correction owns aiming while a target is acquired;
        // standalone RCS remains the fallback when there is no target.
        const auto aimed = aim_.Process(sample, snapshot, aimSettings_, current, compensation);
        if (aim_.Target() && aim_.Acting()) {
            result = aimed;
            if (aimSettings_.compensateRecoil) visualOffset_ = {
                aimSettings_.vertical ? sample.punch.pitch : 0,
                aimSettings_.horizontal ? sample.punch.yaw : 0, 0};
        }
    }
    results_[next_++ % results_.size()] = {sample, result, true};
    return result;
}
}
