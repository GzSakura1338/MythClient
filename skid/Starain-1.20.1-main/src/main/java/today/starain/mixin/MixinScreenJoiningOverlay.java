package today.starain.mixin;

import net.minecraft.client.Minecraft;
import net.minecraft.client.gui.GuiGraphics;
import net.minecraft.client.gui.screens.ConnectScreen;
import net.minecraft.client.gui.screens.ReceivingLevelScreen;
import net.minecraft.client.gui.screens.multiplayer.JoinMultiplayerScreen;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.injection.At;
import org.spongepowered.asm.mixin.injection.Inject;
import org.spongepowered.asm.mixin.injection.callback.CallbackInfo;
import today.starain.ui.injection.InjectedJoinScreen;
import today.starain.utils.render.skia.Skia;
import today.starain.utils.render.skia.context.SkiaContext;

@Mixin({JoinMultiplayerScreen.class, ConnectScreen.class, ReceivingLevelScreen.class})
public abstract class MixinScreenJoiningOverlay {

    @Inject(method = "render", at = @At("TAIL"))
    private void starain$paintJoiningOverlay(GuiGraphics guiGraphics,
                                             int mouseX, int mouseY,
                                             float partialTick,
                                             CallbackInfo ci) {
        if (!InjectedJoinScreen.isJoining()) return;
        try {
            SkiaContext.draw((context) -> {
                Skia.save();
                Skia.scale((float) Minecraft.getInstance().getWindow().getGuiScale());
                InjectedJoinScreen.renderJoiningOverlay(mouseX, mouseY);
                Skia.restore();
            });
        } catch (Throwable e) {
            e.printStackTrace();
        }
    }
}
