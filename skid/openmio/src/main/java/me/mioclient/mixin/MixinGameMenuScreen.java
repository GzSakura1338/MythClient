package me.mioclient.mixin;

import me.mioclient.BaritoneHelper_3;
import me.mioclient.feature.ConfirmDisconnect;
import me.mioclient.module.exploit.IllegalDisconnect;
import me.mioclient.module.misc.AntiQuit;
import net.minecraft.client.gui.screen.GameMenuScreen;
import net.minecraft.client.gui.screen.Screen;
import net.minecraft.text.Text;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.Shadow;
import org.spongepowered.asm.mixin.Unique;
import org.spongepowered.asm.mixin.injection.At;
import org.spongepowered.asm.mixin.injection.Inject;
import org.spongepowered.asm.mixin.injection.callback.CallbackInfo;

/* compiled from: 0.java */
@Mixin({GameMenuScreen.class})
/* loaded from: mio-yarn.jar:me/mioclient/mixin/MixinGameMenuScreen.class */
public abstract class MixinGameMenuScreen extends Screen {

    @Unique
    private boolean mio$ignoreDisconnect;
    private static IllegalDisconnect illegaldisconnect = (IllegalDisconnect) BaritoneHelper_3.baritoneHelper_4.getModule117(IllegalDisconnect.class);
    private static AntiQuit antiquit = (AntiQuit) BaritoneHelper_3.baritoneHelper_4.getModule117(AntiQuit.class);

    @Shadow
    protected abstract void method_47632();

    protected MixinGameMenuScreen(Text text) {
        super(text);
    }

    @Inject(method = {"disconnect"}, at = {@At("HEAD")}, cancellable = true)
    private void disconnectHook(CallbackInfo callbackInfo) {
        if (antiquit.isToggled() && antiquit.disconnect.getValue().booleanValue() && !this.mio$ignoreDisconnect) {
            this.client.setScreen(new ConfirmDisconnect(() -> {
                this.mio$ignoreDisconnect = true;
                method_47632();
            }));
            callbackInfo.cancel();
        } else {
            if (!illegaldisconnect.isToggled() || this.client.player == null || this.client.isInSingleplayer()) {
                return;
            }
            illegaldisconnect.do639();
            callbackInfo.cancel();
        }
    }
}
