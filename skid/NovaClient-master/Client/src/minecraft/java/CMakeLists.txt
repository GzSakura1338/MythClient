# Explicit source ownership; add a new implementation in its feature group.
target_sources(NovaClientBase PRIVATE
    # Infrastructure
    Env.cpp
    HookBridge.cpp
    JniLookup.cpp
    RandomName.cpp

    # bytecode
    bytecode/ClassEdit.cpp
    bytecode/ClassFile.cpp

    # game
    game/GameBacktrack.cpp
    game/GameBridgeAssist.cpp
    game/GameChest.cpp
    game/GameWorldCamera.cpp
    game/GameChat.cpp
    game/GameVictory.cpp
    game/GameBeds.cpp
    game/GameBreaker.cpp
    game/GameClick.cpp
    game/GameCritical.cpp
    game/GameKeepSprint.cpp
    game/GameCompass.cpp
    game/GameEntity.cpp
    game/GameFreeLook.cpp
    game/GameFreeCam.cpp
    game/GameHud.cpp
    game/GamePresentationName.cpp
    game/GameInput.cpp
    game/GameInventory.cpp
    game/GameInventoryStack.cpp
    game/GameElytra.cpp
    game/GameInventoryScreen.cpp
    game/GameNetwork.cpp
    game/GameOptions.cpp
    game/GameMovementKeys.cpp
    game/GameSpeed.cpp
    game/GamePlace.cpp
    game/GamePlayer.cpp
    game/GameItemUse.cpp
    game/GameCharge.cpp
    game/GamePearlPhase.cpp
    game/GameMace.cpp
    game/GameCrystal.cpp
    game/GameAnchor.cpp
    game/GameBlockPlace.cpp
    game/GameFakePlayer.cpp
    game/GameMLG.cpp
    game/GamePlayerList.cpp
    game/GameRotation.cpp
    game/GameScore.cpp
    game/GameSkin.cpp
    game/GameSlotSpoof.cpp
    game/GameTeams.cpp
    game/GameTool.cpp
)
