#pragma once

#include "minecraft/java/api/base.h"
#include "minecraft/java/api/game_entity.h"
#include "minecraft/java/api/game_inventory.h"

namespace nova::java {

// --- AutoTools ---------------------------------------------------------------
//
// The world reads the auto tool needs: what the crosshair is on, whether the game
// is part-way through breaking it, and which hotbar slot cuts it fastest.
//
// The last one is the whole module. Starain scores every slot with the game's own
// destroy speed against that exact block -- so a pickaxe beats a shovel on stone
// without either being named -- folds in efficiency the way vanilla's mining maths
// wants it, and refuses a few stacks outright.

// True while MultiPlayerGameMode.isDestroying is set, which is between the first
// and the last tick of a block break.
bool GameModeDestroying();

// Minecraft.hitResult, when it landed on a block. False for a miss, an entity hit,
// or an unreadable client.
bool HitBlockPos(BlockPos& out);

// Inventory.selected -- the hotbar slot the player is holding. Read and written
// here rather than through the scaffold layer so that the two stay independent.
bool SelectedHotbarSlot(int& out);
bool SetSelectedHotbarSlot(int value);

// The hotbar slot whose stack cuts the block at `pos` fastest, or -1 when nothing
// in the bar beats an empty hand. This is Starain's AutoTools.getBestTool, kept
// whole including its quirks: a god item is never picked, a sword is only picked
// when the block is a cobweb, and experience-dropping and redstone ores skip the
// efficiency term.
//
// False only when the world cannot be read at all, in which case `out` is -1.
bool BestToolSlotFor(const BlockPos& pos, int& out);

}
