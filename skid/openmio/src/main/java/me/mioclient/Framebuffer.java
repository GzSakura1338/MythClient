package me.mioclient;

import com.mojang.blaze3d.platform.GlStateManager;
import com.mojang.blaze3d.systems.RenderSystem;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import net.minecraft.client.MinecraftClient;
import org.lwjgl.opengl.GL30;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/Framebuffer.class */
public class Framebuffer extends net.minecraft.client.gl.Framebuffer {
    public static final int num = 2;
    public static final int num2 = GL30.glGetInteger(36183);
    public static final Map<Integer, Framebuffer> map = new HashMap();
    public static final List<Framebuffer> list = new ArrayList();
    public final int num3;
    public int num4;
    public int num5;
    public boolean flag;

    public Framebuffer(int i) {
        super(true);
        int clamp = Math.clamp(i, 2, num2);
        if ((clamp & (clamp - 1)) != 0) {
            throw new IllegalArgumentException(String.valueOf(clamp));
        }
        this.num3 = clamp;
        setClearColor(1.0f, 1.0f, 1.0f, 0.0f);
    }

    public static boolean is2803() {
        return !list.isEmpty();
    }

    public static Framebuffer getFramebuffer2804(int i) {
        return map.computeIfAbsent(Integer.valueOf(i), num3 -> {
            return new Framebuffer(i);
        });
    }

    public static void do2805(int i, java.lang.Runnable runnable) {
        do2806(i, MinecraftClient.getInstance().getFramebuffer(), runnable);
    }

    public static void do2806(int i, net.minecraft.client.gl.Framebuffer framebuffer, java.lang.Runnable runnable) {
        RenderSystem.assertOnRenderThreadOrInit();
        Framebuffer framebuffer2804 = getFramebuffer2804(i);
        framebuffer2804.resize(framebuffer.textureWidth, framebuffer.textureHeight, true);
        GlStateManager._glBindFramebuffer(36008, framebuffer.fbo);
        GlStateManager._glBindFramebuffer(36009, framebuffer2804.fbo);
        GlStateManager._glBlitFrameBuffer(0, 0, framebuffer2804.textureWidth, framebuffer2804.textureHeight, 0, 0, framebuffer2804.textureWidth, framebuffer2804.textureHeight, 16384, 9729);
        framebuffer2804.beginWrite(true);
        runnable.run();
        framebuffer2804.endWrite();
        GlStateManager._glBindFramebuffer(36008, framebuffer2804.fbo);
        GlStateManager._glBindFramebuffer(36009, framebuffer.fbo);
        GlStateManager._glBlitFrameBuffer(0, 0, framebuffer2804.textureWidth, framebuffer2804.textureHeight, 0, 0, framebuffer2804.textureWidth, framebuffer2804.textureHeight, 16384, 9729);
        framebuffer2804.clear(true);
        framebuffer.beginWrite(false);
    }

    public void resize(int i, int i2, boolean z) {
        if (this.textureWidth == i && this.textureHeight == i2) {
            return;
        }
        super.resize(i, i2, z);
    }

    public void initFbo(int i, int i2, boolean z) {
        RenderSystem.assertOnRenderThreadOrInit();
        int maxSupportedTextureSize = RenderSystem.maxSupportedTextureSize();
        if (i <= 0 || i > maxSupportedTextureSize || i2 <= 0 || i2 > maxSupportedTextureSize) {
            throw new IllegalArgumentException("%d x %d (Out of bounds).".formatted(Integer.valueOf(i), Integer.valueOf(i2)));
        }
        this.viewportWidth = i;
        this.viewportHeight = i2;
        this.textureWidth = i;
        this.textureHeight = i2;
        this.fbo = GlStateManager.glGenFramebuffers();
        GlStateManager._glBindFramebuffer(36160, this.fbo);
        this.num4 = GlStateManager.glGenRenderbuffers();
        GlStateManager._glBindRenderbuffer(36161, this.num4);
        GL30.glRenderbufferStorageMultisample(36161, this.num3, 32856, i, i2);
        GlStateManager._glBindRenderbuffer(36161, 0);
        this.num5 = GlStateManager.glGenRenderbuffers();
        GlStateManager._glBindRenderbuffer(36161, this.num5);
        GL30.glRenderbufferStorageMultisample(36161, this.num3, 6402, i, i2);
        GlStateManager._glBindRenderbuffer(36161, 0);
        GL30.glFramebufferRenderbuffer(36160, 36064, 36161, this.num4);
        GL30.glFramebufferRenderbuffer(36160, 36096, 36161, this.num5);
        this.colorAttachment = MinecraftClient.getInstance().getFramebuffer().getColorAttachment();
        this.depthAttachment = MinecraftClient.getInstance().getFramebuffer().getDepthAttachment();
        checkFramebufferStatus();
        clear(z);
        endRead();
    }

    public void delete() {
        RenderSystem.assertOnRenderThreadOrInit();
        endRead();
        endWrite();
        if (this.fbo > -1) {
            GlStateManager._glBindFramebuffer(36160, 0);
            GlStateManager._glDeleteFramebuffers(this.fbo);
            this.fbo = -1;
        }
        if (this.num4 > -1) {
            GlStateManager._glDeleteRenderbuffers(this.num4);
            this.num4 = -1;
        }
        if (this.num5 > -1) {
            GlStateManager._glDeleteRenderbuffers(this.num5);
            this.num5 = -1;
        }
        this.colorAttachment = -1;
        this.depthAttachment = -1;
        this.textureWidth = -1;
        this.textureHeight = -1;
    }

    public void beginWrite(boolean z) {
        super.beginWrite(z);
        if (this.flag) {
            return;
        }
        list.add(this);
        this.flag = true;
    }

    public void endWrite() {
        super.endWrite();
        if (this.flag) {
            this.flag = false;
            list.remove(this);
        }
    }
}
