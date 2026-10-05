#pragma once

#include <memory>

namespace nova::module {

class Module;
std::unique_ptr<Module> MakeAutoMLGModule();
std::unique_ptr<Module> MakeAutoChargeModule();
std::unique_ptr<Module> MakeAutoPearlModule();
std::unique_ptr<Module> MakePearlPhaseModule();
std::unique_ptr<Module> MakeAutoMaceModule();
std::unique_ptr<Module> MakeAutoTotemModule();
std::unique_ptr<Module> MakeAutoReplenishModule();
std::unique_ptr<Module> MakeLegitCrystalAuraModule();
std::unique_ptr<Module> MakeAutoCrystalModule();
std::unique_ptr<Module> MakeLegitAnchorAuraModule();
std::unique_ptr<Module> MakeAnchorAuraModule();
std::unique_ptr<Module> MakeFeetPlaceModule();
std::unique_ptr<Module> MakeFakePlayerModule();
std::unique_ptr<Module> MakeElytraFlyModule();
std::unique_ptr<Module> MakeInventoryMoveModule();

std::unique_ptr<Module> MakeClickGuiModule();
std::unique_ptr<Module> MakeArrayListModule();
std::unique_ptr<Module> MakeWatermarkModule();
std::unique_ptr<Module> MakeFullBrightModule();
std::unique_ptr<Module> MakeFreeLookModule();
std::unique_ptr<Module> MakeFreeCamModule();
std::unique_ptr<Module> MakeExampleModule();
std::unique_ptr<Module> MakeSprintModule();
std::unique_ptr<Module> MakeSpeedModule();
std::unique_ptr<Module> MakeNoJumpDelayModule();
std::unique_ptr<Module> MakeKnockBackBalancerModule();
std::unique_ptr<Module> MakeAutoClickerModule();
std::unique_ptr<Module> MakeAimAssistModule();
std::unique_ptr<Module> MakeKillAuraModule();
std::unique_ptr<Module> MakeKeepSprintModule();
std::unique_ptr<Module> MakeDisplaceModule();
std::unique_ptr<Module> MakeThrowableAuraModule();
std::unique_ptr<Module> MakeAutoBlockModule();
std::unique_ptr<Module> MakeAutoPotModule();
std::unique_ptr<Module> MakeAutoRodModule();
std::unique_ptr<Module> MakeTeamsModule();
std::unique_ptr<Module> MakeAntiBotModule();
std::unique_ptr<Module> MakeHealthBypassModule();
std::unique_ptr<Module> MakeImeFixModule();
std::unique_ptr<Module> MakeGhostHandModule();
std::unique_ptr<Module> MakeVelocityModule();
std::unique_ptr<Module> MakeAntiKbModule();
std::unique_ptr<Module> MakeBacktrackModule();
std::unique_ptr<Module> MakeStuckModule();
std::unique_ptr<Module> MakeFastPlaceModule();
std::unique_ptr<Module> MakeScaffoldModule();
std::unique_ptr<Module> MakeBreakerModule();
std::unique_ptr<Module> MakeChestStealerModule();
std::unique_ptr<Module> MakeInvManagerModule();
std::unique_ptr<Module> MakeHypixelAutoPlayModule();
std::unique_ptr<Module> MakeRefillModule();
std::unique_ptr<Module> MakeAutoToolsModule();
std::unique_ptr<Module> MakeBridgeAssistModule();
std::unique_ptr<Module> MakeBlockInModule();
std::unique_ptr<Module> MakeCompassModule();
std::unique_ptr<Module> MakeChestEspModule();
std::unique_ptr<Module> MakeItemEspModule();
std::unique_ptr<Module> MakeBedPlatesModule();
std::unique_ptr<Module> MakeNameTagsModule();
std::unique_ptr<Module> MakeTimerModule();
std::unique_ptr<Module> MakeEsp2DModule();
std::unique_ptr<Module> MakeTargetHudModule();
std::unique_ptr<Module> MakeTargetEspModule();
std::unique_ptr<Module> MakePlayerListModule();
std::unique_ptr<Module> MakeAutoBlockHudModule();
std::unique_ptr<Module> MakeMusicHudModule();
std::unique_ptr<Module> MakeNotificationsModule();
std::unique_ptr<Module> MakeCSWinModule();

}
