package me.mioclient.mixin;

import me.mioclient.SearchHelper_4;
import me.mioclient.event.CharEvent;
import me.mioclient.event.KeyEvent;
import me.mioclient.module.misc.BetterChat;
import net.minecraft.client.Keyboard;
import net.minecraft.client.MinecraftClient;
import org.spongepowered.asm.mixin.Final;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.Shadow;
import org.spongepowered.asm.mixin.injection.At;
import org.spongepowered.asm.mixin.injection.Inject;
import org.spongepowered.asm.mixin.injection.callback.CallbackInfo;
import org.spongepowered.asm.mixin.injection.callback.CallbackInfoReturnable;

/* compiled from: 0.java */
@Mixin({Keyboard.class})
/* loaded from: mio-yarn.jar:me/mioclient/mixin/MixinKeyboard.class */
public class MixinKeyboard {

    @Shadow
    @Final
    private MinecraftClient field_1678;

    @Inject(method = {"onKey"}, at = {@At("TAIL")}, cancellable = true)
    private void onKey(long j, int i, int i2, int i3, int i4, CallbackInfo callbackInfo) {
        if (i >= 0 && this.field_1678.currentScreen == null && i3 == 1) {
            KeyEvent keyEvent = new KeyEvent(i, false);
            SearchHelper_4.baritoneHelper.getObject1794(keyEvent);
            if (keyEvent.is2403()) {
                callbackInfo.cancel();
            }
        }
    }

    @Inject(method = {"onChar"}, at = {@At("HEAD")})
    public void onChar(long j, int i, int i2, CallbackInfo callbackInfo) {
        if (this.field_1678.currentScreen == null && j == this.field_1678.getWindow().getHandle()) {
            SearchHelper_4.baritoneHelper.getObject1794(new CharEvent(i));
        }
    }

    @Inject(method = {"processF3"}, at = {@At(value = "INVOKE", target = "Lnet/minecraft/client/gui/hud/ChatHud;clear(Z)V", shift = At.Shift.BEFORE)})
    private void processF3Pre(int i, CallbackInfoReturnable<Boolean> callbackInfoReturnable) {
        BetterChat.flag2 = true;
    }

    @Inject(method = {"processF3"}, at = {@At(value = "INVOKE", target = "Lnet/minecraft/client/gui/hud/ChatHud;clear(Z)V", shift = At.Shift.AFTER)})
    private void processF3Post(int i, CallbackInfoReturnable<Boolean> callbackInfoReturnable) {
        BetterChat.flag2 = false;
    }
}
