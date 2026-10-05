#include <algorithm>
#include <chrono>
#include <format>
#include "minecraft/events/GameEvents.h"
#include "minecraft/modules/Module.h"
#include "minecraft/modules/Registry.h"
#include "minecraft/modules/misc/antibot/AntiBot.h"
#include "minecraft/modules/misc/teams/Teams.h"
#include "minecraft/java/api/game_chest.h"
#include "minecraft/java/api/game_entity.h"
#include "minecraft/java/api/game_presentation_name.h"
#include "minecraft/java/api/game_rotation.h"
#include "minecraft/java/api/game_skin.h"
#include "minecraft/java/api/game_freecam.h"
#include "minecraft/host/presentation/PlayerPortrait.h"
#include "ui/clickgui/ClickGui.h"
#include "ui/overlay/OverlaySpin.h"
#include "ui/hud/playerlist/PlayerListHud.h"

namespace nova::module {
namespace {
namespace list = ui::playerlist;
struct Frame {
    std::vector<list::Snapshot> players;
    list::Snapshot preview;
    std::uint64_t generation = 0;
    float yaw = 0;
};

class PlayerListModule final : public Module {
public:
    PlayerListModule() : Module("Player List",Category::Render,"General","HUD") {
        enableOverlay();
        // Same defaults and bounds as NameTags; unlimited means loaded players only.
        limited_ = &addBool("Limit Range",false);
        range_ = &addNumber("Range",8,512,1,48,{},[](double v){return std::format("{:.0f}",v);},
            [this]{return limited_->get();});
        maximum_ = &addNumber("Max Players",1,20,1,6,{},[](double v){return std::format("{:.0f}",v);});
        addSettings(view_.Options().Rows());
        tick_ = event::EventBus::Get().Subscribe<event::GameTickEvent>([this](auto& e){
            if (e.type == event::EventType::Pre) Gather();
        });
    }
private:
    bool renderWhenDisabled() const override { return true; }
    void onDisable() override {
        ui::ScopedSpin lock(mutex_); frame_.players.clear(); frame_.preview = {};
    }
    void Gather() {
        if (!isEnabled()) { player_.Reset(); world_.Reset(); selection_.Reset(); return; }
        Frame frame;
        auto self = java::PlayerEntity(), world = java::LevelEntity();
        if (self.Valid() != player_.Valid() || world.Valid() != world_.Valid() ||
            (self.Valid() && !self.IsSame(player_)) || (world.Valid() && !world.IsSame(world_))) {
            ++generation_; selection_.Reset();
            player_ = java::PlayerEntity(); world_ = java::LevelEntity();
        }
        frame.generation = generation_;
        rotation::Vec3 origin;
        if (self.Valid() && world.Valid() && self.Position(origin)) {
            float pitch = 0;
            java::PlayerRotation(frame.yaw,pitch);
            std::vector<java::EntityHandle> players;
            std::vector<list::Snapshot> candidates;
            if (java::WorldPlayers(players)) for (const auto& entity : players) {
                if (!entity.Valid() || entity.IsSame(self) || java::FreeCamCamera(entity.raw()) ||
                    !entity.IsPlayer() || !entity.IsAlive() || entity.IsSpectator() || antibot::IsBot(entity)) continue;
                double distance = 0; rotation::Vec3 position; int id = 0;
                if (!entity.DistanceTo(self,distance) || !list::InRange(distance,limited_->get(),range_->get()) ||
                    !entity.Position(position) || !java::EntityId(entity,id)) continue;
                list::Snapshot entry;
                entry.id = id; entry.distance = static_cast<float>(distance);
                entry.bearing = static_cast<float>(std::atan2(position.z-origin.z,position.x-origin.x)*180./3.141592653589793-90.);
                if (!std::isfinite(entry.bearing)) continue;
                candidates.push_back(std::move(entry));
            }
            const double now = std::chrono::duration<double>(std::chrono::steady_clock::now().time_since_epoch()).count();
            selection_.Update(candidates,static_cast<std::size_t>(maximum_->get()),now);
            for (auto& entry : candidates) {
                if (frame.players.size() >= static_cast<std::size_t>(maximum_->get())) break;
                const auto entity = std::find_if(players.begin(),players.end(),[&](const auto& e){
                    int id = 0; return java::EntityId(e,id) && id == entry.id;
                });
                if (entity == players.end()) continue;
                Fill(*entity,entry);
                entry.teammate = teams::IsTeammate(self,*entity);
                frame.players.push_back(std::move(entry));
            }
            if (clickgui::IsOpen() && frame.players.empty()) {
                Fill(self,frame.preview); frame.preview.bearing = frame.yaw;
            }
        }
        // Publish values only; no entity/JNI references reach the UI thread.
        ui::ScopedSpin lock(mutex_); frame_ = std::move(frame);
    }
    static void Fill(const java::EntityHandle& entity,list::Snapshot& entry) {
        if (!entity.DisplayName(entry.name) || entry.name.empty()) entry.name = "Player";
        entry.name=java::PresentationName(entry.name);
        entry.portrait = minecraft::PlayerPortrait(java::PlayerSkinTexture(entity));
        float health = 0, absorption = 0;
        if (entity.Health(health,absorption) && std::isfinite(health) && health>=0 && std::isfinite(absorption))
            entry.health = health+std::max(0.f,absorption);
    }
    void onRender() override {
        Frame frame;
        {ui::ScopedSpin lock(mutex_); frame = frame_;}
        const bool editing = isEnabled() && clickgui::IsOpen();
        if (editing && frame.players.empty()) {
            if (frame.preview.name.empty()) frame.preview.name = "Player";
            frame.preview.id = -2147483647;
            frame.players.push_back(std::move(frame.preview));
        }
        java::CameraView camera;
        if (isEnabled() && java::ReadCameraView(java::FrameTime(),camera)) frame.yaw = camera.yaw;
        view_.Draw(isEnabled(),editing,frame.players,frame.generation,frame.yaw);
    }
    list::PlayerListHud view_;
    Properties::BoolProperty* limited_ = nullptr;
    Properties::NumberProp *range_ = nullptr, *maximum_ = nullptr;
    event::Subscription tick_;
    java::EntityHandle player_,world_;
    std::uint64_t generation_ = 0;
    list::Selection selection_;
    ui::SpinLock mutex_;
    Frame frame_;
};
}
std::unique_ptr<Module> MakePlayerListModule() { return std::make_unique<PlayerListModule>(); }
}
