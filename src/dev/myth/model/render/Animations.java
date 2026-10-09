package dev.myth.model.render;

import com.mojang.blaze3d.vertex.PoseStack;
import com.mojang.math.Axis;
import dev.myth.UI.setting.Setting;
import dev.myth.model.Category;
import dev.myth.model.Model;
import dev.myth.utils.wrapper.Wrapper;
import java.util.List;
import net.minecraft.client.player.LocalPlayer;
import net.minecraft.tags.ItemTags;
import net.minecraft.util.Mth;
import net.minecraft.world.InteractionHand;
import net.minecraft.world.entity.HumanoidArm;
import net.minecraft.world.entity.LivingEntity;
import net.minecraft.world.item.ItemStack;
import net.minecraft.world.item.ItemUseAnimation;
import org.joml.Quaternionf;

/**
 * Starain 1.21.11 Animations (`module/impl/render/Animations.java`): the custom first person sword
 * block poses. Purely visual, it sends nothing. Right clicking a sword already makes vanilla send
 * the use packet and ViaVersion turns that into the 1.8 block, so the server side needs no help
 * from us - only the pose is missing, because a 26.2 sword has no block use animation of its own.
 */
public final class Animations extends Model implements Wrapper {
    private static Animations instance;
    private static final float DEG = (float) Math.PI / 180;
    private static final List<String> MODES = List.of(
            "Vanilla", "Smooth", "1.7", "Push", "Swing", "Swang", "Swong", "Swaing", "Punch", "Sile", "None");

    public final Setting<String> blockMode = addSetting(Setting.choice("blockMode", "Block Mode", "1.7", MODES));
    public final Setting<Float> positionX = addSetting(Setting.decimal("positionX", "Position X", 0, -1, 1).step(.01));
    public final Setting<Float> positionY = addSetting(Setting.decimal("positionY", "Position Y", 0, -1, 1).step(.01));
    public final Setting<Float> positionZ = addSetting(Setting.decimal("positionZ", "Position Z", 0, -1, 1).step(.01));
    public final Setting<Float> swingSpeed = addSetting(Setting.decimal("swingSpeed", "Swing Speed", 1, .1f, 2).step(.01));
    public final Setting<Boolean> customEquippedProgress = addSetting(
            Setting.bool("customEquippedProgress", "Custom Equipped Progress", false).group());
    public final Setting<Float> equippedProgress = addSetting(Setting.decimal("equippedProgress", "Equipped Progress", 1, 0, 1)
            .step(.01).visibleWhen(() -> customEquippedProgress.get() && customEquippedProgress.isExpanded()));
    public final Setting<Boolean> hideOffhand = addSetting(Setting.bool("hideOffhand", "Hide Offhand", false));
    public final Setting<Boolean> othersBlocking = addSetting(Setting.bool("othersBlocking", "Others Blocking", true)
            .description("Shows the third person block pose for other players on a 1.8 server."));

    public Animations() {
        super("animations", "Animations", Category.RENDER);
        setDescription("Custom first-person sword block animations.");
        instance = this;
    }

    public static Animations get() { return instance; }

    /**
     * Starain drives this by reflecting on LivingEntity.swingDuration, a field 26.2 does not have -
     * the duration is computed in getCurrentSwingDuration from the item's swing animation, so their
     * own guard makes the setting a no-op there. MixinLivingEntity scales that return instead.
     */
    public static float swingSpeedClamped() {
        return instance != null && instance.isEnabled() ? instance.swingSpeed.get() : 1;
    }

    /** MixinAvatarRenderer: vanilla cannot tell a 26.2 sword is blocking, see that class. */
    public static boolean showsOthersBlocking() {
        return instance != null && instance.isEnabled() && instance.othersBlocking.get();
    }

    public boolean shouldHideOffhand() { return hideOffhand.get() && shouldShowBlockAnimation(); }

    public boolean shouldOverride(InteractionHand hand, ItemStack stack) {
        if (!isEnabled() || blockMode.get().equals("None") || hand != InteractionHand.MAIN_HAND) return false;
        if (!stack.is(ItemTags.SWORDS)) return false;
        return shouldShowBlockAnimation();
    }

