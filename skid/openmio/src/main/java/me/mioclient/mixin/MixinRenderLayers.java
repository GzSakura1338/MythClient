package me.mioclient.mixin;

import com.mojang.blaze3d.systems.RenderSystem;
import me.mioclient.BaritoneHelper_3;
import me.mioclient.module.render.NoRender;
import net.minecraft.block.BlockState;
import net.minecraft.client.render.RenderLayer;
import net.minecraft.client.render.RenderLayers;
import net.minecraft.client.render.TexturedRenderLayers;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.injection.At;
import org.spongepowered.asm.mixin.injection.Inject;
import org.spongepowered.asm.mixin.injection.callback.CallbackInfoReturnable;

/* compiled from: 0.java */
@Mixin({RenderLayers.class})
/* loaded from: mio-yarn.jar:me/mioclient/mixin/MixinRenderLayers.class */
public class MixinRenderLayers {
    private static NoRender norender = (NoRender) BaritoneHelper_3.baritoneHelper_4.getModule117(NoRender.class);

    @Inject(method = {"getEntityBlockLayer"}, at = {@At("HEAD")}, cancellable = true)
    private static void getEntityBlockLayer(BlockState blockState, boolean z, CallbackInfoReturnable<RenderLayer> callbackInfoReturnable) {
        if (norender.get1995() == 1.0f || RenderSystem.getShaderColor()[3] == 1.0f) {
            return;
        }
        callbackInfoReturnable.setReturnValue(TexturedRenderLayers.getEntityTranslucentCull());
        callbackInfoReturnable.cancel();
    }
}
