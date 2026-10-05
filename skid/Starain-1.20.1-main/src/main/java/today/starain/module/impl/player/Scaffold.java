package today.starain.module.impl.player;

import com.mojang.blaze3d.platform.InputConstants;
import com.mojang.blaze3d.systems.RenderSystem;
import com.mojang.blaze3d.vertex.*;
import net.minecraft.client.renderer.GameRenderer;
import net.minecraft.core.BlockPos;
import net.minecraft.core.Direction;
import net.minecraft.network.protocol.game.ServerboundMovePlayerPacket;
import net.minecraft.network.protocol.game.ServerboundSwingPacket;
import net.minecraft.util.Mth;
import net.minecraft.world.InteractionHand;
import net.minecraft.world.InteractionResult;
import net.minecraft.world.item.BlockItem;
import net.minecraft.world.item.ItemStack;
import net.minecraft.world.item.StandingAndWallBlockItem;
import net.minecraft.world.level.block.*;
import net.minecraft.world.level.block.state.BlockState;
import net.minecraft.world.phys.AABB;
import net.minecraft.world.phys.BlockHitResult;
import net.minecraft.world.phys.Vec3;
import org.joml.Matrix4f;
import today.starain.Starain;
import today.starain.event.api.EventType;
import today.starain.event.api.Priority;
import today.starain.event.api.annotations.EventPriority;
import today.starain.event.api.annotations.EventTarget;
import today.starain.event.impl.GameTickEvent;
import today.starain.event.impl.MoveInputEvent;
import today.starain.event.impl.PlayerTickEvent;
import today.starain.event.impl.RenderWorldEvent;
import today.starain.module.Category;
import today.starain.module.ClientModule;
import today.starain.ui.Theme;
import today.starain.utils.client.ChatUtils;
import today.starain.utils.math.MathUtils;
import today.starain.utils.player.FallingPlayer;
import today.starain.utils.player.MoveUtils;
import today.starain.utils.rotation.RayCastUtil;
import today.starain.utils.rotation.Rotation;
import today.starain.utils.rotation.RotationManager;
import today.starain.utils.rotation.RotationUtils;
import today.starain.value.impl.BooleanValue;
import today.starain.value.impl.ColorValue;
import today.starain.value.impl.ModeValue;
import today.starain.value.impl.NumberValue;

import java.awt.*;
import java.util.ArrayList;
import java.util.List;

public class Scaffold extends ClientModule {
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
    private final BooleanValue telly = new BooleanValue("Telly", false, this);
    private final BooleanValue esp = new BooleanValue("ESP", true, this);
    private final BooleanValue espSync = new BooleanValue("ESP Sync Color", false, this, esp::get);
    private final ColorValue espColor = new ColorValue("ESP Color", new Color(150, 0, 0, 180), this, () -> esp.get() && !espSync.get());
    private final NumberValue indexMultiplier = new NumberValue("ESP Sync Color Index Multiplier", 0.0, 5.0, 2.0, 0.01, this, () -> esp.get() && espSync.get());
    private final BooleanValue espFill = new BooleanValue("ESP Fill", true, this, esp::get);
    private final BooleanValue espOutline = new BooleanValue("ESP Outline", true, this, esp::get);
    private final NumberValue espCount = new NumberValue("ESP Count", 1, 10, 5, 1, this, esp::get);
    private static final int ESP_LIFE = 14;
    private final List<EspBox> espTrail = new ArrayList<>();
    private final BooleanValue snap = new BooleanValue("Snap", false, this, () -> !telly.get());
    private final NumberValue snapDist = new NumberValue("Snap Dist", 0, 1, 0.1, 0.01, this, snap::get);
    private final BooleanValue degrees45 = new BooleanValue("45 Degrees", false, this);
    private final BooleanValue swing = new BooleanValue("Swing", true, this);
    private final ModeValue rotMod = new ModeValue("RotationMode", "Rise", new String[]{"Rise", "Heypixel"}, this);
    private final ModeValue rayCast = new ModeValue("RayCast", "Normal", new String[]{"Normal", "Strict"}, this);
    private final NumberValue rotateSpeed = new NumberValue("RotSpeed", 1.0, 180.0, 127.0, 1.0, this);
    private final NumberValue rotateSpeed2 = new NumberValue("RotSpeed2", 1.0, 180.0, 36.0, 1.0, this, () -> rotMod.isMode("Heypixel"));
    private final NumberValue rotateBackSpeed = new NumberValue("BackSpeed", 1.0, 180.0, 180.0, 1.0, this, telly::get);
    private final NumberValue tellyTick = new NumberValue("TellyTicks", 0.0, 6.0, 1.0, 1.0, this, telly::get);
    private final BooleanValue safeWalk = new BooleanValue("SafeWalk", false, this, () -> !telly.get());
    private final NumberValue edgeDist = new NumberValue("Edge Dist", 0, 1, 0.1, 0.01, this, () -> safeWalk.get() && !telly.get());
    private int airTick;
    private int yLevel;
    private BlockPos blockPos;
    private Direction enumFacing;
    private int oldSlot = -1;
    private Rotation rotation;
    private int rotateCount = 0;
    private float forwardInput, strafeInput;
    private float inputYaw;
    private float rawInputYaw;
    private double lengthSqr = 4.5D * 4.5D;

