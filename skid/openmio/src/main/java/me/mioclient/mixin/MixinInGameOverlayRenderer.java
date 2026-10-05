package me.mioclient.mixin;

import me.mioclient.BaritoneHelper_3;
import me.mioclient.module.render.NoRender;
import net.minecraft.client.MinecraftClient;
import net.minecraft.client.gui.hud.InGameOverlayRenderer;
import net.minecraft.client.texture.Sprite;
import net.minecraft.client.util.math.MatrixStack;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.injection.At;
import org.spongepowered.asm.mixin.injection.Inject;
import org.spongepowered.asm.mixin.injection.callback.CallbackInfo;

/* compiled from: 0.java */
@Mixin({InGameOverlayRenderer.class})
/* loaded from: mio-yarn.jar:me/mioclient/mixin/MixinInGameOverlayRenderer.class */
public class MixinInGameOverlayRenderer {
    private static NoRender norender = (NoRender) BaritoneHelper_3.baritoneHelper_4.getModule117(NoRender.class);

    @Inject(method = {"renderFireOverlay"}, at = {@At("HEAD")}, cancellable = true)
    private static void onRenderFireOverlay(MinecraftClient minecraftClient, MatrixStack matrixStack, CallbackInfo callbackInfo) {
        if (norender.isToggled() && norender.fire.getValue().booleanValue()) {
            callbackInfo.cancel();
        }
    }

    @Inject(method = {"renderUnderwaterOverlay"}, at = {@At("HEAD")}, cancellable = true)
    private static void onRenderUnderwaterOverlay(MinecraftClient minecraftClient, MatrixStack matrixStack, CallbackInfo callbackInfo) {
        if (norender.isToggled() && norender.blindness.getValue().booleanValue()) {
            callbackInfo.cancel();
        }
    }

    @Inject(method = {"renderInWallOverlay"}, at = {@At("HEAD")}, cancellable = true)
    private static void render(Sprite sprite, MatrixStack matrixStack, CallbackInfo callbackInfo) {
        if (norender.isToggled() && norender.blindness.getValue().booleanValue()) {
            callbackInfo.cancel();
        }
    }
}
