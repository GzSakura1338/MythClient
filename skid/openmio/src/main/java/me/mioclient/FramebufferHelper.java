package me.mioclient;

import net.minecraft.client.MinecraftClient;
import net.minecraft.client.gl.SimpleFramebuffer;
import net.minecraft.client.render.OutlineVertexConsumerProvider;
import net.minecraft.entity.Entity;
import org.lwjgl.glfw.GLFW;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/FramebufferHelper.class */
public abstract class FramebufferHelper implements SearchHelper_4 {
    public OutlineVertexConsumerProvider outlineVertexConsumerProvider;
    public net.minecraft.client.gl.Framebuffer framebuffer;
    public FramebufferHelper_3 framebufferHelper_3;

    public void do753(String str) {
        this.outlineVertexConsumerProvider = new OutlineVertexConsumerProvider(minecraftClient.getBufferBuilders().getEntityVertexConsumers());
        this.framebuffer = new SimpleFramebuffer(minecraftClient.getWindow().getFramebufferWidth(), minecraftClient.getWindow().getFramebufferHeight(), false, MinecraftClient.IS_SYSTEM_MAC);
        this.framebuffer.setClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        this.framebufferHelper_3 = new FramebufferHelper_3("/base.vert", new ArgumentTypeHelper().getArgumentTypeHelper2919(str).getString2921("/\u0001.frag"));
    }

    public void do754(String str, String str2) {
        this.outlineVertexConsumerProvider = new OutlineVertexConsumerProvider(minecraftClient.getBufferBuilders().getEntityVertexConsumers());
        this.framebuffer = new SimpleFramebuffer(minecraftClient.getWindow().getFramebufferWidth(), minecraftClient.getWindow().getFramebufferHeight(), false, MinecraftClient.IS_SYSTEM_MAC);
        this.framebuffer.setClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        this.framebufferHelper_3 = new FramebufferHelper_3(new ArgumentTypeHelper().getArgumentTypeHelper2919(str).getString2921("/\u0001.vert"), new ArgumentTypeHelper().getArgumentTypeHelper2919(str2).getString2921("/\u0001.frag"));
    }

    public abstract boolean is755();

    public abstract boolean is756(Entity entity);

    public void do757() {
    }

    public void do758() {
    }

    public abstract void do759();

    public void do760() {
        if (is755()) {
            this.framebuffer.clear(MinecraftClient.IS_SYSTEM_MAC);
            minecraftClient.getFramebuffer().beginWrite(false);
        }
    }

    public void do761(java.lang.Runnable runnable) {
        if (is755()) {
            do757();
            runnable.run();
            do758();
            minecraftClient.getFramebuffer().beginWrite(false);
            FramebufferHelperSearchHelper4.do323(this.framebuffer.getColorAttachment(), 0);
            this.framebufferHelper_3.do1436();
            this.framebufferHelper_3.do1441("u_Size", minecraftClient.getWindow().getFramebufferWidth(), minecraftClient.getWindow().getFramebufferHeight());
            this.framebufferHelper_3.do1439("u_Texture", 0);
            this.framebufferHelper_3.do1440("u_Time", GLFW.glfwGetTime());
            do759();
            FramebufferHelper_2.do865();
        }
    }

    public void do762(int i, int i2) {
        if (this.framebuffer == null) {
            return;
        }
        this.framebuffer.resize(i, i2, MinecraftClient.IS_SYSTEM_MAC);
    }
}
