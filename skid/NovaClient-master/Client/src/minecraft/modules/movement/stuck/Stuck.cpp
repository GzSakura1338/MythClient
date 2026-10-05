#include <deque>
#include <memory>

#include "minecraft/events/EventBus.h"
#include "minecraft/events/GameEvents.h"
#include "minecraft/modules/Module.h"
#include "minecraft/modules/Registry.h"
#include "minecraft/java/api/base.h"
#include "minecraft/java/api/game_network.h"
#include "minecraft/java/api/game_player.h"

namespace nova::module {
namespace {

constexpr int kHoldTicks = 20;
constexpr int kStatusTick = 10;

class StuckModule final : public Module {
public:
    StuckModule() : Module("Stuck", Category::Movement, "General", "General") {
        tickSubscription_ = event::EventBus::Get().Subscribe<event::PlayerTickEvent>(
            [this](event::PlayerTickEvent& e) { OnPlayerTick(e); });
        packetSubscription_ = event::EventBus::Get().Subscribe<event::PacketEvent>(
            [this](event::PacketEvent& e) { OnPacket(e); });
    }

    ~StuckModule() override { ClearQueue(); }

private:
    void onEnable() override {
        ticks_ = kHoldTicks;
        stage_ = 0;
        ClearQueue();
    }

    void onDisable() override { ClearQueue(); }

    void OnPlayerTick(event::PlayerTickEvent& e) {
        if (!isEnabled())
            return;
        if (nova::java::PlayerOnGround()) {
            setEnabled(false);
            return;
        }

        switch (stage_) {
            case 0:
                if (ticks_ > 0) {
                    e.Cancel();
                    if (ticks_ == kStatusTick)
                        nova::java::SendStatusOnlyPacket(nova::java::PlayerOnGround());
                    ticks_--;
                } else {
                    ticks_ = kHoldTicks;
                }
                break;
            case 1:
                stage_ = 2;
                break;
            case 2:
                while (!packets_.empty()) {
                    nova::java::SendPacketNoEvent(packets_.front());
                    PopFront();
                }
                stage_ = 0;
                break;
            default:
                break;
        }
    }

    void OnPacket(event::PacketEvent& e) {
        if (!isEnabled() || e.type != event::EventType::Outgoing || stage_ != 0)
            return;
        if (!nova::java::IsUseItemPacket(e.packet))
            return;

        PushPacket(e.packet);
        e.Cancel();
        stage_ = 1;
    }

    void PushPacket(jobject packet) {
        JniAttach attach;
        if (!attach)
            return;
        jobject kept = attach.env->NewGlobalRef(packet);
        if (kept)
            packets_.push_back(kept);
    }

    void PopFront() {
        JniAttach attach;
        if (attach)
            attach.env->DeleteGlobalRef(packets_.front());
        packets_.pop_front();
    }

    void ClearQueue() {
        JniAttach attach;
        if (!attach) {
            packets_.clear();
            return;
        }
        while (!packets_.empty()) {
            attach.env->DeleteGlobalRef(packets_.front());
            packets_.pop_front();
        }
    }

    event::Subscription tickSubscription_;
    event::Subscription packetSubscription_;
    std::deque<jobject> packets_;
    int ticks_ = kHoldTicks;
    int stage_ = 0;
};

}

std::unique_ptr<Module> MakeStuckModule() {
    return std::make_unique<StuckModule>();
}

}
