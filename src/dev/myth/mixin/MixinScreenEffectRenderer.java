package dev.myth.mixin;

import com.llamalad7.mixinextras.injector.ModifyExpressionValue;
import com.llamalad7.mixinextras.injector.v2.WrapWithCondition;
import com.mojang.blaze3d.vertex.PoseStack;
import dev.myth.model.render.NoRender;
import net.minecraft.client.Minecraft;
import net.minecraft.client.renderer.ScreenEffectRenderer;
import net.minecraft.client.renderer.SubmitNodeCollector;
import net.minecraft.client.renderer.texture.TextureAtlasSprite;
import net.minecraft.world.level.block.state.BlockState;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.injection.At;

/**
 * The first-person overlays Mio drops: Fire removes the flames, and Blindness also takes the
 * water and in-wall overlays with it, exactly like MixinInGameOverlayRenderer does there.
 */
@Mixin(ScreenEffectRenderer.class)
public abstract class MixinScreenEffectRenderer {
    @WrapWithCondition(method = "submit", at = @At(value = "INVOKE",
            target = "Lnet/minecraft/client/renderer/ScreenEffectRenderer;submitFire(Lcom/mojang/blaze3d/vertex/PoseStack;Lnet/minecraft/client/renderer/SubmitNodeCollector;Lnet/minecraft/client/renderer/texture/TextureAtlasSprite;)V"))
    private boolean myth$fireOverlay(PoseStack poseStack, SubmitNodeCollector collector, TextureAtlasSprite sprite) {
        NoRender noRender = NoRender.active();
        return noRender == null || !noRender.fire.get();
    }

    @WrapWithCondition(method = "submit", at = @At(value = "INVOKE",
            target = "Lnet/minecraft/client/renderer/ScreenEffectRenderer;submitWater(Lnet/minecraft/client/Minecraft;Lcom/mojang/blaze3d/vertex/PoseStack;Lnet/minecraft/client/renderer/SubmitNodeCollector;)V"))
    private boolean myth$waterOverlay(Minecraft minecraft, PoseStack poseStack, SubmitNodeCollector collector) {
        NoRender noRender = NoRender.active();
        return noRender == null || !noRender.blindness.get();
    }

    @ModifyExpressionValue(method = "submit", at = @At(value = "INVOKE",
            target = "Lnet/minecraft/client/renderer/ScreenEffectRenderer;getViewBlockingState(Lnet/minecraft/world/entity/player/Player;)Lnet/minecraft/world/level/block/state/BlockState;"))
    private BlockState myth$inWallOverlay(BlockState original) {
        NoRender noRender = NoRender.active();
        return noRender != null && noRender.blindness.get() ? null : original;
    }
}
