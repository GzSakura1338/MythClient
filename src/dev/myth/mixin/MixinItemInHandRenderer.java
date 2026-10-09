package dev.myth.mixin;

import com.mojang.blaze3d.vertex.PoseStack;
import dev.myth.model.render.Animations;
import dev.myth.model.render.ViewModel;
import net.minecraft.client.player.AbstractClientPlayer;
import net.minecraft.client.renderer.ItemInHandRenderer;
import net.minecraft.client.renderer.SubmitNodeCollector;
import net.minecraft.world.InteractionHand;
import net.minecraft.world.entity.HumanoidArm;
import net.minecraft.world.entity.LivingEntity;
import net.minecraft.world.item.ItemDisplayContext;
import net.minecraft.world.item.ItemStack;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.Shadow;
import org.spongepowered.asm.mixin.injection.At;
import org.spongepowered.asm.mixin.injection.Inject;
import org.spongepowered.asm.mixin.injection.callback.CallbackInfo;

/**
 * Two things share this hook. Starain 1.21.11 MixinHeldItemRenderer supplies the sword block pose,
 * and Mio MixinHeldItemRenderer supplies the ViewModel transform. Both are moved onto 26.2's submit
 * pipeline: renderArmWithItem is the private submitArmWithItem here and takes a SubmitNodeCollector.
 */
@Mixin(ItemInHandRenderer.class)
public abstract class MixinItemInHandRenderer {
    @Shadow
    public abstract void renderItem(LivingEntity mob, ItemStack itemStack, ItemDisplayContext type,
                                    PoseStack poseStack, SubmitNodeCollector submitNodeCollector, int lightCoords);

    @Shadow
    protected abstract void applyItemArmTransform(PoseStack poseStack, HumanoidArm arm, float inverseArmHeight);

    @Inject(method = "submitArmWithItem", at = @At("HEAD"), cancellable = true)
    private void myth$blockAnimation(AbstractClientPlayer player, float frameInterp, float xRot, InteractionHand hand,
                                     float attack, ItemStack itemStack, float inverseArmHeight, PoseStack poseStack,
                                     SubmitNodeCollector submitNodeCollector, int lightCoords, CallbackInfo ci) {
        Animations animations = Animations.get();
        if (animations == null || !animations.isEnabled()) return;

        if (hand == InteractionHand.OFF_HAND && animations.shouldHideOffhand()) {
            ci.cancel();
            return;
        }
        if (player.isScoping()) return;

        HumanoidArm arm = hand == InteractionHand.MAIN_HAND ? player.getMainArm() : player.getMainArm().getOpposite();
        boolean right = arm == HumanoidArm.RIGHT;
        boolean suppressVanilla = animations.shouldSuppressVanillaBlockAnimation(player, hand, itemStack);
        boolean override = animations.shouldOverride(hand, itemStack);
        if (!suppressVanilla && !override) return;

        poseStack.pushPose();
        if (override) animations.applyCustomBlockAnimation(poseStack, arm, inverseArmHeight, attack);
        else applyItemArmTransform(poseStack, arm, inverseArmHeight);
        // This branch draws the item itself, so the renderItem injection below never sees it.
        if (ViewModel.active()) ViewModel.get().applyItem(poseStack, hand, itemStack);
        renderItem(player, itemStack,
                right ? ItemDisplayContext.FIRST_PERSON_RIGHT_HAND : ItemDisplayContext.FIRST_PERSON_LEFT_HAND,
                poseStack, submitNodeCollector, lightCoords);
        poseStack.popPose();
        ci.cancel();
    }

    /**
     * Mio applies the ViewModel transform at the renderItem call rather than at the head, so the
     * offset and scale land in the item local space vanilla has already set up. 26.2 reaches that
     * call from several branches of submitArmWithItem and each one is a valid site.
     */
    @Inject(method = "submitArmWithItem", at = @At(value = "INVOKE",
            target = "Lnet/minecraft/client/renderer/ItemInHandRenderer;renderItem(Lnet/minecraft/world/entity/LivingEntity;"
                    + "Lnet/minecraft/world/item/ItemStack;Lnet/minecraft/world/item/ItemDisplayContext;"
                    + "Lcom/mojang/blaze3d/vertex/PoseStack;Lnet/minecraft/client/renderer/SubmitNodeCollector;I)V"))
    private void myth$viewModelItem(AbstractClientPlayer player, float frameInterp, float xRot, InteractionHand hand,
                                    float attack, ItemStack itemStack, float inverseArmHeight, PoseStack poseStack,
                                    SubmitNodeCollector submitNodeCollector, int lightCoords, CallbackInfo ci) {
        if (ViewModel.active()) ViewModel.get().applyItem(poseStack, hand, itemStack);
    }

    /** Mio renderArmHoldingItem: the bare arm takes the main hand offset and scale, no rotation. */
    @Inject(method = "renderPlayerArm", at = @At("HEAD"))
    private void myth$viewModelArm(PoseStack poseStack, SubmitNodeCollector submitNodeCollector, int lightCoords,
                                   float inverseArmHeight, float attackValue, HumanoidArm arm, CallbackInfo ci) {
        if (ViewModel.active()) ViewModel.get().applyArm(poseStack);
    }
}
