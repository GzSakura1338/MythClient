package me.mioclient.mixin;

import me.mioclient.BaritoneHelper_3;
import me.mioclient.module.player.Freecam;
import me.mioclient.module.render.Xray;
import net.minecraft.client.render.chunk.ChunkOcclusionDataBuilder;
import net.minecraft.util.math.BlockPos;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.injection.At;
import org.spongepowered.asm.mixin.injection.Inject;
import org.spongepowered.asm.mixin.injection.callback.CallbackInfo;

/* compiled from: 0.java */
@Mixin({ChunkOcclusionDataBuilder.class})
/* loaded from: mio-yarn.jar:me/mioclient/mixin/MixinChunkOcclusionDataBuilder.class */
public class MixinChunkOcclusionDataBuilder {
    private static Freecam freecam = (Freecam) BaritoneHelper_3.baritoneHelper_4.getModule117(Freecam.class);

    @Inject(method = {"markClosed"}, at = {@At("HEAD")}, cancellable = true)
    private void markClosedHook(BlockPos blockPos, CallbackInfo callbackInfo) {
        if (Xray.getXray3073().isToggled() || freecam.isToggled()) {
            callbackInfo.cancel();
        }
    }
}
