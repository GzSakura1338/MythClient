package me.mioclient.mixin;

import java.util.ArrayList;
import java.util.Iterator;
import java.util.List;
import java.util.Map;
import me.mioclient.PassesHelper;
import net.minecraft.client.gl.Framebuffer;
import net.minecraft.client.gl.PostEffectPass;
import net.minecraft.client.gl.PostEffectProcessor;
import org.spongepowered.asm.mixin.Final;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.Shadow;
import org.spongepowered.asm.mixin.injection.At;
import org.spongepowered.asm.mixin.injection.Inject;
import org.spongepowered.asm.mixin.injection.callback.CallbackInfo;

/* compiled from: 0.java */
@Mixin({PostEffectProcessor.class})
/* loaded from: mio-yarn.jar:me/mioclient/mixin/ShaderEffectMixin.class */
public class ShaderEffectMixin implements PassesHelper {
    private final List<String> fakedBufferNames = new ArrayList();

    @Shadow
    @Final
    private Map<String, Framebuffer> field_1495;

    @Shadow
    @Final
    private List<PostEffectPass> field_1497;

    @Override // me.mioclient.PassesHelper
    public List<PostEffectPass> getPasses() {
        return this.field_1497;
    }

    @Override // me.mioclient.PassesHelper
    public void addFakeTarget(String str, Framebuffer framebuffer) {
        Framebuffer framebuffer2 = this.field_1495.get(str);
        if (framebuffer2 == framebuffer) {
            return;
        }
        if (framebuffer2 != null) {
            Iterator<PostEffectPass> it = this.field_1497.iterator();
            while (it.hasNext()) {
                PostProcessShaderMixin postProcessShaderMixin = (PostProcessShaderMixin)((PostEffectPass) it.next());
                if (((PostEffectPass) postProcessShaderMixin).input == framebuffer2) {
                    postProcessShaderMixin.setInput(framebuffer);
                }
                if (((PostEffectPass) postProcessShaderMixin).output == framebuffer2) {
                    postProcessShaderMixin.setOutput(framebuffer);
                }
            }
            this.field_1495.remove(str);
            this.fakedBufferNames.remove(str);
        }
        this.field_1495.put(str, framebuffer);
        this.fakedBufferNames.add(str);
    }

    @Inject(method = {"close"}, at = {@At("HEAD")})
    void close(CallbackInfo callbackInfo) {
        try {
            Iterator<String> it = this.fakedBufferNames.iterator();
            while (it.hasNext()) {
                this.field_1495.remove(it.next());
            }
        } catch (Exception e) {
        }
    }
}
