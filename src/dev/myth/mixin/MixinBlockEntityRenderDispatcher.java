package dev.myth.mixin;

import dev.myth.model.render.NoRender;
import net.minecraft.client.Minecraft;
import net.minecraft.client.renderer.blockentity.BlockEntityRenderDispatcher;
import net.minecraft.client.renderer.blockentity.state.BlockEntityRenderState;
import net.minecraft.client.renderer.feature.ModelFeatureRenderer;
import net.minecraft.core.BlockPos;
import net.minecraft.world.level.block.entity.BlockEntity;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.injection.At;
import org.spongepowered.asm.mixin.injection.Inject;
import org.spongepowered.asm.mixin.injection.callback.CallbackInfoReturnable;

/** NoRender TileEntities: drop the render state past TileDistance, measured like Mio from the camera block. */
@Mixin(BlockEntityRenderDispatcher.class)
public abstract class MixinBlockEntityRenderDispatcher {
    @Inject(method = "tryExtractRenderState", at = @At("HEAD"), cancellable = true)
    private void myth$tileEntities(BlockEntity blockEntity, float partialTicks,
                                   ModelFeatureRenderer.CrumblingOverlay breakProgress, boolean isGloballyRendered,
                                   CallbackInfoReturnable<BlockEntityRenderState> cir) {
        NoRender noRender = NoRender.active();
        if (noRender == null || !noRender.tileEntities.get()) return;
        BlockPos camera = Minecraft.getInstance().gameRenderer.mainCamera().blockPosition();
        int distance = noRender.tileDistance.get();
        if (blockEntity.getBlockPos().distSqr(camera) > (double) distance * distance) cir.setReturnValue(null);
    }
}
