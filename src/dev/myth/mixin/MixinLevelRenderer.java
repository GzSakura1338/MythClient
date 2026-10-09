package dev.myth.mixin;

import com.llamalad7.mixinextras.injector.ModifyExpressionValue;
import com.mojang.blaze3d.opengl.GlTexture;
import com.mojang.blaze3d.pipeline.RenderTarget;
import dev.myth.Myth;
import dev.myth.model.render.Shader;
import net.minecraft.client.Minecraft;
import net.minecraft.client.renderer.LevelRenderer;
import net.minecraft.client.renderer.PostChain;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.injection.At;
import org.spongepowered.asm.mixin.injection.Inject;
import org.spongepowered.asm.mixin.injection.callback.CallbackInfo;

/**
 * Vanilla composites its own entity outline post chain whenever anything is outlined. The Shader
 * ESP marks its targets through that same mechanism, so without this the vanilla glow would be
 * drawn over the ported shaders in the team colour. Dropping the chain here also skips the
 * {@code hasAnyOutline()} branch that follows, which is exactly what is wanted.
 *
 * <p>render() looks up only this one chain; the transparency chain comes from getTransparencyChain.
 */
@Mixin(LevelRenderer.class)
public abstract class MixinLevelRenderer {
    @ModifyExpressionValue(method = "render", at = @At(value = "INVOKE",
            target = "Lnet/minecraft/client/renderer/ShaderManager;getPostChain(Lnet/minecraft/resources/Identifier;Ljava/util/Set;)Lnet/minecraft/client/renderer/PostChain;"))
    private PostChain myth$dropVanillaOutline(PostChain original) {
        return Shader.suppressesVanillaOutline() ? null : original;
    }

    /**
     * The blit that puts the outline target on screen, and it is NOT part of the post chain above:
     * GameRenderer calls it right after renderLevel, gated only on shouldShowEntityOutlines. With
     * the chain dropped the target still holds the raw filled silhouette, so leaving this alone
     * pastes a solid white body over everything - including the pass below, which runs first.
     */
    @Inject(method = "doEntityOutline", at = @At("HEAD"), cancellable = true)
    private void myth$skipOutlineBlit(CallbackInfo ci) {
        if (Shader.suppressesVanillaOutline()) ci.cancel();
    }

    /**
     * Where Mio composites: straight onto the game image, in place of the chain dropped above. The
     * frame graph has executed by now, so the outline target holds this frame's silhouettes and the
     * main target holds the world. The HUD is drawn later, so the ESP stays under it.
     */
    @Inject(method = "render", at = @At("TAIL"))
    private void myth$shaderEsp(CallbackInfo ci) {
        Shader shader = Shader.active();
        if (shader == null) return;
        // Vanilla only clears the outline target when something is outlined; with nothing marked it
        // still holds an older frame, so without this the ESP would stick to the last thing seen.
        boolean any = shader.hasOutlines();
        shader.clearFrame();
        if (!any) return;
        RenderTarget outline = ((dev.myth.mixin.access.LevelRendererAccess) this).myth$entityOutlineTarget();
        RenderTarget main = Minecraft.getInstance().gameRenderer.mainRenderTarget();
        if (outline == null || main == null) return;
        if (!(outline.getColorTexture() instanceof GlTexture source)) return;
        if (!(main.getColorTexture() instanceof GlTexture target)) return;
        Myth.ENTITY_SHADER.composite(source.glId(), target.glId(), main.width, main.height, shader.style());
    }
}
