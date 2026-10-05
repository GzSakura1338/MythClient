package me.mioclient.mixin;

import me.mioclient.BaritoneHelper_3;
import me.mioclient.PresetHelperFeature;
import me.mioclient.SearchHelper_4;
import me.mioclient.module.client.HUD;
import me.mioclient.module.client.UI;
import net.minecraft.client.gui.DrawContext;
import net.minecraft.client.gui.screen.Screen;
import net.minecraft.client.gui.screen.TitleScreen;
import net.minecraft.text.Text;
import net.minecraft.util.Formatting;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.injection.At;
import org.spongepowered.asm.mixin.injection.Inject;
import org.spongepowered.asm.mixin.injection.callback.CallbackInfo;

/* compiled from: 0.java */
@Mixin({TitleScreen.class})
/* loaded from: mio-yarn.jar:me/mioclient/mixin/MixinTitleScreen.class */
public class MixinTitleScreen extends Screen implements SearchHelper_4 {
    private static HUD h = (HUD) BaritoneHelper_3.baritoneHelper_4.getModule117(HUD.class);
    private static UI c = (UI) BaritoneHelper_3.baritoneHelper_4.getModule117(UI.class);

    protected MixinTitleScreen(Text text) {
        super(text);
    }

    @Inject(method = {"render"}, at = {@At("TAIL")})
    private void renderHook(DrawContext drawContext, int i, int i2, float f, CallbackInfo callbackInfo) {
        if (PresetHelperFeature.flag) {
            return;
        }
        drawContext.drawTextWithShadow(minecraftClient.textRenderer, "Mio " + String.valueOf(Formatting.WHITE) + "v2 " + String.valueOf(Formatting.GRAY) + "(patch " + "02/03/2025 22:47".substring(0, 10) + ")", 2, 2, h.setting8.getValue().getColor1911(h, 0.0f).getRGB());
    }

    public boolean keyPressed(int i, int i2, int i3) {
        if (c.getKeybind().is1947() || c.getKeybind().get1945() != i) {
            return true;
        }
        c.enable();
        BaritoneHelper_3.getMixinTitleScreenSearchHelper42216().do2835();
        return true;
    }
}
