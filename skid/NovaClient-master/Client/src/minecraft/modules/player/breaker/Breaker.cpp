#include "minecraft/modules/player/breaker/Breaker.h"
#include "minecraft/modules/player/breaker/BreakerLogic.h"
#include "minecraft/modules/Module.h"
#include "minecraft/modules/player/automlg/AutoMLG.h"
#include "minecraft/modules/player/autocharge/AutoCharge.h"
#include "minecraft/modules/Registry.h"
#include "minecraft/events/GameEvents.h"
#include "minecraft/java/api/game_breaker.h"
#include "minecraft/java/api/game_click.h"
#include "minecraft/java/api/game_compass.h"
#include "minecraft/java/api/game_network.h"
#include "minecraft/java/api/game_place.h"
#include "minecraft/java/api/game_rotation.h"
#include "minecraft/utils/rotation/RotationManager.h"
#include "minecraft/utils/rotation/Mth.h"
#include "minecraft/host/render/WorldProjection.h"
#include "ui/hud/breaker/BreakerHud.h"
#include "ui/clickgui/ClickGui.h"
#include "imgui.h"
#include <atomic>
#include <chrono>
#include <deque>
#include <format>
#include <mutex>
#include <random>

namespace nova::module {
namespace {
using namespace breaker;
std::atomic<bool> mining = false, blocksAura = false;
class Breaker final : public Module {
  public:
    Breaker() : Module("Breaker", Category::Player, "General", "General") {
        enableOverlay();
        mode_ = &addMode("Mode", 0, {"Normal", "Instant"});
        range_ = &addNumber("Range", 1, 6, .1, 4.5, {}, [](double v) { return std::format("{:.1f}", v); });
        bed_ = &addBool("Bed", true);
        keep_ = &addBool("Keep Break Progress When Out Of Range", true);
        cancel_ = &addBool("Cancel Velocity Whilst Breaking", true);
        delay_ = &addBool("Delay Velocity Until Bed Broken", false);
        through_ = &addBool("Through Walls", true);
        empty_ = &addBool("Empty Surrounding", false, {}, [this] { return through_->get(); });
        surroundings_ = &addMode("Surroundings", 0, {"Single", "Full"});
        layers_ = &addNumber(
            "Surrounding Layers", 1, 6, 1, 3, {}, [](double v) { return std::format("{:.0f}", v); },
            [this] { return surroundings_->get() == 1; });
        stand_ =
            &addBool("Show Best Stand Position", false, {}, [this] { return surroundings_->get() == 1; });
        heypixel_ = &addBool("Heypixel", false, {}, [this] { return through_->get() && empty_->get(); });
        ground_ = &addBool("Watchdog Ground Spoof", false);
        attack_ = &addBool("Attack While Breaking", true);
        percentage_ = &addBool("Show Breaker Percentage", true);
        rotate_ = &addBool("Rotate", true);
        ends_ = &addBool("Only Rotate at Start and Stop", true);
        own_ = &addBool("Whitelist Own Bed", true);
        slow_ = &addBool("Slow Down In Air", true);
        correction_ = &addMode("Movement Correction", 0, {"Off", "Rise", "Traditional", "Backwards Sprint"});
        auto &bus = event::EventBus::Get();
        tick_ = bus.Subscribe<event::GameTickEvent>(
            [this](auto &e) {
                if (e.type == event::EventType::Pre)
                    Tick();
            },
            event::Priority::High);
        motion_ = bus.Subscribe<event::MotionEvent>([this](auto &e) { Motion(e); }, event::Priority::High);
        packet_ = bus.Subscribe<event::PacketEvent>([this](auto &e) { Packet(e); }, event::Priority::High);
        respawn_ = bus.Subscribe<event::RespawnEvent>([this](auto &) {
            {
                std::lock_guard lock(packetsMutex_);
                pendingHome_.reset();
            }
            reset_.store(true);
            drop_.store(true);
        });
        input_ = bus.Subscribe<event::MoveInputEvent>(
            [this](auto &e) {
                if (!active_ || !rotationOwned_)
                    return;
                float yaw = 0, pitch = 0;
                if (!java::PlayerRotation(yaw, pitch))
                    return;
                if (settings_.movement == 1) {
                    rotation::MoveInput input{e.forward, e.strafe};
                    rotation::FixMovement(input, rotation::GetRotation().yaw, yaw);
                    e.forward = input.forward;
                    e.strafe = input.strafe;
                }
                if (settings_.movement == 3) {
                    const float direction = rotation::Direction(e.forward, e.strafe, yaw);
                    if (std::abs(std::fmod(rotation::GetRotation().yaw, 360.f) -
                                 std::fmod(direction, 360.f)) > 45)
                        java::SetPlayerSprinting(false);
                }
            },
            event::Priority::Lowest);
        strafe_ = bus.Subscribe<event::StrafeEvent>(
            [this](auto &e) {
                if (active_ && rotationOwned_ && (settings_.movement == 0 || settings_.movement == 3)) {
                    float p;
                    java::PlayerRotation(e.yaw, p);
                }
            },
            event::Priority::Lowest);
        jump_ = bus.Subscribe<event::JumpEvent>(
            [this](auto &e) {
                if (active_ && rotationOwned_ && settings_.movement == 0) {
                    float p;
                    java::PlayerRotation(e.yaw, p);
                }
            },
            event::Priority::Lowest);
    }
    ~Breaker() override {
        tick_.Reset();
        motion_.Reset();
        packet_.Reset();
        respawn_.Reset();
        input_.Reset();
        strafe_.Reset();
        jump_.Reset();
        if (active_.exchange(false))
            java::SetIncomingPacketsWanted(false);
        Finish(false);
        mining = false;
        blocksAura = false;
    }
    std::string suffix() const override { return mode_->get() == 0 ? "Normal" : "Instant"; }

