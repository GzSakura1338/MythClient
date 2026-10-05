#include <algorithm>
#include <atomic>
#include <cmath>
#include <format>
#include <limits>
#include <memory>
#include <random>
#include <vector>

#include "minecraft/events/EventBus.h"
#include "minecraft/events/GameEvents.h"
#include "minecraft/compat/Profile.h"
#include "minecraft/compat/lunar189/LunarRuntime.h"
#include "minecraft/modules/Module.h"
#include "minecraft/modules/player/automlg/AutoMLG.h"
#include "minecraft/modules/player/autocharge/AutoCharge.h"
#include "minecraft/modules/Registry.h"
#include "minecraft/modules/player/breaker/Breaker.h"
#include "minecraft/modules/player/bridgeassist/BridgeAssistState.h"
#include "minecraft/java/api/game_bridgeassist.h"
#include "minecraft/java/api/game_click.h"
#include "minecraft/java/api/game_entity.h"
#include "minecraft/java/api/game_inventory.h"
#include "minecraft/java/api/game_place.h"
#include "minecraft/java/api/game_rotation.h"
#include "ui/clickgui/ClickGui.h"
#include "minecraft/utils/rotation/RotationManager.h"
#include "minecraft/utils/rotation/Mth.h"

namespace nova::module {
namespace {
class BridgeAssistModule final : public Module {
public:
    BridgeAssistModule() : Module("BridgeAssist", Category::Player, "General", "General") {
        auto aimFov = subNumber("FOV", 0, 360, 1, 360, [](double value) {
            return std::format("{:.0f} deg", value);
        });
        aimFov_ = static_cast<Properties::NumberProp*>(aimFov.getProp());
        prePlace_ = &addBool("Pre place", false, {std::move(aimFov)});
        auto holdBlock = subBool("Hold Block", true);
        holdBlock_ = static_cast<Properties::BoolProperty*>(holdBlock.getProp());
        switch_ = &addBool("Switch", false, {std::move(holdBlock)});
        auto number = [this](std::vector<Properties::PropDesc>& rows, const char* label,
                            double min, double max, double step, double value, Properties::NumberProp*& dest) {
            auto row = subNumber(label, min, max, step, value, [step](double v) {
                return step < 1 ? std::format("{:.2f}", v) : std::format("{:.0f} ms", v);
            });
            dest = static_cast<Properties::NumberProp*>(row.getProp()); rows.push_back(std::move(row));
        };
        std::vector<Properties::PropDesc> sneaking;
        number(sneaking, "Edge offset", 0, 0.3, 0.01, 0, edgeOffset_);
        number(sneaking, "Unsneak delay", 50, 300, 5, 50, unsneakDelay_);
        number(sneaking, "Sneak on jump", 0, 500, 5, 0, sneakOnJump_);
        std::vector<Properties::PropDesc> conditions;
        auto condition = [this, &conditions](const char* name, Properties::BoolProperty*& dest) {
            auto row = subBool(name, false); dest = static_cast<Properties::BoolProperty*>(row.getProp());
            conditions.push_back(std::move(row));
        };
        condition("Sneak key pressed", sneakKeyPressed_);
        condition("Holding blocks", holdingBlocks_);
        condition("Looking down", lookingDown_);
        condition("Not moving forward", notMovingForward_);
        addSettings({Properties::PropDesc("Sneaking", nullptr, std::move(sneaking)),
                     Properties::PropDesc("Conditions", nullptr, std::move(conditions))});
        auto& bus = event::EventBus::Get();
        input_ = bus.Subscribe<event::MoveInputEvent>([this](auto& e) { OnInput(e); }, event::Priority::Highest);
        packet_ = bus.Subscribe<event::PacketEvent>([this](auto& e) {
            if (placementArmed_.load(std::memory_order_acquire) && e.type == event::EventType::Outgoing && java::PacketIsUseItemOn(e.packet))
                placedPacket_.store(true, std::memory_order_release);
        });
        // Raven gathers this before vanilla mouse-over, not just before item use.
        rotation_ = bus.Subscribe<event::ClientRotationEvent>([this](auto&) { PrePlace(); });
        interact_ = bus.Subscribe<event::PrePlayerInteractEvent>([this](auto&) {
            // Modern profiles currently expose the interaction hook only.
            if (!compat::IsLegacyLunar()) PrePlace();
        });
        tick_ = bus.Subscribe<event::GameTickEvent>([this](auto& e) {
            if (e.type != event::EventType::Pre) return;
            const auto serial = reset_.load();
            if (serial != seenReset_ || java::ScreenOpen() || clickgui::IsOpen()) {
                ResetGameState(); seenReset_ = serial;
            }
            if (!switch_->get()) ReleaseSlot();
        });
    }
    std::string suffix() const override {
        const double value = edgeOffset_->get();
        if (value == std::round(value)) return std::format("{:.0f}", value);
        auto text = std::format("{:.2f}", value);
        if (text.back() == '0') text.pop_back();
        return text;
    }
private:
    double Random() {
        // The reflective loader does not register PE static TLS. Keep the engine
        // on the module; all consumers run on the Minecraft client thread.
        return std::generate_canonical<double, 53>(random_);
    }
    void onEnable() override { reset_.fetch_add(1); }
    void onDisable() override { placementArmed_.store(false); reset_.fetch_add(1); }
    void ReleaseSlot() {
        if (savedSlot_ < 0) return;
        // Never restore a previous world's selection onto a new player.
        int selected = -1;
        const auto self = java::PlayerEntity();
        if (savedSlot_ >= 0 && self && player_ && self.IsSame(player_) &&
            java::SelectedSlot(selected) && selected == visibleSlot_)
            java::SetSelectedSlot(savedSlot_);
        savedSlot_ = visibleSlot_ = -1;
    }
    void SwitchBlock() {
        if (!switch_->get()) { ReleaseSlot(); return; }
        if (holdBlock_->get() && !java::HoldingBlockItem()) {
            // A non-block selection belongs to the user; stop owning the slot.
            savedSlot_ = visibleSlot_ = -1;
            return;
        }
        // Use Scaffold's block filter and stack reserve, without taking its spoof slot.
        const int slot = java::FindPlaceableBlockSlot(1);
        if (slot < 0) { ReleaseSlot(); return; }
        int selected = -1;
        if (!java::SelectedSlot(selected)) return;
        const int restore = savedSlot_ < 0 || selected != visibleSlot_ ? selected : savedSlot_;
        if (selected != slot && !java::SetSelectedSlot(slot)) return;
        savedSlot_ = restore;
        visibleSlot_ = slot;
    }
    void ResetGameState() {
        ReleaseSlot();
        ReleaseRotation();
        // Restore a key we released, using the physical binding rather than hard-coded Shift.
        if (state_.forceRelease) {
            java::BridgePlayer p;
            if (java::ReadBridgePlayer(p)) java::SetKeyShiftDown(p.manualSneak);
        }
        state_.Reset(); player_.Reset(); placedPacket_.store(false); placementArmed_.store(false); lastTick_ = -1;
    }
    void OnInput(event::MoveInputEvent& e) {
        if(automlg::IsWorking()||autocharge::IsWorking()){ResetGameState();return;}
        const auto serial = reset_.load();
        if (serial != seenReset_) { ResetGameState(); seenReset_ = serial; }
        if (!isEnabled()) return;
        auto self = java::PlayerEntity();
        if (!self || java::ScreenOpen() || clickgui::IsOpen()) { ResetGameState(); return; }
        if (!player_ || !self.IsSame(player_)) { ResetGameState(); player_ = std::move(self); }
        java::BridgePlayer p;
        bool onGround = false; int tick = 0;
        if (!java::ReadBridgePlayer(p) || !player_.OnGround(onGround) || !player_.TickCount(tick) || p.flying) {
            ResetGameState(); return;
        }
        if (lastTick_ > tick) { ResetGameState(); player_ = java::PlayerEntity(); }
        lastTick_ = tick;
        SwitchBlock();
        auto setKey = [](bool down) { java::SetKeyShiftDown(down); };
        const bool require = sneakKeyPressed_->get(), manual = p.manualSneak;
        struct PublishPlacement {
            BridgeAssistModule& module;
            bool require;
            ~PublishPlacement() { module.placementArmed_.store(module.state_.sneaking && require, std::memory_order_release); }
        } publish{*this, require};
        if (placedPacket_.exchange(false)) state_.placed = true;
        if (!require && state_.forceRelease) state_.Repress(e, manual, setKey);
        if (manual && !require) { state_.ResetTimers(); return; }
        if (require && (!manual || (e.forward == 0 && e.strafe == 0))) {
            if (!manual) state_.ResetTimers();
            state_.Repress(e, manual, setKey); return;
        }
        if ((notMovingForward_->get() && e.forward > 0) ||
            (lookingDown_->get() && java::PlayerPitch() < 70) ||
            (holdingBlocks_->get() && !java::HoldingBlockItem())) {
            state_.Clear(e, require, manual, setKey); return;
        }
        if (e.jump && onGround && (e.forward != 0 || e.strafe != 0) && sneakOnJump_->get() > 0 &&
            (!require || state_.forceRelease)) {
            state_.jumpStart = tick;
            state_.jumpTicks = bridgeassist::SneakState::DelayTicks(sneakOnJump_->get(), Random());
            state_.Press(e, manual, setKey); return;
        }
        double offset = 0;
        if (!java::BridgeEdgeOffset(e.forward, e.strafe, e.jump, offset)) { state_.Clear(e, require, manual, setKey); return; }
        const auto release = [&] {
            state_.TryRelease(e, tick, unsneakDelay_->get(), Random(), require, manual, onGround, setKey);
        };
        if (std::isnan(offset)) {
            if (e.jump && (sneakOnJump_->get() <= 0 || (e.forward == 0 && e.strafe == 0))) {
                if (state_.sneaking) release();
            } else if (onGround) state_.Press(e, manual, setKey);
            else if (state_.sneaking) release();
        } else if (offset > edgeOffset_->get()) state_.Press(e, manual, setKey);
        else if (state_.sneaking) release();
    }
    void ReleaseRotation() {
        const auto& state = rotation::State();
        if (ownsRotation_ && state.direct && state.priority == rotation::RotationPriority::Medium &&
            state.rotations.yaw == appliedRotation_.yaw && state.rotations.pitch == appliedRotation_.pitch)
            rotation::ReleaseDirectRotations();
        ownsRotation_ = false;
    }
    void PrePlace() {
        struct ReleaseUnused {
            BridgeAssistModule& module;
            bool used = false;
            ~ReleaseUnused() { if (!used) module.ReleaseRotation(); }
        } request{*this};
        if (!isEnabled() || automlg::IsWorking() || autocharge::IsWorking() || !prePlace_->get() || java::ScreenOpen() || clickgui::IsOpen() || !java::HoldingBlockItem()) return;
        if (breaker::OwnsMining()) return;
        java::BridgePlayer p;
        if (!java::ReadBridgePlayer(p) || p.flying || (lookingDown_->get() && java::PlayerPitch() < 70) ||
            (notMovingForward_->get() && p.forward > 0)) return;
        auto self = java::PlayerEntity(); java::Box box; rotation::Vec3 eye;
        float yaw = 0, pitch = 0;
        if (!self || !self.BoundingBox(box) || !self.EyePosition(1, eye) || !java::PlayerRotation(yaw,pitch)) return;
        const double halfFov = aimFov_->get() * 0.5;
        if (!std::isfinite(halfFov) || !std::isfinite(pitch)) return;
        struct Face { java::BlockPos block; int side; };
        std::vector<Face> targets;
        int y = static_cast<int>(std::floor(box.minY)) - 1;
        for (int x=static_cast<int>(std::floor(box.minX)); x<=static_cast<int>(std::floor(box.maxX)); ++x)
            for (int z=static_cast<int>(std::floor(box.minZ)); z<=static_cast<int>(std::floor(box.maxZ)); ++z) {
                java::BlockPos stand{x,y,z};
                if (java::BlockStateReplaceable(stand)) continue;
                for (int side : {2,3,5,4}) {
                    java::BlockPos placed{x+java::FacingStepX(side),y,z+java::FacingStepZ(side)};
                    if (java::BlockStateReplaceable(placed)) targets.push_back({stand,side});
                }
            }
        if (targets.empty()) return;
        // Raven falls back to the last server rotation, not the live camera.
        const auto& rotations = rotation::State();
        rotation::Rotation previous{yaw,pitch};
        if (rotations.hasLastRotations)
            previous = rotations.hasAnimation ? rotations.animationRotation : rotations.lastRotations;
        else if (compat::IsLegacyLunar())
            compat::lunar::PlayerReportedRotation(previous.yaw, previous.pitch);
        const auto base = rotations.active && !ownsRotation_ ? rotation::GetRotation() : previous;
        float bestDelta = std::numeric_limits<float>::max(), bestPitch = 0;
        for (float sample=60; sample<=90;) {
            sample += std::clamp(1.0f + static_cast<float>(Random()*2-1) * (0.3f+0.2f*0.4f), 0.4f, 1.8f);
            float candidate=std::min(sample,90.0f);
            // Candidates share the camera yaw, so the cone angle is the pitch
            // difference. Use the live camera, not the previous silent rotation.
            if (std::abs(static_cast<double>(candidate)-pitch)>halfFov) continue;
            constexpr float pi=3.1415927f, radians=pi/180.0f;
            const float horizontal=-rotation::mth::Cos(-candidate*radians);
            const rotation::Vec3 direction{rotation::mth::Sin(-yaw*radians-pi)*horizontal,
                rotation::mth::Sin(-candidate*radians),rotation::mth::Cos(-yaw*radians-pi)*horizontal};
            java::BlockHitResultInfo hit;
            if (java::WorldClip(eye,eye.Add(direction.x*p.reach,direction.y*p.reach,direction.z*p.reach),self,hit) &&
                hit.kind==java::HitKind::Block && hit.hasBlockPos && hit.direction>1) {
                for (const auto& target : targets) if (hit.direction==target.side && hit.blockPos.x==target.block.x &&
                    hit.blockPos.y==target.block.y && hit.blockPos.z==target.block.z) {
                    float distance=std::abs(candidate-base.pitch);
                    if (distance<bestDelta) { bestDelta=distance; bestPitch=candidate; }
                    break;
                }
            }
            if (sample>=90) break;
        }
        if (bestDelta==std::numeric_limits<float>::max()) return;
        const float dy=rotation::mth::WrapDegrees(yaw-base.yaw), dp=bestPitch-base.pitch;
        const float magnitude=static_cast<float>(std::sqrt(static_cast<double>(dy*dy+dp*dp)));
        rotation::Rotation result{yaw,std::clamp(bestPitch,-90.0f,90.0f)};
        if (magnitude>=0.001f) {
            constexpr float range=0.6f*static_cast<float>(20.0/100.0);
            float step=45.0f * (1.0f-range/2.0f+static_cast<float>(Random()*range));
            const float proximity=static_cast<float>(std::pow(static_cast<double>(std::min(1.0f,magnitude/180.0f)),0.7));
            step*=std::max(0.8f,1.0f-0.2f*(1.0f-proximity));
            const float scale=std::min(step,magnitude)/magnitude;
            result={base.yaw+dy*scale,std::clamp(base.pitch+dp*scale,-90.0f,90.0f)};
        }
        // RotationHelper applies fixRotation globally after BridgeAssist's smoother.
        // Keep this local so other Nova rotation modes retain their own quantization.
        double sensitivity=0;
        if (!java::PlayerSensitivity(sensitivity) || !std::isfinite(sensitivity)) return;
        const float factor=static_cast<float>(sensitivity)*0.6f+0.2f;
        const double gcd=static_cast<double>(factor*factor*factor)*1.2;
        if (gcd>0) {
            const float unwrapped=previous.yaw +
                (std::fmod(std::fmod(result.yaw-previous.yaw+180.0f,360.0f)+360.0f,360.0f)-180.0f);
            // Java Math.round rounds ties toward +infinity, including negative deltas.
            result.yaw=previous.yaw+static_cast<float>(std::floor(double(unwrapped-previous.yaw)/gcd+0.5)*gcd);
            result.pitch=std::clamp(previous.pitch+static_cast<float>(std::floor(double(result.pitch-previous.pitch)/gcd+0.5)*gcd),-90.0f,90.0f);
        }
        // The Raven smoother has already chosen the step. Do not apply Nova's random smoother again.
        request.used = rotation::SetDirectRotations(result);
        if (request.used) { ownsRotation_=true; appliedRotation_=rotation::State().rotations; }
    }
    Properties::BoolProperty *prePlace_, *switch_, *holdBlock_, *sneakKeyPressed_, *holdingBlocks_, *lookingDown_, *notMovingForward_;
    Properties::NumberProp *aimFov_, *edgeOffset_, *unsneakDelay_, *sneakOnJump_;
    event::Subscription input_, packet_, rotation_, interact_, tick_;
    bool ownsRotation_ = false;
    rotation::Rotation appliedRotation_;
    std::atomic<unsigned> reset_{0}; unsigned seenReset_=0;
    std::atomic<bool> placedPacket_{false};
    std::atomic<bool> placementArmed_{false};
    int lastTick_ = -1;
    int savedSlot_ = -1, visibleSlot_ = -1;
    java::EntityHandle player_;
    bridgeassist::SneakState state_;
    std::mt19937 random_{std::random_device{}()};
};
}
std::unique_ptr<Module> MakeBridgeAssistModule() { return std::make_unique<BridgeAssistModule>(); }
}
