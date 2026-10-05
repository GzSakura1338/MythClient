package me.mioclient.mixin;

import com.mojang.blaze3d.systems.RenderSystem;
import java.awt.Color;
import me.mioclient.BaritoneHelper_3;
import me.mioclient.module.render.NoRender;
import me.mioclient.module.render.SkyColor;
import net.minecraft.client.render.BackgroundRenderer;
import net.minecraft.client.render.Camera;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.injection.At;
import org.spongepowered.asm.mixin.injection.Inject;
import org.spongepowered.asm.mixin.injection.callback.CallbackInfo;

/* compiled from: 0.java */
@Mixin({BackgroundRenderer.class})
/* loaded from: mio-yarn.jar:me/mioclient/mixin/MixinBackgroundRenderer.class */
public class MixinBackgroundRenderer {
    private static NoRender norender = (NoRender) BaritoneHelper_3.baritoneHelper_4.getModule117(NoRender.class);
    private static SkyColor skycolor = (SkyColor) BaritoneHelper_3.baritoneHelper_4.getModule117(SkyColor.class);

    @Inject(method = {"applyFog"}, at = {@At("TAIL")})
    private static void onApplyFog(Camera camera, BackgroundRenderer.FogType fogType, float f, boolean z, float f2, CallbackInfo callbackInfo) {
        if (norender.isToggled() && norender.fog.getValue().booleanValue() && (fogType == BackgroundRenderer.FogType.FOG_TERRAIN || norender.sky.getValue().booleanValue())) {
            RenderSystem.setShaderFogStart(0.0f);
            RenderSystem.setShaderFogEnd(f * norender.range.getValue().floatValue());
        }
        if (skycolor.isToggled() && skycolor.is3136()) {
            Color value = skycolor.fog.getValue();
            RenderSystem.setShaderFogColor(value.getRed() / 255.0f, value.getGreen() / 255.0f, value.getBlue() / 255.0f, value.getAlpha() / 255.0f);
        }
    }
}
