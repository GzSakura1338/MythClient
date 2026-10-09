package dev.myth.mixin;

import dev.myth.model.render.NoRender;
import net.minecraft.client.gui.GuiGraphicsExtractor;
import net.minecraft.client.gui.screens.Screen;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.injection.At;
import org.spongepowered.asm.mixin.injection.Inject;
import org.spongepowered.asm.mixin.injection.callback.CallbackInfo;

/** NoRender Tint: the dark wash a screen paints over the world behind it. */
@Mixin(Screen.class)
public abstract class MixinScreen {
    @Inject(method = "extractTransparentBackground", at = @At("HEAD"), cancellable = true)
    private void myth$tint(GuiGraphicsExtractor graphics, CallbackInfo ci) {
        NoRender noRender = NoRender.active();
        if (noRender != null && noRender.tint.get()) ci.cancel();
    }
}
