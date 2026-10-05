#pragma once

#include "minecraft/java/api/game_entity.h"

namespace nova::java {

bool ScreenOpen();

bool AttackTargetIsBlock();

bool ClearMissTime();

bool TargetedBlockBreakable();

bool InventoryScreenOpen();

bool ClickHoveredSlot(bool quickMove);

bool AttackTargetEntity(EntityHandle& out);

bool AttackEntity(const EntityHandle& target);

bool AttackAndSwing(const EntityHandle& target);

bool SwingHand();

}
