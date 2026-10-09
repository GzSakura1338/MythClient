package dev.myth.mixin;

import dev.myth.model.render.NoRender;
import net.minecraft.client.gui.components.BossHealthOverlay;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.injection.At;
import org.spongepowered.asm.mixin.injection.Inject;
import org.spongepowered.asm.mixin.injection.callback.CallbackInfoReturnable;

/** Mio hides the bar and the sky darkening it asks for together. */
@Mixin(BossHealthOverlay.class)
public abstract class MixinBossHealthOverlay {
    @Inject(method = "shouldDarkenScreen", at = @At("HEAD"), cancellable = true)
    private void myth$bossBars(CallbackInfoReturnable<Boolean> cir) {
        NoRender noRender = NoRender.active();
        if (noRender != null && noRender.bossBars.get()) cir.setReturnValue(false);
    }
}
