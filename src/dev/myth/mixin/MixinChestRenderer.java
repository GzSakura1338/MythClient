package dev.myth.mixin;

import com.llamalad7.mixinextras.sugar.Local;
import dev.myth.model.render.Shader;
import net.minecraft.client.renderer.blockentity.ChestRenderer;
import net.minecraft.client.renderer.blockentity.state.ChestRenderState;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.injection.At;
import org.spongepowered.asm.mixin.injection.ModifyArg;

/**
 * Shader ESP for chests. A block entity has no EntityRenderState, so the trick used for entities
 * does not apply; what ChestRenderer does have is the outlineColor it hands to submitModel, and
 * that lands in the same outline group the shaders read. Covers chests, trapped chests and ender
 * chests, which all go through this renderer.
 */
@Mixin(ChestRenderer.class)
public abstract class MixinChestRenderer {
    @ModifyArg(method = "submit", index = 8, at = @At(value = "INVOKE",
            // submitModel is a default on OrderedSubmitNodeCollector, but the call is compiled
            // against the declared parameter type, so that is the owner in the bytecode.
            target = "Lnet/minecraft/client/renderer/SubmitNodeCollector;submitModel(Lnet/minecraft/client/model/Model;Ljava/lang/Object;Lcom/mojang/blaze3d/vertex/PoseStack;IIILnet/minecraft/client/resources/model/sprite/SpriteId;Lnet/minecraft/client/resources/model/sprite/SpriteGetter;ILnet/minecraft/client/renderer/feature/ModelFeatureRenderer$CrumblingOverlay;)V"))
    private int myth$chestOutline(int outlineColor, @Local(argsOnly = true) ChestRenderState state) {
        int shaderOutline = Shader.chestOutline(state.blockPos);
        return shaderOutline != 0 ? shaderOutline : outlineColor;
    }
}