    public Scaffold() {
        setName("Scaffold");
        setCategory(Category.PLAYER);
    }

    public static Vec3 getVec3(BlockPos pos, Direction face) {
        double x = pos.getX() + 0.5;
        double y = pos.getY() + 0.5;
        double z = pos.getZ() + 0.5;

        if (face != Direction.UP && face != Direction.DOWN) {
            y += 0.08;
        } else {
            x += MathUtils.getRandomInRange(-0.3, 0.3);
            z += MathUtils.getRandomInRange(-0.3, 0.3);
        }

        if (face == Direction.WEST || face == Direction.EAST) {
            z += MathUtils.getRandomInRange(-0.3, 0.3);
        }

        if (face == Direction.SOUTH || face == Direction.NORTH) {
            x += MathUtils.getRandomInRange(-0.3, 0.3);
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
        if (block instanceof FlowerBlock || block instanceof BushBlock || block instanceof FungusBlock || block instanceof CropBlock) {
            return false;
        }

        return !(block instanceof SlabBlock) && !BLACKLISTED_BLOCKS.contains(block);
    }

    @EventTarget
    private void onPreTick(PlayerTickEvent event) {
        int slotId = findBlockSlot();
        if (slotId != -1 && mc.player.getInventory().selected != slotId) {
            mc.player.getInventory().selected = slotId;
        }

        if (mc.player.onGround()) {
            airTick = 0;
            yLevel = Mth.floor(mc.player.getY()) - 1;
        } else {
            airTick++;
        }

        getBlockInfo();
        recordEspTrail();
//
        boolean reachable = true;
        if (mc.player.getDeltaMovement().y < -0.1 && blockPos != null) {
            FallingPlayer fallingPlayer = new FallingPlayer(mc.player);
            fallingPlayer.calculate(2);
            if (blockPos.getY() > fallingPlayer.getY()) {
                reachable = false;
            }
        }
        double strength = new Vec3(mc.player.getDeltaMovement().x, 0, mc.player.getDeltaMovement().z).length();
        if (strength >= 1.5D) {
            ChatUtils.addChatMessage("你也是要飞了: ", String.valueOf(strength));
        }
        if ((!reachable || strength >= 1.5D) && rotateCount <= 8 && getBlockCount() >= 1 && isValidStack(mc.player.getInventory().getSelected())) {
//                ChatUtil.info("working");
            Starain.skipTicks++;
            rotateCount++;
            if (blockPos == null) return;
            Rotation rotation = getRotation(blockPos, enumFacing);
            RotationManager.rotations = rotation;
            RotationManager.setActive(true);
            mc.getConnection().send(new ServerboundMovePlayerPacket.Rot(rotation.getYaw(), rotation.getPitch(), mc.player.onGround()));
            InteractionResult result = mc.gameMode.useItemOn(
                    mc.player,
                    InteractionHand.MAIN_HAND,
                    new BlockHitResult(getVec3(blockPos, enumFacing), enumFacing, blockPos, false)
            );
            if (swing.get() && result.consumesAction()) {
                mc.getConnection().send(new ServerboundSwingPacket(InteractionHand.MAIN_HAND));
            }
            onPreTick(event);
            return;
        } else {
            rotateCount = 0;
        }
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

//    @EventTarget
//    public void onMotion(MotionEvent event) {
//        if (mc.player == null || event.getType() != EventType.PRE || telly.get() || !safeWalk.get()) {
//            return;
//        }
//
//        mc.options.keyShift.setDown(mc.player.onGround() && isOnBlockEdge(edgeDist.get().floatValue()));
//    }

    @Override
    public void onEnable() {
        if (mc.player != null) {
            oldSlot = mc.player.getInventory().selected;
        }

        airTick = 0;
        blockPos = null;
        enumFacing = null;
        rotation = null;

        int blockCount = getBlockCount();
        rotateCount = 0;
        espTrail.clear();
    }

    @Override
    public void onDisable() {
        if (mc.player == null) {
            return;
        }

        boolean isHoldingShift = InputConstants.isKeyDown(mc.getWindow().getWindow(), mc.options.keyShift.getDefaultKey().getValue());
        mc.options.keyShift.setDown(isHoldingShift);

        if (oldSlot != -1) {
            mc.player.getInventory().selected = oldSlot;
        }
        yLevel = 0;
        espTrail.clear();
    }

    private void handleTelly() {
        if (mc.player.onGround() && (strafeInput != 0 || forwardInput != 0)) {
            RotationManager.setRotations(new Rotation(rawInputYaw, rotation == null ? mc.player.getXRot() : rotation.getPitch()), rotateBackSpeed.get().floatValue());
            return;
        }
        rotation = getRotation(blockPos, enumFacing);
        int speed = rotateSpeed.get().intValue();
        if (rotMod.isMode("Heypixel")) {
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

    private int getBlockCount() {
        if (mc.player == null) {
            return 0;
        }

        int total = 0;
        for (int i = 0; i < 9; i++) {
            ItemStack stack = mc.player.getInventory().getItem(i);
            if (isValidStack(stack)) {
                total += stack.getCount();
            }
        }
        return total;
    }

    private void place() {
        if (!onAir() || blockPos == null || enumFacing == null || !isValidStack(mc.player.getInventory().getSelected())) {
            return;
        }

        if (rayCast.isMode("Normal") && !RayCastUtil.overBlock(RotationManager.getRotation(), blockPos)) return;
        if (rayCast.isMode("Strict") && !RayCastUtil.overBlock(RotationManager.getRotation(), blockPos, enumFacing))
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

    private static final class EspBox {
        final BlockPos pos;
        int life;

        EspBox(BlockPos pos, int life) {
            this.pos = pos;
            this.life = life;
        }
    }

    @EventTarget
    public void onEspAge(GameTickEvent event) {
        if (event.getType() != EventType.PRE) {
            return;
        }
        espTrail.removeIf(box -> --box.life <= 0);
    }

    private void recordEspTrail() {
        if (blockPos == null || enumFacing == null) {
            return;
        }
        BlockPos placed = blockPos.relative(enumFacing).immutable();
        int idx = -1;
        for (int i = 0; i < espTrail.size(); i++) {
            if (espTrail.get(i).pos.equals(placed)) {
                idx = i;
                break;
            }
        }
        EspBox box = (idx >= 0) ? espTrail.remove(idx) : new EspBox(placed, ESP_LIFE);
        box.life = ESP_LIFE;
        espTrail.add(0, box);
        int max = Math.max(1, espCount.get().intValue());
        while (espTrail.size() > max) {
            espTrail.remove(espTrail.size() - 1);
        }
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
                Rotation candidate = new Rotation(yaw + MathUtils.getRandomInRange(-0.3F, 0.3F), pitch + MathUtils.getRandomInRange(-0.3F, 0.3F));
                boolean matches = rayCast.isMode("Normal")
                        ? RayCastUtil.overBlock(candidate, pos)
                        : RayCastUtil.overBlock(candidate, pos, direction);
                if (matches) {
                    return candidate;
                }
            }

            for (int pitch = -90; pitch < 90; pitch++) {
                Rotation candidate = new Rotation(yaw, pitch);
                boolean matches = rayCast.isMode("Normal")
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

    @EventTarget
    public void onScaffoldEsp(RenderWorldEvent event) {
        if (!esp.get() || mc.player == null || mc.level == null) {
            return;
        }
        if (!espFill.get() && !espOutline.get()) {
            return;
        }
        List<EspBox> blocks = new ArrayList<>(espTrail);
        int max = Math.max(1, espCount.get().intValue());
        if (blocks.size() > max) {
            blocks = blocks.subList(0, max);
        }
        if (blocks.isEmpty()) {
            if (blockPos == null || enumFacing == null) return;
            blocks.add(new EspBox(blockPos.relative(enumFacing).immutable(), ESP_LIFE));
        }

        Color base = espColor.get();

        PoseStack stack = event.getPoseStack();
        stack.pushPose();
        Vec3 cam = mc.getEntityRenderDispatcher().camera.getPosition();
        stack.translate(-cam.x, -cam.y, -cam.z);

        RenderSystem.enableBlend();
        RenderSystem.defaultBlendFunc();
        RenderSystem.disableDepthTest();
        RenderSystem.depthMask(false);
        RenderSystem.setShader(GameRenderer::getPositionShader);
        RenderSystem.lineWidth(1.5F);

        int count = blocks.size();
        for (int i = 0; i < count; i++) {
            EspBox e = blocks.get(i);
            float lifeFrac = Math.max(0.0F, Math.min(1.0F, e.life / (float) ESP_LIFE));
            Color c = espSync.get() ? Theme.accent((int) (-i * indexMultiplier.get().floatValue())) : base;
            float r = c.getRed() / 255.0F;
            float g = c.getGreen() / 255.0F;
            float b = c.getBlue() / 255.0F;
            float baseA = espSync.get() ? 0.7F : c.getAlpha() / 255.0F;
            float a = baseA * lifeFrac;
            AABB box = new AABB(e.pos);
            if (espFill.get()) {
                RenderSystem.setShaderColor(r, g, b, a);
                drawSolidBox(box, stack);
            }
            if (espOutline.get()) {
                RenderSystem.setShaderColor(r, g, b, Math.min(1.0F, a * 1.6F));
                drawOutlinedBox(box, stack);
            }
        }

        RenderSystem.setShaderColor(1.0F, 1.0F, 1.0F, 1.0F);
        RenderSystem.depthMask(true);
        RenderSystem.enableDepthTest();
        RenderSystem.disableBlend();
        stack.popPose();
    }

    private static void drawSolidBox(AABB bb, PoseStack matrixStack) {
        Tesselator tessellator = RenderSystem.renderThreadTesselator();
        BufferBuilder bufferBuilder = tessellator.getBuilder();
        Matrix4f matrix = matrixStack.last().pose();
        bufferBuilder.begin(VertexFormat.Mode.QUADS, DefaultVertexFormat.POSITION);
        bufferBuilder.vertex(matrix, (float) bb.minX, (float) bb.minY, (float) bb.minZ).endVertex();
        bufferBuilder.vertex(matrix, (float) bb.maxX, (float) bb.minY, (float) bb.minZ).endVertex();
        bufferBuilder.vertex(matrix, (float) bb.maxX, (float) bb.minY, (float) bb.maxZ).endVertex();
        bufferBuilder.vertex(matrix, (float) bb.minX, (float) bb.minY, (float) bb.maxZ).endVertex();
        bufferBuilder.vertex(matrix, (float) bb.minX, (float) bb.maxY, (float) bb.minZ).endVertex();
        bufferBuilder.vertex(matrix, (float) bb.minX, (float) bb.maxY, (float) bb.maxZ).endVertex();
        bufferBuilder.vertex(matrix, (float) bb.maxX, (float) bb.maxY, (float) bb.maxZ).endVertex();
        bufferBuilder.vertex(matrix, (float) bb.maxX, (float) bb.maxY, (float) bb.minZ).endVertex();
        bufferBuilder.vertex(matrix, (float) bb.minX, (float) bb.minY, (float) bb.minZ).endVertex();
        bufferBuilder.vertex(matrix, (float) bb.minX, (float) bb.maxY, (float) bb.minZ).endVertex();
        bufferBuilder.vertex(matrix, (float) bb.maxX, (float) bb.maxY, (float) bb.minZ).endVertex();
        bufferBuilder.vertex(matrix, (float) bb.maxX, (float) bb.minY, (float) bb.minZ).endVertex();
        bufferBuilder.vertex(matrix, (float) bb.maxX, (float) bb.minY, (float) bb.minZ).endVertex();
        bufferBuilder.vertex(matrix, (float) bb.maxX, (float) bb.maxY, (float) bb.minZ).endVertex();
        bufferBuilder.vertex(matrix, (float) bb.maxX, (float) bb.maxY, (float) bb.maxZ).endVertex();
        bufferBuilder.vertex(matrix, (float) bb.maxX, (float) bb.minY, (float) bb.maxZ).endVertex();
        bufferBuilder.vertex(matrix, (float) bb.minX, (float) bb.minY, (float) bb.maxZ).endVertex();
        bufferBuilder.vertex(matrix, (float) bb.maxX, (float) bb.minY, (float) bb.maxZ).endVertex();
        bufferBuilder.vertex(matrix, (float) bb.maxX, (float) bb.maxY, (float) bb.maxZ).endVertex();
        bufferBuilder.vertex(matrix, (float) bb.minX, (float) bb.maxY, (float) bb.maxZ).endVertex();
        bufferBuilder.vertex(matrix, (float) bb.minX, (float) bb.minY, (float) bb.minZ).endVertex();
        bufferBuilder.vertex(matrix, (float) bb.minX, (float) bb.minY, (float) bb.maxZ).endVertex();
        bufferBuilder.vertex(matrix, (float) bb.minX, (float) bb.maxY, (float) bb.maxZ).endVertex();
        bufferBuilder.vertex(matrix, (float) bb.minX, (float) bb.maxY, (float) bb.minZ).endVertex();
        BufferUploader.drawWithShader(bufferBuilder.end());
    }

    private static void drawOutlinedBox(AABB bb, PoseStack matrixStack) {
        Matrix4f matrix = matrixStack.last().pose();
        BufferBuilder bufferBuilder = Tesselator.getInstance().getBuilder();
        RenderSystem.setShader(GameRenderer::getPositionShader);
        bufferBuilder.begin(VertexFormat.Mode.DEBUG_LINES, DefaultVertexFormat.POSITION);
        bufferBuilder.vertex(matrix, (float) bb.minX, (float) bb.minY, (float) bb.minZ).endVertex();
        bufferBuilder.vertex(matrix, (float) bb.maxX, (float) bb.minY, (float) bb.minZ).endVertex();
        bufferBuilder.vertex(matrix, (float) bb.maxX, (float) bb.minY, (float) bb.minZ).endVertex();
        bufferBuilder.vertex(matrix, (float) bb.maxX, (float) bb.minY, (float) bb.maxZ).endVertex();
        bufferBuilder.vertex(matrix, (float) bb.maxX, (float) bb.minY, (float) bb.maxZ).endVertex();
        bufferBuilder.vertex(matrix, (float) bb.minX, (float) bb.minY, (float) bb.maxZ).endVertex();
        bufferBuilder.vertex(matrix, (float) bb.minX, (float) bb.minY, (float) bb.maxZ).endVertex();
        bufferBuilder.vertex(matrix, (float) bb.minX, (float) bb.minY, (float) bb.minZ).endVertex();
        bufferBuilder.vertex(matrix, (float) bb.minX, (float) bb.minY, (float) bb.minZ).endVertex();
        bufferBuilder.vertex(matrix, (float) bb.minX, (float) bb.maxY, (float) bb.minZ).endVertex();
        bufferBuilder.vertex(matrix, (float) bb.maxX, (float) bb.minY, (float) bb.minZ).endVertex();
        bufferBuilder.vertex(matrix, (float) bb.maxX, (float) bb.maxY, (float) bb.minZ).endVertex();
        bufferBuilder.vertex(matrix, (float) bb.maxX, (float) bb.minY, (float) bb.maxZ).endVertex();
        bufferBuilder.vertex(matrix, (float) bb.maxX, (float) bb.maxY, (float) bb.maxZ).endVertex();
        bufferBuilder.vertex(matrix, (float) bb.minX, (float) bb.minY, (float) bb.maxZ).endVertex();
        bufferBuilder.vertex(matrix, (float) bb.minX, (float) bb.maxY, (float) bb.maxZ).endVertex();
        bufferBuilder.vertex(matrix, (float) bb.minX, (float) bb.maxY, (float) bb.minZ).endVertex();
        bufferBuilder.vertex(matrix, (float) bb.maxX, (float) bb.maxY, (float) bb.minZ).endVertex();
        bufferBuilder.vertex(matrix, (float) bb.maxX, (float) bb.maxY, (float) bb.minZ).endVertex();
        bufferBuilder.vertex(matrix, (float) bb.maxX, (float) bb.maxY, (float) bb.maxZ).endVertex();
        bufferBuilder.vertex(matrix, (float) bb.maxX, (float) bb.maxY, (float) bb.maxZ).endVertex();
        bufferBuilder.vertex(matrix, (float) bb.minX, (float) bb.maxY, (float) bb.maxZ).endVertex();
        bufferBuilder.vertex(matrix, (float) bb.minX, (float) bb.maxY, (float) bb.maxZ).endVertex();
        bufferBuilder.vertex(matrix, (float) bb.minX, (float) bb.maxY, (float) bb.minZ).endVertex();
        BufferUploader.drawWithShader(bufferBuilder.end());
    }

    @Override
    public String getSuffix() {
        return telly.get() ? "Telly" : snap.get() ? "Snap" : "Normal";
    }
}
