package me.mioclient;

import me.mioclient.mixin.ducks.DuckWorldRenderer;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/ShaderFramebufferHelper.class */
public abstract class ShaderFramebufferHelper extends FramebufferHelper {
    public net.minecraft.client.gl.Framebuffer framebuffer;

    @Override // me.mioclient.FramebufferHelper
    public void do757() {
        DuckWorldRenderer duckWorldRenderer = (DuckWorldRenderer)(minecraftClient.worldRenderer);
        this.framebuffer = minecraftClient.worldRenderer.getEntityOutlinesFramebuffer();
        duckWorldRenderer.setEntityOutlinesFramebuffer(this.framebuffer);
    }

    @Override // me.mioclient.FramebufferHelper
    public void do758() {
        if (this.framebuffer == null) {
            return;
        }
        ((DuckWorldRenderer)(minecraftClient.worldRenderer)).setEntityOutlinesFramebuffer(this.framebuffer);
        this.framebuffer = null;
    }

    public void do866() {
        do761(() -> {
            this.outlineVertexConsumerProvider.draw();
        });
    }
}
