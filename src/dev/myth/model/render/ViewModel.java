package dev.myth.model.render;

import com.mojang.blaze3d.vertex.PoseStack;
import com.mojang.math.Axis;
import dev.myth.UI.setting.Setting;
import dev.myth.model.Category;
import dev.myth.model.Model;
import dev.myth.utils.wrapper.Wrapper;
import net.minecraft.world.InteractionHand;
import net.minecraft.world.entity.HumanoidArm;
import net.minecraft.world.item.ItemStack;
import net.minecraft.world.item.ItemUseAnimation;

/**
 * Mio ViewModel (`module/render/ViewModel.java` plus its half of `mixin/MixinHeldItemRenderer`):
 * moves, scales and rotates the first person item, with Arm applying the main hand offset and
 * scale to the bare arm as well. Defaults are Mio's reset values - no offset, scale 1, no rotation.
 */
public final class ViewModel extends Model implements Wrapper {
    private static ViewModel instance;

    public final Setting<Boolean> mainHand = addSetting(Setting.bool("mainHand", "MainHand", true).group());
    public final Setting<Float> mainX = addSetting(pos("mainX", "X", mainHand));
    public final Setting<Float> mainY = addSetting(pos("mainY", "Y", mainHand));
    public final Setting<Float> mainZ = addSetting(pos("mainZ", "Z", mainHand));
    public final Setting<Float> mainScaleX = addSetting(scale("mainScaleX", "ScaleX", mainHand));
    public final Setting<Float> mainScaleY = addSetting(scale("mainScaleY", "ScaleY", mainHand));
    public final Setting<Float> mainScaleZ = addSetting(scale("mainScaleZ", "ScaleZ", mainHand));
    public final Setting<Float> mainRotateX = addSetting(rotate("mainRotateX", "RotateX", mainHand));
    public final Setting<Float> mainRotateY = addSetting(rotate("mainRotateY", "RotateY", mainHand));
    public final Setting<Float> mainRotateZ = addSetting(rotate("mainRotateZ", "RotateZ", mainHand));

    public final Setting<Boolean> offHand = addSetting(Setting.bool("offHand", "OffHand", true).group());
    public final Setting<Float> offX = addSetting(pos("offX", "X", offHand));
    public final Setting<Float> offY = addSetting(pos("offY", "Y", offHand));
    public final Setting<Float> offZ = addSetting(pos("offZ", "Z", offHand));
    public final Setting<Float> offScaleX = addSetting(scale("offScaleX", "ScaleX", offHand));
    public final Setting<Float> offScaleY = addSetting(scale("offScaleY", "ScaleY", offHand));
    public final Setting<Float> offScaleZ = addSetting(scale("offScaleZ", "ScaleZ", offHand));
    public final Setting<Float> offRotateX = addSetting(rotate("offRotateX", "RotateX", offHand));
    public final Setting<Float> offRotateY = addSetting(rotate("offRotateY", "RotateY", offHand));
    public final Setting<Float> offRotateZ = addSetting(rotate("offRotateZ", "RotateZ", offHand));

    public final Setting<Boolean> arm = addSetting(Setting.bool("arm", "Arm", true)
            .description("Applies the main hand offset and scale to the bare arm too."));
    public final Setting<Boolean> eating = addSetting(Setting.bool("eating", "Eating", false)
            .description("Keeps transforming food and potions while they are being used."));

    public ViewModel() {
        super("viewmodel", "ViewModel", Category.RENDER);
        setDescription("Transforms your first person view model.");
        instance = this;
    }

    private static Setting<Float> pos(String id, String name, Setting<Boolean> parent) {
        return Setting.decimal(id, name, 0, -1, 1).step(.01).visibleWhen(() -> parent.get() && parent.isExpanded());
    }

    private static Setting<Float> scale(String id, String name, Setting<Boolean> parent) {
        return Setting.decimal(id, name, 1, 0, 3).step(.01).visibleWhen(() -> parent.get() && parent.isExpanded());
    }

    private static Setting<Float> rotate(String id, String name, Setting<Boolean> parent) {
        return Setting.decimal(id, name, 0, -180, 180).step(1).unit("°")
                .visibleWhen(() -> parent.get() && parent.isExpanded());
    }

    public static ViewModel get() { return instance; }
    public static boolean active() { return instance != null && instance.isEnabled(); }

    /** Mio applies nothing while food or a potion is actually being drunk, unless Eating is on. */
    private boolean suppressedWhileUsing(ItemStack stack) {
        if (eating.get() || mc.player == null) return false;
        ItemUseAnimation animation = stack.getUseAnimation();
        boolean consumable = animation == ItemUseAnimation.EAT || animation == ItemUseAnimation.DRINK;
        return consumable && mc.player.getUseItem() == stack;
    }

    /** Mio mirrors X for a left handed player so the offset stays on the same side of the screen. */
    private float mirrored(float x) {
        return mc.player != null && mc.player.getMainArm() == HumanoidArm.LEFT ? -x : x;
    }

    /** The item transform, applied where Mio applies it: right before the item itself is drawn. */
    public void applyItem(PoseStack poseStack, InteractionHand hand, ItemStack stack) {
        if (suppressedWhileUsing(stack)) return;
        if (hand == InteractionHand.MAIN_HAND) {
            if (!mainHand.get()) return;
            poseStack.translate(mirrored(-mainX.get()), mainY.get(), mainZ.get());
            poseStack.scale(mainScaleX.get(), mainScaleY.get(), mainScaleZ.get());
            poseStack.mulPose(Axis.XN.rotationDegrees(mainRotateX.get()));
            poseStack.mulPose(Axis.YP.rotationDegrees(mainRotateY.get()));
            poseStack.mulPose(Axis.ZP.rotationDegrees(mainRotateZ.get()));
            return;
        }
        if (!offHand.get()) return;
        poseStack.translate(mirrored(offX.get()), offY.get(), offZ.get());
        poseStack.scale(offScaleX.get(), offScaleY.get(), offScaleZ.get());
        poseStack.mulPose(Axis.XN.rotationDegrees(offRotateX.get()));
        poseStack.mulPose(Axis.YN.rotationDegrees(offRotateY.get()));
        poseStack.mulPose(Axis.ZN.rotationDegrees(offRotateZ.get()));
    }

    /** Mio renderArmHoldingItem: offset and scale only, no rotation, and only for the main hand. */
    public void applyArm(PoseStack poseStack) {
        if (!arm.get() || !mainHand.get()) return;
        poseStack.translate(mirrored(-mainX.get()), mainY.get(), mainZ.get());
        poseStack.scale(mainScaleX.get(), mainScaleY.get(), mainScaleZ.get());
    }

    @Override public String getInfo() {
        float scale = mainScaleX.get();
        return scale == 1 ? null : "%.2fx".formatted(scale);
    }
}
