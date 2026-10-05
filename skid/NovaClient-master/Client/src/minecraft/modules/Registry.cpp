#include "config/ConfigManager.h"
#include "minecraft/events/EventBus.h"
#include "minecraft/modules/ModuleManager.h"
#include "minecraft/modules/Registry.h"
#include "minecraft/host/presentation/UiCatalog.h"
#include "minecraft/host/presentation/Config.h"
#include "ui/clickgui/panel/ClientSettings.h"
#include "minecraft/utils/rotation/RotationBridge.h"
#include "minecraft/compat/Profile.h"

namespace nova::module {

void RegisterBuiltinModules() {
    static bool registered = false;
    if (registered)
        return;
    registered = true;

    ModuleManager& manager = ModuleManager::Get();
    manager.Add(MakeAutoMLGModule());
    manager.Add(MakeAutoChargeModule());
    manager.Add(MakeAutoPearlModule());
    manager.Add(MakePearlPhaseModule());
    manager.Add(MakeAutoMaceModule());
    manager.Add(MakeAutoTotemModule());
    manager.Add(MakeLegitCrystalAuraModule());
    manager.Add(MakeAutoCrystalModule());
    manager.Add(MakeLegitAnchorAuraModule());
    manager.Add(MakeAnchorAuraModule());
    manager.Add(MakeFeetPlaceModule());
    manager.Add(MakeFakePlayerModule());
    manager.Add(MakeElytraFlyModule());
    manager.Add(MakeTeamsModule());
    manager.Add(MakeAntiBotModule());
    manager.Add(MakeHealthBypassModule());
    if (compat::IsLunar12111() || compat::IsLunar189()) manager.Add(MakeImeFixModule());
    manager.Add(MakeGhostHandModule());
    manager.Add(MakeKeepSprintModule());
    manager.Add(MakeExampleModule());
    manager.Add(MakeStuckModule());
    manager.Add(MakeScaffoldModule());
    manager.Add(MakeBreakerModule());
    manager.Add(MakeChestStealerModule());
    manager.Add(MakeInvManagerModule());
    manager.Add(MakeHypixelAutoPlayModule());
    manager.Add(MakeRefillModule());
    manager.Add(MakeAutoReplenishModule());
    manager.Add(MakeAutoToolsModule());
    manager.Add(MakeBridgeAssistModule());
    manager.Add(MakeBlockInModule());
    manager.Add(MakeClickGuiModule());
    manager.Add(MakeFreeLookModule());
    manager.Add(MakeFreeCamModule());
    manager.Add(MakeArrayListModule());
    manager.Add(MakeWatermarkModule());
    if (compat::Has(compat::Capability::NetworkEvents)) {
        manager.Add(MakeVelocityModule());
        manager.Add(MakeAntiKbModule());
        manager.Add(MakeBacktrackModule());
        manager.Add(MakeKnockBackBalancerModule());
    }
    if (compat::Has(compat::Capability::RotationEvents) && compat::Has(compat::Capability::ClickInput))
        manager.Add(MakeKillAuraModule());
    if (compat::IsLegacyLunar()) manager.Add(MakeThrowableAuraModule());
    manager.Add(MakeAutoBlockModule());
    manager.Add(MakeAutoPotModule());
    manager.Add(MakeAutoRodModule());
    manager.Add(MakeDisplaceModule());
    if (compat::Has(compat::Capability::BasicPlace)) manager.Add(MakeFastPlaceModule());
    if (compat::Has(compat::Capability::RotationEvents)) manager.Add(MakeAimAssistModule());
    if (compat::Has(compat::Capability::ClickInput)) manager.Add(MakeAutoClickerModule());
    if (compat::Has(compat::Capability::Settings)) manager.Add(MakeSprintModule());
    if (compat::Has(compat::Capability::RotationEvents) && compat::Has(compat::Capability::Settings))
        manager.Add(MakeSpeedModule());
    if (compat::IsLunar189()) manager.Add(MakeInventoryMoveModule());
    manager.Add(MakeNoJumpDelayModule());
    if (compat::IsLegacyLunar() || compat::UsesGlfwModernRuntime()) manager.Add(MakeTimerModule());
    if (compat::Has(compat::Capability::PlayerEffects)) manager.Add(MakeFullBrightModule());
    manager.Add(MakeCompassModule());
    manager.Add(MakeChestEspModule());
    manager.Add(MakeItemEspModule());
    manager.Add(MakeBedPlatesModule());
    manager.Add(MakeNameTagsModule());
    manager.Add(MakeEsp2DModule());
    manager.Add(MakeTargetHudModule());
    manager.Add(MakeTargetEspModule());
    manager.Add(MakePlayerListModule());
    manager.Add(MakeAutoBlockHudModule());
    manager.Add(MakeMusicHudModule());
    manager.Add(MakeNotificationsModule());
    manager.Add(MakeCSWinModule());

    nova::minecraft::InstallUiCatalog();

    if (compat::Has(compat::Capability::RotationEvents)) nova::rotation::InstallBridge();

    manager.Seal();

    nova::event::EventBus::Get().Seal();

    nova::config::SetClientSettings(&nova::menu::panel::ClientSettings::Get().rows());
    nova::minecraft::InstallConfig();
    nova::config::ConfigManager::Get().Init();
}

}