    /** A real block-animation item in hand: take the pose over so vanilla's does not fight ours. */
    public boolean shouldSuppressVanillaBlockAnimation(LivingEntity player, InteractionHand hand, ItemStack stack) {
        if (!isEnabled() || player == null || hand != InteractionHand.MAIN_HAND) return false;
        if (!stack.is(ItemTags.SWORDS)) return false;
        return player.isUsingItem() && player.getUsedItemHand() == hand
                && stack.getUseAnimation() == ItemUseAnimation.BLOCK;
    }

    private boolean shouldShowBlockAnimation() {
        LocalPlayer player = mc.player;
        if (!isEnabled() || blockMode.get().equals("None") || player == null) return false;
        if (!player.getMainHandItem().is(ItemTags.SWORDS)) return false;
        boolean offhandBusy = player.isUsingItem()
                && player.getUseItem().getUseAnimation() != ItemUseAnimation.BLOCK;
        // The autoblock wins over a busy offhand; otherwise the use key is the trigger.
        if (dev.myth.model.combat.KillAura.blockingNow()) return true;
        if (offhandBusy) return false;
        return mc.options.keyUse.isDown();
    }

    public void applyCustomBlockAnimation(PoseStack poseStack, HumanoidArm arm, float equippedProg, float swingProgress) {
        int side = arm == HumanoidArm.RIGHT ? 1 : -1;
        float equip = customEquippedProgress.get() ? equippedProg * equippedProgress.get() : equippedProg;
        poseStack.translate(positionX.get(), positionY.get(), positionZ.get());
        switch (blockMode.get()) {
            case "1.7" -> apply17(poseStack, equip, swingProgress, side);
            case "Push" -> applyPush(poseStack, equip, swingProgress, side);
            case "Swing" -> applySwing(poseStack, equip, swingProgress, side);
            case "Swang" -> applySwang(poseStack, equip, swingProgress, side);
            case "Swong" -> applySwong(poseStack, equip, swingProgress, side);
            case "Swaing" -> applySwaing(poseStack, equip, swingProgress, side);
            case "Punch" -> applyPunch(poseStack, equip, swingProgress, side);
            case "Sile" -> applySile(poseStack, equip, swingProgress, side);
            case "Smooth" -> applySmooth(poseStack, equip, swingProgress, side);
            default -> applyDefault(poseStack, equip, side);
        }
    }

    private static float swingSquare(float progress) { return Mth.sin(progress * progress * (float) Math.PI); }
    private static float swingRoot(float progress) { return Mth.sin(Mth.sqrt(progress) * (float) Math.PI); }

    /** The opening translate and the -102.25 / 13.365 / 78.05 block rotation every mode but 1.7 shares. */
    private static void base(PoseStack poseStack, float equip, int side, float z, boolean halfEquip) {
        poseStack.translate(side * .56f, -.52f + (halfEquip ? equip / 2 : equip) * -.6f, z);
        poseStack.translate(side * -.1414214f, .08f, .1414214f);
        poseStack.mulPose(Axis.XP.rotation(-102.25f * DEG));
        poseStack.mulPose(Axis.YP.rotation(side * 13.365f * DEG));
        poseStack.mulPose(Axis.ZP.rotation(side * 78.05f * DEG));
    }

    private void apply17(PoseStack poseStack, float equip, float progress, int side) {
        poseStack.translate(side * .56f, -.52f + equip * -.6f, -.72f);
        float square = swingSquare(progress), root = swingRoot(progress);
        poseStack.mulPose(Axis.YP.rotation(side * (45 + square * -20) * DEG));
        poseStack.mulPose(Axis.ZP.rotation(side * root * -10 * DEG));
        poseStack.mulPose(Axis.XP.rotation(root * -80 * DEG));
        poseStack.mulPose(Axis.YP.rotation(side * -45 * DEG));
        poseStack.scale(.9f, .9f, .9f);
        poseStack.translate(-.2f, .126f, .2f);
        poseStack.mulPose(Axis.XP.rotation(-102.25f * DEG));
        poseStack.mulPose(Axis.YP.rotation(side * 15 * DEG));
        poseStack.mulPose(Axis.ZP.rotation(side * 80 * DEG));
    }

