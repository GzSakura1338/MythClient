package dev.myth.mixin;

import dev.myth.model.render.NoRender;
import net.minecraft.client.renderer.culling.Frustum;
import net.minecraft.client.renderer.extract.LevelExtractor;
import net.minecraft.world.entity.Entity;
import net.minecraft.world.entity.monster.warden.Warden;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.injection.At;
import org.spongepowered.asm.mixin.injection.Inject;
import org.spongepowered.asm.mixin.injection.callback.CallbackInfoReturnable;

/**
 * 26.2 decides what to draw while extracting, so the EntityTypes set (Removal = Partial) and
 * Wardens both come down to one visibility answer instead of Mio's two separate cancels.
 */
@Mixin(LevelExtractor.class)
public abstract class MixinLevelExtractor {
    @Inject(method = "isEntityVisible", at = @At("HEAD"), cancellable = true)
    private void myth$hideEntities(Entity entity, Frustum frustum, double camX, double camY, double camZ,
                                   CallbackInfoReturnable<Boolean> cir) {
        NoRender noRender = NoRender.active();
        if (noRender == null) return;
        if (noRender.hidesEntity(entity)) { cir.setReturnValue(false); return; }
        if (noRender.wardens.get() && entity instanceof Warden) {
            int distance = noRender.wardenDistance.get();
            if (entity.distanceToSqr(camX, camY, camZ) > (double) distance * distance) cir.setReturnValue(false);
        }
    }
}
