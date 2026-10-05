package today.starain.mixin;

import com.mojang.blaze3d.vertex.PoseStack;
import net.minecraft.client.player.AbstractClientPlayer;
import net.minecraft.client.renderer.ItemInHandRenderer;
import net.minecraft.client.renderer.MultiBufferSource;
import net.minecraft.world.InteractionHand;
import net.minecraft.world.entity.HumanoidArm;
import net.minecraft.world.entity.LivingEntity;
import net.minecraft.world.item.ItemDisplayContext;
import net.minecraft.world.item.ItemStack;
import net.minecraft.world.item.SwordItem;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.Shadow;
import org.spongepowered.asm.mixin.injection.At;
import org.spongepowered.asm.mixin.injection.Inject;
import org.spongepowered.asm.mixin.injection.callback.CallbackInfo;
import today.starain.Starain;
import today.starain.module.impl.render.Animations;

/**
 * 格挡动画注入点。仅在 Animations 模块判定"持剑格挡"时接管主手渲染：应用 1.7 / Push 姿态、
 * 画出剑、cancel 掉 vanilla 的这次 renderArmWithItem。其余情况完全交回 vanilla。
 */
@Mixin(ItemInHandRenderer.class)
public class MixinItemInHandRenderer {

    @Shadow
    public void renderItem(LivingEntity entity, ItemStack itemStack, ItemDisplayContext displayContext,
                           boolean leftHand, PoseStack poseStack, MultiBufferSource buffer, int combinedLight) {
        throw new AssertionError();
    }

    @Inject(method = "renderArmWithItem", at = @At("HEAD"), cancellable = true)
    private void starain$blockAnimation(AbstractClientPlayer player, float partialTicks, float pitch,
                                        InteractionHand hand, float swingProgress, ItemStack stack,
                                        float equipProgress, PoseStack poseStack, MultiBufferSource buffer,
                                        int light, CallbackInfo ci) {
        if (hand != InteractionHand.MAIN_HAND || !(stack.getItem() instanceof SwordItem) || player.isScoping()) {
            return;
        }

        Animations animations = Starain.getInstance().getModuleManager().getModule(Animations.class);
        if (animations == null || !animations.isEnabled() || !animations.shouldBlock()) {
            return;
        }

        HumanoidArm arm = player.getMainArm();
        int i = arm == HumanoidArm.RIGHT ? 1 : -1;

        poseStack.pushPose();
        animations.applyBlockPose(poseStack, i, swingProgress);
        renderItem(player, stack,
                i == 1 ? ItemDisplayContext.FIRST_PERSON_RIGHT_HAND : ItemDisplayContext.FIRST_PERSON_LEFT_HAND,
                i != 1, poseStack, buffer, light);
        poseStack.popPose();

        ci.cancel();
    }
}
