package dev.myth.model.player;

import com.mojang.blaze3d.platform.InputConstants;
import net.minecraft.core.BlockPos;
import net.minecraft.core.Direction;
import net.minecraft.util.Mth;
import net.minecraft.world.InteractionHand;
import net.minecraft.world.InteractionResult;
import net.minecraft.world.item.BlockItem;
import net.minecraft.world.item.ItemStack;
import net.minecraft.world.item.StandingAndWallBlockItem;
import net.minecraft.world.level.block.*;
import net.minecraft.world.level.block.state.BlockState;
import net.minecraft.world.phys.BlockHitResult;
import net.minecraft.world.phys.Vec3;

import dev.myth.event.api.Priority;
import dev.myth.event.api.annotations.EventPriority;
import dev.myth.event.api.annotations.EventTarget;
import dev.myth.event.impl.MoveInputEvent;
import dev.myth.event.impl.PlayerTickEvent;
import dev.myth.model.Category;
import dev.myth.model.Model;
import dev.myth.utils.wrapper.Wrapper;
import dev.myth.UI.setting.Setting;
import dev.myth.utils.player.MoveUtils;
import dev.myth.utils.rotation.RayCastUtil;
import dev.myth.utils.rotation.Rotation;
import dev.myth.utils.rotation.RotationManager;
import dev.myth.utils.rotation.RotationUtils;

import java.util.List;

/** Starain Scaffold, without ESP; rotations and MoveFix use the shared utilities. */
public final class Scaffold extends Model implements Wrapper {
    private static final List<Block> BLACKLISTED_BLOCKS = List.of(
            Blocks.AIR,
            Blocks.WATER,
            Blocks.LAVA,
            Blocks.ENCHANTING_TABLE,
            Blocks.GLASS_PANE,
            Blocks.IRON_BARS,
            Blocks.SNOW,
            Blocks.COAL_ORE,
            Blocks.DIAMOND_ORE,
            Blocks.EMERALD_ORE,
            Blocks.CHEST,
            Blocks.TRAPPED_CHEST,
            Blocks.TORCH,
            Blocks.ANVIL,
            Blocks.NOTE_BLOCK,
            Blocks.JUKEBOX,
            Blocks.TNT,
            Blocks.GOLD_ORE,
            Blocks.IRON_ORE,
            Blocks.LAPIS_ORE,
            Blocks.STONE_PRESSURE_PLATE,
            Blocks.LIGHT_WEIGHTED_PRESSURE_PLATE,
            Blocks.HEAVY_WEIGHTED_PRESSURE_PLATE,
            Blocks.STONE_BUTTON,
            Blocks.LEVER,
            Blocks.TALL_GRASS,
            Blocks.TRIPWIRE,
            Blocks.TRIPWIRE_HOOK,
            Blocks.RAIL,
            Blocks.CORNFLOWER,
            Blocks.RED_MUSHROOM,
            Blocks.BROWN_MUSHROOM,
            Blocks.VINE,
            Blocks.SUNFLOWER,
            Blocks.LADDER,
            Blocks.FURNACE,
            Blocks.SAND,
            Blocks.CACTUS,
            Blocks.DISPENSER,
            Blocks.DROPPER,
            Blocks.CRAFTING_TABLE,
            Blocks.COBWEB,
            Blocks.PUMPKIN,
            Blocks.COBBLESTONE_WALL,
            Blocks.OAK_FENCE,
            Blocks.REDSTONE_TORCH,
            Blocks.FLOWER_POT
    );
    public final Setting<Boolean> telly = addSetting(Setting.bool("telly", "Telly", false));
    public final Setting<Boolean> snap = addSetting(Setting.bool("snap", "Snap", false).visibleWhen(() -> !telly.get()));
    public final Setting<Float> snapDist = addSetting(Setting.decimal("snapDist", "Snap Dist", .1f, 0, 1).step(.01).visibleWhen(snap::get));
    public final Setting<Boolean> degrees45 = addSetting(Setting.bool("degrees45", "45 Degrees", false));
    public final Setting<Boolean> swing = addSetting(Setting.bool("swing", "Swing", true));
    public final Setting<String> rotMod = addSetting(Setting.choice("rotationMode", "RotationMode", "Rise", List.of("Rise", "Heypixel")));
    public final Setting<String> rayCast = addSetting(Setting.choice("rayCast", "RayCast", "Normal", List.of("Normal", "Strict")));
    public final Setting<Integer> rotateSpeed = addSetting(Setting.integer("rotateSpeed", "RotSpeed", 127, 1, 180));
    public final Setting<Integer> rotateSpeed2 = addSetting(Setting.integer("rotateSpeed2", "RotSpeed2", 36, 1, 180).visibleWhen(() -> rotMod.get().equals("Heypixel")));
    public final Setting<Integer> rotateBackSpeed = addSetting(Setting.integer("rotateBackSpeed", "BackSpeed", 180, 1, 180).visibleWhen(telly::get));
    public final Setting<Integer> tellyTick = addSetting(Setting.integer("tellyTick", "TellyTicks", 1, 0, 6).visibleWhen(telly::get));
    public final Setting<Boolean> safeWalk = addSetting(Setting.bool("safeWalk", "SafeWalk", false).visibleWhen(() -> !telly.get()));
    public final Setting<Float> edgeDist = addSetting(Setting.decimal("edgeDist", "Edge Dist", .1f, 0, 1).step(.01).visibleWhen(() -> safeWalk.get() && !telly.get()));
    private int airTick;
    private int yLevel;
    private BlockPos blockPos;
    private Direction enumFacing;
    private int oldSlot = -1;
    private Rotation rotation;
    private float forwardInput, strafeInput;
    private float inputYaw;
    private float rawInputYaw;
    private double lengthSqr = 4.5D * 4.5D;

