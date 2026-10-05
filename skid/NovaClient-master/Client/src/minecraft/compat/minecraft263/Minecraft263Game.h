#pragma once

#include <cstdint>
#include <unordered_map>
#include "minecraft/java/api/base.h"
#include "minecraft/java/api/game_rotation.h"
#include "minecraft/java/api/game_entity.h"
#include "minecraft/java/api/game_inventory.h"
#include "minecraft/java/api/game_compass.h"
#include "minecraft/java/api/game_chest.h"
#include "minecraft/java/api/hook_bridge.h"

namespace nova::compat::mc263 {

bool PlayerOnGround();
bool PlayerUsingItem();
bool PlayerAttackCooldownReady();
bool PlayerInCreative();
bool PlayerCanEdit();
bool HoldingWeapon();
bool HoldingSword();
bool AddNightVision();
bool RemoveNightVision();
bool MouseGrabbed();
bool ClickAttack();
bool SetAttackDown(bool down);
bool ClearMissTime();
bool AttackTargetIsBlock();
bool TargetedBlockBreakable();
bool InventoryScreenOpen();
bool ClickHoveredSlot(bool quickMove);
bool PlayerRotation(float& yaw, float& pitch, bool previous = false);
bool SetPlayerRotation(float yaw, float pitch);
bool PlayerEyePosition(double& x, double& y, double& z);
bool PlayerSensitivity(double& out);
bool RenderDistance(int& out);
bool Fov(int& out);
int ClientFps();
void PickCursor(float partialTicks);
bool IsPosRot(void* packet);
bool ReadPosRot(void* packet, java::PosRot& out);
void* MakePosRot(const java::PosRot& value);
enum class EntityBool { Player, Spectator, Pickable, Alive, Invisible, OnGround };
enum class EntityInt { Id, Ticks, Hurt, Death };
enum class EntityFloat { EyeHeight, PickRadius, Health, Absorption, MaxHealth, FallDistance };
bool ReadEntityBool(void* object, EntityBool property, bool& out);
bool ReadEntityInt(void* object, EntityInt property, int& out);
bool ReadEntityFloat(void* object, EntityFloat property, float& out);
bool EntityPosition(void* object, rotation::Vec3& out, bool previous = false);
bool EntityRenderPositionPair(void* object, rotation::Vec3& now, rotation::Vec3& before);
bool EntityEyePosition(void* object, float partialTicks, rotation::Vec3& out);
bool EntityBox(void* object, java::Box& out);
bool EntityName(void* object, std::string& out, bool formatted);
bool EntitySameTeam(void* object, void* other, bool& out);
bool EntitySameVehicle(void* object, void* other);
bool EntitySetHealth(void* object, float value);
java::EntityHandle PlayerEntity();
java::EntityHandle LevelEntity();
java::EntityHandle EntityById(int id);
bool WorldPlayers(std::vector<java::EntityHandle>& out);
bool WorldEntities(const java::EntityHandle& source, const java::Box& area, std::vector<java::EntityHandle>& out);
float FrameTime();
void JNICALL WorldCameraHook(JNIEnv*, jclass, jobject camera, jobject modelView, jobject projection);
bool EntityTypes(const java::EntityHandle& entity, unsigned& out);
bool WorldClip(const rotation::Vec3& from, const rotation::Vec3& to, const java::EntityHandle& source, java::BlockHitResultInfo& out, java::ClipMode mode=java::ClipMode::Outline);
java::EntityHandle MakeEntityHit(const java::EntityHandle& entity, const rotation::Vec3& location);
bool BoxClip(const java::Box& box, const rotation::Vec3& from, const rotation::Vec3& to, bool& hit, rotation::Vec3& out);
bool DirectionFromRotation(float pitch, float yaw, rotation::Vec3& out);
enum class PacketKind { UseItem, UseItemOnly, VelocityDelay, Motion, PositionOrExplode, Ping, Explode, Position, Disconnect, Respawn, ContainerClose, MovePlayer, UseItemOn, Interact, PlayerAction, ReduceDelay };
bool IsPacket(jobject packet, PacketKind kind);
bool SendPacket(jobject packet);
bool SendStatusOnlyPacket(bool onGround);
bool ReplayIncoming(JNIEnv* env, jobject packet, jobject listener);
bool HandleIncomingPacket(jobject packet);
bool ReadMotionPacket(jobject packet, int& id, int& x, int& y, int& z);
bool AttackTargetEntity(java::EntityHandle& out);
bool AttackEntity(const java::EntityHandle& target);
bool SwingHand();
bool SwingHand(bool offhand,bool animate);
bool RightClickDelay(int& out);
bool SetRightClickDelay(int value);
bool HoldingBlockItem();
enum class InputKey { Use, Jump, Sneak };
bool KeyDown(InputKey key);
bool SetKeyShiftDown(bool down);
bool PlayerDeltaMovement(rotation::Vec3& out);
bool SetPlayerDeltaMovement(double x, double y, double z);
bool SetPlayerSprinting(bool value);
bool PlayerSprinting(bool& out);
bool PlayerSpeed(float& out);
bool InventoryStack(int index, bool armor, jobject& out);
bool ReadItemFacts(jobject stack, java::ItemFacts& out);
bool ItemDisplayName(jobject stack, std::string& out);
bool InventoryHasItem(java::KnownItem item);
int InventoryCount(bool armor);
jobject CurrentScreen();
bool ScreenIsContainerScreen(jobject screen);
bool ScreenIsInventoryScreen(jobject screen);
bool ScreenMenu(jobject screen, jobject& out);
bool MenuContainerId(jobject menu, int& out);
bool MenuIsPlayerInventory(jobject menu);
bool MenuIsChest(jobject menu);
bool MenuRowCount(jobject menu, int& out);
bool MenuSlotStack(jobject menu, int slot, jobject& out);
bool PlayerMenu(jobject& out);
bool ScreenTitle(jobject screen, std::string& out);
bool TranslatedText(const char* key, std::string& out);
bool ClickMenuSlot(int containerId, int slot, int button, java::ClickKind click);
bool CloseContainer();
bool SendContainerClose(int containerId);
bool ReadCompassView(float partialTick, java::CompassView& out);
bool WorldSpawn(int& x, int& z);
bool WindowMetrics(int& width, int& height, double& guiScale);
std::uint64_t PlayerSkinTexture(const java::EntityHandle& entity);
bool ReadHealthScore(jobject packet, std::string& owner, int& score);
bool ReadStoredHealthScores(std::unordered_map<std::string,int>& scores,std::string* objectives);
enum class BlockProperty { CollisionEmpty, OpensMenu, Replaceable, ScaffoldExcluded };
bool ReadBlockProperty(const java::BlockPos& pos, BlockProperty property);
bool SelectedSlot(int& out);
bool SetSelectedSlot(int slot);
bool HotbarSlotPlaceable(int slot);
int HotbarSlotCount(int slot);
int FacingStep(int facing, int axis);
int FacingOpposite(int facing);
bool UseItemOnBlock(const java::BlockPos& pos, int facing, const rotation::Vec3& hitVector);
bool SendMoveRotation(float yaw, float pitch, bool onGround);
bool SendSwingPacket();
bool GameModeDestroying();
bool HitBlockPos(java::BlockPos& out);
bool BestToolSlotFor(const java::BlockPos& pos, int& out);
bool LoadedChestBoxes(std::vector<java::ChestBox>& out);
bool ReadCameraView(float partialTick, java::CameraView& out);
bool ReadChestOpenEvent(jobject packet, int& x, int& y, int& z);

bool InventoryOffhand(int index, jobject& out);
const bridge::HookDef* Hooks(int& count);
bool PrepareHooks(JNIEnv* env);
void ReleaseGame(JNIEnv* env);
}
