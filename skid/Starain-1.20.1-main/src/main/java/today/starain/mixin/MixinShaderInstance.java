package today.starain.mixin;

import com.mojang.blaze3d.shaders.Uniform;
import com.mojang.blaze3d.systems.RenderSystem;
import net.minecraft.client.renderer.ShaderInstance;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.injection.At;
import org.spongepowered.asm.mixin.injection.Inject;
import org.spongepowered.asm.mixin.injection.callback.CallbackInfo;
import today.starain.module.impl.render.WorldColor;

@Mixin(ShaderInstance.class)
public class MixinShaderInstance {

    @Inject(method = "apply()V", at = @At("HEAD"), require = 0)
    private void starain$worldColor(CallbackInfo ci) {
        ShaderInstance self = (ShaderInstance) (Object) this;
        if (!WorldColor.shouldTintShader(self.getName())) {
            return;
        }
        Uniform modulator = self.COLOR_MODULATOR;
        if (modulator == null) {
            return;
        }
        float[] c = WorldColor.modulate(RenderSystem.getShaderColor());
        modulator.set(c[0], c[1], c[2], c[3]);
    }
}
