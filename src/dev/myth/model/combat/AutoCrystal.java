package dev.myth.model.combat;

import dev.myth.Myth;
import dev.myth.UI.WorldEsp;
import dev.myth.UI.setting.Setting;
import dev.myth.event.api.EventType;
import dev.myth.event.api.annotations.EventTarget;
import dev.myth.event.impl.MotionEvent;
import dev.myth.event.impl.RespawnEvent;
import dev.myth.model.Category;
import dev.myth.model.Model;
import dev.myth.utils.CrystalDamage;
import dev.myth.utils.TimerUtils;
import dev.myth.utils.rotation.RotationManager;
import dev.myth.utils.rotation.RotationUtils;
import java.time.Duration;
import java.util.ArrayList;
import java.util.List;
import net.minecraft.client.Minecraft;
import net.minecraft.core.BlockPos;
import net.minecraft.core.Direction;
import net.minecraft.world.InteractionHand;
import net.minecraft.world.entity.Entity;
import net.minecraft.world.entity.ExperienceOrb;
import net.minecraft.world.entity.item.ItemEntity;
import net.minecraft.world.entity.projectile.arrow.AbstractArrow;
import net.minecraft.world.entity.projectile.throwableitemprojectile.ThrownExperienceBottle;
import net.minecraft.world.entity.boss.enderdragon.EndCrystal;
import net.minecraft.world.entity.player.Player;
import net.minecraft.world.item.Items;
import net.minecraft.world.level.ClipContext;
import net.minecraft.world.level.Level;
import net.minecraft.world.level.block.Blocks;
import net.minecraft.world.phys.AABB;
import net.minecraft.world.phys.BlockHitResult;
import net.minecraft.world.phys.HitResult;
import net.minecraft.world.phys.Vec3;

/** Mio AutoCrystal: picks placements and hits by explosion damage against the nearest player. */
public final class AutoCrystal extends Model {
    public final Setting<Boolean> place = addSetting(Setting.bool("place", "Place", true).group());
    public final Setting<Integer> placeDelay = addSetting(Setting.integer("placeDelay", "PlaceDelay", 0, 0, 1000)
            .unit("ms").visibleWhen(() -> place.isExpanded()));
    public final Setting<Float> placeRange = addSetting(Setting.decimal("placeRange", "PlaceRange", 4.5f, 0, 6)
            .step(.1).visibleWhen(() -> place.isExpanded()));
    public final Setting<Float> placeWallRange = addSetting(Setting.decimal("placeWallRange", "PlaceWallRange", 3.5f, 0, 6)
            .step(.1).visibleWhen(() -> place.isExpanded()));
    public final Setting<Float> minPlaceDamage = addSetting(Setting.decimal("minPlaceDamage", "MinPlaceDamage", 6, 0, 20)
            .step(.5).visibleWhen(() -> place.isExpanded()));
    public final Setting<Float> maxSelfPlace = addSetting(Setting.decimal("maxSelfPlace", "MaxSelfPlace", 8, 0, 20)
            .step(.5).visibleWhen(() -> place.isExpanded()));
    public final Setting<Boolean> placeRotate = addSetting(Setting.bool("placeRotate", "PlaceRotate", true)
            .visibleWhen(() -> place.isExpanded()));

    public final Setting<Boolean> destroy = addSetting(Setting.bool("break", "Break", true).group());
    public final Setting<Integer> hitDelay = addSetting(Setting.integer("hitDelay", "HitDelay", 0, 0, 1000)
            .unit("ms").visibleWhen(() -> destroy.isExpanded()));
    public final Setting<Float> breakRange = addSetting(Setting.decimal("breakRange", "BreakRange", 4.5f, 0, 6)
            .step(.1).visibleWhen(() -> destroy.isExpanded()));
    public final Setting<Float> breakWallRange = addSetting(Setting.decimal("breakWallRange", "BreakWallRange", 3.5f, 0, 6)
            .step(.1).visibleWhen(() -> destroy.isExpanded()));
    public final Setting<Float> minBreakDamage = addSetting(Setting.decimal("minBreakDamage", "MinBreakDamage", 6, 0, 20)
            .step(.5).visibleWhen(() -> destroy.isExpanded()));
    public final Setting<Float> maxSelfHit = addSetting(Setting.decimal("maxSelfHit", "MaxSelfHit", 8, 0, 20)
            .step(.5).visibleWhen(() -> destroy.isExpanded()));
    public final Setting<Boolean> breakRotate = addSetting(Setting.bool("breakRotate", "BreakRotate", true)
            .visibleWhen(() -> destroy.isExpanded()));