    private void applyPush(PoseStack poseStack, float equip, float progress, int side) {
        base(poseStack, equip, side, -.82f, false);
        float square = swingSquare(progress), root = swingRoot(progress);
        poseStack.mulPose(Axis.XP.rotation(square * -10 * DEG));
        poseStack.mulPose(Axis.YP.rotation(square * -10 * DEG));
        poseStack.mulPose(Axis.ZP.rotation(square * -10 * DEG));
        poseStack.mulPose(Axis.XP.rotation(root * -10 * DEG));
        poseStack.mulPose(Axis.YP.rotation(root * -10 * DEG));
        poseStack.mulPose(Axis.ZP.rotation(root * -10 * DEG));
    }

    private void applySwing(PoseStack poseStack, float equip, float progress, int side) {
        base(poseStack, equip, side, -.72f, true);
        poseStack.mulPose(Axis.YP.rotation(swingSquare(progress) * -20 * DEG));
        poseStack.mulPose(Axis.ZP.rotation(swingRoot(progress) * -20 * DEG));
        poseStack.mulPose(Axis.XP.rotation(swingRoot(progress) * -40 * DEG));
    }

    private void applySwang(PoseStack poseStack, float equip, float progress, int side) {
        base(poseStack, equip, side, -.72f, true);
        float root = swingRoot(progress);
        poseStack.mulPose(new Quaternionf().rotateXYZ((float) Math.toRadians(root * 40),
                (float) Math.toRadians(-root * 15), (float) Math.toRadians(root * 15)));
    }

    private void applySwong(PoseStack poseStack, float equip, float progress, int side) {
        base(poseStack, equip, side, -.72f, true);
        float square = swingSquare(progress);
        poseStack.mulPose(new Quaternionf().rotateXYZ((float) Math.toRadians(-square * 30),
                (float) Math.toRadians(square * 20), (float) Math.toRadians(-square * 20)));
    }

    private void applySwaing(PoseStack poseStack, float equip, float progress, int side) {
        base(poseStack, equip, side, -.72f, true);
        float square = swingSquare(progress);
        poseStack.mulPose(new Quaternionf().rotateXYZ((float) Math.toRadians(-square * 30),
                (float) Math.toRadians(-square / 19), (float) Math.toRadians(square / 20)));
    }

    private void applyPunch(PoseStack poseStack, float equip, float progress, int side) {
        base(poseStack, equip, side, -.72f, false);
        float root = swingRoot(progress);
        poseStack.translate(.1f, .2f, .3f);
        poseStack.mulPose(Axis.XP.rotation(-root * 10 * DEG));
        poseStack.mulPose(Axis.YP.rotation(-root * 30 * DEG));
        poseStack.mulPose(Axis.ZP.rotation(-root * 30 * DEG));
    }

    private void applySile(PoseStack poseStack, float equip, float progress, int side) {
        base(poseStack, equip, side, -.72f, false);
        float root = swingRoot(progress), square = swingSquare(progress);
        poseStack.mulPose(new Quaternionf().rotateXYZ((float) Math.toRadians(-root * 55),
                (float) Math.toRadians(side * -square * 35), 0));
        poseStack.scale(.92f, .92f, .92f);
        poseStack.translate(side * .08f, mc.player != null && mc.player.isCrouching() ? -.06f : -.12f, .18f);
    }

    private void applySmooth(PoseStack poseStack, float equip, float progress, int side) {
        base(poseStack, equip, side, -.72f, false);
        poseStack.mulPose(Axis.YP.rotation(swingSquare(progress) * -20 * DEG));
        poseStack.mulPose(Axis.ZP.rotation(swingRoot(progress) * -20 * DEG));
        poseStack.mulPose(Axis.XP.rotation(swingRoot(progress) * -80 * DEG));
    }

    private void applyDefault(PoseStack poseStack, float equip, int side) {
        base(poseStack, equip, side, -.72f, false);
    }

    @Override public String getInfo() { return blockMode.get(); }
}
