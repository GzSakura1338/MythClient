package today.starain.module.impl.player;

import net.minecraft.core.BlockPos;
import net.minecraft.network.protocol.game.ServerboundMovePlayerPacket;
import net.minecraft.world.InteractionHand;
import net.minecraft.world.item.Items;
import net.minecraft.world.level.ClipContext;
import net.minecraft.world.level.block.state.BlockState;
import net.minecraft.world.level.material.FluidState;
import net.minecraft.world.level.material.Fluids;
import net.minecraft.world.phys.BlockHitResult;
import net.minecraft.world.phys.HitResult;
import net.minecraft.world.phys.Vec3;
import today.starain.Starain;
import today.starain.event.api.EventType;
import today.starain.event.api.annotations.EventTarget;
import today.starain.event.impl.GameTickEvent;
import today.starain.event.impl.MotionEvent;
import today.starain.event.impl.MoveInputEvent;
import today.starain.module.Category;
import today.starain.module.ClientModule;
import today.starain.utils.client.NetworkUtils;
import today.starain.utils.player.InventoryUtils;
import today.starain.utils.rotation.Rotation;
import today.starain.utils.rotation.RotationManager;
import today.starain.utils.rotation.RotationUtils;
import today.starain.value.impl.BooleanValue;
import today.starain.value.impl.ModeValue;
import today.starain.value.impl.NumberValue;


public class NoFall extends ClientModule {
    private final ModeValue mode = new ModeValue("Mode", "WaterBucket", new String[]{"WaterBucket", "Grim"}, this);
    private final NumberValue fallDistance = new NumberValue("FallDistance", 3.0, 15.0, 3.0, 0.1, this);
    private final NumberValue rotateSpeed = new NumberValue("Rotation Speed", 1.0, 180.0, 180.0, 1.0, this);
    private final BooleanValue solidCheck = new BooleanValue("SolidCheck", true, this);
    private final BooleanValue retrieve = new BooleanValue("Retrieve", true, this);
    private final BooleanValue antiFire = new BooleanValue("AntiFire", true, this);

    // ---- Grim 模式状态 ----
    private float grimLastFallDistance;
    private boolean grimQueuedJump;

    // ---- WaterBucket 模式状态 ----
    private Integer restoreSlot;
    private boolean attemptedThisFall;
    private boolean retrievePending;
    private int retrieveTriesLeft;
    private Integer retrieveSlot;
    private BlockPos placedWaterPos;
    private boolean lastChanceSkipApplied;
    private int postRetrievePauseTicks;
    private int ambientPickupCooldownTicks;
    private int firePlaceCooldownTicks;

    public NoFall() {
        setName("NoFall");
        setCategory(Category.PLAYER);
    }

    @Override
    public void onEnable() {
        resetState();
    }

    @Override
    public void onDisable() {
        resetState();
    }

    private void resetState() {
        restoreSlot = null;
        attemptedThisFall = false;
        retrievePending = false;
        retrieveTriesLeft = 0;
        retrieveSlot = null;
        placedWaterPos = null;
        lastChanceSkipApplied = false;
        postRetrievePauseTicks = 0;
        ambientPickupCooldownTicks = 0;
        firePlaceCooldownTicks = 0;

        grimLastFallDistance = 0.0F;
        grimQueuedJump = false;
    }

    public boolean isInPostRetrievePause() {
        return postRetrievePauseTicks > 0;
    }

