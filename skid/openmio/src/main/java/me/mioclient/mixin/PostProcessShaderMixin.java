package me.mioclient.mixin;

import net.minecraft.client.gl.Framebuffer;
import net.minecraft.client.gl.PostEffectPass;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.Mutable;
import org.spongepowered.asm.mixin.gen.Accessor;

/* compiled from: 0.java */
@Mixin({PostEffectPass.class})
/* loaded from: mio-yarn.jar:me/mioclient/mixin/PostProcessShaderMixin.class */
public interface PostProcessShaderMixin {
    @Accessor("input")
    @Mutable
    void setInput(Framebuffer framebuffer);

    @Accessor("output")
    @Mutable
    void setOutput(Framebuffer framebuffer);
}
