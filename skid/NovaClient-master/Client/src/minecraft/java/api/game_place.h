#pragma once

#include "base.h"
#include "minecraft/java/api/game_entity.h"
#include "minecraft/utils/rotation/Vec3.h"
#include <string_view>

namespace nova::java {

bool RightClickDelay(int& out);

bool SetRightClickDelay(int value);

bool UseKeyDown();

bool HoldingBlockItem();

float PlayerPitch();

// --- scaffold ---------------------------------------------------------------
//
// Everything below exists for the scaffold: it reads the world one block at a
// time, walks the hotbar looking for something it is willing to place, and puts a
// block down against a named face.
//
// The face is passed as net.minecraft.core.Direction's declaration order --
// DOWN, UP, NORTH, SOUTH, WEST, EAST -- which is the order rotation::BlockFace
// already mirrors.

bool BlockStateCollisionEmpty(const BlockPos& pos);

bool BlockStateOpensMenu(const BlockPos& pos);

bool BlockStateReplaceable(const BlockPos& pos);

// Rise's exact candidate-origin blacklist, separate from menu/collision checks.
bool BlockStateScaffoldExcluded(const BlockPos& pos);
// Shared native adapter for the game's Block / BlockState registry description.
bool ScaffoldExcludedObject(JNIEnv* env, jobject blockOrState);
bool ScaffoldExcludedRegistryName(std::string_view name);

bool SelectedSlot(int& out);

bool SetSelectedSlot(int value);

// True when a hotbar slot holds something the scaffold will place: a block that
// is not a flower, a slab, or a wall-standing block, is not one of the
// forty-six blacklisted blocks, and does not advertise a server menu.
bool HotbarSlotPlaceable(int slot);

// Stack size of a placeable slot, 0 otherwise.
int HotbarSlotCount(int slot);

// Skip stacks with no blocks available above the requested reserve.
int FindPlaceableBlockSlot(int reserve = 0);

int CountPlaceableBlocks();

// Direction's step vector and opposite, read off the game once rather than
// assumed, for the face index the rest of this section uses.
int FacingStepX(int facing);

int FacingStepY(int facing);

int FacingStepZ(int facing);

int FacingOpposite(int facing);

bool UseItemOnBlock(const BlockPos& pos, int facing, const rotation::Vec3& hitVector);

bool SendMoveRotation(float yaw, float pitch, bool onGround);

bool SendSwingPacket();

// LocalPlayer.swing(InteractionHand.MAIN_HAND) -- the arm animation *and* the
// packet, which is what the game itself does on a normal right click. The packet
// alone, above, is for the burst path, where the original also sends only the
// packet.
bool SwingMainHand();

bool KeyJumpDown();

bool KeyShiftDown();

bool SetKeyShiftDown(bool down);

bool PlayerDeltaMovement(rotation::Vec3& out);

// Entity.setDeltaMovement and Entity.setSprinting. The knockback packet writes the
// first through lerpMotion; a module that adjusts the motion it left behind has to
// write it here.
bool SetPlayerDeltaMovement(double x, double y, double z);
bool SetPlayerSprinting(bool sprinting);

bool PlayerSpeed(float& out);

bool PlayerSprinting(bool& out);

// The client drops the local player's own tick for a few ticks after the scaffold
// sends its placement rotation, so that the rotation is what the next movement is
// built from. The countdown lives here because the hook that spends it does.
void RequestPlayerTickSkip();

bool ShouldSkipPlayerTick(jobject entity);

}