    @EventTarget
    public void onTick(GameTickEvent event) {
        if (event.getType() != EventType.PRE) return;
        if (mc.player == null || mc.level == null) return;
        if (mc.player.isFallFlying()) return;

        if (postRetrievePauseTicks > 0) {
            postRetrievePauseTicks--;
        }
        if (ambientPickupCooldownTicks > 0) {
            ambientPickupCooldownTicks--;
        }
        if (firePlaceCooldownTicks > 0) {
            firePlaceCooldownTicks--;
        }

        if (restoreSlot != null) {
            mc.player.getInventory().selected = restoreSlot;
            restoreSlot = null;
        }

        if (mc.player.onGround() || mc.player.fallDistance <= 0.0F) {
            attemptedThisFall = false;
            lastChanceSkipApplied = false;
        }


        if (retrievePending) {
            if (retrieveTriesLeft-- <= 0) {
                retrievePending = false;
                return;
            }
            if (retrieveSlot == null) {
                retrieveSlot = InventoryUtils.findItemHotbar(Items.BUCKET);
                if (retrieveSlot == null) {
                    retrievePending = false;
                    return;
                }
            }

            if (mc.player.getInventory().items.get(retrieveSlot).getItem() == Items.WATER_BUCKET) {
                retrievePending = false;
                retrieveSlot = null;
                placedWaterPos = null;
                postRetrievePauseTicks = Math.max(postRetrievePauseTicks, 1);
                return;
            }

            if (placedWaterPos == null || !isWaterSource(placedWaterPos)) {
                retrievePending = false;
                retrieveSlot = null;
                placedWaterPos = null;
                return;
            }

            Rotation pickupRotation = RotationUtils.calculate(placedWaterPos);
            BlockHitResult hit = rayCastWaterSource(pickupRotation, 4.5D);
            if (hit.getType() == HitResult.Type.MISS || !hit.getBlockPos().equals(placedWaterPos)) {
                retrievePending = false;
                retrieveSlot = null;
                placedWaterPos = null;
                return;
            }

            requestRotation(pickupRotation);
            saveAndSwitch(retrieveSlot);
            mc.gameMode.useItem(mc.player, InteractionHand.MAIN_HAND);
            return;
        }

        if (!attemptedThisFall
                && !retrievePending
                && placedWaterPos == null
                && postRetrievePauseTicks == 0
                && ambientPickupCooldownTicks == 0
                && mc.player.fallDistance <= 0.5F
                && InventoryUtils.findItemHotbar(Items.WATER_BUCKET) == null) {
            Integer emptyBucketSlot = InventoryUtils.findItemHotbar(Items.BUCKET);
            if (emptyBucketSlot != null) {
                BlockPos sourcePos = findNearbyWaterSource();
                if (sourcePos != null) {
                    Rotation pickupRotation = RotationUtils.calculate(sourcePos);
                    BlockHitResult hit = rayCastWaterSource(pickupRotation, 4.5D);
                    if (hit.getType() != HitResult.Type.MISS && hit.getBlockPos().equals(sourcePos)) {
                        requestRotation(pickupRotation);
                        saveAndSwitch(emptyBucketSlot);
                        mc.gameMode.useItem(mc.player, InteractionHand.MAIN_HAND);
                        ambientPickupCooldownTicks = 8;
                        postRetrievePauseTicks = Math.max(postRetrievePauseTicks, 1);
                        return;
                    }
                }
            }
        }


        if (antiFire.get()
                && firePlaceCooldownTicks == 0
                && !retrievePending
                && !mc.player.isFallFlying()
                && mc.player.isOnFire()
                && !mc.player.isInWaterRainOrBubble()) {
            Integer waterSlot = InventoryUtils.findItemHotbar(Items.WATER_BUCKET);
            if (waterSlot != null) {
                BlockPos playerPos = BlockPos.containing(mc.player.getX(), mc.player.getY(), mc.player.getZ());
                if (!solidCheck.get() || hasSolidBelow(playerPos)) {
                    placeWaterWithRetrieve(waterSlot, false);
                    firePlaceCooldownTicks = 8;
                    return;
                }
            }
        }

        if (!mode.isMode("WaterBucket")) return;

        if (attemptedThisFall && !lastChanceSkipApplied && mc.player.getDeltaMovement().y < 0.0D) {
            double remaining = getRemainingFallDistance(2.5D);
            if (remaining > 0.0D && remaining <= 1.05D) {
                if (Starain.skipTicks < 25) {
                    Starain.skipTicks++;
                }
                lastChanceSkipApplied = true;
            }
        }

        if (attemptedThisFall) return;
        if (mc.player.fallDistance < fallDistance.get().floatValue()) return;

        Integer waterSlot = InventoryUtils.findItemHotbar(Items.WATER_BUCKET);
        if (waterSlot == null) return;

        if (solidCheck.get()) {
            BlockPos playerPos = BlockPos.containing(mc.player.getX(), mc.player.getY(), mc.player.getZ());
            if (!hasSolidBelow(playerPos)) return;
        }

        placeWaterWithRetrieve(waterSlot, true);
    }

    @EventTarget
    public void onMotion(MotionEvent event) {
        if (!this.isEnabled()) return;
        if (!mode.isMode("Grim")) return;
        if (mc.player == null) return;
        if (event.getType() != EventType.PRE) return;

        float fallDistance1 = mc.player.fallDistance;

        if (grimLastFallDistance >= fallDistance.get().floatValue() && event.isOnGround()) {
            event.setOnGround(false);
            NetworkUtils.sendPacketNoEvent(new ServerboundMovePlayerPacket.StatusOnly(true));
            Starain.skipTicks++;
            grimQueuedJump = true;
        }

        grimLastFallDistance = fallDistance1;
    }

    @EventTarget
    public void onMoveInput(MoveInputEvent event) {
        if (mc.player == null) return;
        if (!this.isEnabled()) return;
        if (!mode.isMode("Grim")) return;

        if (grimLastFallDistance >= fallDistance.get().floatValue() && mc.player.onGround()) {
            event.setSneak(false);
        }

        if (grimQueuedJump) {
            event.setJump(true);
            grimQueuedJump = false;
        }
    }

