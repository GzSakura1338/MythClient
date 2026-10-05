package me.mioclient.mixin.sodium;

import me.jellysquid.mods.sodium.client.render.chunk.compile.ChunkBuildBuffers;
import me.jellysquid.mods.sodium.client.render.chunk.compile.pipeline.BlockRenderContext;
import me.mioclient.BaritoneHelper_3;
import me.mioclient.module.render.Xray;
import net.minecraft.client.MinecraftClient;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.Pseudo;
import org.spongepowered.asm.mixin.injection.At;
import org.spongepowered.asm.mixin.injection.Inject;
import org.spongepowered.asm.mixin.injection.callback.CallbackInfo;

/* compiled from: 0.java */
@Pseudo
@Mixin(targets = {"me.jellysquid.mods.sodium.client.render.chunk.compile.pipeline.BlockRenderer"}, remap = false)
/* loaded from: mio-yarn.jar:me/mioclient/mixin/sodium/MixinSodiumBlockRenderer.class */
public class MixinSodiumBlockRenderer {
    private static Xray xray = (Xray) BaritoneHelper_3.baritoneHelper_4.getModule117(Xray.class);

    @Inject(method = {"renderModel"}, at = {@At("HEAD")}, cancellable = true)
    private void onRenderModel(BlockRenderContext blockRenderContext, ChunkBuildBuffers chunkBuildBuffers, CallbackInfo callbackInfo) {
        if (MinecraftClient.getInstance().player == null || !xray.isToggled() || xray.is3071(blockRenderContext.pos(), blockRenderContext.state().getBlock())) {
            return;
        }
        callbackInfo.cancel();
    }
}
