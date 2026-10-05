package me.mioclient.mixin;

import me.mioclient.BaritoneHelper_3;
import me.mioclient.PresetHelperFeature;
import me.mioclient.module.client.UI;
import me.mioclient.module.exploit.FakeVanilla;
import net.minecraft.client.gui.screen.Screen;
import net.minecraft.client.gui.screen.multiplayer.MultiplayerScreen;
import net.minecraft.client.gui.widget.CheckboxWidget;
import net.minecraft.client.gui.widget.TextFieldWidget;
import net.minecraft.text.Text;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.injection.At;
import org.spongepowered.asm.mixin.injection.Inject;
import org.spongepowered.asm.mixin.injection.callback.CallbackInfo;
import org.spongepowered.asm.mixin.injection.callback.CallbackInfoReturnable;

/* compiled from: 0.java */
@Mixin({MultiplayerScreen.class})
/* loaded from: mio-yarn.jar:me/mioclient/mixin/MixinMultiplayerScreen.class */
public class MixinMultiplayerScreen extends Screen {
    private static final FakeVanilla fakevanilla = (FakeVanilla) BaritoneHelper_3.baritoneHelper_4.getModule117(FakeVanilla.class);
    private static final UI clickgui = (UI) BaritoneHelper_3.baritoneHelper_4.getModule117(UI.class);

    protected MixinMultiplayerScreen(Text text) {
        super(text);
    }

    @Inject(method = {"init"}, at = {@At("TAIL")})
    private void initHook(CallbackInfo callbackInfo) {
        if (PresetHelperFeature.flag) {
            return;
        }
        addDrawableChild(CheckboxWidget.builder(Text.of("Vanilla"), this.textRenderer).pos(2, 6).checked(fakevanilla.isToggled()).callback((checkboxWidget, z) -> {
            fakevanilla.do495(z);
        }).build());
    }

    @Inject(method = {"keyPressed"}, at = {@At("HEAD")}, cancellable = true)
    private void keyPressed(int i, int i2, int i3, CallbackInfoReturnable<Boolean> callbackInfoReturnable) {
        if (!(getFocused() instanceof TextFieldWidget) && i == clickgui.getKeybind().get1945()) {
            callbackInfoReturnable.setReturnValue(false);
            callbackInfoReturnable.cancel();
            clickgui.enable();
            BaritoneHelper_3.getMixinTitleScreenSearchHelper42216().do2835();
        }
    }
}
