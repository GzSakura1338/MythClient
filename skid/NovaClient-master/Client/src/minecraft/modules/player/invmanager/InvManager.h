#pragma once

namespace nova::module::invmanager {
bool UsesInventoryMovement();

// The settings the chest stealer has to ask about. In Starain they are public
// statics on InvManager and read straight out of that module; here they answer
// from the same live property objects, so changing one takes effect immediately
// whether or not InvManager itself is switched on.

int MaxBlockSize();

int MaxArrowSize();

int MaxProjectileSize();

int WaterBucketCount();

int LavaBucketCount();

bool ShouldKeepProjectile();

}