    public final Setting<Float> targetRange = addSetting(Setting.decimal("targetRange", "TargetRange", 12, 1, 20).step(.5));
    public final Setting<Boolean> antiSuicide = addSetting(Setting.bool("antiSuicide", "AntiSuicide", true));
    public final Setting<Boolean> facePlace = addSetting(Setting.bool("facePlace", "FacePlace", false).group());
    public final Setting<Float> facePlaceHealth = addSetting(Setting.decimal("facePlaceHealth", "FacePlaceHealth", 8, 0, 36)
            .step(.5).visibleWhen(() -> facePlace.isExpanded()));
    public final Setting<Integer> facePlaceArmor = addSetting(Setting.integer("facePlaceArmor", "FacePlaceArmor", 0, 0, 100)
            .unit("%").visibleWhen(() -> facePlace.isExpanded()));
    public final Setting<Boolean> assumeBestArmor = addSetting(Setting.bool("assumeBestArmor", "AssumeBestArmor", false));
    public final Setting<Boolean> basePlace = addSetting(Setting.bool("basePlace", "BasePlace", false).group());
    public final Setting<Boolean> strictDirection = addSetting(Setting.bool("strictDirection", "StrictDirection", true)
            .visibleWhen(() -> basePlace.isExpanded()));
    public final Setting<Boolean> air = addSetting(Setting.bool("air", "Air", false)
            .visibleWhen(() -> basePlace.isExpanded()));
    public final Setting<String> autoSwap = addSetting(Setting.choice("autoSwap", "AutoSwap", "Normal", List.of("None", "Normal")));
    public final Setting<Boolean> swapBack = addSetting(Setting.bool("swapBack", "SwapBack", true));
    public final Setting<Float> rotationSpeed = addSetting(Setting.decimal("rotationSpeed", "RotSpeed", 180, 1, 180));
    public final Setting<Boolean> render = addSetting(Setting.bool("render", "Render", true).group());
    public final Setting<Integer> fill = addSetting(Setting.color("fill", "Fill", 0x401e90ff, true, false)
            .visibleWhen(() -> render.isExpanded()));
    public final Setting<Integer> outline = addSetting(Setting.color("outline", "Outline", 0xc81e90ff, true, false)
            .visibleWhen(() -> render.isExpanded()));
    public final Setting<Float> lineWidth = addSetting(Setting.decimal("lineWidth", "LineWidth", 1.5f, .5f, 5)
            .step(.1).visibleWhen(() -> render.isExpanded()));
    public final Setting<Boolean> fade = addSetting(Setting.bool("fade", "Fade", true)
            .visibleWhen(() -> render.isExpanded()));
    public final Setting<Boolean> multiFade = addSetting(Setting.bool("multiFade", "MultiFade", true)
            .visibleWhen(() -> render.isExpanded()));
    public final Setting<Float> fadeTime = addSetting(Setting.decimal("fadeTime", "FadeTime", 1, .1f, 5)
            .step(.1).unit("s").visibleWhen(() -> render.isExpanded()));

    /** Mio tries DOWN first so a base lands under the crystal rather than beside it. */
    private static final Direction[] BASE_ORDER = {Direction.DOWN, Direction.NORTH, Direction.EAST,
            Direction.SOUTH, Direction.WEST, Direction.UP};
    private final TimerUtils placeTimer = new TimerUtils();
    private final TimerUtils breakTimer = new TimerUtils();
    private Player target;
    private float lastDamage;

    public AutoCrystal() {
        super("autocrystal", "AutoCrystal", Category.COMBAT);
        setDescription("Places and breaks end crystals by explosion damage.");
    }

    @Override public String getInfo() {
        return target == null ? null : "%s %.1f".formatted(target.getPlainTextName(), lastDamage);
    }
    @Override protected void onEnable() { reset(); }
    @Override protected void onDisable() { reset(); }
    @EventTarget private void onRespawn(RespawnEvent event) { reset(); }