    public Scaffold() {
        super("scaffold", "Scaffold", Category.PLAYER);
        setDescription("Places blocks below you using Starain's scaffold and telly logic.");
    }

    private static float randomRange(float min, float max) { return min >= max ? min : new java.util.Random().nextFloat() * (max - min) + min; }
    private static double randomRange(double min, double max) { return min >= max ? min : new java.util.Random().nextDouble() * (max - min) + min; }

    public static Vec3 getVec3(BlockPos pos, Direction face) {
        double x = pos.getX() + 0.5;
        double y = pos.getY() + 0.5;
        double z = pos.getZ() + 0.5;

        if (face != Direction.UP && face != Direction.DOWN) {
            y += 0.08;
        } else {
            x += randomRange(-0.3, 0.3);
            z += randomRange(-0.3, 0.3);
        }

        if (face == Direction.WEST || face == Direction.EAST) {
            z += randomRange(-0.3, 0.3);
        }

        if (face == Direction.SOUTH || face == Direction.NORTH) {
            x += randomRange(-0.3, 0.3);
        }

        return new Vec3(x, y, z);
    }

    public static boolean isValidStack(ItemStack stack) {
        if (stack == null || stack.isEmpty() || !(stack.getItem() instanceof BlockItem)) {
            return false;
        }

        String name = stack.getDisplayName().getString();
        if (name.contains("Click") || name.contains("点击")) {
            return false;
        }

        if (stack.getItem() instanceof StandingAndWallBlockItem) {
            return false;
        }

        Block block = ((BlockItem) stack.getItem()).getBlock();
        if (block instanceof FlowerBlock || block instanceof VegetationBlock || block instanceof NetherFungusBlock || block instanceof CropBlock) {
            return false;
        }

        return !(block instanceof SlabBlock) && !BLACKLISTED_BLOCKS.contains(block);
    }

    @EventTarget
    private void onPreTick(PlayerTickEvent event) {
        if (mc.player == null || mc.level == null || mc.gameMode == null) { resetState(); return; }
        if (mc.gui.screen() != null) return;
        if (oldSlot == -1) oldSlot = mc.player.getInventory().getSelectedSlot();
        int slotId = findBlockSlot();
        if (slotId != -1 && mc.player.getInventory().getSelectedSlot() != slotId) {
            mc.player.getInventory().setSelectedSlot(slotId);
        }

        if (mc.player.onGround()) {
            airTick = 0;
            yLevel = Mth.floor(mc.player.getY()) - 1;
        } else {
            airTick++;
        }

        getBlockInfo();
        if (telly.get()) {
            handleTelly();
        } else {
            handleNormal();
        }
    }

    @EventPriority(Priority.HIGHEST)
    @EventTarget
    public void onMoveInput(MoveInputEvent event) {
        if (mc.player == null) {
            return;
        }
        forwardInput = event.getForward();
        strafeInput = event.getStrafe();
        inputYaw = Mth.wrapDegrees(Math.round((mc.player.getYRot() + (float) Math.toDegrees(Math.atan2(-strafeInput, forwardInput))) / 45.0F) * 45.0F);
        rawInputYaw = Mth.wrapDegrees(Math.round(mc.player.getYRot() + (float) Math.toDegrees(Math.atan2(-strafeInput, forwardInput))));
        if (mc.player.onGround() && !mc.options.keyJump.isDown() && MoveUtils.isMoving() && telly.get()) {
            event.setJump(true);
        }

        if (mc.player.onGround() && isOnBlockEdge(edgeDist.get().floatValue()) && safeWalk.get() && !telly.get()) {
            event.setSneak(true);
        }

    }

