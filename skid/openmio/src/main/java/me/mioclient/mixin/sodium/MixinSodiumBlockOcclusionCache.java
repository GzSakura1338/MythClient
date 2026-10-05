package me.mioclient.mixin.sodium;

import me.mioclient.module.render.Xray;
import net.minecraft.block.BlockState;
import net.minecraft.util.math.BlockPos;
import net.minecraft.util.math.Direction;
import net.minecraft.world.BlockView;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.Pseudo;
import org.spongepowered.asm.mixin.injection.At;
import org.spongepowered.asm.mixin.injection.Inject;
import org.spongepowered.asm.mixin.injection.callback.CallbackInfoReturnable;

/* compiled from: 0.java */
@Pseudo
@Mixin(targets = {"me.jellysquid.mods.sodium.client.render.chunk.compile.pipeline.BlockOcclusionCache"}, remap = false)
/* loaded from: mio-yarn.jar:me/mioclient/mixin/sodium/MixinSodiumBlockOcclusionCache.class */
public class MixinSodiumBlockOcclusionCache {
    @Inject(method = {"shouldDrawSide"}, at = {@At("HEAD")}, cancellable = true, remap = false)
    public void shouldDrawSide(BlockState blockState, BlockView blockView, BlockPos blockPos, Direction direction, CallbackInfoReturnable<Boolean> callbackInfoReturnable) {
        if (Xray.getXray3073().isToggled()) {
            callbackInfoReturnable.cancel();
            callbackInfoReturnable.setReturnValue(Boolean.valueOf(Xray.getXray3073().is3071(blockPos, blockState.getBlock())));
        }
    }
}
