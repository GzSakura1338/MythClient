package today.starain.mixin;

import com.mojang.blaze3d.vertex.PoseStack;
import com.mojang.math.Axis;
import net.minecraft.client.renderer.MultiBufferSource;
import net.minecraft.client.renderer.entity.ItemEntityRenderer;
import net.minecraft.world.entity.item.ItemEntity;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.injection.At;
import org.spongepowered.asm.mixin.injection.Inject;
import org.spongepowered.asm.mixin.injection.ModifyArgs;
import org.spongepowered.asm.mixin.injection.Redirect;
import org.spongepowered.asm.mixin.injection.callback.CallbackInfo;
import org.spongepowered.asm.mixin.injection.invoke.arg.Args;
import today.starain.Starain;
import today.starain.module.impl.render.ItemPhysics;

@Mixin(ItemEntityRenderer.class)
public class MixinItemEntityRenderer {

    private static final String RENDER = "render(Lnet/minecraft/world/entity/item/ItemEntity;FFLcom/mojang/blaze3d/vertex/PoseStack;Lnet/minecraft/client/renderer/MultiBufferSource;I)V";

    @Redirect(
            method = RENDER,
            at = @At(value = "INVOKE", target = "Lnet/minecraft/world/entity/item/ItemEntity;getSpin(F)F")
    )
    private float starain$noSpin(ItemEntity instance, float partialTick) {
        return isPhysics() ? 0.0F : instance.getSpin(partialTick);
    }

    @ModifyArgs(
            method = RENDER,
            at = @At(value = "INVOKE", target = "Lcom/mojang/blaze3d/vertex/PoseStack;translate(FFF)V", ordinal = 0)
    )
    private void starain$killBob(Args args) {
        if (isPhysics()) {
            args.set(1, 0.0F);
        }
    }

    @Inject(
            method = RENDER,
            at = @At(value = "INVOKE", target = "Lcom/mojang/blaze3d/vertex/PoseStack;mulPose(Lorg/joml/Quaternionf;)V", ordinal = 0, shift = At.Shift.AFTER)
    )
    private void starain$layFlat(ItemEntity entity, float entityYaw, float partialTicks, PoseStack poseStack,
                                 MultiBufferSource buffer, int packedLight, CallbackInfo ci) {
        if (isPhysics()) {
            poseStack.mulPose(Axis.XP.rotationDegrees(90.0F));
            poseStack.translate(0.0F, 0.0F, -0.04F);
        }
    }

    private static boolean isPhysics() {
        Starain starain = Starain.getInstance();
        if (starain == null || starain.getModuleManager() == null) {
            return false;
        }
        ItemPhysics module = starain.getModuleManager().getModule(ItemPhysics.class);
        return module != null && module.isEnabled();
    }
}
