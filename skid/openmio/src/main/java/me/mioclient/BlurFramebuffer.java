package me.mioclient;

import com.mojang.blaze3d.platform.GlStateManager;
import com.mojang.blaze3d.systems.RenderSystem;
import net.minecraft.client.MinecraftClient;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/BlurFramebuffer.class */
public class BlurFramebuffer extends net.minecraft.client.gl.Framebuffer {
    public static BlurFramebuffer blurFramebuffer;

    public BlurFramebuffer(int i, int i2) {
        super(false);
        RenderSystem.assertOnRenderThreadOrInit();
        resize(i, i2, true);
        setClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    }

    public static BlurFramebuffer getBlurFramebuffer1999() {
        if (blurFramebuffer == null) {
            blurFramebuffer = new BlurFramebuffer(MinecraftClient.getInstance().getFramebuffer().textureWidth, MinecraftClient.getInstance().getFramebuffer().textureHeight);
        }
        return blurFramebuffer;
    }

    public static void do2000(java.lang.Runnable runnable) {
        net.minecraft.client.gl.Framebuffer framebuffer = MinecraftClient.getInstance().getFramebuffer();
        RenderSystem.assertOnRenderThreadOrInit();
        BlurFramebuffer blurFramebuffer1999 = getBlurFramebuffer1999();
        if (blurFramebuffer1999.textureWidth != framebuffer.textureWidth || blurFramebuffer1999.textureHeight != framebuffer.textureHeight) {
            blurFramebuffer1999.resize(framebuffer.textureWidth, framebuffer.textureHeight, false);
        }
        GlStateManager._glBindFramebuffer(36009, blurFramebuffer1999.fbo);
        blurFramebuffer1999.beginWrite(true);
        runnable.run();
        blurFramebuffer1999.endWrite();
        GlStateManager._glBindFramebuffer(36009, framebuffer.fbo);
        framebuffer.beginWrite(false);
    }

    public static void do2001(float f) {
        net.minecraft.client.gl.Framebuffer framebuffer = MinecraftClient.getInstance().getFramebuffer();
        BlurFramebuffer blurFramebuffer1999 = getBlurFramebuffer1999();
        BlurFramebufferHelper_2.blurFramebufferHelper.do2459("MaskSampler", blurFramebuffer1999);
        BlurFramebufferHelper_2.blurFramebufferHelper.do2458("Radius", f);
        BlurFramebufferHelper_2.blurFramebufferHelper.do2460(SearchHelper_2.get536());
        GlStateManager._glBindFramebuffer(36009, blurFramebuffer1999.fbo);
        blurFramebuffer1999.clear(true);
        GlStateManager._glBindFramebuffer(36009, framebuffer.fbo);
        framebuffer.beginWrite(true);
    }

    public static void do2002(java.lang.Runnable runnable, float f) {
        do2000(runnable);
        do2001(f);
    }
}
