package me.mioclient.mixin;

import me.mioclient.module.render.Xray;
import net.minecraft.block.AbstractBlock;
import net.minecraft.util.math.BlockPos;
import net.minecraft.world.BlockView;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.injection.At;
import org.spongepowered.asm.mixin.injection.Inject;
import org.spongepowered.asm.mixin.injection.callback.CallbackInfoReturnable;

/* compiled from: 0.java */
@Mixin({AbstractBlock.AbstractBlockState.class})
/* loaded from: mio-yarn.jar:me/mioclient/mixin/MixinAbstractBlockState.class */
public class MixinAbstractBlockState {
    @Inject(method = {"getAmbientOcclusionLightLevel"}, at = {@At("HEAD")}, cancellable = true)
    private void getAmbientOcclusionLightLevelHook(BlockView blockView, BlockPos blockPos, CallbackInfoReturnable<Float> callbackInfoReturnable) {
        if (Xray.getXray3073().isToggled()) {
            callbackInfoReturnable.cancel();
            callbackInfoReturnable.setReturnValue(Float.valueOf(1.0f));
        }
    }
}
