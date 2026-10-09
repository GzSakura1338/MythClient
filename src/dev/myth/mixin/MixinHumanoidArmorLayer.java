package dev.myth.mixin;

import com.mojang.blaze3d.vertex.PoseStack;
import dev.myth.model.render.NoRender;
import net.minecraft.client.renderer.SubmitNodeCollector;
import net.minecraft.client.renderer.entity.layers.HumanoidArmorLayer;
import net.minecraft.client.renderer.entity.state.HumanoidRenderState;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.injection.At;
import org.spongepowered.asm.mixin.injection.Inject;
import org.spongepowered.asm.mixin.injection.callback.CallbackInfo;

/**
 * Mio drops the armour pass once Opacity reaches zero. 26.2 submits equipment layers without a
 * tint, so a partial Opacity has nothing to scale and leaves the armour as it is.
 */
@Mixin(HumanoidArmorLayer.class)
public abstract class MixinHumanoidArmorLayer {
    // The erased signature is spelled out because the class also carries a bridge submit.
    @Inject(method = "submit(Lcom/mojang/blaze3d/vertex/PoseStack;Lnet/minecraft/client/renderer/SubmitNodeCollector;ILnet/minecraft/client/renderer/entity/state/HumanoidRenderState;FF)V",
            at = @At("HEAD"), cancellable = true)
    private void myth$armor(PoseStack poseStack, SubmitNodeCollector collector, int lightCoords,
                            HumanoidRenderState state, float yRot, float xRot, CallbackInfo ci) {
        NoRender noRender = NoRender.active();
        if (noRender != null && noRender.armorOpacity() == 0) ci.cancel();
    }
}
