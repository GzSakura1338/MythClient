package me.mioclient.mixin;

import com.mojang.blaze3d.platform.GlStateManager;
import me.mioclient.ShaderSearchHelper4;
import net.minecraft.client.MinecraftClient;
import net.minecraft.client.gl.Framebuffer;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.injection.At;
import org.spongepowered.asm.mixin.injection.Inject;
import org.spongepowered.asm.mixin.injection.callback.CallbackInfo;

/* compiled from: 0.java */
@Mixin({Framebuffer.class})
/* loaded from: mio-yarn.jar:me/mioclient/mixin/MixinFramebuffer.class */
public class MixinFramebuffer {
    @Inject(method = {"bind"}, at = {@At("TAIL")}, cancellable = true)
    private void bind(boolean z, CallbackInfo callbackInfo) {
        if (ShaderSearchHelper4.flag && MinecraftClient.getInstance().getFramebuffer().equals(this)) {
            GlStateManager._glBindFramebuffer(36160, 0);
            callbackInfo.cancel();
        }
    }
}
