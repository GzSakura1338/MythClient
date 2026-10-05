package me.mioclient.mixin;

import me.mioclient.module.render.Xray;
import net.minecraft.block.BlockState;
import net.minecraft.client.render.VertexConsumer;
import net.minecraft.client.render.block.BlockModelRenderer;
import net.minecraft.client.render.model.BakedModel;
import net.minecraft.client.util.math.MatrixStack;
import net.minecraft.util.math.BlockPos;
import net.minecraft.util.math.random.Random;
import net.minecraft.world.BlockRenderView;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.injection.At;
import org.spongepowered.asm.mixin.injection.Inject;
import org.spongepowered.asm.mixin.injection.callback.CallbackInfo;

/* compiled from: 0.java */
@Mixin({BlockModelRenderer.class})
/* loaded from: mio-yarn.jar:me/mioclient/mixin/MixinBlockModelRenderer.class */
public class MixinBlockModelRenderer {
    @Inject(method = {"renderSmooth", "renderFlat"}, at = {@At("HEAD")}, cancellable = true)
    private void renderSmoothHook(BlockRenderView blockRenderView, BakedModel bakedModel, BlockState blockState, BlockPos blockPos, MatrixStack matrixStack, VertexConsumer vertexConsumer, boolean z, Random random, long j, int i, CallbackInfo callbackInfo) {
        if (!Xray.getXray3073().isToggled() || Xray.getXray3073().is3071(blockPos, blockState.getBlock())) {
            return;
        }
        callbackInfo.cancel();
    }
}
