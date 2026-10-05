package today.starain.mixin;

import net.minecraft.client.Minecraft;
import net.minecraft.client.gui.GuiGraphics;
import net.minecraft.client.gui.components.CommandSuggestions;
import net.minecraft.client.gui.components.EditBox;
import net.minecraft.client.gui.screens.ChatScreen;
import net.minecraft.client.gui.screens.Screen;
import net.minecraft.util.Mth;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.Shadow;
import org.spongepowered.asm.mixin.injection.At;
import org.spongepowered.asm.mixin.injection.Inject;
import org.spongepowered.asm.mixin.injection.Redirect;
import org.spongepowered.asm.mixin.injection.callback.CallbackInfoReturnable;
import today.starain.module.impl.client.HUD;
import today.starain.utils.client.ChatHudRenderer;

@Mixin(ChatScreen.class)
public class MixinChatScreen {

    @Shadow
    protected EditBox input;

    @Shadow
    CommandSuggestions commandSuggestions;

    @Redirect(
            method = "render",
            at = @At(value = "INVOKE", target = "Lnet/minecraft/client/gui/GuiGraphics;fill(IIIII)V")
    )
    private void starain$inputFill(GuiGraphics guiGraphics, int x1, int y1, int x2, int y2, int color) {
        if (ChatHudRenderer.isActive()) {
            ChatHudRenderer.pushInput(y1, y2, Minecraft.getInstance().font.width(input.getValue()));
            return;
        }
        if (HUD.isChatBackgroundEnabled()) {
            guiGraphics.fill(x1, y1, x2, y2, color);
        }
    }

    @Redirect(
            method = "render",
            at = @At(value = "INVOKE", target = "Lnet/minecraft/client/gui/components/EditBox;render(Lnet/minecraft/client/gui/GuiGraphics;IIF)V")
    )
    private void starain$inputRender(EditBox box, GuiGraphics guiGraphics, int mouseX, int mouseY, float partialTick) {
        if (!ChatHudRenderer.isActive()) {
            box.render(guiGraphics, mouseX, mouseY, partialTick);
            return;
        }
        guiGraphics.pose().pushPose();
        guiGraphics.pose().translate(ChatHudRenderer.MARGIN, -ChatHudRenderer.MARGIN, 0.0f);
        box.render(guiGraphics, mouseX, mouseY, partialTick);
        guiGraphics.pose().popPose();
    }

    @Inject(method = "mouseScrolled", at = @At("HEAD"), cancellable = true)
    private void starain$scroll(double mouseX, double mouseY, double delta, CallbackInfoReturnable<Boolean> cir) {
        if (!ChatHudRenderer.isActive()) {
            return;
        }
        double clamped = Mth.clamp(delta, -1.0d, 1.0d);
        if (commandSuggestions.mouseScrolled(clamped)) {
            cir.setReturnValue(true);
            return;
        }
        ChatHudRenderer.scroll(clamped * (Screen.hasShiftDown() ? 1.0d : 4.0d));
        cir.setReturnValue(true);
    }
}