    private void reset() { target = null; lastDamage = 0; Myth.WORLD_ESP.clear(this); }

    @EventTarget private void onMotion(MotionEvent event) {
        if (event.getType() != EventType.PRE) return;
        Minecraft mc = Minecraft.getInstance();
        if (mc.player == null || mc.level == null || mc.gameMode == null
                || mc.player.isSpectator() || mc.gui.screen() != null) { reset(); return; }
        target = findTarget(mc);
        if (target == null) { lastDamage = 0; return; }
        // Mio breaks before placing, so a crystal placed this tick is not hit in the same one.
        if (destroy.get()) tryBreak(mc);
        if (place.get()) tryPlace(mc);
    }

    private Player findTarget(Minecraft mc) {
        Player best = null;
        double bestDistance = targetRange.get();
        for (Player player : mc.level.players()) {
            if (player == mc.player || !player.isAlive() || player.isSpectator()) continue;
            double distance = mc.player.distanceTo(player);
            if (distance > bestDistance) continue;
            bestDistance = distance;
            best = player;
        }
        return best;
    }

    /** Mio drops the damage floor once the target is low or its armor is nearly broken. */
    private boolean shouldFacePlace() {
        if (!facePlace.get() || target == null) return false;
        if (target.getHealth() + target.getAbsorptionAmount() <= facePlaceHealth.get()) return true;
        return facePlaceArmor.get() > 0 && CrystalDamage.armorDurability(target) <= facePlaceArmor.get();
    }

    private float damage(Vec3 center, net.minecraft.world.entity.LivingEntity entity) {
        return CrystalDamage.explosion(center, entity, entity == target && assumeBestArmor.get());
    }

    private boolean lethal(Minecraft mc, float self) {
        return antiSuicide.get() && self >= mc.player.getHealth() + mc.player.getAbsorptionAmount();
    }

    private static boolean visible(Minecraft mc, Vec3 position) {
        return mc.level.clip(new ClipContext(mc.player.getEyePosition(), position,
                ClipContext.Block.COLLIDER, ClipContext.Fluid.NONE, mc.player)).getType() == HitResult.Type.MISS;
    }

    private void tryBreak(Minecraft mc) {
        if (!breakTimer.hasElapsed(Duration.ofMillis(hitDelay.get()))) return;
        EndCrystal best = null;
        float bestDamage = 0;
        for (Entity entity : mc.level.entitiesForRendering()) {
            if (!(entity instanceof EndCrystal crystal) || !crystal.isAlive()) continue;
            Vec3 center = crystal.position();
            double distance = mc.player.getEyePosition().distanceTo(center);
            if (distance > (visible(mc, center) ? breakRange.get() : breakWallRange.get())) continue;
            float damage = damage(center, target);
            if (damage <= bestDamage) continue;
            if (damage < minBreakDamage.get() && !shouldFacePlace()) continue;
            float self = damage(center, mc.player);
            if (self > maxSelfHit.get() || lethal(mc, self)) continue;
            bestDamage = damage;
            best = crystal;
        }
        if (best == null) return;
        if (breakRotate.get()) RotationManager.setRotations(RotationUtils.calculate(best), rotationSpeed.get());
        mc.gameMode.attack(mc.player, best);
        mc.player.swing(InteractionHand.MAIN_HAND);
        lastDamage = bestDamage;
        breakTimer.reset();
    }

