package me.mioclient;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/ESPSearchHelper4_3.class */
public final class ESPSearchHelper4_3 implements SearchHelper_4 {
    public static net.minecraft.client.gl.Framebuffer framebuffer;

    public ESPSearchHelper4_3() {
        throw new AssertionError();
    }

    public static void do2887(FramebufferHelper framebufferHelper, boolean z, java.lang.Runnable runnable) {
        do2888(framebufferHelper, z);
        runnable.run();
        minecraftClient.getBufferBuilders().getEntityVertexConsumers().draw();
        do2889(z);
    }

    public static void do2888(FramebufferHelper framebufferHelper, boolean z) {
        if (z) {
            ShaderSearchHelper4.do760();
        }
        framebuffer = ((FramebufferHelper_4) minecraftClient.worldRenderer).getFramebuffer();
        ((FramebufferHelper_4) minecraftClient.worldRenderer).setFramebuffer(framebufferHelper.framebuffer);
        minecraftClient.getFramebuffer().endWrite();
        ShaderSearchHelper4.flag = true;
    }

    public static void do2889(boolean z) {
        ShaderSearchHelper4.flag = false;
        ((FramebufferHelper_4) minecraftClient.worldRenderer).setFramebuffer(framebuffer);
        minecraftClient.getFramebuffer().beginWrite(false);
        if (z) {
            ShaderSearchHelper4.do866();
        }
    }
}
