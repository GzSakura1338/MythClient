package dev.myth.mixin;

import com.llamalad7.mixinextras.injector.ModifyExpressionValue;
import dev.myth.model.render.NoRender;
import net.minecraft.client.player.LocalPlayer;
import net.minecraft.client.renderer.entity.EntityRenderer;
import net.minecraft.client.renderer.entity.state.EntityRenderState;
import net.minecraft.world.entity.Entity;
import net.minecraft.world.entity.boss.enderdragon.EndCrystal;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.injection.At;
import org.spongepowered.asm.mixin.injection.Inject;
import org.spongepowered.asm.mixin.injection.callback.CallbackInfo;

/** NoRender NameTags and the entity half of Fire, both decided while the state is extracted. */
@Mixin(EntityRenderer.class)
public abstract class MixinEntityRenderer {
    /**
     * Shader ESP. Vanilla sets outlineColor here from the glowing effect, and everything downstream
     * - the outline target, hasAnyOutline, the post chain - keys off it, so marking the entity is
     * the whole of what the ESP needs from the render pipeline. White, because the fragment shaders
     * only read the silhouette's alpha and pick the colour themselves.
     */
    @Inject(method = "extractRenderState", at = @At("RETURN"))
    private void myth$shaderOutline(Entity entity, EntityRenderState state, float partialTicks, CallbackInfo ci) {
        if (dev.myth.model.render.Shader.shouldOutline(entity)) state.outlineColor = 0xffffffff;
    }

    @Inject(method = "extractNameTags(Lnet/minecraft/world/entity/Entity;Lnet/minecraft/client/renderer/entity/state/EntityRenderState;FDD)V",
            at = @At("RETURN"))
    private void myth$nameTags(Entity entity, EntityRenderState state, float partialTicks,
                               double nameTagDistance, double belowNameDistance, CallbackInfo ci) {
        NoRender noRender = NoRender.active();
        if (noRender != null && noRender.nameTags.get()) { state.nameTag = null; state.scoreText = null; }
    }

    /** Mio: Fire alone covers the local player and end crystals, Others extends it to everything. */
    @ModifyExpressionValue(method = "extractRenderState", at = @At(value = "INVOKE",
            target = "Lnet/minecraft/world/entity/Entity;displayFireAnimation()Z"))
    private boolean myth$entityFire(boolean original, Entity entity, EntityRenderState state, float partialTicks) {
        NoRender noRender = NoRender.active();
        if (noRender == null || !noRender.fire.get()) return original;
        return !(noRender.fireOthers.get() || entity instanceof LocalPlayer || entity instanceof EndCrystal) && original;
    }
}
