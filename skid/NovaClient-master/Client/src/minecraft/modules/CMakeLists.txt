# Explicit source ownership; add a new implementation in its feature group.
target_sources(NovaClientBase PRIVATE
    # Infrastructure
    Module.cpp
    ModuleManager.cpp
    Registry.cpp

    # combat/aimassist
    combat/aimassist/AimAssist.cpp

    # combat/antikb
    combat/antikb/AntiKB.cpp
    combat/antikb/mix/MixRuntime.cpp

    # combat/antikb/prediction
    combat/antikb/prediction/PredictionRuntime.cpp

    # combat/autoblock
    combat/autoblock/AutoBlock.cpp
    combat/automace/AutoMace.cpp
    combat/autototem/AutoTotem.cpp
    player/autoreplenish/AutoReplenish.cpp
    combat/legitcrystalaura/LegitCrystalAura.cpp
    combat/autocrystal/AutoCrystal.cpp
    combat/legitanchoraura/LegitAnchorAura.cpp
    combat/anchoraura/AnchorAura.cpp
    combat/feetplace/FeetPlace.cpp
    combat/legitcrystalaura/PlacementEsp.cpp
    misc/fakeplayer/FakePlayer.cpp
    combat/autoblock/AutoBlockPacketSync.cpp
    combat/autoblock/AutoBlockSlinky.cpp
    combat/autoblock/AutoBlockHypixel.cpp
    combat/autoblock/AutoBlockLagNew.cpp
    combat/autoblock/AutoBlockModule.cpp
    combat/autoblock/AutoBlockStats.cpp

    # combat/autoclicker
    combat/autoclicker/AutoClicker.cpp

    # combat/autopot
    combat/autopot/AutoPot.cpp

    # combat/autorod
    combat/autorod/AutoRod.cpp

    # combat/backtrack
    combat/backtrack/Backtrack.cpp

    # combat/example
    combat/example/Example.cpp

    # combat/displace
    combat/displace/Displace.cpp
    combat/displace/DisplacePackets.cpp

    # combat/keepsprint
    combat/keepsprint/KeepSprint.cpp

    # combat/killaura
    combat/killaura/KillAura.cpp

    # combat/throwableaura
    combat/throwableaura/ThrowableAura.cpp
    combat/throwableaura/ThrowableVisuals.cpp

    # combat/velocity
    combat/velocity/Velocity.cpp

    # misc/teams
    misc/teams/Teams.cpp

    # misc/antibot
    misc/antibot/AntiBot.cpp
    misc/healthbypass/HealthBypass.cpp
    misc/imefix/ImeFix.cpp
    misc/imefix/ChatInput.cpp

    # misc/ghosthand
    misc/ghosthand/GhostHand.cpp
    misc/ghosthand/GhostHandPlayer.cpp

    # movement/sprint
    movement/elytrafly/ElytraFly.cpp
    movement/elytrafly/AutoFire.cpp
    movement/sprint/Sprint.cpp
    movement/speed/Speed.cpp
    movement/invmove/InventoryMove.cpp

    # movement/nojumpdelay
    movement/nojumpdelay/NoJumpDelay.cpp
    movement/timer/Timer.cpp

    # movement/knockbackbalancer
    movement/knockbackbalancer/KnockBackBalancer.cpp

    # movement/stuck
    movement/stuck/Stuck.cpp

    # player/autotools
    player/automlg/AutoMLG.cpp
    player/autocharge/AutoCharge.cpp
    player/autopearl/AutoPearl.cpp
    player/pearlphase/PearlPhase.cpp
    player/autotools/AutoTools.cpp

    # player/breaker
    player/breaker/Breaker.cpp

    # player/bridgeassist
    player/bridgeassist/BridgeAssist.cpp

    # player/cheststealer
    player/cheststealer/ChestStealer.cpp

    # player/fastplace
    player/fastplace/FastPlace.cpp

    # player/blockin
    player/blockin/BlockIn.cpp
    # player/invmanager
    player/invmanager/InvManager.cpp
    player/hypixelautoplay/HypixelAutoPlay.cpp

    # player/refill
    player/refill/Refill.cpp

    # player/scaffold
    player/scaffold/Scaffold.cpp

    # render/arraylist
    render/arraylist/ArrayList.cpp

    # render/autoblockhud
    render/autoblockhud/AutoBlockHUD.cpp

    # render/chestesp
    render/chestesp/ChestESP.cpp
    render/bedplates/BedPlates.cpp

    # render/itemesp
    render/itemesp/ItemESP.cpp

    # render/clickgui
    render/clickgui/ClickGui.cpp

    # render/compass
    render/compass/Compass.cpp

    # render/freelook
    render/freelook/FreeLook.cpp
    render/freecam/FreeCam.cpp

    # render/fullbright
    render/fullbright/FullBright.cpp

    # render/musichud
    render/musichud/MusicHUD.cpp

    # render/nametags
    render/nametags/NameTags.cpp

    # render/esp2d
    render/esp2d/Esp2D.cpp

    # render/notifications
    render/notifications/Notifications.cpp
    render/cswin/CSWin.cpp

    # render/targethud
    render/targethud/TargetHUD.cpp
    render/targetesp/TargetESP.cpp
    render/playerlist/PlayerList.cpp

    # render/watermark
    render/watermark/Watermark.cpp
)
