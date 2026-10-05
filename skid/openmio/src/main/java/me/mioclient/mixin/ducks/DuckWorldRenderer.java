package me.mioclient.mixin.ducks;

import net.minecraft.client.gl.Framebuffer;
import net.minecraft.client.render.Frustum;
import net.minecraft.client.render.WorldRenderer;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.gen.Accessor;

/* compiled from: 0.java */
@Mixin({WorldRenderer.class})
/* loaded from: mio-yarn.jar:me/mioclient/mixin/ducks/DuckWorldRenderer.class */
public interface DuckWorldRenderer {
    @Accessor
    void setEntityOutlinesFramebuffer(Framebuffer framebuffer);

    @Accessor("frustum")
    Frustum getFrustum();
}