    @Override
    public void onEnable() {
        resetState();
        if (mc.player != null) {
            oldSlot = mc.player.getInventory().getSelectedSlot();
        }

        airTick = 0;
        blockPos = null;
        enumFacing = null;
        rotation = null;

    }

    @Override
    public void onDisable() {
        if (mc.player == null) {
            resetState();
            return;
        }

        boolean isHoldingShift = InputConstants.isKeyDown(mc.getWindow(), mc.options.keyShift.getDefaultKey().getValue());
        mc.options.keyShift.setDown(isHoldingShift);

        if (oldSlot != -1) {
            mc.player.getInventory().setSelectedSlot(oldSlot);
        }
        yLevel = 0;
        resetState();
    }

    @EventTarget public void onRespawn(dev.myth.event.impl.RespawnEvent event) { resetState(); }

    private void resetState() {
        oldSlot = -1; airTick = yLevel = 0;
        blockPos = null; enumFacing = null; rotation = null;
        forwardInput = strafeInput = inputYaw = rawInputYaw = 0;
    }

    private void handleTelly() {
        if (mc.player.onGround() && (strafeInput != 0 || forwardInput != 0)) {
            RotationManager.setRotations(new Rotation(rawInputYaw, rotation == null ? mc.player.getXRot() : rotation.getPitch()), rotateBackSpeed.get().floatValue());
            return;
        }
        rotation = getRotation(blockPos, enumFacing);
        int speed = rotateSpeed.get().intValue();
        if (rotMod.get().equals("Heypixel")) {
            switch (airTick) {
                case 1 -> speed = rotateSpeed.get().intValue();
                default -> speed = rotateSpeed2.get().intValue();
            }
        }
        RotationManager.setRotations(rotation, speed);
        if (airTick > tellyTick.get().intValue()) {
            place();
        }
    }

    private void handleNormal() {
        if (isOnBlockEdge(snapDist.get().floatValue()) || !snap.get() | !mc.player.onGround()) {
            rotation = getRotation(blockPos, enumFacing);
            RotationManager.setRotations(rotation, rotateSpeed.get().doubleValue());
        }

        place();
    }

    private int findBlockSlot() {
        for (int i = 0; i < 9; i++) {
            ItemStack stack = mc.player.getInventory().getItem(i);
            if (isValidStack(stack)) {
                return i;
            }
        }

        return -1;
    }

    private void place() {
        if (!onAir() || blockPos == null || enumFacing == null || !isValidStack(mc.player.getInventory().getSelectedItem())) {
            return;
        }

        if (rayCast.get().equals("Normal") && !RayCastUtil.overBlock(RotationManager.getRotation(), blockPos)) return;
        if (rayCast.get().equals("Strict") && !RayCastUtil.overBlock(RotationManager.getRotation(), blockPos, enumFacing))
            return;

        InteractionResult result = mc.gameMode.useItemOn(
                mc.player,
                InteractionHand.MAIN_HAND,
                new BlockHitResult(getVec3(blockPos, enumFacing), enumFacing, blockPos, false)
        );

        if (swing.get() && result.consumesAction()) {
            mc.player.swing(InteractionHand.MAIN_HAND);
        }
    }

    private int getYLevel() {
        if (((!mc.options.keyJump.isDown()
                && MoveUtils.isMoving()
                && telly.get()))
                && Math.abs(yLevel - (Mth.floor(mc.player.getY()) - 1)) <= 1.25) {
            return yLevel;
        }

        return Mth.floor(mc.player.getY()) - 1;
    }

    private void getBlockInfo() {
        lengthSqr = 4.5D * 4.5D;
        blockPos = null;
        enumFacing = null;

        Vec3 baseVec = mc.player.getEyePosition();
        BlockPos base = BlockPos.containing(baseVec.x, getYLevel(), baseVec.z);
        int baseX = base.getX();
        int baseZ = base.getZ();

        if (!onAir()) {
            return;
        }

        if (checkBlock(baseVec, base)) {
            return;
        }

        for (int d = 1; d <= 6; d++) {
            if (checkBlock(baseVec, new BlockPos(baseX, getYLevel() - d, baseZ))) {
                return;
            }

            for (int x = 0; x <= d; x++) {
                for (int z = 0; z <= d - x; z++) {
                    int y = d - x - z;
                    for (int rev1 = 0; rev1 <= 1; rev1++) {
                        for (int rev2 = 0; rev2 <= 1; rev2++) {
                            BlockPos pos = new BlockPos(
                                    baseX + (rev1 == 0 ? x : -x),
                                    getYLevel() - y,
                                    baseZ + (rev2 == 0 ? z : -z)
                            );
                            checkBlock(baseVec, pos);
                        }
                    }
                }
            }
        }
    }

