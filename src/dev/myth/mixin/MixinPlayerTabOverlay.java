package dev.myth.mixin;

import dev.myth.model.render.NoRender;
import net.minecraft.client.gui.GuiGraphicsExtractor;
import net.minecraft.client.gui.components.PlayerTabOverlay;
import net.minecraft.client.multiplayer.PlayerInfo;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.injection.At;
import org.spongepowered.asm.mixin.injection.Inject;
import org.spongepowered.asm.mixin.injection.callback.CallbackInfo;

/**
 * NoRender TabIcons. Mio forced {@code ClientConnection.isEncrypted()} to false, which 26.2 no
 * longer asks about; the icon this leaves is the ping bar, so that is what gets dropped.
 */
@Mixin(PlayerTabOverlay.class)
public abstract class MixinPlayerTabOverlay {
    @Inject(method = "extractPingIcon", at = @At("HEAD"), cancellable = true)
    private void myth$tabIcons(GuiGraphicsExtractor graphics, int slotWidth, int xo, int yo, PlayerInfo info, CallbackInfo ci) {
        NoRender noRender = NoRender.active();
        if (noRender != null && noRender.tabIcons.get()) ci.cancel();
    }
}
