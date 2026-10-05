package today.starain.mixin;

import net.minecraft.client.gui.GuiGraphics;
import net.minecraft.client.gui.components.CommandSuggestions;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.injection.At;
import org.spongepowered.asm.mixin.injection.Redirect;
import today.starain.utils.client.ChatHudRenderer;

@Mixin(CommandSuggestions.SuggestionsList.class)
public class MixinSuggestionsList {

    @Redirect(
            method = "render",
            at = @At(value = "INVOKE", target = "Lnet/minecraft/client/gui/GuiGraphics;fill(IIIII)V")
    )
    private void starain$suggestionFill(GuiGraphics guiGraphics, int x1, int y1, int x2, int y2, int color) {
        if (!ChatHudRenderer.captureSuggestion(x1, y1, x2, y2, color)) {
            guiGraphics.fill(x1, y1, x2, y2, color);
        }
    }
}
