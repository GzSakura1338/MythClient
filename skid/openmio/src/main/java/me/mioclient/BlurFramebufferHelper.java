package me.mioclient;

import com.mojang.blaze3d.systems.RenderSystem;
import java.io.IOException;
import java.util.Iterator;
import java.util.Objects;
import java.util.function.Consumer;
import net.minecraft.client.MinecraftClient;
import net.minecraft.client.gl.JsonEffectShaderProgram;
import net.minecraft.client.gl.PostEffectPass;
import net.minecraft.client.gl.PostEffectProcessor;
import net.minecraft.util.Identifier;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/BlurFramebufferHelper.class */
public class BlurFramebufferHelper {
    public final PostEffectProcessor postEffectProcessor;
    public int num;
    public int num2;

    public BlurFramebufferHelper(Identifier identifier, Consumer<BlurFramebufferHelper> consumer) {
        MinecraftClient minecraftClient = MinecraftClient.getInstance();
        try {
            this.postEffectProcessor = new PostEffectProcessor(minecraftClient.getTextureManager(), minecraftClient.getResourceManager(), minecraftClient.getFramebuffer(), identifier);
            do2457();
            consumer.accept(this);
        } catch (Exception e) {
            throw new java.lang.RuntimeException(e);
        }
    }

    public static BlurFramebufferHelper getBlurFramebufferHelper2456(String str, Consumer<BlurFramebufferHelper> consumer) {
        return new BlurFramebufferHelper(Identifier.of(SearchHelper_4.is1471() ? "mio" : "minecraft", String.format("shaders/post/%s.json", str)), consumer);
    }

    public void do2457() {
        MinecraftClient minecraftClient = MinecraftClient.getInstance();
        int framebufferWidth = minecraftClient.getWindow().getFramebufferWidth();
        int framebufferHeight = minecraftClient.getWindow().getFramebufferHeight();
        if (this.num == framebufferWidth && this.num2 == framebufferHeight) {
            return;
        }
        this.postEffectProcessor.setupDimensions(framebufferWidth, framebufferHeight);
        this.num = framebufferWidth;
        this.num2 = framebufferHeight;
    }

    public void do2458(String str, float f) {
        ((PassesHelper) this.postEffectProcessor).getPasses().stream().map(postEffectPass -> {
            return postEffectPass.getProgram().getUniformByName(str);
        }).filter((v0) -> {
            return Objects.nonNull(v0);
        }).forEach(glUniform -> {
            glUniform.set(f);
        });
    }

    public void do2459(String str, net.minecraft.client.gl.Framebuffer framebuffer) {
        Iterator<PostEffectPass> it = ((PassesHelper) this.postEffectProcessor).getPasses().iterator();
        while (it.hasNext()) {
            JsonEffectShaderProgram program = it.next().getProgram();
            Objects.requireNonNull(framebuffer);
            program.bindSampler(str, framebuffer::getColorAttachment);
        }
    }

    public void do2460(float f) {
        do2457();
        RenderSystem.disableBlend();
        RenderSystem.disableDepthTest();
        RenderSystem.resetTextureMatrix();
        this.postEffectProcessor.render(f);
        MinecraftClient.getInstance().getFramebuffer().beginWrite(true);
        RenderSystem.disableBlend();
        RenderSystem.blendFunc(770, 771);
        RenderSystem.enableDepthTest();
    }

    public PostEffectProcessor getPostEffectProcessor2461() {
        return this.postEffectProcessor;
    }
}