    private void tryPlace(Minecraft mc) {
        if (!placeTimer.hasElapsed(Duration.ofMillis(placeDelay.get()))) return;
        boolean offhand = mc.player.getOffhandItem().is(Items.END_CRYSTAL);
        int slot = offhand ? -1 : crystalSlot(mc);
        int previous = mc.player.getInventory().getSelectedSlot();
        if (!offhand && slot == -1) return;
        // Bail before the scan when swapping is off and the crystals are not already held.
        if (!offhand && slot != previous && autoSwap.get().equals("None")) return;

        BlockPos best = null;
        float bestDamage = 0;
        boolean canBase = basePlace.get() && slotOf(mc, Items.OBSIDIAN) != -1;
        int radius = (int) Math.ceil(placeRange.get());
        BlockPos origin = mc.player.blockPosition();
        int eyeY = (int) Math.floor(mc.player.getEyeY());
        var cursor = new BlockPos.MutableBlockPos();
        for (int x = -radius; x <= radius; x++) for (int y = -radius; y <= radius; y++) for (int z = -radius; z <= radius; z++) {
            cursor.set(origin.getX() + x, eyeY + y, origin.getZ() + z);
            if (!crystalSpace(mc.level, cursor)) continue;
            // Mio also scores spots it would first have to build a base on, but only
            // consider those while obsidian is actually at hand, or a plain spot would be lost.
            if (!isCrystalBase(mc.level, cursor)
                    && !(canBase && canBuild(mc.level, cursor) && baseDirection(mc, cursor) != null)) continue;
            Vec3 center = crystalCenter(cursor);
            double distance = mc.player.getEyePosition().distanceTo(center);
            if (distance > (visible(mc, center) ? placeRange.get() : placeWallRange.get())) continue;
            float damage = damage(center, target);
            if (damage <= bestDamage) continue;
            if (damage < minPlaceDamage.get() && !shouldFacePlace()) continue;
            float self = damage(center, mc.player);
            if (self > maxSelfPlace.get() || lethal(mc, self)) continue;
            bestDamage = damage;
            best = cursor.immutable();
        }
        if (best == null) return;

        lastDamage = bestDamage;
        if (!isCrystalBase(mc.level, best)) { placeBase(mc, best, previous); return; }

        boolean swapped = !offhand && slot != previous;
        if (swapped) mc.player.getInventory().setSelectedSlot(slot);
        Direction face = face(mc, best);
        Vec3 hit = Vec3.atCenterOf(best).add(Vec3.atLowerCornerOf(face.getUnitVec3i()).scale(.5));
        InteractionHand hand = offhand ? InteractionHand.OFF_HAND : InteractionHand.MAIN_HAND;
        if (placeRotate.get()) RotationManager.setRotations(RotationUtils.calculate(hit), rotationSpeed.get());
        mc.gameMode.useItemOn(mc.player, hand, new BlockHitResult(hit, face, best, false));
        mc.player.swing(hand);
        if (swapped && swapBack.get()) mc.player.getInventory().setSelectedSlot(previous);
        highlight(best);
        placeTimer.reset();
    }

    /** Mio do1134: lay the obsidian the crystal will stand on, then let the next pass place it. */
    private void placeBase(Minecraft mc, BlockPos pos, int previous) {
        int slot = slotOf(mc, Items.OBSIDIAN);
        if (slot == -1) return;
        Direction direction = baseDirection(mc, pos);
        if (direction == null && !air.get()) return;
        if (slot != previous) {
            if (autoSwap.get().equals("None")) return;
            mc.player.getInventory().setSelectedSlot(slot);
        }
        BlockPos clicked;
        Direction face;
        if (direction == null) {
            // Mio's airplace clicks the empty spot itself through its first visible face.
            var faces = visibleFaces(mc, pos);
            clicked = pos;
            face = faces.isEmpty() ? Direction.DOWN : faces.getFirst();
        } else {
            clicked = pos.relative(direction);
            face = direction.getOpposite();
        }
        Vec3 hit = Vec3.atCenterOf(pos).add(Vec3.atLowerCornerOf(direction == null
                ? face.getUnitVec3i() : direction.getUnitVec3i()).scale(.5));
        if (placeRotate.get()) RotationManager.setRotations(RotationUtils.calculate(hit), rotationSpeed.get());
        mc.gameMode.useItemOn(mc.player, InteractionHand.MAIN_HAND, new BlockHitResult(hit, face, clicked, false));
        mc.player.swing(InteractionHand.MAIN_HAND);
        if (slot != previous && swapBack.get()) mc.player.getInventory().setSelectedSlot(previous);
        highlight(pos);
        placeTimer.reset();
    }

    private void highlight(BlockPos pos) {
        if (!render.get()) return;
        Myth.WORLD_ESP.submit(this, new WorldEsp.Style(fill.color(theme()), outline.color(theme()),
                lineWidth.get(), fadeTime.get(), fade.get(), multiFade.get()), new AABB(pos));
    }

    private static int theme() { return Myth.UI_MANAGER.getThemeColor(); }

