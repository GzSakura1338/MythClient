package dev.myth.mixin;

import dev.myth.model.render.NoRender;
import net.minecraft.client.multiplayer.chat.GuiMessageTag;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.injection.At;
import org.spongepowered.asm.mixin.injection.Inject;
import org.spongepowered.asm.mixin.injection.callback.CallbackInfo;

/**
 * NoRender MessageIndicator. 26.2 hands the chat tag to a ChatGraphicsAccess, so both drawing
 * implementations drop the coloured bar and its icon. Mio also pulled the text back by 3 px;
 * that offset now lives inside the chat layout pass and is left alone.
 */
@Mixin(targets = {"net.minecraft.client.gui.components.ChatComponent$DrawingBackgroundGraphicsAccess",
                  "net.minecraft.client.gui.components.ChatComponent$DrawingFocusedGraphicsAccess"})
public abstract class MixinChatComponentTags {
    @Inject(method = "handleTag", at = @At("HEAD"), cancellable = true)
    private void myth$messageIndicator(int x0, int y0, int x1, int y1, float opacity, GuiMessageTag tag, CallbackInfo ci) {
        NoRender noRender = NoRender.active();
        if (noRender != null && noRender.messageIndicator.get()) ci.cancel();
    }

    @Inject(method = "handleTagIcon", at = @At("HEAD"), cancellable = true)
    private void myth$messageIndicatorIcon(int left, int bottom, boolean forceVisible, GuiMessageTag tag,
                                           GuiMessageTag.Icon icon, CallbackInfo ci) {
        NoRender noRender = NoRender.active();
        if (noRender != null && noRender.messageIndicator.get()) ci.cancel();
    }
}
