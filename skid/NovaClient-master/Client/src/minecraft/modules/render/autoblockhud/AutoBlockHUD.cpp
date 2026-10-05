#include "minecraft/modules/Module.h"
#include "minecraft/modules/Registry.h"
#include "minecraft/modules/combat/autoblock/AutoBlockStats.h"
#include "minecraft/events/EventBus.h"
#include "minecraft/events/GameEvents.h"
#include "minecraft/java/api/game_entity.h"
#include "minecraft/java/api/game_skin.h"
#include "minecraft/host/presentation/PlayerPortrait.h"
#include "ui/clickgui/ClickGui.h"
#include "ui/hud/autoblockhud/AutoBlockHud.h"
#include <atomic>

namespace nova::module {
namespace {
class AutoBlockHudModule final : public Module {
public:
    AutoBlockHudModule() : Module("AutoBlockHUD",Category::Render,"General","Overlay") {
        enableOverlay();
        addSettings(view_.Options().Rows());
        tick_=event::EventBus::Get().Subscribe<event::GameTickEvent>([this](event::GameTickEvent& e) {
            if(!isEnabled() || e.type!=event::EventType::Pre)return;
            if(!autoblock::ReadCoverage().active && !clickgui::IsOpen())return;
            const auto self=java::PlayerEntity();
            skin_.store(self ? java::PlayerSkinTexture(self) : 0);
        });
    }
private:
    bool renderWhenDisabled() const override { return true; }
    void onRender() override {
        const auto stats=autoblock::ReadCoverage();
        view_.Draw(isEnabled(),clickgui::IsOpen(),{stats.active,stats.percent,minecraft::PlayerPortrait(skin_.load())});
    }
    ui::autoblockhud::AutoBlockHud view_;
    event::Subscription tick_;
    std::atomic<std::uint64_t> skin_{0};
};
}
std::unique_ptr<Module> MakeAutoBlockHudModule() { return std::make_unique<AutoBlockHudModule>(); }
}