  private:
    Settings ReadSettings() const {
        Settings s;
        s.mode = mode_->get();
        s.range = range_->get();
        s.layers = int(layers_->get());
        s.movement = correction_->get();
        s.bed = bed_->get();
        s.keepProgress = keep_->get();
        s.cancelVelocity = cancel_->get();
        s.delayVelocity = delay_->get();
        s.through = through_->get();
        s.empty = empty_->get();
        s.full = surroundings_->get() == 1;
        s.stand = stand_->get();
        s.heypixel = heypixel_->get();
        s.groundSpoof = ground_->get();
        s.attack = attack_->get();
        s.percentage = percentage_->get();
        s.rotate = rotate_->get();
        s.onlyEnds = ends_->get();
        s.own = own_->get();
        s.slowAir = slow_->get();
        return s;
    }
    void onEnable() override {
        {
            std::lock_guard lock(packetsMutex_);
            pendingHome_.reset();
        }
        reset_ = true;
        active_ = true;
        java::SetIncomingPacketsWanted(true);
    }
    void onDisable() override {
        if (active_.exchange(false))
            java::SetIncomingPacketsWanted(false);
        reset_ = true;
        spoof_ = false;
        blink_ = false;
        mining = false;
        blocksAura = false;
        Finish(true);
    }
    static long long Now() {
        return std::chrono::duration_cast<std::chrono::milliseconds>(
                   std::chrono::steady_clock::now().time_since_epoch())
            .count();
    }
    struct HeldPacket {
        jobject packet;
        bool incoming;
    };
    void Finish(bool replay, bool incoming = true, bool outgoing = true) {
        std::deque<jobject> in;
        std::deque<HeldPacket> out;
        {
            std::lock_guard lock(packetsMutex_);
            if (incoming) {
                in.swap(incoming_);
                delayed_ = false;
            }
            if (outgoing)
                out.swap(outgoing_);
        }
        if (in.empty() && out.empty())
            return;
        JniAttach attach;
        if (!attach)
            return;
        for (auto p : in) {
            if (replay)
                java::HandleIncomingPacket(p);
            attach.env->DeleteGlobalRef(p);
        }
        for (auto p : out) {
            if (replay) {
                if (p.incoming)
                    java::HandleIncomingPacket(p.packet);
                else
                    java::SendPacketNoEvent(p.packet);
            }
            attach.env->DeleteGlobalRef(p.packet);
        }
    }
    void Clear(java::BreakerWorld &w, bool replay, bool clearHome = true) {
        if (holding_)
            w.Stop();
        holding_ = false;
        target_.reset();
        last_.reset();
        standPos_.reset();
        lastCheck_.reset();
        if (clearHome) {
            home_.reset();
            lastStand_ = lastSearch_ = 0;
        }
        cooldown_ = 0;
        progress_ = 0;
        ReleaseMovementFix();
        mining = false;
        blocksAura = false;
        spoof_ = false;
        blink_ = false;
        {
            std::lock_guard lock(packetsMutex_);
            packetTarget_.reset();
            breaking_ = false;
            // A teleport from the new world may already be queued by Netty.
            // Its anchor must survive the client-thread reset below.
        }
        Finish(replay);
        Publish();
    }
    void ReleaseMovementFix() {
        if (rotationOwned_)
            rotation::SetMoveInputFix(previousMoveFix_);
        rotationOwned_ = false;
    }
    void Revalidate(java::BreakerWorld &w, Vec player, Vec eye, int ticks) {
        if (!target_ || !settings_.through || !settings_.empty || !settings_.full) {
            lastCheck_.reset();
            return;
        }
        if (!lastCheck_) {
            lastCheck_ = player;
            return;
        }
        if (DistanceSquared(*lastCheck_, player) < .25 || ticks - lastSearch_ < 5)
            return;
        lastCheck_ = player;
        lastSearch_ = ticks;
        const auto candidates = Candidates(w, eye, Beds(w, player), settings_);
        const auto *best = Best(candidates);
        const Candidate *current = nullptr;
        for (const auto &c : candidates)
            if (Same(c.blocks.front(), Floor(*target_)) && (!current || c.cost < current->cost))
                current = &c;
        if (!best || Same(best->blocks.front(), Floor(*target_)))
            return;
        if (current) {
            const double saved = w.Progress() / std::max(w.Read(Floor(*target_)).relative, 1.e-4f);
            if (best->cost >= (current->cost - saved) * .9)
                return;
        }
        w.SetProgress(0);
        progress_ = 0;
        target_.reset();
        holding_ = false;
    }
    void Tick() {
        if (!active_ && !reset_)
            return;
        java::BreakerWorld w;
        if(automlg::IsWorking()||autocharge::IsWorking()){Clear(w,true);return;}
        if (reset_.exchange(false))
            Clear(w, !drop_.exchange(false));
        if (!active_)
            return;
        auto self = java::PlayerEntity();
        Vec player, eye;
        int ticks = 0;
        if (!w.Ready() || !self.Position(player) || !self.EyePosition(1, eye) || !self.TickCount(ticks)) {
            Clear(w, false);
            return;
        }
        settings_ = ReadSettings();
        ReleaseMovementFix();
        bool changed = false;
        {
            std::lock_guard lock(packetsMutex_);
            if (pendingHome_) {
                home_ = pendingHome_;
                pendingHome_.reset();
            }
            changed = changed_;
            changed_ = false;
            packetSettings_ = settings_;
            selfId_ = 0;
            java::EntityId(self, selfId_);
        }
        if (java::ScreenOpen() || clickgui::IsOpen() || !settings_.bed) {
            Clear(w, true, false);
            return;
        }
        if (changed && target_ && w.Read(Floor(*target_)).air)
            target_.reset();
        if (!settings_.delayVelocity)
            Finish(true, true, false);
        if (!settings_.groundSpoof) {
            spoof_ = false;
            blink_ = false;
            Finish(true, false, true);
        }
        bool expired = false;
        {
            std::lock_guard lock(packetsMutex_);
            expired = (!incoming_.empty() || !outgoing_.empty()) && Now() - queueStart_.load() > 5000;
        }
        if (overflow_.exchange(false) || expired) {
            spoof_ = false;
            blink_ = false;
            Finish(true);
        }
        if (settings_.stand && settings_.through && settings_.empty && settings_.full) {
            if (ticks - lastStand_ >= 10) {
                lastStand_ = ticks;
                standPos_ = Stand(w, player, eye.y - player.y, settings_);
            }
        } else
            standPos_.reset();
        if (--cooldown_ <= 0) {
            Revalidate(w, player, eye, ticks);
            if (!target_ ||
                DistanceSquared(player, *target_) > (settings_.range + 2.5) * (settings_.range + 2.5) ||
                w.Read(Floor(*target_)).air) {
                if (last_ && !settings_.keepProgress)
                    w.SetProgress(0);
                last_ = target_;
                target_ = Find(w, player, eye, settings_, home_);
                if (holding_ && !target_) {
                    const float progress = w.Progress();
                    w.Stop();
                    if (settings_.keepProgress)
                        w.SetProgress(progress);
                }
                holding_ = false;
            }
            // Publish before controller calls: their synchronous outgoing packets
            // must see the same target and ground-spoof settings as this tick.
            PublishPacket();
            if (target_) {
                const Pos pos = Floor(*target_);
                progress_ = w.Progress();
                event::BlockDamageEvent damage(pos.x, pos.y, pos.z);
                (void)event::EventBus::Get().Post(damage);
                std::uniform_real_distribution<double> jitter(-.125, .125);
                const Vec point{pos.x + .5 + jitter(random_), pos.y + .1, pos.z + .5 + jitter(random_)};
                const double dx = point.x - eye.x, dy = point.y - eye.y, dz = point.z - eye.z;
                const rotation::Rotation wanted{
                    float(std::atan2(dz, dx) * 180 / 3.141592653589793 - 90),
                    float(-std::atan2(dy, std::hypot(dx, dz)) * 180 / 3.141592653589793)};
                if (settings_.rotate &&
                    (!settings_.onlyEnds || progress_ == 0 || progress_ > 1 - w.Read(pos).relative - .001)) {
                    previousMoveFix_ = rotation::MoveInputFix();
                    rotation::SetRotations(wanted, 360., nullptr, rotation::RotationPriority::Highest);
                    rotation::SetMoveInputFix(false);
                    rotationOwned_ = true;
                }
                const auto direction = settings_.onlyEnds ? wanted : rotation::GetRotation();
                Vec look;
                int face = 1;
                bool hit = java::Vec3DirectionFromRotation(direction.pitch, direction.yaw, look) &&
                           w.Hit(pos, eye,
                                 eye.Add(look.x * (settings_.range + 1), look.y * (settings_.range + 1),
                                         look.z * (settings_.range + 1)),
                                 face);
                if (settings_.mode == 1) {
                    if (w.Instant(pos, settings_.slowAir)) {
                        target_.reset();
                        cooldown_ = 20;
                    }
                    holding_ = false;
                } else {
                    holding_ = hit;
                    mining = hit;
                    if (hit)
                        w.Damage(pos, face, settings_.slowAir);
                }
            } else {
                mining = false;
                progress_ = 0;
            }
        }
        PublishPacket();
        bool breaking;
        {
            std::lock_guard lock(packetsMutex_);
            breaking = breaking_;
        }
        blocksAura = breaking && !settings_.attack;
        if (!breaking)
            Finish(true, true, false);
        Publish();
    }
    void PublishPacket() {
        std::lock_guard lock(packetsMutex_);
        packetTarget_ = target_ ? std::optional<Pos>(Floor(*target_)) : std::nullopt;
        // Instant clears target for 20 ticks; Rise retains breakingBed while
        // waiting. Release only when the next target scan finds no bed.
        if (target_)
            breaking_ = true;
        else if (cooldown_ <= 0)
            breaking_ = false;
    }
    void Motion(event::MotionEvent &e) {
        if (!active_ || e.type != event::EventType::Pre || !spoof_)
            return;
        blink_ = true;
        e.onGround = true;
        if (java::PlayerOnGround()) {
            spoof_ = false;
            blink_ = false;
            Finish(true, false, true);
        }
    }
    void Packet(event::PacketEvent &e) {
        if (!active_ || !e.packet || e.IsCancelled())
            return;
        if (e.type == event::EventType::Incoming &&
            (java::IsDisconnectPacket(e.packet) || java::IsRespawnPacket(e.packet))) {
            {
                std::lock_guard lock(packetsMutex_);
                pendingHome_.reset();
            }
            drop_ = true;
            reset_ = true;
            spoof_ = false;
            blink_ = false;
            Finish(false);
            return;
        }
        if (e.type == event::EventType::Incoming && java::IsPlayerPositionPacket(e.packet)) {
            Vec position, current;
            auto self = java::PlayerEntity();
            if (java::BreakerTeleport(e.packet, position) && self.Position(current) &&
                DistanceSquared(current, position) > 1600) {
                std::lock_guard lock(packetsMutex_);
                pendingHome_ = position;
            }
        }
        std::unique_lock lock(packetsMutex_);
        if (!active_)
            return;
        bool hold = false, blinkHold = false;
        if (e.type == event::EventType::Incoming) {
            int id = 0, x = 0, y = 0, z = 0;
            const bool velocity = java::IsMotionPacket(e.packet) &&
                                  java::ReadMotionPacket(e.packet, id, x, y, z) && id == selfId_;
            if (packetSettings_.delayVelocity &&
                ((velocity && breaking_) || (delayed_ && java::IsPingPacket(e.packet)))) {
                hold = true;
                delayed_ = true;
            } else if (velocity && packetTarget_ && packetSettings_.cancelVelocity) {
                e.Cancel();
                return;
            }
            Pos pos;
            bool air = false;
            if (packetSettings_.cancelVelocity && packetTarget_ &&
                java::BreakerBlockUpdate(e.packet, pos, air) && air && Same(pos, *packetTarget_))
                changed_ = true;
            if (!hold && blink_ && java::BreakerBlinkPacket(e.packet, true)) {
                hold = true;
                blinkHold = true;
            }
        } else if (e.type == event::EventType::Outgoing) {
            if (packetSettings_.groundSpoof && packetTarget_ && java::BreakerDigPacket(e.packet))
                spoof_ = true;
            // Blink begins in the next motion callback, matching Rise's order:
            // the START/STOP that enabled ground spoof must go out immediately.
            hold = blink_ && java::BreakerBlinkPacket(e.packet, false);
            blinkHold = true;
        }
        if (!hold)
            return;
        if ((blinkHold ? outgoing_.size() : incoming_.size()) >= 4096) {
            overflow_ = true;
            return;
        }
        JniAttach attach;
        if (!attach)
            return;
        auto global = attach.env->NewGlobalRef(e.packet);
        if (attach.env->ExceptionCheck()) {
            attach.env->ExceptionClear();
            return;
        }
        if (!global)
            return;
        if (incoming_.empty() && outgoing_.empty())
            queueStart_ = Now();
        if (blinkHold)
            outgoing_.push_back({global, e.type == event::EventType::Incoming});
        else
            incoming_.push_back(global);
        e.Cancel();
    }
    void Publish() {
        if (!holding_)
            mining = false;
        std::lock_guard lock(renderMutex_);
        visualTarget_ = target_ ? std::optional<Pos>(Floor(*target_)) : std::nullopt;
        visualStand_ = standPos_;
        visualProgress_ = progress_;
        visualPercentage_ = settings_.percentage;
    }
    static void Box(const minecraft::projection::CameraBasis &basis, ImVec2 display, Pos p, float height,
                    bool fill) {
        ui::ProjectedBox points;
        std::array<bool,8> valid;
        for (int i = 0; i < 8; ++i) {
            minecraft::projection::ScreenPoint s;
            valid[i] = minecraft::projection::Project(
                basis,
                {p.x + double(i & 1), p.y + ((i & 2) ? double(height) : 0.), p.z + double((i >> 2) & 1)},
                display, s);
            points[i] = {s.x, s.y};
        }
        ui::BreakerHud::DrawBox(points,valid,fill);
    }
    void onRender() override {
        if (!ImGui::GetCurrentContext() || !ImGui::GetFont())
            return;
        std::optional<Pos> target, stand;
        float progress;
        bool percentage;
        {
            std::lock_guard lock(renderMutex_);
            target = visualTarget_;
            stand = visualStand_;
            progress = visualProgress_;
            percentage = visualPercentage_;
        }
        if (!target && !stand)
            return;
        const float animation=view_.Advance(progress,ImGui::GetIO().DeltaTime);
        java::CameraView view;
        const float partial = java::FrameTime();
        if (!java::ReadCameraView(partial, view))
            return;
        const auto basis = minecraft::projection::MakeBasis(view, partial);
        const auto display = ImGui::GetIO().DisplaySize;
        if (target)
            Box(basis, display, *target, std::clamp(animation, .005f, 1.f), true);
        if (stand)
            Box(basis, display, *stand, 1, false);
        if (target && percentage) {
            int windowWidth=0,windowHeight=0;
            double guiScale=0;
            float bottomInset=0;
            if(java::WindowMetrics(windowWidth,windowHeight,guiScale) && windowHeight>0 && std::isfinite(guiScale) && guiScale>0)
                bottomInset=static_cast<float>(44*display.y*guiScale/windowHeight);
            view_.DrawProgress(bottomInset);
        }
    }
    Properties::ModeProperty *mode_{}, *surroundings_{}, *correction_{};
    Properties::NumberProp *range_{}, *layers_{};
    Properties::BoolProperty *bed_{}, *keep_{}, *cancel_{}, *delay_{}, *through_{}, *empty_{}, *stand_{},
        *heypixel_{}, *ground_{}, *attack_{}, *percentage_{}, *rotate_{}, *ends_{}, *own_{}, *slow_{};
    event::Subscription tick_, motion_, packet_, respawn_, input_, strafe_, jump_;
    Settings settings_, packetSettings_;
    std::atomic<bool> active_ = false, reset_ = false, drop_ = false, spoof_ = false, blink_ = false,
                      overflow_ = false;
    std::atomic<long long> queueStart_ = 0;
    std::optional<Vec> target_, last_, home_, lastCheck_, pendingHome_;
    std::optional<Pos> standPos_, packetTarget_, visualTarget_, visualStand_;
    int cooldown_ = 0, lastStand_ = 0, lastSearch_ = 0, selfId_ = 0;
    bool holding_ = false, rotationOwned_ = false, breaking_ = false, delayed_ = false, changed_ = false,
         visualPercentage_ = true;
    float progress_ = 0, visualProgress_ = 0;
    ui::BreakerHud view_;
    std::mutex packetsMutex_, renderMutex_;
    std::deque<jobject> incoming_;
    std::deque<HeldPacket> outgoing_;
    std::mt19937 random_{std::random_device{}()};
    bool previousMoveFix_ = true;
};
} // namespace
std::unique_ptr<Module> MakeBreakerModule() { return std::make_unique<Breaker>(); }
namespace breaker {
bool OwnsMining() { return mining.load(); }
bool BlocksAura() { return blocksAura.load(); }
} // namespace breaker
} // namespace nova::module