    private boolean checkBlock(Vec3 baseVec, BlockPos pos) {
        if (!onAir()) {
            return false;
        }

        if (pos.getY() > getYLevel()) {
            return false;
        }

        Vec3 center = new Vec3(pos.getX() + 0.5, pos.getY(), pos.getZ() + 0.5);
        for (Direction direction : Direction.values()) {
            Vec3 normal = new Vec3(direction.getStepX(), direction.getStepY(), direction.getStepZ());
            Vec3 hit = center.add(normal.scale(0.5));
            BlockPos baseBlockPos = pos.offset(direction.getStepX(), direction.getStepY(), direction.getStepZ());

            BlockState state = mc.level.getBlockState(baseBlockPos);
            if (state.getCollisionShape(mc.level, baseBlockPos).isEmpty() || state.getMenuProvider(mc.level, baseBlockPos) != null) {
                continue;
            }

            Vec3 relevant = hit.subtract(baseVec);
            if (relevant.lengthSqr() <= lengthSqr && relevant.dot(normal) >= 0.0D) {
                if (direction.getOpposite() == Direction.UP && MoveUtils.isMoving() && !mc.options.keyJump.isDown()) {
                    continue;
                }
                lengthSqr = relevant.lengthSqr();

                blockPos = baseBlockPos;
                enumFacing = direction.getOpposite();
                return true;
            }
        }

        return false;
    }

    private Rotation getRotation(BlockPos pos, Direction direction) {
        if (rotation == null) {
            return new Rotation(Mth.wrapDegrees(mc.player.getYRot() - 135.0F), 82.0F);
        }

        if (!onAir() || pos == null || direction == null) {
            return rotation;
        }

        Rotation calculated = RotationUtils.calculate(pos, direction);
        float[] yawArray = new float[]{
                -135F,
                -90F,
                -45F,
                0F,
                45F,
                90F,
                135F,
                180F
        };
        float baseYaw = Mth.wrapDegrees(inputYaw - 180);

        for (int i = 1; i < yawArray.length; i++) {
            float key = yawArray[i];
            int j = i - 1;
            while (j >= 0 && Math.abs(Mth.wrapDegrees(baseYaw - yawArray[j])) > Math.abs(Mth.wrapDegrees(baseYaw - key))) {
                yawArray[j + 1] = yawArray[j];
                j = j - 1;
            }
            yawArray[j + 1] = key;
        }

        float[] pitchArray = {75.0F, 82.0F, 87.0F};

        float[] finalYawArray = new float[yawArray.length + 1];
        System.arraycopy(yawArray, 0, finalYawArray, 0, yawArray.length);
        finalYawArray[yawArray.length] = calculated.getYaw();

        for (float yaw : finalYawArray) {
            if (degrees45.get() && yaw % 90 == 0) {
                continue;
            }
            for (float pitch : pitchArray) {
                Rotation candidate = new Rotation(yaw + randomRange(-0.3F, 0.3F), pitch + randomRange(-0.3F, 0.3F));
                boolean matches = rayCast.get().equals("Normal")
                        ? RayCastUtil.overBlock(candidate, pos)
                        : RayCastUtil.overBlock(candidate, pos, direction);
                if (matches) {
                    return candidate;
                }
            }

            for (int pitch = -90; pitch < 90; pitch++) {
                Rotation candidate = new Rotation(yaw, pitch);
                boolean matches = rayCast.get().equals("Normal")
                        ? RayCastUtil.overBlock(candidate, pos)
                        : RayCastUtil.overBlock(candidate, pos, direction);
                if (matches) {
                    return candidate;
                }
            }
        }

        return calculated;
    }

    private boolean onAir() {
        Vec3 baseVec = mc.player.getEyePosition();
        BlockPos base = BlockPos.containing(baseVec.x, getYLevel(), baseVec.z);
        return mc.level.getBlockState(base).canBeReplaced();
    }

    private boolean isOnBlockEdge(float sensitivity) {
        if (!mc.player.onGround()) return false;
        double posX = mc.player.getX();
        double posZ = mc.player.getZ();
        double relX = posX - Math.floor(posX);
        double relZ = posZ - Math.floor(posZ);

        double distToMinX = relX;
        double distToMaxX = 1.0 - relX;
        double distToMinZ = relZ;
        double distToMaxZ = 1.0 - relZ;

        double closestX = Math.min(distToMinX, distToMaxX);
        double closestZ = Math.min(distToMinZ, distToMaxZ);

        return Math.min(closestX, closestZ) <= sensitivity;
    }

    @Override
    public String getInfo() { return telly.get() ? "Telly" : snap.get() ? "Snap" : "Normal"; }
}