    private static boolean isCrystalBase(Level level, BlockPos pos) {
        var state = level.getBlockState(pos);
        return state.is(Blocks.OBSIDIAN) || state.is(Blocks.BEDROCK);
    }

    /** EndCrystalItem: the block above must be empty with nothing standing in its 1x2 space. */
    private static boolean crystalSpace(Level level, BlockPos pos) {
        BlockPos above = pos.above();
        if (!level.isEmptyBlock(above)) return false;
        return level.getEntities((Entity) null, new AABB(above.getX(), above.getY(), above.getZ(),
                above.getX() + 1d, above.getY() + 2d, above.getZ() + 1d)).isEmpty();
    }

    /** Mio is3042: replaceable, inside the border, and nothing but loose drops standing in it. */
    private static boolean canBuild(Level level, BlockPos pos) {
        if (!level.getBlockState(pos).canBeReplaced() || !level.getWorldBorder().isWithinBounds(pos)) return false;
        for (Entity entity : level.getEntities((Entity) null, new AABB(pos))) {
            if (entity instanceof ItemEntity || entity instanceof ExperienceOrb
                    || entity instanceof ThrownExperienceBottle || entity instanceof AbstractArrow) continue;
            return false;
        }
        return true;
    }

    /** Mio getDirection3030: the first neighbour solid enough to click the base against. */
    private Direction baseDirection(Minecraft mc, BlockPos pos) {
        for (Direction direction : BASE_ORDER) {
            BlockPos neighbour = pos.relative(direction);
            if (neighbour.getY() > mc.level.getMaxY()) continue;
            if (mc.level.getBlockState(neighbour).canBeReplaced()) continue;
            if (strictDirection.get() && !visibleFaces(mc, neighbour).contains(direction.getOpposite())) continue;
            return direction;
        }
        return null;
    }

    /** Mio getList3031: faces that are not buried and that actually face the eye. */
    private static List<Direction> visibleFaces(Minecraft mc, BlockPos pos) {
        List<Direction> faces = new ArrayList<>(List.of(Direction.values()));
        for (Direction direction : Direction.values()) {
            var state = mc.level.getBlockState(pos.relative(direction));
            if (!state.canBeReplaced() && state.isSolidRender()) faces.remove(direction);
        }
        if (pos.getY() + 1 > mc.level.getMaxY()) faces.remove(Direction.UP);
        Vec3 eye = mc.player.getEyePosition();
        Vec3 center = Vec3.atCenterOf(pos);
        double dx = eye.x - center.x, dz = eye.z - center.z;
        if (dx > .5) faces.remove(Direction.WEST);
        else if (dx < -.5) faces.remove(Direction.EAST);
        else { faces.remove(Direction.WEST); faces.remove(Direction.EAST); }
        if (dz > .5) faces.remove(Direction.NORTH);
        else if (dz < -.5) faces.remove(Direction.SOUTH);
        else { faces.remove(Direction.NORTH); faces.remove(Direction.SOUTH); }
        if (eye.y < pos.getY()) faces.remove(Direction.UP);
        if (eye.y > pos.getY()) faces.remove(Direction.DOWN);
        return faces;
    }

    /** The crystal spawns centred on the block above, which is the explosion centre. */
    private static Vec3 crystalCenter(BlockPos pos) {
        return new Vec3(pos.getX() + .5, pos.getY() + 1d, pos.getZ() + .5);
    }

    private static Direction face(Minecraft mc, BlockPos pos) {
        Direction best = Direction.UP;
        double bestDistance = Double.MAX_VALUE;
        Vec3 eye = mc.player.getEyePosition();
        for (Direction direction : Direction.values()) {
            if (!mc.level.getBlockState(pos.relative(direction)).isAir()) continue;
            double distance = Vec3.atCenterOf(pos).add(Vec3.atLowerCornerOf(direction.getUnitVec3i()).scale(.5)).distanceTo(eye);
            if (distance >= bestDistance) continue;
            bestDistance = distance;
            best = direction;
        }
        return best;
    }

    private static int crystalSlot(Minecraft mc) { return slotOf(mc, Items.END_CRYSTAL); }

    private static int slotOf(Minecraft mc, net.minecraft.world.item.Item item) {
        for (int slot = 0; slot < 9; slot++)
            if (mc.player.getInventory().getItem(slot).is(item)) return slot;
        return -1;
    }
}