    private BlockPos findNearbyWaterSource() {
        BlockPos base = BlockPos.containing(mc.player.getX(), mc.player.getY(), mc.player.getZ());
        BlockPos best = null;
        double bestDist = Double.POSITIVE_INFINITY;
        int radius = 4;
        for (int dy = -1; dy <= 1; dy++) {
            for (int dx = -radius; dx <= radius; dx++) {
                for (int dz = -radius; dz <= radius; dz++) {
                    BlockPos pos = base.offset(dx, dy, dz);
                    if (!isWaterSource(pos)) continue;
                    double dist = mc.player.position().distanceToSqr(pos.getX() + 0.5, pos.getY() + 0.5, pos.getZ() + 0.5);
                    if (dist >= bestDist) continue;
                    Rotation rotation = RotationUtils.calculate(pos);
                    BlockHitResult hit = rayCastWaterSource(rotation, 4.5D);
                    if (hit.getType() == HitResult.Type.MISS || !hit.getBlockPos().equals(pos)) continue;
                    best = pos;
                    bestDist = dist;
                }
            }
        }
        return best;
    }

    private void requestRotation(Rotation rotation) {
        RotationManager.setRotations(rotation, rotateSpeed.get().floatValue());
    }

    private void saveAndSwitch(int targetSlot) {
        restoreSlot = mc.player.getInventory().selected;
        mc.player.getInventory().selected = targetSlot;
    }

    private void placeWaterWithRetrieve(int waterSlot, boolean markAttemptedThisFall) {
        Rotation placeRotation = new Rotation(mc.player.getYRot(), 90.0F);
        requestRotation(placeRotation);
        saveAndSwitch(waterSlot);
        mc.gameMode.useItem(mc.player, InteractionHand.MAIN_HAND);

        if (markAttemptedThisFall) {
            attemptedThisFall = true;
        }

        retrievePending = retrieve.get();
        retrieveTriesLeft = retrievePending ? 2 : 0;
        retrieveSlot = null;
        placedWaterPos = computePlacedWaterPos(placeRotation);
    }

    private BlockPos computePlacedWaterPos(Rotation placeRotation) {
        BlockHitResult hit = rayCastBlock(placeRotation, 4.5D);
        if (hit.getType() == HitResult.Type.MISS) return null;
        return hit.getBlockPos().relative(hit.getDirection());
    }

    private BlockHitResult rayCastBlock(Rotation rotation, double range) {
        Vec3 eyePos = mc.player.getEyePosition(1.0F);
        Vec3 lookVec = Vec3.directionFromRotation(rotation.getPitch(), rotation.getYaw());
        Vec3 endVec = eyePos.add(lookVec.scale(range));
        return mc.level.clip(new ClipContext(
                eyePos,
                endVec,
                ClipContext.Block.OUTLINE,
                ClipContext.Fluid.NONE,
                mc.player
        ));
    }

    private BlockHitResult rayCastWaterSource(Rotation rotation, double range) {
        Vec3 eyePos = mc.player.getEyePosition(1.0F);
        Vec3 lookVec = Vec3.directionFromRotation(rotation.getPitch(), rotation.getYaw());
        Vec3 endVec = eyePos.add(lookVec.scale(range));
        return mc.level.clip(new ClipContext(
                eyePos,
                endVec,
                ClipContext.Block.OUTLINE,
                ClipContext.Fluid.SOURCE_ONLY,
                mc.player
        ));
    }

    private boolean isWaterSource(BlockPos pos) {
        FluidState fluidState = mc.level.getFluidState(pos);
        return fluidState.getType() == Fluids.WATER && fluidState.isSource();
    }

    private boolean hasSolidBelow(BlockPos playerPos) {
        return isSolidAndNonInteractive(playerPos.below()) || isSolidAndNonInteractive(playerPos.below(2));
    }

    private boolean isSolidAndNonInteractive(BlockPos pos) {
        BlockState state = mc.level.getBlockState(pos);
        boolean hasCollision = !state.getCollisionShape(mc.level, pos).isEmpty();
        boolean hasNoMenu = state.getMenuProvider(mc.level, pos) == null;
        return hasCollision && hasNoMenu;
    }

    private double getRemainingFallDistance(double maxRange) {
        Vec3 start = new Vec3(mc.player.getX(), mc.player.getBoundingBox().minY, mc.player.getZ());
        Vec3 end = start.add(0.0D, -maxRange, 0.0D);
        BlockHitResult hit = mc.level.clip(new ClipContext(
                start,
                end,
                ClipContext.Block.COLLIDER,
                ClipContext.Fluid.NONE,
                mc.player
        ));
        if (hit.getType() == HitResult.Type.MISS) return Double.POSITIVE_INFINITY;
        return start.y - hit.getLocation().y;
    }
}
